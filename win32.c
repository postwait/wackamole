/*
 * Copyright (c) 2001-2005 OmniTI, Inc. All rights reserved
 *
 * Licensed to the Backhand project to be distributed under the term of
 * the Wackamole license.
 */

#include "config.h"

#undef read
#undef write
#undef close

#ifndef DONT_USE_THREADS
/* cancellation is not 100% implemented; it's dangerous in any case,
 * and there is no need to run threaded in wackamole unless you're
 * embedding perl... which we don't support on win32 right now. */

struct pthread_thread_t {
	struct pthread_thread_t *next, *prev;
	int refcount;
	int joinable;

	void *retval;
	
	pthread_t thread_id;
	HANDLE kill_me;
	HANDLE my_handle;
};

static CRITICAL_SECTION thread_mutex;
static struct pthread_thread_t *threads = NULL;

/* pthread_cancel is evil */
int pthread_setcanceltype(int type, int *oldtype)
{
	if (type != PTHREAD_CANCEL_DEFERRED) {
		return -1;
	}
	if (oldtype) *oldtype = PTHREAD_CANCEL_DEFERRED;
	return 0;
}

struct __win32_pthread_create_param {
	void *(*start_routine)(void *);
	void *arg;
	struct pthread_thread_t *t;
};

static struct pthread_thread_t *find_thread(pthread_t id)
{
	struct pthread_thread_t *thr;

	EnterCriticalSection(&thread_mutex);
	for (thr = threads; thr; thr = thr->next) {
		if (thr->thread_id == id) {
			InterlockedIncrement(&thr->refcount);
			LeaveCriticalSection(&thread_mutex);
			return thr;
		}
	}
	LeaveCriticalSection(&thread_mutex);
	return NULL;
}

static void delref_thread(struct pthread_thread_t *t)
{
	EnterCriticalSection(&thread_mutex);
	if (InterlockedDecrement(&t->refcount)) {
		LeaveCriticalSection(&thread_mutex);
		return;
	}

	if (t->next)
		t->next->prev = t->prev;
	if (t->prev)
		t->prev->next = t->next;
	else
		threads = t->next;

	LeaveCriticalSection(&thread_mutex);

	CloseHandle(t->kill_me);
	CloseHandle(t->my_handle);
	free(t);
}

void pthread_exit(void *retval)
{
	struct pthread_thread_t *t;
	
	t = find_thread(GetCurrentThreadId());
	t->retval = retval;

	delref_thread(t);

	ExitThread((DWORD)retval);
}

int pthread_cancel(pthread_t threadid)
{
	struct pthread_thread_t *t;
	
	t = find_thread(threadid);

	if (t) {
		SetEvent(t->kill_me);
		delref_thread(t);
		return 0;
	}
	return -1;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
	return 0;
}

static DWORD WINAPI __win32_pthread_create_bridge(LPVOID param)
{
	struct __win32_pthread_create_param p = *(struct __win32_pthread_create_param*)param;
	free(param);

	pthread_exit(p.start_routine(p.arg));

	/* not reached */
	return 0;
}

int pthread_create(pthread_t *thread, pthread_attr_t *attr,
		void *(*start_routine)(void *), void *arg)
{
	struct __win32_pthread_create_param *p = malloc(sizeof(*p));
	HANDLE thr;
	struct pthread_thread_t *the_thread = calloc(1, sizeof(*p));

	the_thread->refcount = attr->detached ? 1 : 2;
	the_thread->kill_me = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (threads == NULL) {
		InitializeCriticalSection(&thread_mutex);
		threads = the_thread;
	} else {
		EnterCriticalSection(&thread_mutex);
		the_thread->next = threads;
		if (threads)
			threads->prev = the_thread;
		LeaveCriticalSection(&thread_mutex);
	}
	
	p->start_routine = start_routine;
	p->arg = arg;
	
	the_thread->my_handle = CreateThread(NULL, 0, __win32_pthread_create_bridge, p, 0, &the_thread->thread_id);

	if (the_thread->my_handle) {
		*thread = the_thread->thread_id;
		return 0;
	}

	the_thread->refcount = 1;
	delref_thread(the_thread);

	free(p);
	return -1;
}

int pthread_attr_init(pthread_attr_t *attr)
{
	memset(attr, 0, sizeof(*attr));
	return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int state)
{
	attr->detached = state;
	return 0;
}

int pthread_mutex_init(pthread_mutex_t *mutex, void *attr)
{
	InitializeCriticalSection(mutex);
	return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	EnterCriticalSection(mutex);
	return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	LeaveCriticalSection(mutex);
	return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	DeleteCriticalSection(mutex);
	return 0;
}
#endif

/* usleep.  This is a pthread cancellation point */
int usleep(unsigned long usec)
{
	int msec = usec / 1000;
#ifndef DONT_USE_THREADS
	struct pthread_thread_t *t;

	t = find_thread(GetCurrentThreadId());
	if (t) {
		if (WAIT_OBJECT_0 == WaitForSingleObjectEx(t->kill_me, msec, 0)) {
			pthread_exit(NULL);
			/* not reached */
			return -1;
		}
		return 0;
	}
#endif
	Sleep(msec);
	return 0;
}


static void update_errno_from_win(DWORD code)
{
	switch (code) {
		case WSAEWOULDBLOCK: errno = EAGAIN; break;
		default:
			errno = code;
	}
}

int wackamole_win32_write(int fd, const void *buf, int len)
{
	if (_get_osfhandle(fd) == 0xffffffff) {
		int r;

		r = send(fd, buf, len, 0);
		if (r == SOCKET_ERROR) {
			update_errno_from_win(WSAGetLastError());
			return -1;
		}
		return r;
	}
	return write(fd, buf, len);
}

int wackamole_win32_read(int fd, void *buf, int len)
{
	if (_get_osfhandle(fd) == 0xffffffff) {
		int r;

		r = recv(fd, buf, len, 0);
		if (r == SOCKET_ERROR) {
			update_errno_from_win(WSAGetLastError());
			return -1;
		}
		return r;
	}
	return read(fd, buf, len);
}

int wackamole_win32_close(int fd)
{
	if (_get_osfhandle(fd) == 0xffffffff) {
		return closesocket(fd);
	} else {
		return close(fd);
	}
}

void *dlopen(const char *name, int flags)
{
	return LoadLibrary(name);
}

void *dlsym(void *handle, const char *symbol)
{
	return GetProcAddress(handle, symbol);
}

void dlclose(void *handle)
{
	FreeLibrary(handle);
}

const char *dlerror(void)
{
	return "Unknown Error";
}

