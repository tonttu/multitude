#if !defined (RADIANT_INTRUSIVEPTR_HPP)
#define RADIANT_INTRUSIVEPTR_HPP

#include <cassert>

// #define INTRUSIVE_PTR_DEBUG
#ifdef INTRUSIVE_PTR_DEBUG

#include "Export.hpp"
#include "CallStack.hpp"

#include <map>

#define INTRUSIVE_PTR_DEBUG_ACQUIRE \
  IntrusivePtrDebug::add(m_ptr, this)

#define INTRUSIVE_PTR_DEBUG_RELEASE \
  IntrusivePtrDebug::remove(m_ptr, this)

namespace Radiant
{
  namespace IntrusivePtrDebug
  {
    typedef std::map<const void *, Radiant::CallStack> CallMap;

    RADIANT_API CallMap fetch(const void * ptr);
    RADIANT_API void add(const void * ptr, const void * intrusivePtr);
    RADIANT_API void remove(const void * ptr, const void * intrusivePtr);
  };
}

#else
#define INTRUSIVE_PTR_DEBUG_ACQUIRE
#define INTRUSIVE_PTR_DEBUG_RELEASE
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
      if(ptr) {
        intrusive_ptr_add_ref(ptr);
        INTRUSIVE_PTR_DEBUG_ACQUIRE;
      }
    }

    IntrusivePtr(const IntrusivePtr<T> & iptr) : m_ptr(iptr.m_ptr)
    {
      if(m_ptr) {
        intrusive_ptr_add_ref(m_ptr);
        INTRUSIVE_PTR_DEBUG_ACQUIRE;
      }
    }

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y> & iptr) : m_ptr(iptr.get())
    {
      if(m_ptr) {
        intrusive_ptr_add_ref(m_ptr);
        INTRUSIVE_PTR_DEBUG_ACQUIRE;
      }
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

    T & operator* () const
    {
      assert(m_ptr);
      return *m_ptr;
    }

    T * operator-> () const
    {
      assert(m_ptr);
      return m_ptr;
    }

    inline T * get() const { return m_ptr; }

    /// Implicit "bool" conversion with safe bool idiom
    typedef T * (IntrusivePtr::*bool_type)() const;
    operator bool_type() const { return m_ptr ? &IntrusivePtr<T>::get : 0; }

    bool operator! () const { return m_ptr == 0; }

  private:
    inline void deref()
    {
      if(m_ptr) intrusive_ptr_release(m_ptr);
      INTRUSIVE_PTR_DEBUG_RELEASE;
    }
    inline void ref(T * ptr)
    {
      m_ptr = ptr;
      if(ptr) intrusive_ptr_add_ref(ptr);
      INTRUSIVE_PTR_DEBUG_ACQUIRE;
    }

    T * m_ptr;
  };

  template <typename T, typename Y> inline bool operator==( const IntrusivePtr<T> & lhs, const IntrusivePtr<Y> & rhs) { return lhs.get() == rhs.get(); }
  template <typename T, typename Y> inline bool operator!=( const IntrusivePtr<T> & lhs, const IntrusivePtr<Y> & rhs) { return !(lhs == rhs); }

  template <typename T, typename Y> inline bool operator== ( const IntrusivePtr<T> & lhs, const Y * rhs) { return lhs.get() == rhs; }
  template <typename T, typename Y> inline bool operator!= ( const IntrusivePtr<T> & lhs, const Y * rhs) { return !(lhs == rhs); }
  template <typename T, typename Y> inline bool operator== ( const Y * lhs, const IntrusivePtr<T> & rhs) { return rhs == lhs; }
  template <typename T, typename Y> inline bool operator!= ( const Y * lhs, const IntrusivePtr<T> & rhs) { return rhs != lhs; }

  template <typename T, typename Y> inline bool operator< (const IntrusivePtr<T> & lhs, const IntrusivePtr<Y> & rhs) { return lhs.get() < rhs.get(); }
  template <typename T, typename Y> inline bool operator< (const IntrusivePtr<T> & lhs, const Y * rhs) { return lhs.get() < rhs; }
  template <typename T, typename Y> inline bool operator< (const T * lhs, const IntrusivePtr<Y> & rhs) { return lhs < rhs.get(); }
}

#endif // RADIANT_INTRUSIVEPTR_HPP