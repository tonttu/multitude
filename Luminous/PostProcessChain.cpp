/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "PostProcessChain.hpp"

#include <Luminous/PostProcessFilter.hpp>

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

  void PostProcessChain::insert(PostProcessContextPtr ctx)
  {
    m_d->m_chain.insert(std::make_pair(ctx->order(), ctx));
  }

  bool PostProcessChain::hasFilterType(const std::type_info & type)
  {
    for(FilterChain::iterator it = m_d->m_chain.begin(); it != m_d->m_chain.end(); ++it) {
      auto & ctx = *it->second;
      if(typeid(ctx) == type)
        return true;
    }
    return false;
  }

  bool PostProcessChain::contains(const PostProcessFilterPtr & filter) const
  {
    for(FilterChain::const_iterator it = m_d->m_chain.begin(); it != m_d->m_chain.end(); ++it)
    {
      const PostProcessContext & ptr = *it->second;
      if(ptr.filter() == filter)
         return true;
    }
    return false;
  }

  PostProcessContextPtr PostProcessChain::get(const PostProcessFilterPtr & filter) const
  {
    for(FilterChain::const_iterator it = m_d->m_chain.begin(); it != m_d->m_chain.end(); ++it)
    {
      const PostProcessContextPtr ptr = it->second;
      if(ptr->filter() == filter)
         return ptr;
    }
    return nullptr;
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

  void PostProcessChain::prepare()
  {
    // Reorder chain if necessary (copied from Widget::updateInternal)
    for(FilterChain::iterator it = m_d->m_chain.begin(); it != m_d->m_chain.end(); ) {

      FilterChain::iterator cur = it;
      ++it;

      PostProcessContext & ctx = *cur->second;

      if(ctx.order() != (*cur).first) {
        m_d->m_chain.insert(FilterChain::value_type(ctx.order(), std::move(cur->second)));
        m_d->m_chain.erase(cur);
      }
    }
  }
}
