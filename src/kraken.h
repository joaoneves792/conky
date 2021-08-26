

#ifndef KRAKEN_CONKY_H
#define KRAKEN_CONKY_H

#define KRAKEN_SUBSYSTEM "hidraw"
#define KRAKEN_MAX_DEV_PATH_CHARS 1024

#define KRAKEN_NZXT_X_SERIES_VENDOR "1e71"
#define KRAKEN_NZXT_X_SERIES_PRODUCT "2007"

#define KRAKEN_REPORT_SIZE 64

struct kraken_state{
  int fd;
  unsigned int rpm;
  double liquid_temp;
  unsigned int duty;
};


int update_kraken(void);
void print_kraken_rpm(struct text_object *, char *, unsigned int);
void print_kraken_liquid_temp(struct text_object *, char *, unsigned int);
void print_kraken_duty(struct text_object *, char *, unsigned int);
void free_kraken(struct text_object *);

#endif
