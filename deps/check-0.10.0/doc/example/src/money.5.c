#include <stdlib.h>
#include "money.h"

struct Money
{
    int amount;
    char *currency;
};

Money *money_create(int amount, char *currency)
{
    Money *m = malloc(sizeof(Money));

    if (m == NULL)
    {
        return NULL;
    }

    m->amount = amount;
    m->currency = currency;
    return m;
}

int money_amount(Money * m)
{
    return m->amount;
}

char *money_currency(Money * m)
{
    return m->currency;
}

void money_free(Money * m)
{
    free(m);
    return;
}
