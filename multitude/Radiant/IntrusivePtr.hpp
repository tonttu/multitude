#if !defined (RADIANT_INTRUSIVEPTR_HPP)
#define RADIANT_INTRUSIVEPTR_HPP

#include <cassert>

// #define INTRUSIVE_PTR_DEBUG
#ifdef INTRUSIVE_PTR_DEBUG

#include "Export.hpp"
#include "CallStack.hpp"

#include <map>
// for std::nullptr_t
#include <cstddef>

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
  /// This is also declared in <cstddef> in std-namespace, but if you /
  /// any other library (Xlib!) happen to include <stddef.h>, then it isn't
  /// found anymore. Use this to be safe.
  typedef decltype(nullptr) nullptr_t;

  template <typename T>
  class IntrusivePtr
  {
  public:
    typedef T element_type;

    template <typename Y> friend class IntrusivePtr;

    IntrusivePtr() : m_ptr(nullptr) {}
    IntrusivePtr(T * ptr) : m_ptr(ptr)
    {
      if(ptr) {
        intrusive_ptr_add_ref(ptr);
        INTRUSIVE_PTR_DEBUG_ACQUIRE;
      }
    }

    // Need to have explicit version of this, since the template version isn't
    // user-defined copy constructor, and the default copy ctor for T would be
    // used instead of the template one
    IntrusivePtr(const IntrusivePtr<T> & iptr) : m_ptr(iptr.m_ptr)
    {
      if(m_ptr) {
        intrusive_ptr_add_ref(m_ptr);
        INTRUSIVE_PTR_DEBUG_ACQUIRE;
      }
    }

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y> & iptr) : m_ptr(iptr.m_ptr)
    {
      if(m_ptr) {
        intrusive_ptr_add_ref(m_ptr);
        INTRUSIVE_PTR_DEBUG_ACQUIRE;
      }
    }

    IntrusivePtr(IntrusivePtr<T> && iptr) : m_ptr(iptr.m_ptr)
    {
      iptr.m_ptr = nullptr;
    }

    template <typename Y>
    IntrusivePtr(IntrusivePtr<Y> && iptr) : m_ptr(iptr.m_ptr)
    {
      iptr.m_ptr = nullptr;
    }

    virtual ~IntrusivePtr()
    {
      deref();
    }

    template <typename Y>
    IntrusivePtr<T> & operator= (const IntrusivePtr<Y> & iptr)
    {
      deref();
      ref(iptr.m_ptr);
      return *this;
    }

    IntrusivePtr<T> & operator= (const IntrusivePtr<T> & iptr)
    {
      deref();
      ref(iptr.m_ptr);
      return *this;
    }

    IntrusivePtr<T> & operator= (IntrusivePtr<T> && iptr)
    {
      deref();
      m_ptr = iptr.m_ptr;
      iptr.m_ptr = nullptr;
      return *this;
    }

    template <typename Y>
    IntrusivePtr<T> & operator= (IntrusivePtr<Y> && iptr)
    {
      deref();
      m_ptr = iptr.m_ptr;
      iptr.m_ptr = nullptr;
      return *this;
    }

    IntrusivePtr<T> & operator= (T * ptr)
    {
      deref();
      ref(ptr);
      return *this;
    }

    template <typename Y>
    IntrusivePtr<Y> static_pointer_cast()
    {
      return IntrusivePtr<Y>(static_cast<Y*>(m_ptr));
    }

    template<typename Y>
    IntrusivePtr<Y> dynamic_pointer_cast() const
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

    explicit operator bool() const
    {
      return m_ptr!=nullptr;
    }

    bool operator! () const { return m_ptr == 0; }

    /// These operators must be inside the class so that they can access m_ptr
    /// Using &* -hack will crash the application with null pointers, and
    /// adding friends is uglier.

    template <typename Y>
    inline bool operator==(const IntrusivePtr<Y> & rhs) const { return m_ptr == rhs.m_ptr; }

    template <typename Y>
    inline bool operator!=(const IntrusivePtr<Y> & rhs) const { return m_ptr != rhs.m_ptr; }

    template <typename Y>
    inline bool operator== (const Y * rhs) const { return m_ptr == rhs; }

    inline bool operator== (nullptr_t) const { return m_ptr == nullptr; }
    inline bool operator!= (nullptr_t) const { return m_ptr != nullptr; }

    template <typename Y>
    inline bool operator!= (const Y * rhs) const { return m_ptr != rhs; }

    template <typename Y>
    inline bool operator< (const IntrusivePtr<Y> & rhs) const { return m_ptr < rhs.m_ptr; }

    template <typename Y>
    inline bool operator< (const Y * rhs) const { return m_ptr < rhs; }

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

  template <typename T, typename Y> inline bool operator== (const Y * lhs, const IntrusivePtr<T> & rhs) { return rhs == lhs; }
  template <typename T, typename Y> inline bool operator!= (const Y * lhs, const IntrusivePtr<T> & rhs) { return rhs != lhs; }
  template <typename T, typename Y> inline bool operator< (const T * lhs, const IntrusivePtr<Y> & rhs) { return !(rhs == lhs || rhs < lhs); }
  template <typename T> inline bool operator== (nullptr_t, const IntrusivePtr<T> & rhs) { return rhs == nullptr; }
  template <typename T> inline bool operator!= (nullptr_t, const IntrusivePtr<T> & rhs) { return rhs != nullptr; }
}

#endif // RADIANT_INTRUSIVEPTR_HPP
