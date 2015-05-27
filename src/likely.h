#ifndef _LIKELY_H_
#define _LIKELY_H_

// Macro to provide branch meta-data
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#endif
