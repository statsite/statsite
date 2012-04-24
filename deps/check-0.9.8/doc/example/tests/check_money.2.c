#include <check.h>
#include "../src/money.h"

START_TEST (test_money_create)
{
  Money *m;
  m = money_create (5, "USD");
  fail_unless (money_amount (m) == 5, 
	       "Amount not set correctly on creation");
  fail_unless (strcmp (money_currency (m), "USD") == 0,
	       "Currency not set correctly on creation");
  money_free (m);
}
END_TEST

int
main (void)
{
  return 0;
}
