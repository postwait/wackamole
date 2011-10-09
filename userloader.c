#include "config.h"
#include "alarm.h"
#include "userloader.h"
#include "configuration.h"
#ifdef USE_EMBEDDED_PERL
#include "perl.h"
#endif

static int handler_count = 0;
static struct dlfuncs handlers[1024] = { {0} };

int register_shared(char *path, char *func, char type) {
    struct dlfuncs *handler;
    char fullpath[MAXPATHLEN];
    char fullsymbol[128];
    
#ifdef NEED_SYMBOL_PREFIX
    snprintf(fullsymbol, sizeof(fullsymbol), "_%s", func);
#else
    strncpy(fullsymbol, func, sizeof(fullsymbol));
#endif
    snprintf(fullpath, MAXPATHLEN, "%s/%s", default_library_path, path);
    if((type != DLFUNCS_TYPE_ON_UP) &&
        (type != DLFUNCS_TYPE_ON_DOWN) &&
        (type != DLFUNCS_TYPE_POST_UP) &&
        (type != DLFUNCS_TYPE_POST_DOWN)) {
        wack_alarm( PRINT, "register_shared called with unknown type: %d",
                    type );
        return -1;
    }
    handler = &handlers[handler_count];
    handler->handle = dlopen(fullpath, RTLD_NOW|RTLD_GLOBAL);
    if(!handler->handle) {
        strncat(fullpath, "."BUNDLEEXT, MAXPATHLEN);
        handler->handle = dlopen(fullpath, RTLD_NOW|RTLD_GLOBAL);
    }
    if(!handler->handle) {
        wack_alarm( PRINT, "register_shared open of %s failed: %s",
                    fullpath, dlerror() );
        return -2;
    }
    if((handler->handler = dlsym(handler->handle, fullsymbol)) == NULL) {
        wack_alarm( PRINT, "register_shared could not find symbol %s: %s",
                    fullsymbol, dlerror() );
        dlclose(handler->handle);
        return -3;
    }
    handler->genre = DLFUNCS_GENRE_DL;
    handler->type = type;
    handler_count++;
    return type;
}
int register_perl(char *func, char type) {
    struct dlfuncs *handler;

    handler = &handlers[handler_count];
    handler->genre = DLFUNCS_GENRE_PERL;
    handler->func = func;
    handler->type = type;
    handler_count++;
    return type;
}
int deregister_all() {
    int i;
    for(i=0;i<handler_count;i++) {
        dlclose(handlers[i].handle);
    }
    handler_count = 0;
    return i;
}
int execute_all_user_simple(char type) {
    int i, count = 0;
    struct interface safe_pseudo;
    struct interface safe_extras[MAX_DEP_IF];
    struct interface safe_real;

    memset(&safe_pseudo, 0, sizeof(struct interface));
    memset(safe_extras,  0, sizeof(struct interface)*MAX_DEP_IF);
    memset(&safe_real,   0, sizeof(struct interface));

    for(i=0;i<handler_count;i++) {
        if(handlers[i].type == type) {
            switch(handlers[i].genre) {
                case DLFUNCS_GENRE_DL:
                    handlers[i].handler(safe_pseudo, safe_extras, safe_real);
                    break;
#ifdef USE_EMBEDDED_PERL
                case DLFUNCS_GENRE_PERL:
                    perl_handler(handlers[i].func,
                                 &safe_pseudo, safe_extras, &safe_real);
                    break;
#endif
                default:
                    wack_alarm(PRINT, "Unknown user function genre: %d", 
                               handlers[i].genre);
            }
            count++;
        }
    }
    return count;
}
int execute_all_user(struct interface pseudo,
                     struct interface *extras,
                     struct interface real,
                     char type) {
    int i, count = 0;
    struct interface safe_pseudo;
    struct interface safe_extras[MAX_DEP_IF];
    struct interface safe_real;

    /* Let's copy this so people can't f them up */
    memcpy(&safe_pseudo, &pseudo, sizeof(struct interface));
    memcpy(safe_extras, extras, sizeof(struct interface)*MAX_DEP_IF);
    memcpy(&safe_real, &real, sizeof(struct interface));
    for(i=0;i<handler_count;i++) {
        if(handlers[i].type == type) {
            switch(handlers[i].genre) {
                case DLFUNCS_GENRE_DL:
                    handlers[i].handler(safe_pseudo, safe_extras, safe_real);
                    break;
#ifdef USE_EMBEDDED_PERL
                case DLFUNCS_GENRE_PERL:
                    perl_handler(handlers[i].func,
                                 &safe_pseudo, safe_extras, &safe_real);
                    break;
#endif
                default:
                    wack_alarm(PRINT, "Unknown user function genre: %d", 
                               handlers[i].genre);
            }
            count++;
        }
    }
    return count;
}
