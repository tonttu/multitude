#ifndef RADIANT_DEFINES_HPP
#define RADIANT_DEFINES_HPP

#ifdef WIN32

#define MULTI_ATTR_DEPRECATED

#else

#define MULTI_ATTR_DEPRECATED __attribute__ ((deprecated))

#endif

#endif // DEFINES_HPP
