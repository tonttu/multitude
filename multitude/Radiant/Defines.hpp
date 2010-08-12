#ifndef RADIANT_DEFINES_HPP
#define RADIANT_DEFINES_HPP

#ifdef __GNUC__
#define MULTI_ATTR_DEPRECATED(f) f __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define MULTI_ATTR_DEPRECATED(f) __declspec(deprecated) f
#else
#define MULTI_ATTR_DEPRECATED(f) f
#endif


#endif // DEFINES_HPP
