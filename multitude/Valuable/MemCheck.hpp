#ifndef VALUABLE_MEMCHECK_HPP
#define VALUABLE_MEMCHECK_HPP

namespace Valuable {
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

#endif // VALUABLE_MEMCHECK_HPP
