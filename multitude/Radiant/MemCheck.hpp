#ifndef RADIANT_MEMCHECK_HPP
#define RADIANT_MEMCHECK_HPP

namespace Radiant {
  class MemCheck
  {
  public:
#ifdef MULTI_MEMCHECK
    MemCheck();
    MemCheck(const MemCheck & s);
    virtual ~MemCheck();
    MemCheck & operator=(const MemCheck & s);
#endif
  };
}

#endif // RADIANT_MEMCHECK_HPP
