/*
 * types.h
 */

#ifndef __TYPES_H__
#define __TYPES_H__

#ifndef UINT8
#define UINT8   unsigned char
#endif

#ifndef UINT16
#define UINT16   unsigned short
#endif

#ifndef UINT32
#define UINT32   unsigned int
#endif

#ifndef INT8
#define INT8    char
#endif

#ifndef INT16
#define INT16   short
#endif

#ifndef INT32
#define INT32   int
#endif


#ifndef NULL
#define NULL     0   
#endif


#ifndef __s8
#define __s8    signed char
#endif

#ifndef __u8
#define __u8    unsigned char
#endif

#ifndef __s16
#define __s16   signed short
#endif

#ifndef __u16
#define __u16   unsigned short
#endif

#ifndef __s32
#define __s32   signed int
#endif

#ifndef __u32
#define __u32   unsigned int
#endif


// Internal macros.

#define _F_START(f)             (0 ? f)
#define _F_END(f)               (1 ? f)
#define _F_SIZE(f)              (1 + _F_END(f) - _F_START(f))
#define _F_MASK(f)              (((1 << _F_SIZE(f)) - 1) << _F_START(f))
#define _F_NORMALIZE(v, f)      (((v) & _F_MASK(f)) >> _F_START(f))
#define _F_DENORMALIZE(v, f)    (((v) << _F_START(f)) & _F_MASK(f))

// Global macros.
#define FIELD_GET(x, reg, field) 			(_F_NORMALIZE((x), reg ## _ ## field))
#define FIELD_SET(x, reg, field, value) 	( (x & ~_F_MASK(reg ## _ ## field)) | _F_DENORMALIZE(reg ## _ ## field ## _ ## value, reg ## _ ## field))
#define FIELD_VALUE(x, reg, field, value) 	( (x & ~_F_MASK(reg ## _ ## field)) | _F_DENORMALIZE(value, reg ## _ ## field) )
#define FIELD_CLEAR(reg, field) 			( ~ _F_MASK(reg ## _ ## field) )

#define FIELD_INIT(reg, field, value)   	_F_DENORMALIZE(reg ## _ ## field ## _ ## value, reg ## _ ## field)
#define FIELD_INIT_VAL(reg, field, value)  	(_F_DENORMALIZE(value, reg ## _ ## field))
#define FIELD_GET_MASK(reg, field)      	_F_MASK(reg ## _ ## field)

enum {
    BUFFER0 = 0,
    BUFFER1 = 1,
};

enum {
    Y_BASE = 0,
    U_BASE = 1,
    V_BASE = 2,
};

#endif /* __TYPES_H__ */

