/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#if !defined (RADIANT_INTRUSIVEPTR_HPP)
#define RADIANT_INTRUSIVEPTR_HPP

#include "Platform.hpp"
#include "SafeBool.hpp"

#include <cassert>
#include <functional>

#include <QAtomicInt>

#ifdef INTRUSIVE_PTR_DEBUG

#include "Export.hpp"
#include "CallStack.hpp"

#include <QByteArray>

#include <map>
// for std::nullptr_t
#include <cstddef>

#define INTRUSIVE_PTR_DEBUG_ACQUIRE \
  if(m_counter) IntrusivePtrDebug::add(m_counter, this, typeid(*m_ptr))

#define INTRUSIVE_PTR_DEBUG_RELEASE \
  IntrusivePtrDebug::remove(m_counter, this)

#define INTRUSIVE_PTR_DEBUG_MOVE \
  IntrusivePtrDebug::move(m_counter, &iptr, this)

namespace Radiant
{
  struct IntrusivePtrCounter;
  namespace IntrusivePtrDebug
  {
    struct CallMap
    {
      QByteArray name;
      std::map<const void *, Radiant::CallStack> links;
    };

    typedef std::map<const void *, CallMap> CallMapDB;

    RADIANT_API CallMap fetch(const Radiant::IntrusivePtrCounter * counter);
    RADIANT_API CallMapDB db();
    RADIANT_API void add(const Radiant::IntrusivePtrCounter * counter, const void * intrusivePtr, const std::type_info & type);
    RADIANT_API void move(const Radiant::IntrusivePtrCounter * counter, const void * intrusivePtrFrom, const void * intrusivePtrTo);
    RADIANT_API void remove(const Radiant::IntrusivePtrCounter * counter, const void * intrusivePtr);
  }
}

#else
#define INTRUSIVE_PTR_DEBUG_ACQUIRE
#define INTRUSIVE_PTR_DEBUG_RELEASE
#define INTRUSIVE_PTR_DEBUG_MOVE
#endif

Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline uint qHash(uintptr_t, uint seed) Q_DECL_NOTHROW;

namespace Radiant
{
  /// This is also declared in &lt;cstddef&gt; in std-namespace, but if you /
  /// any other library (Xlib!) happen to include &lt;stddef.h&gt;, then it isn't
  /// found anymore. Use this to be safe.
  typedef decltype(nullptr) nullptr_t;

  /// @cond

  struct IntrusivePtrCounter
  {
    IntrusivePtrCounter() : useCount(0), weakCount(1) {}
    QAtomicInt useCount;
    QAtomicInt weakCount;
  };

  /// @endcond

  template <typename T>
  class IntrusivePtr;

  /// This class implements weak pointers for the IntrusivePtr class. The weak
  /// associated to an intrusive pointer. Whenever the contents of the weak
  /// pointer are accessed, it must be converted to an intrusive pointer with
  /// the lock() method or a special constructor. If the intrusive pointer
  /// associated with this weak pointer is released, the conversion will fail
  /// and an intrusive pointer to nullptr is returned. In order to use
  /// intrusive pointers with custom objects, a single function @em
  /// intrusivePtrGetCounter must be defined for the wrapped object type.
  template <typename T>
  class IntrusiveWeakPtr
  {
  public:
    /// Type of the object pointed to
    typedef T element_type;

    template <typename Y> friend class IntrusivePtr;
    template <typename Y> friend class IntrusiveWeakPtr;

    /// Null constructor
    IntrusiveWeakPtr() : m_ptr(nullptr), m_counter(nullptr) {}

    /// Construct a weak pointer from the given raw pointer
    /// @param ptr raw pointer to construct from
    /// @tparam Y Type of the object whose pointer is stored in this pointer
    template <typename Y>
    explicit IntrusiveWeakPtr(Y * ptr) : m_ptr(ptr), m_counter(nullptr)
    {
      if(ptr) {
        m_counter = intrusivePtrGetCounter(*ptr);
        m_counter->weakCount.ref();
      }
    }

