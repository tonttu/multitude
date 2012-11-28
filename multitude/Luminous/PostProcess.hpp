#ifndef POSTPROCESS_HPP
#define POSTPROCESS_HPP

#include <functional>
#include <memory>
#include <list>

/// @cond
namespace Luminous
{
  class PostProcessContext;

  namespace PostProcess
  {
    typedef std::function<std::shared_ptr<PostProcessContext>() > InitFunc;

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
