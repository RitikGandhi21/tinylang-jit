#include <stdio.h>
#include <time.h>

static long fib(long n) {
  if (n < 2) return n;
  return fib(n - 1) + fib(n - 2);
}

int main(void) {
  clock_t t0 = clock();
  long r = fib(35);
  clock_t t1 = clock();
  printf("%ld\n", r);
  printf("time_ms=%.3f\n", 1000.0 * (t1 - t0) / CLOCKS_PER_SEC);
  return 0;
}
