#ifndef RADIANT_PLATFORM_HPP
#define RADIANT_PLATFORM_HPP

// Is this OS X?
#ifdef __APPLE_CC__
#   define RADIANT_OSX 1
// Is this Windows?
#elif WIN32
#   define RADIANT_WIN32 1
// Is this Linux?
#elif __linux__
#   define RADIANT_LINUX 1
#else
#   error "Unsupported platform!"
#endif

#endif
