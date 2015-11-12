#ifndef _UTILS_H_
#define _UTILS_H_

/**
 * Convert a quantile to a percentil integer, such as: 0.50 -> 50, 0.99 -> 99, 0.999 -> 999
 * @arg quantile
 * @arg percentile Output.
 * @return 0 on success, negative on error.
 */
extern int to_percentile(double quantile, int *percentile);

#endif
