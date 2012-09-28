#ifndef POSTPROCESSFACTORY_HPP
#define POSTPROCESSFACTORY_HPP

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
      int order;
    };

    typedef std::list<Creator> InitList;
  }
}
/// @endcond

#endif // POSTPROCESSFACTORY_HPP
