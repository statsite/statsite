#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>
#include "timer.h"

START_TEST(test_timer_init_and_destroy)
{
    timer t;
    double quants[] = {0.5, 0.90, 0.99};
    int res = init_timer(0.01, (double*)&quants, 3, &t);
    fail_unless(res == 0);

    res = destroy_timer(&t);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_timer_init_add_destroy)
{
    timer t;
    double quants[] = {0.5, 0.90, 0.99};
    int res = init_timer(0.01, (double*)&quants, 3, &t);
    fail_unless(res == 0);

    fail_unless(timer_add_sample(&t, 100, 1.0) == 0);
    fail_unless(timer_count(&t) == 1);
    fail_unless(timer_sum(&t) == 100);
    fail_unless(timer_squared_sum(&t) == 10000);
    fail_unless(timer_min(&t) == 100);
    fail_unless(timer_max(&t) == 100);
    fail_unless(timer_mean(&t) == 100);
    fail_unless(timer_stddev(&t) == 0);
    fail_unless(timer_query(&t, 0.5) == 100);

    res = destroy_timer(&t);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_timer_add_loop)
{
    timer t;
    double quants[] = {0.5, 0.90, 0.99};
    int res = init_timer(0.01, (double*)&quants, 3, &t);
    fail_unless(res == 0);

    for (int i=1; i<=100; i++)
        fail_unless(timer_add_sample(&t, i, 1.0) == 0);

    fail_unless(timer_count(&t) == 100);
    fail_unless(timer_sum(&t) == 5050);
    fail_unless(timer_squared_sum(&t) == 338350);
    fail_unless(timer_min(&t) == 1);
    fail_unless(timer_max(&t) == 100);
    fail_unless(timer_mean(&t) == 50.5);
    fail_unless(round(timer_stddev(&t)*1000)/1000 == 29.011);
    fail_unless(timer_query(&t, 0.5) == 50);
    fail_unless(timer_query(&t, 0.90) >= 89 && timer_query(&t, 0.90) <= 91);
    fail_unless(timer_query(&t, 0.99) >= 98 && timer_query(&t, 0.99) <= 100);

    res = destroy_timer(&t);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_timer_sample_rate)
{
  timer t;
  double quants[] = {0.5, 0.90, 0.99};
  int res = init_timer(0.01, (double*)&quants, 3, &t);
  fail_unless(res == 0);

  for (int i=1; i<=100; i++)
      fail_unless(timer_add_sample(&t, i, 0.5) == 0);

  fail_unless(timer_count(&t) == 200);
  fail_unless(timer_sum(&t) == 10100);
  fail_unless(timer_squared_sum(&t) == 676700);
  fail_unless(timer_min(&t) == 1);
  fail_unless(timer_max(&t) == 100);
  fail_unless(timer_mean(&t) == 50.5);
  fail_unless(round(timer_stddev(&t)*1000)/1000 == 28.939);
  fail_unless(timer_query(&t, 0.5) >= 50 && timer_query(&t, 0.5) <= 51);
  fail_unless(timer_query(&t, 0.90) >= 89 && timer_query(&t, 0.90) <= 91);
  fail_unless(timer_query(&t, 0.99) >= 98 && timer_query(&t, 0.99) <= 100);

  res = destroy_timer(&t);
  fail_unless(res == 0);
}
END_TEST

static double diff(double a, double b) {
    double m = fmax(a,b);
    double part = abs(a - b);
    return part / m;
}

START_TEST(test_timer_sample_rate_2)
{
  timer t, t1;
  double quants[] = {0.5, 0.90, 0.99};
  int res = init_timer(0.01, (double*)&quants, 3, &t);
  fail_unless(res == 0);

  res = init_timer(0.01, (double*)&quants, 3, &t1);
  fail_unless(res == 0);

  for (int i=0; i<100; i++) {
      fail_unless(timer_add_sample(&t, i, 0.25) == 0);

      fail_unless(timer_add_sample(&t1, i, 1) == 0);
      fail_unless(timer_add_sample(&t1, i, 1) == 0);
      fail_unless(timer_add_sample(&t1, i, 1) == 0);
      fail_unless(timer_add_sample(&t1, i, 1) == 0);
  }

  fail_unless(timer_count(&t) == timer_count(&t1));
  fail_unless(timer_sum(&t) == timer_sum(&t1));
  fail_unless(timer_squared_sum(&t) == timer_squared_sum(&t1));
  fail_unless(timer_min(&t) == timer_min(&t1));
  fail_unless(timer_max(&t) == timer_max(&t1));
  fail_unless(timer_mean(&t) == timer_mean(&t1));
  fail_unless(diff(timer_stddev(&t), timer_stddev(&t1)) < 0.001);
  fail_unless(diff(timer_query(&t, 0.5), timer_query(&t1, 0.5)) < 0.02);
  fail_unless(diff(timer_query(&t, 0.90), timer_query(&t1, 0.90)) < 0.02);
  fail_unless(diff(timer_query(&t, 0.99), timer_query(&t1, 0.99)) < 0.02);

  res = destroy_timer(&t);
  fail_unless(res == 0);

  res = destroy_timer(&t1);
  fail_unless(res == 0);
}
END_TEST


START_TEST(test_timer_variable_sample_rate)
{
  timer t, t1;
  double quants[] = {0.5, 0.90, 0.99};
  int res = init_timer(0.01, (double*)&quants, 3, &t);
  fail_unless(res == 0);

  res = init_timer(0.01, (double*)&quants, 3, &t1);
  fail_unless(res == 0);

  srandom(42);

  for (int i=0; i<100; i++) {
      int count = ( random( ) % 100 ) + 1;

      for (int c = 0; c < count; c++) {
            fail_unless(timer_add_sample(&t1, i, 1) == 0);
      }
      fail_unless(timer_add_sample(&t, i, 1.0 / (double)count) == 0);
  }

  fail_unless(diff(timer_count(&t), timer_count(&t1)) < 0.001);
  fail_unless(diff(timer_sum(&t), timer_sum(&t1)) < 0.001);
  fail_unless(diff(timer_squared_sum(&t), timer_squared_sum(&t1)) < 0.001);
  fail_unless(timer_min(&t) == timer_min(&t1));
  fail_unless(timer_max(&t) == timer_max(&t1));
  fail_unless(diff(timer_mean(&t), timer_mean(&t1)) < 0.001);
  fail_unless(diff(timer_stddev(&t), timer_stddev(&t1)) < 0.001);
  fail_unless(diff(timer_query(&t, 0.5), timer_query(&t1, 0.5)) <= 0.02);
  fail_unless(diff(timer_query(&t, 0.90), timer_query(&t1, 0.90)) <= 0.02);
  fail_unless(diff(timer_query(&t, 0.99), timer_query(&t1, 0.99)) <= 0.02);

  res = destroy_timer(&t);
  fail_unless(res == 0);

  res = destroy_timer(&t1);
  fail_unless(res == 0);
}
END_TEST