    /// Construct an intrusive weak pointer from the given intrusive pointer
    /// @param iptr intrusive pointer to convert from
    /// @tparam Y Type of the object whose pointer is stored in this pointer
    template <typename Y>
    IntrusiveWeakPtr(const IntrusivePtr<Y> & iptr);

    /// Construct a copy of the given weak pointer
    /// @param wptr weak pointer to copy
    IntrusiveWeakPtr(const IntrusiveWeakPtr<T> & wptr) : m_ptr(wptr.m_ptr), m_counter(wptr.m_counter)
    {
      if(m_counter)
        m_counter->weakCount.ref();
    }

    /// Construct a copy of the given intrusive weak pointer
    /// @param wptr weak pointer to copy
    /// @tparam Y Type of the object whose pointer is stored in this pointer
    template <typename Y>
    IntrusiveWeakPtr(const IntrusiveWeakPtr<Y> & wptr) : m_ptr(wptr.m_ptr), m_counter(wptr.m_counter)
    {
      if(m_counter)
        m_counter->weakCount.ref();
    }

    /// Move the given weak pointer
    /// @param wptr weak pointer to move
    IntrusiveWeakPtr(IntrusiveWeakPtr<T> && wptr) noexcept : m_ptr(wptr.m_ptr), m_counter(wptr.m_counter)
    {
      wptr.m_ptr = nullptr;
      wptr.m_counter = nullptr;
    }

    /// Move the given weak pointer
    /// @param wptr weak pointer to move
    /// @tparam Y Type of the object whose pointer is stored in this pointer
    template <typename Y>
    IntrusiveWeakPtr(IntrusiveWeakPtr<Y> && wptr) noexcept : m_ptr(wptr.m_ptr), m_counter(wptr.m_counter)
    {
      wptr.m_ptr = nullptr;
      wptr.m_counter = nullptr;
    }

    /// Assign the given intrusive weak pointer
    /// @param wptr intrusive weak pointer to assign
    /// @return reference to this
    IntrusiveWeakPtr & operator=(const IntrusiveWeakPtr<T> & wptr)
    {
      deref();
      m_ptr = wptr.m_ptr;
      m_counter = wptr.m_counter;
      if(m_counter)
        m_counter->weakCount.ref();
      return *this;
    }

    /// Assign the given intrusive weak pointer
    /// @param wptr intrusive weak pointer to assign
    /// @return reference to this
    /// @tparam Y Type of the object whose pointer is stored in this pointer
    template <typename Y>
    IntrusiveWeakPtr & operator=(const IntrusiveWeakPtr<Y> & wptr)
    {
      deref();
      m_ptr = wptr.m_ptr;
      m_counter = wptr.m_counter;
      if(m_counter)
        m_counter->weakCount.ref();
      return *this;
    }

    /// Move the given intrusive weak pointer
    /// @param wptr intrusive weak pointer to move
    /// @return reference to this
    IntrusiveWeakPtr & operator=(IntrusiveWeakPtr<T> && wptr) noexcept
    {
      deref();
      m_ptr = wptr.m_ptr;
      m_counter = wptr.m_counter;
      wptr.m_ptr = nullptr;
      wptr.m_counter = nullptr;
      return *this;
    }

    /// Move the given intrusive weak pointer
    /// @param wptr intrusive weak pointer to move
    /// @return reference to this
    /// @tparam Y Type of the object whose pointer is stored in this pointer
    template <typename Y>
    IntrusiveWeakPtr & operator=(IntrusiveWeakPtr<Y> && wptr) noexcept
    {
      deref();
      m_ptr = wptr.m_ptr;
      m_counter = wptr.m_counter;
      wptr.m_ptr = nullptr;
      wptr.m_counter = nullptr;
      return *this;
    }

    /// Destructor
    ~IntrusiveWeakPtr() noexcept
    {
      deref();
    }

    /// Convert the intrusive weak pointer to intrusive pointer. This function
    /// will return an intrusive pointer to the object pointed by this intrusive
    /// weak pointer. If the object has been released an intrusive pointer to
    /// nullptr is returned.
    /// @return intrusive pointer to the object or to nullptr
    /// @tparam Y Type of the object that is pointed to by return value
    template <typename Y>
    IntrusivePtr<Y> lock() const;

