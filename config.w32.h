/* Don't edit this file, edit config.w32.h instead */
#ifndef CONFIG_H
#define CONFIG_H

#define _WIN32_WINNT 0x0501
#include <winsock2.h>
#undef interface

#define BUNDLEEXT	"obj"
#define HAVE_GOOD_VARGS

#define IFNAMSIZ	256 /* can be huge names on windows */
#define ETH_ALEN 6

#define HAVE_SP_H 1
#define HAVE_PCAP_H 1
#define HAVE_STDIO_H 1
#define HAVE_SOCKLEN_T 1
#define HAVE_ERRNO_H 1
#define HAVE_ASSERT_H 1

#define HAVE_INTXX_T 1
#define HAVE_U_INTXX_T 1
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef unsigned __int8 u_int_8_t;
typedef unsigned __int16 u_int_16_t;
typedef unsigned __int32 u_int_32_t;

/* pthread stubs */

#define DONT_USE_THREADS 1

#ifndef DONT_USE_THREADS
typedef DWORD pthread_t;
typedef CRITICAL_SECTION pthread_mutex_t;

typedef struct {
	int detached;
} pthread_attr_t;

enum {
	PTHREAD_CREATE_DETACHED = 1,

	PTHREAD_CANCEL_DEFERRED = 0,
	PTHREAD_CANCEL_ASYNCHRONOUS = 1,
};

int pthread_setcanceltype(int type, int *oldtype);
int pthread_cancel(pthread_t thread);

int pthread_create(pthread_t *thread, pthread_attr_t *attr,
		void *(*start_routine)(void *), void *arg);
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int state);
int pthread_attr_destroy(pthread_attr_t *attr);

int pthread_mutex_init(pthread_mutex_t *mutex, void *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
#endif

/* fun with posix descriptors */

#define read _hide_ms_read
#define write _hide_ms_write
#define close _hide_ms_close
#include <io.h>
#undef read
#undef write
#undef close

int wackamole_win32_write(int fd, const void *buf, int len);
int wackamole_win32_read(int fd, void *buf, int len);
int wackamole_win32_close(int fd);

#define read wackamole_win32_read
#define write wackamole_win32_write
#define close wackamole_win32_close
#define EWOULDBLOCK	EAGAIN

enum {
	RTLD_GLOBAL
};

void *dlopen(const char *name, int flags);
void *dlsym(void *handle, const char *symbol);
void dlclose(void *handle);
const char *dlerror(void);

#include "defines.h"

#include <signal.h>

#endif

