#include <EXTERN.h>
#include <perl.h>
#include "perl.h"
#include "config.h"

EXTERN_C void xs_init();

static PerlInterpreter *my_perl = NULL;

static SV *my_eval_sv(SV *sv, I32 coe) {
    SV *retval;
    STRLEN n_a;
    dSP;

    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    PUTBACK;
    eval_sv(sv, G_SCALAR);

    SPAGAIN;
    retval = POPs;
    if(coe && SvTRUE(ERRSV)) {
        croak(SvPVx(ERRSV, n_a));
    }
    SvREFCNT_inc(retval);
    PUTBACK;
    FREETMPS;
    LEAVE;
    return retval;
}

void perl_startup() {
    char *embedding[] = { "", "-e", "0" };

    my_perl = perl_alloc();
    perl_construct( my_perl );

    perl_parse(my_perl, xs_init, 3, embedding, NULL);
}

void perl_shutdown() {
    if(my_perl) {
        perl_destruct(my_perl);
        perl_free(my_perl);
        my_perl = NULL;
    }
}

int perl_inc(char *path) {
    SV *retval;
    SV *command = NEWSV(1024+20, 0);

    sv_setpvf(command, "use lib '%s';", path);
    retval = my_eval_sv(command, TRUE);
    SvREFCNT_dec(command);

    return SvIV(retval);
}

int perl_use(char *module) {
    int val;
    SV *retval;
    SV *command = NEWSV(1024+20, 0);

    sv_setpvf(command, "use %s;", module);
    retval = my_eval_sv(command, TRUE);
    SvREFCNT_dec(command);
    val = SvIV(retval);
    SvREFCNT_dec(retval);
    return val;
}

int perl_handler(char *func, struct interface *p,
                 struct interface *e,
                 struct interface *r) {
    int retval;
    int i;
    HV *phv, *rhv;
    AV *eav;

    dSP;                                  /* initialize stack pointer      */
    ENTER;                                /* everything created after here */
    SAVETMPS;                             /* ...is a temporary variable.   */
    PUSHMARK(SP);                         /* remember the stack pointer    */
    phv = newHV();
    hv_store(phv, "ifname", sizeof("ifname")-1, sv_2mortal(newSVpv(p->ifname,0)),0);
    hv_store(phv, "ip", sizeof("ip")-1, sv_2mortal(newSVpv(inet_ntoa(p->ipaddr),0)),0);
    hv_store(phv, "broadcast", sizeof("broadcast")-1, sv_2mortal(newSVpv(inet_ntoa(p->bcast),0)),0);
    hv_store(phv, "netmask", sizeof("netmask")-1, sv_2mortal(newSVpv(inet_ntoa(p->netmask),0)),0);

    eav = newAV();
    for(i = 0; i < MAX_DEP_IF && e[i].ipaddr.s_addr != 0; i++) {
        HV* tmphv;
        tmphv = newHV();
        hv_store(tmphv, "ifname", sizeof("ifname")-1, sv_2mortal(newSVpv(e[i].ifname,0)),0);
        hv_store(tmphv, "ip", sizeof("ip")-1, sv_2mortal(newSVpv(inet_ntoa(e[i].ipaddr),0)),0);
        hv_store(tmphv, "broadcast", sizeof("broadcast")-1, sv_2mortal(newSVpv(inet_ntoa(e[i].bcast),0)),0);
        hv_store(tmphv, "netmask", sizeof("netmask")-1, sv_2mortal(newSVpv(inet_ntoa(e[i].netmask),0)),0);
        av_push(eav, (SV*)sv_2mortal(newRV_inc((SV *)tmphv)));
    }    
    rhv = newHV();
    hv_store(rhv, "ifname", sizeof("ifname")-1, sv_2mortal(newSVpv(r->ifname,0)),0);
    hv_store(rhv, "ip", sizeof("ip")-1, sv_2mortal(newSVpv(inet_ntoa(r->ipaddr),0)),0);
    hv_store(rhv, "broadcast", sizeof("broadcast")-1, sv_2mortal(newSVpv(inet_ntoa(r->bcast),0)),0);
    hv_store(rhv, "netmask", sizeof("netmask")-1, sv_2mortal(newSVpv(inet_ntoa(r->netmask),0)),0);
    
    XPUSHs((SV*)sv_2mortal(newRV_inc((SV *)phv)));/* push the psuedo interface onto the stack  */
    XPUSHs((SV*)sv_2mortal(newRV_inc((SV *)eav)));/* push the extra interface array ref onto the stack  */
    XPUSHs((SV*)sv_2mortal(newRV_inc((SV *)rhv)));/* push the real interface onto the stack  */
    PUTBACK;                              /* make local stack pointer global */
    call_pv(func, G_SCALAR);              /* call the function             */
    SPAGAIN;                              /* refresh stack pointer         */
                                          /* pop the return value from stack */
    retval = POPi;
    PUTBACK;
    FREETMPS;                             /* free that return value        */
    LEAVE;                                /* ...and the XPUSHed "mortal" args.*/
    return retval;
}