    /// Convert the intrusive weak pointer to intrusive pointer. This function
    /// will return an intrusive pointer to the object pointed by this intrusive
    /// weak pointer. If the object has been released an intrusive pointer to
    /// nullptr is returned.
    /// @return intrusive pointer to the object or to nullptr
    IntrusivePtr<T> lock() const;

    /// Compare two intrusive weak pointers
    /// @param rhs intrusive weak pointer to compare
    /// @return true if this pointer is less than the given pointer; otherwise false
    /// @tparam Type of the object pointed by parameter pointer
    template <typename Y>
    inline bool operator< (const IntrusiveWeakPtr<Y> & rhs) const { return m_counter < rhs.m_counter; }

    /// Compare if two intrusive weak pointers are equal
    /// @param rhs intrusive weak pointer to compare
    /// @return true if the pointers are equal; otherwise false
    /// @tparam Type of the object pointed by parameter pointer
    template <typename Y>
    inline bool operator==(const IntrusiveWeakPtr<Y> & rhs) const { return m_counter == rhs.m_counter; }

    /// Compare if two intrusive weak pointers are inequal
    /// @param rhs intrusive weak pointer to compare
    /// @return true if the pointers are inequal; otherwise false
    /// @tparam Type of the object pointed by parameter pointer
    template <typename Y>
    inline bool operator!=(const IntrusiveWeakPtr<Y> & rhs) const { return m_counter != rhs.m_counter; }

    /// Reset the intrusive weak pointer to nullptr
    void reset()
    {
      deref();
      m_ptr = nullptr;
      m_counter = nullptr;
    }

    /// @returns the counter object, can be null
    const IntrusivePtrCounter * counter() const { return m_counter; }

  private:
    inline void deref()
    {
      if(m_counter && !m_counter->weakCount.deref())
        delete m_counter;
    }

  private:
    T * m_ptr;
    IntrusivePtrCounter * m_counter;
  };

  /// This class implements an intrusive pointer. Intrusive pointer is a light
  /// version of a shared pointer. The dynamically allocated object the pointer
  /// points to handles the reference counter. Thus the reference count is
  /// stored in a single location and the pointer size is kept to a minimum.
  template <typename T>
  class IntrusivePtr : public SafeBool< IntrusivePtr<T> >
  {
  public:
    /// Type of the object pointed to
    typedef T element_type;

    template <typename Y> friend class IntrusivePtr;
    template <typename Y> friend class IntrusiveWeakPtr;

    /// Constructor for null pointer
    IntrusivePtr() : m_ptr(nullptr), m_counter(nullptr) {}

    /// Construct a new intrusive pointer to the given object
    /// @param ptr pointer to the object
    IntrusivePtr(T * ptr) : m_ptr(ptr), m_counter(nullptr)
    {
      if(ptr) {
        m_counter = intrusivePtrGetCounter(*ptr);
        m_counter->useCount.ref();
        INTRUSIVE_PTR_DEBUG_ACQUIRE;
      }
    }

    /// @cond

    template <typename Y>
    IntrusivePtr(Y * ptr, IntrusivePtrCounter * counter) : m_ptr(nullptr), m_counter(nullptr)
    {
      if(ptr) {
        m_ptr = ptr;
        m_counter = counter;
        m_counter->useCount.ref();
        INTRUSIVE_PTR_DEBUG_ACQUIRE;
      }
    }

    /// @endcond

    /// Construct a copy of an intrusive pointer
    /// @param iptr intrusive pointer to copy
    // Need to have explicit version of this, since the template version isn't
    // user-defined copy constructor, and the default copy ctor for T would be
    // used instead of the template one
    IntrusivePtr(const IntrusivePtr<T> & iptr) : m_ptr(iptr.m_ptr), m_counter(iptr.m_counter)
    {
      assert((m_ptr != nullptr) == (m_counter != nullptr));
      if(m_counter) {
        m_counter->useCount.ref();
        INTRUSIVE_PTR_DEBUG_ACQUIRE;
      }
    }

