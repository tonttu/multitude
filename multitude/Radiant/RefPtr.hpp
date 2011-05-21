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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "Radiant/Platform.hpp"

#include <memory>

#if defined(__GCCXML__)
namespace std {
template<class T> class shared_ptr {
public:
  typedef T element_type;

  shared_ptr(); // never throws
  template<class Y> explicit shared_ptr(Y * p);
  template<class Y, class D> shared_ptr(Y * p, D d);
  template<class Y, class D, class A> shared_ptr(Y * p, D d, A a);
  ~shared_ptr(); // never throws

  shared_ptr(shared_ptr const & r); // never throws
  template<class Y> shared_ptr(shared_ptr<Y> const & r); // never throws
  template<class Y> shared_ptr(shared_ptr<Y> const & r, T * p); // never throws
  template<class Y> explicit shared_ptr(std::auto_ptr<Y> & r);

  shared_ptr & operator=(shared_ptr const & r); // never throws
  template<class Y> shared_ptr & operator=(shared_ptr<Y> const & r); // never throws
  template<class Y> shared_ptr & operator=(std::auto_ptr<Y> & r);

  void reset(); // never throws
  template<class Y> void reset(Y * p);
  template<class Y, class D> void reset(Y * p, D d);
  template<class Y, class D, class A> void reset(Y * p, D d, A a);
  template<class Y> void reset(shared_ptr<Y> const & r, T * p); // never throws

  T & operator*() const; // never throws
  T * operator->() const; // never throws
  T * get() const; // never throws

  operator bool() const;

  bool unique() const; // never throws
  long use_count() const; // never throws

  void swap(shared_ptr & b); // never throws
};
template<class T> class weak_ptr {

public:
  typedef T element_type;

  weak_ptr();

  template<class Y> weak_ptr(shared_ptr<Y> const & r);
  weak_ptr(weak_ptr const & r);
  template<class Y> weak_ptr(weak_ptr<Y> const & r);

  ~weak_ptr();

  weak_ptr & operator=(weak_ptr const & r);
  template<class Y> weak_ptr & operator=(weak_ptr<Y> const & r);
  template<class Y> weak_ptr & operator=(shared_ptr<Y> const & r);

  long use_count() const;
  bool expired() const;
  shared_ptr<T> lock() const;

  void reset();
  void swap(weak_ptr<T> & b);
};

template<typename Signature  // Function type R (T1, T2, ..., TN)
         >
class function {
public:
  // construct/copy/destruct
  function();
  function(const function&);
  ~function();

  // invocation
  //result_type operator()(arg1_type, arg2_type, ..., argN_type) const;
};


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
      // assert(m_ptr);
      return *m_ptr;
    }
    const T & operator* () const
    {
      // assert(m_ptr);
      return *m_ptr;
    }

    T * operator-> ()
    {
      // assert(m_ptr);
      return m_ptr;
    }
    const T * operator-> () const
    {
      // assert(m_ptr);
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

#endif // DOXYGEN_SHOULD_SKIP_THIS

#endif
