#include "alarm.h"
#include "arpcache.h"

int main() {
  wack_alarm_set(PRINT | ARPING | WACK_DEBUG | EXIT);
  sample_arp_cache();
  return 0;
}
