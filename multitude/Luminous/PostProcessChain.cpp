#include "PostProcessChain.hpp"

namespace Luminous
{
  PostProcessChain::PostProcessChain()
  {
  }

  PostProcessChain::~PostProcessChain()
  {
  }

  void PostProcessChain::add(PostProcessFilterPtr filter)
  {
    if(m_chain.empty()) {
      m_chain.insert(std::make_pair(FilterChain::key_type(0), filter));
    } else {
      FilterChain::key_type key = m_chain.rbegin()->first;
      m_chain.insert(std::make_pair(key+1, filter));
    }
  }

  void PostProcessChain::insert(PostProcessFilterPtr filter, int index)
  {
    m_chain.insert(std::make_pair(index, filter));
  }

  bool PostProcessChain::contains(int order) const
  {
    return m_chain.find(order) != m_chain.end();
  }

  const PostProcessFilterPtr PostProcessChain::front() const
  {
    if(m_chain.empty())
      return PostProcessFilterPtr();

    return m_chain.begin()->second;
  }

  const PostProcessChain::FilterChain & PostProcessChain::filters() const
  {
    return m_chain;
  }
}
