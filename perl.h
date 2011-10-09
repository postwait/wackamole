#ifndef __PERL_H_
#define __PERL_H_

struct interface;
void perl_startup();
void perl_shutdown();
int perl_inc(char *path);
int perl_use(char *module);
int perl_handler_simple(char *func);
int perl_handler(char *func,
                 struct interface *p,
                 struct interface *e,
                 struct interface *r);

#endif
