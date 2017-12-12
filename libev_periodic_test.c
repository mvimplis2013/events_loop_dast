#include <stdio.h>
#include <sys/time.h>
#include <ev.h>

//const int log_size = 10000;
#define LOG_SIZE 1000000

double log[LOG_SIZE];
int log_index = 0;

double now_usec() {
  struct timeval tv;

  gettimeofday(&tv, NULL);

  double ret = (double)(1000000.0*tv.tv_sec + tv.tv_usec);

  return ret;
}

void clock_cb(EV_P_ ev_periodic *w, int re) {
  double now = now_usec();
  log[log_index++] = now;
  if (now > log[0]+200000.0 || log_index >= LOG_SIZE) {
    ev_unloop(EV_A_ EVUNLOOP_ALL);
  }
}

int main(int argc, char *argv[]) {
  log_index = 0;
  log[log_index++] = now_usec();

  struct ev_loop *loop = ev_default_loop(0);

  ev_periodic tick;
  ev_periodic_init(&tick, clock_cb, 0., 0.001, 0);
  ev_periodic_start(loop, &tick);

  ev_loop(loop, 0);

  int i;
  for (i=0; i<log_index; i++) {
    printf("[%d] = %f\n", i, log[i]/1000000.0);
  }

  return 0;
}
