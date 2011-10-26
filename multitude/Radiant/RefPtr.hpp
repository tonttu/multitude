/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RADIANT_REF_PTR_HPP
#define RADIANT_REF_PTR_HPP

/// @cond

#include "Radiant/Platform.hpp"

#include <cstddef>
#include <cassert>

// try to detect C++11
#if defined(RADIANT_CXX11)
  #include <memory>
#elif defined(__GCCXML__)
  #include <generator/gccxml_tr1.hpp>
#else
  #if defined(__GNUC__) || defined(RADIANT_LINUX) || defined(RADIANT_OSX)
    #include <tr1/memory>
  #elif defined(RADIANT_WINDOWS) && defined(_HAS_TR1)
    #include <memory>
  #else
    #include <boost/tr1/memory.hpp>
  #endif
  namespace std
  {
    using tr1::shared_ptr;
    using tr1::weak_ptr;
    using tr1::swap;
    using tr1::get_deleter;
    using tr1::static_pointer_cast;
    using tr1::dynamic_pointer_cast;
    using tr1::const_pointer_cast;
    using tr1::enable_shared_from_this;
  }
#endif

namespace Radiant
{
  template <typename T>
  class IntrusivePtr
  {
  public:
    typedef T element_type;

    IntrusivePtr() : m_ptr(0) {}
    IntrusivePtr(T * ptr) : m_ptr(ptr)
    {
      if(ptr) ptr->ref();
    }

    IntrusivePtr(const IntrusivePtr<T> & iptr) : m_ptr(iptr.m_ptr)
    {
      if(m_ptr) m_ptr->ref();
    }

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y> & iptr) : m_ptr(iptr.get())
    {
      if(m_ptr) m_ptr->ref();
    }

    virtual ~IntrusivePtr()
    {
      deref();
    }

    IntrusivePtr<T> & operator= (const IntrusivePtr<T> & iptr)
    {
      deref();
      ref(iptr.m_ptr);
      return *this;
    }

    template <typename Y>
    IntrusivePtr<T> & operator= (const IntrusivePtr<Y> & iptr)
    {
      deref();
      ref(iptr.get());
      return *this;
    }

    IntrusivePtr<T> & operator= (T * ptr)
    {
      deref();
      ref(ptr);
      return *this;
    }

    template <typename Y>
    IntrusivePtr<Y> cast()
    {
      return IntrusivePtr<Y>(dynamic_cast<Y*>(m_ptr));
    }

    T & operator* ()
    {
      assert(m_ptr);
      return *m_ptr;
    }
    const T & operator* () const
    {
      assert(m_ptr);
      return *m_ptr;
    }

    T * operator-> ()
    {
      assert(m_ptr);
      return m_ptr;
    }
    const T * operator-> () const
    {
      assert(m_ptr);
      return m_ptr;
    }

    inline T * get() const { return m_ptr; }

    /// Implicit "bool" conversion
    typedef T * IntrusivePtr::*bool_type;
    operator bool_type() const { return m_ptr ? &IntrusivePtr<T>::m_ptr : 0; }

    bool operator! () const { return m_ptr == 0; }

    template <typename Y>
    bool operator== (const Y * ptr) const { return m_ptr == ptr; }
    template <typename Y>
    bool operator!= (const Y * ptr) const { return m_ptr != ptr; }

    template <typename Y>
    bool operator== (const IntrusivePtr<Y> & that) const { return m_ptr == that.m_ptr; }
    template <typename Y>
    bool operator!= (const IntrusivePtr<Y> & that) const { return m_ptr != that.m_ptr; }

    template <typename Y>
    bool operator< (const IntrusivePtr<Y> & ptr) const { return m_ptr < ptr.get(); }

  private:
    inline void deref()
    {
      if(m_ptr && !m_ptr->deref())
        delete m_ptr;
    }
    inline void ref(T * ptr)
    {
      m_ptr = ptr;
      if(ptr) ptr->ref();
    }

    T * m_ptr;
  };
}

/// @endcond

#endif
