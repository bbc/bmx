#ifndef INTTYPES_H_
#define INTTYPES_H_

/* subset of C99 inttypes.h for Microsoft Visual Studio build */

#if defined(_MSC_VER)


/* exact width integer types */

#if (_MSC_VER >= 1600)

/* Visual Studio 2010 provides a stdint.h */

#include <stdint.h>

#else

typedef unsigned char       uint8_t;
typedef unsigned __int16    uint16_t;
typedef unsigned __int32    uint32_t;
typedef unsigned __int64    uint64_t;

typedef signed char         int8_t;
typedef __int16             int16_t;
typedef __int32             int32_t;
typedef __int64             int64_t;


#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS)

#define INT8_MIN    (-128)
#define INT16_MIN   (-32767 - 1)
#define INT32_MIN   (-2147483647 - 1)
#define INT64_MIN   (-9223372036854775807LL - 1)

#define INT8_MAX    127
#define INT16_MAX   32767
#define INT32_MAX   2147483647
#define INT64_MAX   9223372036854775807LL

#define UINT8_MAX   255
#define UINT16_MAX  65535
#define UINT32_MAX  4294967295U
#define UINT64_MAX  18446744073709551615ULL


#define INT64_C(c)  c ## i64
#define UINT64_C(c) c ## ui64

#endif


#endif


/* print format specifiers */

#define PRId64  "I64d"
#define PRIu64  "I64u"
#define PRIx64  "I64x"

#if defined(_WIN64)
#define PRIszt  "I64u"
#else
#define PRIszt  "u"
#endif



#endif

#endif

