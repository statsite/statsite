#include <check.h>
#include "../src/money.h"

START_TEST(test_money_create)
{
    Money *m;

    m = money_create(5, "USD");
    ck_assert_int_eq(money_amount(m), 5);
    ck_assert_str_eq(money_currency(m), "USD");
    money_free(m);
}
END_TEST

int main(void)
{
    return 0;
}