    /// Construct a copy of an intrusive pointer
    /// @param iptr intrusive pointer to copy
    /// @tparam Y Type of the object pointed by parameter pointer
    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y> & iptr) : m_ptr(iptr.m_ptr), m_counter(iptr.m_counter)
    {
      assert((m_ptr != nullptr) == (m_counter != nullptr));
      if(m_counter) {
        m_counter->useCount.ref();
        INTRUSIVE_PTR_DEBUG_ACQUIRE;
      }
    }

    /// A move constructor for intrusive pointers
    /// @param iptr intrusive pointer to move
    IntrusivePtr(IntrusivePtr<T> && iptr) noexcept : m_ptr(iptr.m_ptr), m_counter(iptr.m_counter)
    {
      iptr.m_ptr = nullptr;
      iptr.m_counter = nullptr;
      INTRUSIVE_PTR_DEBUG_MOVE;
    }

    /// A move constructor for intrusive pointers
    /// @param iptr intrusive pointer to move
    /// @tparam Y Type of the object pointed by parameter pointer
    template <typename Y>
    IntrusivePtr(IntrusivePtr<Y> && iptr) noexcept : m_ptr(iptr.m_ptr), m_counter(iptr.m_counter)
    {
      iptr.m_ptr = nullptr;
      iptr.m_counter = nullptr;
      INTRUSIVE_PTR_DEBUG_MOVE;
    }

    /// Construct an intrusive pointer from instrusive weak pointer
    /// @param wptr weak pointer to construct from
    /// @tparam Y Type of the object pointed by parameter pointer
    template <typename Y>
    IntrusivePtr(const IntrusiveWeakPtr<Y> & wptr) : m_ptr(nullptr), m_counter(nullptr) {
      if(wptr.m_counter) {
        int count;
        do {
          count = wptr.m_counter->useCount.load();
          if(count == 0)
            return;
        } while (!wptr.m_counter->useCount.testAndSetOrdered(count, count + 1));
        m_ptr = wptr.m_ptr;
        m_counter = wptr.m_counter;
        INTRUSIVE_PTR_DEBUG_ACQUIRE;
      }
    }

    /// Destructor
    ~IntrusivePtr() noexcept
    {
      deref();
    }

    /// Assign an intrusive pointer
    /// @param iptr intrusive pointer to assign
    /// @return reference to this
    /// @tparam Y Type of the object pointed by parameter pointer
    template <typename Y>
    IntrusivePtr<T> & operator= (const IntrusivePtr<Y> & iptr)
    {
      deref();
      ref(iptr.m_ptr, iptr.m_counter);
      return *this;
    }

    /// Assign an intrusive pointer
    /// @param iptr intrusive pointer to assign
    /// @return reference to this
    IntrusivePtr<T> & operator= (const IntrusivePtr<T> & iptr)
    {
      deref();
      ref(iptr.m_ptr, iptr.m_counter);
      return *this;
    }

    /// Move an intrusive pointer
    /// @param iptr intrusive pointer to move
    /// @return reference to this
    IntrusivePtr<T> & operator= (IntrusivePtr<T> && iptr) noexcept
    {
      deref();
      m_ptr = iptr.m_ptr;
      m_counter = iptr.m_counter;
      iptr.m_ptr = nullptr;
      iptr.m_counter = nullptr;
      INTRUSIVE_PTR_DEBUG_MOVE;
      return *this;
    }

    /// Move an intrusive pointer
    /// @param iptr intrusive pointer to move
    /// @return reference to this
    /// @tparam Y Type of the object pointed by parameter pointer
    template <typename Y>
    IntrusivePtr<T> & operator= (IntrusivePtr<Y> && iptr) noexcept
    {
      deref();
      m_ptr = iptr.m_ptr;
      m_counter = iptr.m_counter;
      iptr.m_ptr = nullptr;
      iptr.m_counter = nullptr;
      INTRUSIVE_PTR_DEBUG_MOVE;
      return *this;
    }

