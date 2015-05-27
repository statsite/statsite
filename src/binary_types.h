#ifndef _BINARY_TYPES_H_
#define _BINARY_TYPES_H_

/*
 * Binary defines
 */
#define BIN_TYPE_KV             0x1
#define BIN_TYPE_COUNTER        0x2
#define BIN_TYPE_TIMER          0x3
#define BIN_TYPE_SET            0x4
#define BIN_TYPE_GAUGE          0x5
#define BIN_TYPE_GAUGE_DELTA    0x6

#define BIN_OUT_NO_TYPE 0x0
#define BIN_OUT_SUM     0x1
#define BIN_OUT_SUM_SQ  0x2
#define BIN_OUT_MEAN    0x3
#define BIN_OUT_COUNT   0x4
#define BIN_OUT_STDDEV  0x5
#define BIN_OUT_MIN     0x6
#define BIN_OUT_MAX     0x7
#define BIN_OUT_HIST_FLOOR    0x8
#define BIN_OUT_HIST_BIN      0x9
#define BIN_OUT_HIST_CEIL     0xa
#define BIN_OUT_RATE     0xb
#define BIN_OUT_SAMPLE_RATE     0xc
#define BIN_OUT_PCT     0x80

#endif
