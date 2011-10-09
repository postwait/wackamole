#ifndef _CONFIG_HELP_H_
#define _CONFIG_HELP_H_ 1

#define YYSTYPE WACKSTYPE

typedef union {
  int number;
  struct {
    struct in_addr addr;
    struct in_addr network;
    int mask;
    char iface[IFNAMSIZ];
  } ip;
  struct timeval tv;
  char *string;
} WACKSTYPE;
extern WACKSTYPE wacklval;
extern int wacksemanticerr;

int wackerror(char *str);

#endif /* _CONFIG_HELP_H_ */
