#include <stdlib.h>
#include "money.h"

struct Money
{
    int amount;
};

Money *money_create(int amount, char *currency)
{
    return NULL;
}

int money_amount(Money * m)
{
    return m->amount;
}

char *money_currency(Money * m)
{
    return NULL;
}

void money_free(Money * m)
{
    return;
}
