#ifndef POSTPROCESSCHAIN_HPP
#define POSTPROCESSCHAIN_HPP

#include "PostProcessFilter.hpp"

#include <map>

namespace Luminous
{
  class PostProcessFilter;
  typedef std::shared_ptr<PostProcessFilter> PostProcessFilterPtr;

  class LUMINOUS_API PostProcessChain
  {
  public:
    /// @todo create iterator that skips over disabled filters
    typedef std::map<unsigned,PostProcessFilterPtr> FilterChain;

  public:
    PostProcessChain();
    virtual ~PostProcessChain();

    /// Adds a filter to the end of the chain.
    void add(PostProcessFilterPtr filter);

    /// Inserts a filter at the given index
    void insert(PostProcessFilterPtr filter, int index);

    bool contains(int index) const;

    bool empty() const { return m_chain.empty(); }
    size_t size() const { return m_chain.size(); }

    const PostProcessFilterPtr front() const;
    const FilterChain & filters() const;

  private:
    FilterChain m_chain;
  };
}
#endif // POSTPROCESSCHAIN_HPP