    /// Reset the intrusive pointer to given object
    /// @param ptr object to reset the pointer to
    /// @tparam Y Type of the object pointed by parameter pointer
    template <typename Y>
    void reset(Y * ptr)
    {
      deref();
      m_ptr = ptr;
      if(ptr) {
        m_counter = intrusivePtrGetCounter(*ptr);
        m_counter->useCount.ref();
        INTRUSIVE_PTR_DEBUG_ACQUIRE;
      } else {
        m_counter = nullptr;
      }
    }

    /// Reset the intrusive pointer to nullptr
    void reset()
    {
      deref();
      m_ptr = nullptr;
      m_counter = nullptr;
    }

    /// Shortcut for creating weak pointer from this
    IntrusiveWeakPtr<T> weak() const
    {
      return IntrusiveWeakPtr<T>(*this);
    }

    /// Assign a raw pointer to the intrusive pointer
    /// @param ptr raw pointer to assign
    /// @return reference to this
    IntrusivePtr<T> & operator= (T * ptr)
    {
      reset(ptr);
      return *this;
    }

    /// Perform static_cast to another intrusive pointer type
    /// @return intrusive pointer to another type
    /// @tparam Y Type of the object pointed by pointer returned
    template <typename Y>
    IntrusivePtr<Y> static_pointer_cast() const
    {
      return IntrusivePtr<Y>(static_cast<Y*>(m_ptr), m_counter);
    }

    /// Perform a dynamic_cast to another intrusive pointer type
    /// @return intrusive pointer to different type or to nullptr if the cast fails
    /// @tparam Y Type of the object pointed by pointer returned
    template<typename Y>
    IntrusivePtr<Y> dynamic_pointer_cast() const
    {
      return IntrusivePtr<Y>(dynamic_cast<Y*>(m_ptr), m_counter);
    }

    /// Convert the intrusive pointer to raw pointer
    /// @return raw pointer to the object
    T & operator* () const
    {
      assert(m_ptr);
      return *m_ptr;
    }

    /// Dereference the intrusive pointer
    /// @return raw pointer to the object
    T * operator-> () const
    {
      assert(m_ptr);
      return m_ptr;
    }

    /// Used to implemented the safe-bool idiom for intrusive pointers.
    /// @return true if the pointer is nullptr; otherwise false
    bool boolean_test() const
    {
      return m_ptr!=nullptr;
    }

    /// Check if the intrusive pointer is null
    /// @return true if the intrusive pointer is nullptr; otherwise false
    bool operator! () const { return m_ptr == nullptr; }

    /// @cond

    // Do not use unless you know exactly what you are doing
    T * unsafeRaw() const { return m_ptr; }

    /// @endcond

    // These operators must be inside the class so that they can access m_ptr
    // Using &* -hack will crash the application with null pointers, and
    // adding friends is uglier.

    /// Compare if two intrusive pointers are equal
    /// @param rhs intrusive pointer to compare
    /// @return true if the pointers are equal; otherwise false
    /// @tparam Y Type of the object pointed by parameter pointer
    template <typename Y>
    inline bool operator==(const IntrusivePtr<Y> & rhs) const { return m_counter == rhs.m_counter; }

    /// Compare if two intrusive pointers are inequal
    /// @param rhs intrusive pointer to compare
    /// @return true if the pointers are inequal; otherwise false
    /// @tparam Y Type of the object pointed by parameter pointer
    template <typename Y>
    inline bool operator!=(const IntrusivePtr<Y> & rhs) const { return m_counter != rhs.m_counter; }

    /// Compare if intrusive pointer is equal to the given intrusive weak pointer
    /// @param rhs intrusive weak pointer to compare
    /// @return true if the pointers are equal; otherwise false
    /// @internal Compares counters instead of pointers, since the m_ptr in weak ptr
    /// might be old dangling / reallocated pointer
    /// @tparam Y Type of the object pointed by parameter pointer
    template <typename Y>
    inline bool operator==(const IntrusiveWeakPtr<Y> & rhs) const { return m_counter == rhs.m_counter; }

