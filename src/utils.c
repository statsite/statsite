#include "utils.h"

int to_percentile(double quantile, int *percentile) {
    if (quantile < 0 || quantile > 1) {
        return -1;
    }

    /* Make sure the percentile has at least two digits */
    quantile *= 100;

    /*
     * Shift the double and get rid of the trailing 0s
     */
    while (quantile != (int)quantile) {
        quantile *= 10;
    }
    *percentile = (int) quantile;
    return 0;
}
