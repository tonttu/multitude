/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef POSTPROCESSCHAIN_HPP
#define POSTPROCESSCHAIN_HPP

/// @cond

#include "PostProcessContext.hpp"

#include <map>
#include <iterator>

namespace Luminous
{
  /** PostProcessChain is a container of post process filters. The class
      is used to add new filters and iterate through enabled filters.
      This class shouldn't be used directly, but rather filters should
      be added to the system filter chain through
      Application::addPostProcessFilter.
  */
  class LUMINOUS_API PostProcessChain
  {
  public:
    /// Reserved filter indices for the system
    enum SystemFilters {
      Color_Correction = (1 << 20)
    };

    PostProcessChain();
    virtual ~PostProcessChain();

    /// Container for filters
    typedef std::multimap<unsigned, PostProcessContextPtr> FilterChain;

    /// An iterator that skips disabled filters
    template <class IteratorType>
    class FilterIteratorT : public std::iterator<std::forward_iterator_tag,
        typename std::iterator_traits<IteratorType>::value_type>
    {
    public:
      /// Constructs an empty iterator
      FilterIteratorT() { }

      /// Constructs an past-the-end iterator
      explicit FilterIteratorT(IteratorType end)
        : m_current(end), m_end(end)
      {
      }

      /// Constructs an iterator with given range
      /// @param begin Iterator to the first element
      /// @param end Iterator after the last element
      FilterIteratorT(IteratorType begin, IteratorType end)
        : m_current(begin), m_end(end)
      {
        while(m_current != end && !m_current->second->enabled())
          m_current++;
      }

      /// Dereference operator
      const FilterChain::mapped_type & operator*()  const { return m_current->second;  }
      /// Arrow operator
      FilterChain::mapped_type operator->() const { return m_current->second; }

      /// Prefix increment operator (++it)
      FilterIteratorT & operator++()
      {
        do {
          ++m_current;
        } while(m_current != m_end && !m_current->second->enabled());
        return *this;
      }

      /// Postfix increment operator (it++)
      FilterIteratorT operator++(int)
      {
        FilterIteratorT it(*this);
        ++*this;
        return it;
      }

      /// Check if the iterators point to the same filter
      friend bool operator==(const FilterIteratorT & lhs,
                             const FilterIteratorT & rhs)
      {
        return lhs.m_current == rhs.m_current;
      }

      /// Check if the iterators point to the different filters
      friend bool operator!=(const FilterIteratorT& lhs,
                             const FilterIteratorT& rhs)
      {
        return !(lhs == rhs);
      }

    private:
      IteratorType m_current;
      IteratorType m_end;
    };

    typedef FilterIteratorT<FilterChain::iterator> FilterIterator;
    typedef FilterIteratorT<FilterChain::const_iterator> ConstFilterIterator;

    /// Returns the iterator to the first enabled filter
    FilterIterator begin();
    /// @copydoc begin
    ConstFilterIterator begin() const;

    /// Returns the iterator to the element after the last enabled filter
    FilterIterator end();
    /// @copydoc end
    ConstFilterIterator end() const;

    /// Inserts a filter into the chain. The position in the chain is specified
    /// by the order attribute of the filter. Items with the same order are preserved
    /// in the order they were added in.
    void insert(PostProcessContextPtr ctx);

    /// Checks if the given filter type is present in the filters.
    /// @param type Type info of the filter
    /// @return true if the filter is present
    bool hasFilterType(const std::type_info & type);

    /// Checks if the given filter type is present in the filters.
    /// @see hasFilterType(const std::type_info&)
    /// @return true if the filter is present
    template <typename T>
    bool hasFilterType() { return hasFilterType(typeid(T)); }

    /// Checks if a filter instance already exists in the chain
    bool contains(const PostProcessFilterPtr & filter) const;

    /// Gets the context related to the given filter
    /// @param filter filter to search for
    /// @return context for the given filter
    PostProcessContextPtr get(const PostProcessFilterPtr & filter) const;

    /// Checks if the chain is empty
    /// @return true if chain is empty or only contains disabled filters
    bool empty() const;

    /// Returns the number of enabled filters
    size_t numEnabledFilters() const;

    /// Returns the size of the chain (including disabled filters)
    size_t size() const;

  private:
    class D;
    D * m_d;

    void prepare();

    friend class RenderContext;
  };
}

/// @endcond

#endif // POSTPROCESSCHAIN_HPP