    /// Compare if intrusive pointer is inequal to the given intrusive weak pointer
    /// @param rhs intrusive weak pointer to compare
    /// @return true if the pointers are inequal; otherwise false
    /// @tparam Y Type of the object pointed by parameter pointer
    template <typename Y>
    inline bool operator!=(const IntrusiveWeakPtr<Y> & rhs) const { return m_counter != rhs.m_counter; }

    /// Compare if the intrusive pointer is equal to the given raw pointer
    /// @param rhs raw pointer to compare to
    /// @return true if the pointers are equal; otherwise false
    /// @tparam Y Type of the object pointed by parameter pointer
    template <typename Y>
    inline bool operator== (const Y * rhs) const { return m_counter == intrusivePtrGetCounter(*rhs); }

    /// Compare if the intrusive pointer is equal to nullptr
    /// @return true if the intrusive pointer is nullptr
    inline bool operator== (nullptr_t) const { return m_ptr == nullptr; }
    /// Compare if the intrusive pointer is inequal to nullptr
    /// @return true if the intrusive pointer is nullptr
    inline bool operator!= (nullptr_t) const { return m_ptr != nullptr; }

    /// Compare if the intrusive pointer is inequal to the given raw pointer
    /// @param rhs raw pointer to compare to
    /// @return true if the pointers are inequal; otherwise false
    /// @tparam Y Type of the object pointed by parameter pointer
    template <typename Y>
    inline bool operator!= (const Y * rhs) const { return m_counter != intrusivePtrGetCounter(*rhs); }

    /// Compare the order of two intrusive pointers
    /// @param rhs intrusive pointer to compare
    /// @return true if this intrusive pointer is less than the given intrusive pointer; otherwise false
    /// @tparam Y Type of the object pointed by parameter pointer
    template <typename Y>
    inline bool operator< (const IntrusivePtr<Y> & rhs) const { return m_counter < rhs.m_counter; }

    /// Compare the intrusive pointer to a raw pointer
    /// @param rhs raw pointer to compare
    /// @return true if this intrusive pointer is less than the given raw pointer; otherwise false
    /// @tparam Y Type of the object pointed by parameter pointer
    template <typename Y>
    inline bool operator< (const Y * rhs) const { return m_counter < intrusivePtrGetCounter(*rhs); }

    /// @returns the counter object, can be null
    const IntrusivePtrCounter * counter() const { return m_counter; }

  private:
    inline void deref()
    {
      if(m_counter) {
        assert(m_counter->useCount > 0);
        INTRUSIVE_PTR_DEBUG_RELEASE;
        if(!m_counter->useCount.deref()) {
          intrusivePtrRelease(m_ptr);
          if(!m_counter->weakCount.deref())
            delete m_counter;
        }
      }
    }
    inline void ref(T * ptr, IntrusivePtrCounter * counter)
    {
      m_ptr = ptr;
      m_counter = counter;
      if(m_counter)
        m_counter->useCount.ref();
      INTRUSIVE_PTR_DEBUG_ACQUIRE;
    }

