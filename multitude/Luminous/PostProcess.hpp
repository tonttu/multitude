#ifndef POSTPROCESS_HPP
#define POSTPROCESS_HPP

#include <functional>
#include <memory>
#include <list>

/// @cond
namespace Luminous
{
  class PostProcessFilter;

  namespace PostProcess
  {
    typedef std::function<std::shared_ptr<PostProcessFilter>() > InitFunc;

    struct Creator
    {
      InitFunc func;
      unsigned index;
    };

    typedef std::list<Creator> InitList;
  }
}
/// @endcond

#endif // POSTPROCESS_HPP
