#include "NotCopyable.hpp"

namespace Patterns
{

  NotCopyable::NotCopyable()
  {}

  NotCopyable::~NotCopyable()
  {}


  NotCopyable::NotCopyable(const NotCopyable &)
  {}

  const NotCopyable & NotCopyable::operator = (const NotCopyable &)
  {
    return * this; 
  }
}