  private:
    T * m_ptr;
    IntrusivePtrCounter * m_counter;
  };

  template <typename T>
  template <typename Y>
  IntrusiveWeakPtr<T>::IntrusiveWeakPtr(const IntrusivePtr<Y> & iptr) : m_ptr(iptr.m_ptr), m_counter(iptr.m_counter)
  {
    if(m_counter)
      m_counter->weakCount.ref();
  }

  template <typename T>
  template <typename Y>
  IntrusivePtr<Y> IntrusiveWeakPtr<T>::lock() const
  {
    return *this;
  }

  template <typename T>
  IntrusivePtr<T> IntrusiveWeakPtr<T>::lock() const
  {
    return *this;
  }

  /// Check if a raw pointer and an intrusive pointer are equal
  /// @param lhs Left side operand
  /// @param rhs Right side operand
  /// @return True if pointers are equal, false otherwise
  /// @tparam T Type of the object pointed by intrusive pointer
  /// @tparam Y Type of the object pointed by raw pointer
  template <typename T, typename Y> inline bool operator== (const Y * lhs, const IntrusivePtr<T> & rhs) { return rhs == lhs; }

  /// Check if a raw pointer and an intrusive pointer are inequal
  /// @param lhs Left side operand
  /// @param rhs Right side operand
  /// @return True if pointers are inequal, false otherwise
  /// @tparam T Type of the object pointed by intrusive pointer
  /// @tparam Y Type of the object pointed by raw pointer
  template <typename T, typename Y> inline bool operator!= (const Y * lhs, const IntrusivePtr<T> & rhs) { return rhs != lhs; }

  /// Compare operator for a raw pointer and an instrusive pointer
  /// @param lhs Left side operand
  /// @param rhs Right side operand
  /// @return True if raw pointer is lesser, false otherwise
  /// @tparam T Type of the object pointed by raw pointer
  /// @tparam Y Type of the object pointed by intrusive pointer
  template <typename T, typename Y> inline bool operator< (const T * lhs, const IntrusivePtr<Y> & rhs) { return !(rhs == lhs || rhs < lhs); }

  /// Compare the order of intrusive and weak pointers
  template <typename T, typename Y>
  inline bool operator< (const IntrusivePtr<T> & lhs, const IntrusiveWeakPtr<Y> & rhs) { return lhs.counter() < rhs.counter(); }
  template <typename T, typename Y>
  inline bool operator< (const IntrusiveWeakPtr<T> & lhs, const IntrusivePtr<Y> & rhs) { return lhs.counter() < rhs.counter(); }

  /// Check if nullptr is equal to intrusive pointer
  /// @param rhs Right side operand
  /// @return True if rhs is pointing to nullptr, false otherwise
  /// @tparam T Type of the object pointed by intrusive pointer
  template <typename T> inline bool operator== (nullptr_t, const IntrusivePtr<T> & rhs) { return rhs == nullptr; }

  /// Check if nullptr is inequal to intrusive pointer
  /// @param rhs Right side operand
  /// @return True if rhs is not pointing to nullptr, false otherwise
  /// @tparam T Type of the object pointed by intrusive pointer
  template <typename T> inline bool operator!= (nullptr_t, const IntrusivePtr<T> & rhs) { return rhs != nullptr; }

  /// Check if two different type intrusive pointers are equal
  /// @param lhs Left side operand
  /// @param rhs Right side operand
  /// @return True if both pointers are pointing to same object
  /// @tparam T Type of the object pointed by lhs
  /// @tparam Y Type of the object pointed by rhs
  template <typename T, typename Y> inline bool operator== (const IntrusiveWeakPtr<T> & lhs, const IntrusivePtr<Y> & rhs) { return rhs == lhs; }

  /// Check if two different type intrusive pointers are inequal
  /// @param lhs Left side operand
  /// @param rhs Right side operand
  /// @return True if pointers are not pointing to same object
  /// @tparam T Type of the object pointed by lhs
  /// @tparam Y Type of the object pointed by rhs
  template <typename T, typename Y> inline bool operator!= (const IntrusiveWeakPtr<T> & lhs, const IntrusivePtr<Y> & rhs) { return rhs != lhs; }

  template <typename T>
  inline uint qHash(const IntrusivePtr<T> & k) noexcept
  {
    return ::qHash(reinterpret_cast<uintptr_t>(k.counter()), 0);
  }

  template <typename T>
  inline uint qHash(const IntrusiveWeakPtr<T> & k) noexcept
  {
    return ::qHash(reinterpret_cast<uintptr_t>(k.counter()), 0);
  }
}

namespace std
{
  template <typename T>
  struct hash<Radiant::IntrusivePtr<T>>
  {
    std::size_t operator()(const Radiant::IntrusivePtr<T> & k) const
    {
      return hash<uintptr_t>()(reinterpret_cast<uintptr_t>(k.counter()));
    }
  };

  template <typename T>
  struct hash<Radiant::IntrusiveWeakPtr<T>>
  {
    std::size_t operator()(const Radiant::IntrusiveWeakPtr<T> & k) const
    {
      return hash<uintptr_t>()(reinterpret_cast<uintptr_t>(k.counter()));
    }
  };
}

#endif // RADIANT_INTRUSIVEPTR_HPP
