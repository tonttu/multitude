#include "PostProcessChain.hpp"

namespace Luminous
{
  class PostProcessChain::D
  {
  public:
    PostProcessChain::FilterChain m_chain;
  };

  PostProcessChain::PostProcessChain()
    : m_d(new D())
  {
  }

  PostProcessChain::~PostProcessChain()
  {
    delete m_d;
  }

  bool PostProcessChain::add(PostProcessContextPtr ctx)
  {
    std::pair<FilterChain::iterator,bool> result;
    if(m_d->m_chain.empty()) {
      result = m_d->m_chain.insert(std::make_pair(FilterChain::key_type(0), ctx));
    } else {
      FilterChain::key_type key = m_d->m_chain.rbegin()->first;
      result = m_d->m_chain.insert(std::make_pair(key+1, ctx));
    }
    return result.second;
  }

  bool PostProcessChain::insert(PostProcessContextPtr ctx, unsigned index)
  {
    std::pair<FilterChain::iterator,bool> result =
        m_d->m_chain.insert(std::make_pair(index, ctx));
    return result.second;
  }

  bool PostProcessChain::contains(unsigned index) const
  {
    return m_d->m_chain.find(index) != m_d->m_chain.end();
  }

  PostProcessChain::FilterIterator PostProcessChain::begin()
  {
    return FilterIterator(m_d->m_chain.begin(), m_d->m_chain.end());
  }

  PostProcessChain::ConstFilterIterator PostProcessChain::begin() const
  {
    return ConstFilterIterator(m_d->m_chain.begin(), m_d->m_chain.end());
  }

  PostProcessChain::FilterIterator PostProcessChain::end()
  {
    return FilterIterator(m_d->m_chain.end(), m_d->m_chain.end());
  }

  PostProcessChain::ConstFilterIterator PostProcessChain::end() const
  {
    return ConstFilterIterator(m_d->m_chain.end(), m_d->m_chain.end());
  }

  bool PostProcessChain::empty() const
  {
    return begin() == end();
  }

  size_t PostProcessChain::numEnabledFilters() const
  {
    ConstFilterIterator it = begin();
    size_t i = 0;
    while(it != end()) {
      ++it;
      i++;
    }
    return i;
  }

  size_t PostProcessChain::size() const
  {
    return m_d->m_chain.size();
  }
}
