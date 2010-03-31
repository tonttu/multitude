
#ifndef HAVE_STDINT_H
#define HAVE_STDINT_H 1


#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

typedef signed __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef signed __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;

  /*
  typedef char int8_t;
  typedef unsigned char uint8_t;

  typedef short int16_t;
  typedef unsigned short uint16_t;

  typedef int int32_t;
  typedef unsigned uint32_t;

  typedef long long int64_t;
  typedef unsigned long long uint64_t;
  */
  typedef int64_t intmax_t;

#ifdef __cplusplus
}		/* extern "C" */
#endif	/* __cplusplus */

#endif
