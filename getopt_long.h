#ifndef GETOPT_LONG_H
#define GETOPT_LONG_H

char *optarg;
int optind;
int opterr;
int optopt;

struct option {
  const char *name;
  int has_arg;
  int *flag;
  int val;
};

#define no_argument             0
#define required_argument       1
#define optional_argument       2

int getopt_long (int argc, char *const *argv, const char *shortopts,
                        const struct option *longopts, int *longind);

#endif
