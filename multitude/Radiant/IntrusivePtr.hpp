#if !defined (RADIANT_INTRUSIVEPTR_HPP)
#define RADIANT_INTRUSIVEPTR_HPP

#include <cassert>

//#define INTRUSIVE_PTR_DEBUG
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
  public:
    inline IntrusivePtr( int n ) : m_ptr((T*)nullptr) { (void)n; assert(n==0); }
    inline IntrusivePtr( T * object = nullptr ) : m_ptr(object) { ref(); INTRUSIVE_PTR_DEBUG_ACQUIRE; }

    inline IntrusivePtr( const IntrusivePtr<T> & ptr ) : m_ptr((T*)nullptr) { ref(ptr.get()); INTRUSIVE_PTR_DEBUG_ACQUIRE; }

    template <typename Y> inline IntrusivePtr( const IntrusivePtr<Y> & ptr )
      : m_ptr((T*)nullptr)
    {
      assert(ptr == nullptr || dynamic_cast<T *>(ptr.get()) != nullptr);
      ref(ptr.get());
      INTRUSIVE_PTR_DEBUG_ACQUIRE;
    }

    inline ~IntrusivePtr() { deref(); }

    inline const IntrusivePtr<T> & operator=(const IntrusivePtr<T> & ptr) { ref(ptr.get()); return *this; }
    inline const IntrusivePtr<T> & operator=(T * ptr) { ref(ptr); return *this; }
    inline const IntrusivePtr<T> & operator=(int n) { (void)n; assert(n == 0); ref((T*)nullptr); return *this; }

    /// Implicit "bool" conversion with safe bool idiom
    typedef T * (IntrusivePtr::*bool_type)() const;
    inline operator bool_type() const { return m_ptr ? &IntrusivePtr<T>::get : 0; }

    inline bool operator! () const { return m_ptr == nullptr; }

    inline void reset() { ref((T*)nullptr); }
    inline T * get() const { return m_ptr; }
    template <typename Y> IntrusivePtr<Y> cast() { return IntrusivePtr<Y>(dynamic_cast<Y*>(m_ptr)); }

    inline T * operator->() const { return get(); }

    inline T & operator*() { assert(m_ptr); return *m_ptr; }
    inline const T & operator*() const { assert(m_ptr); return *m_ptr; }

  private:
    inline void ref() const
    {
      if (m_ptr)
        m_ptr->ref();
      INTRUSIVE_PTR_DEBUG_ACQUIRE;
    }

    inline void ref(T * ptr)
    {
      deref();
      m_ptr = ptr;
      ref();
    }

    inline void deref()
    {
      if (m_ptr && m_ptr->deref() == 0) {
        delete m_ptr;
        m_ptr = nullptr;
      }
      INTRUSIVE_PTR_DEBUG_RELEASE;
    }

  private:
    T * m_ptr;
  };

  template <typename T>
  class RefCounted
  {
  public:
    inline virtual ~RefCounted() { assert (m_refCount == 0); }  // Assert if there's stale pointers
    inline const RefCounted<T> & operator=(const RefCounted<T> & rhs ) { m_refCount = 0; }

    inline size_t refCount() const { return m_refCount; }
    inline size_t ref() const { return ++m_refCount; }
    inline size_t deref() const { return --m_refCount; }

  protected:
    inline RefCounted()
      : m_refCount(0)
    {
      /// @todo Will break if we allocate these things on the stack of course. Should we bother?
      /// There's no 100% guaranteed way to prevent stack allocation.
      /// Could overload operator new/delete, but we probably don't want to go there.
      /// Maybe a private ctor and factory function might help
    }
    inline RefCounted( const RefCounted<T> & rhs ) : m_refCount(0) {}

  private:
    mutable size_t m_refCount;
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