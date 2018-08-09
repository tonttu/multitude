#pragma once

#include "Attribute.hpp"

namespace Valuable
{
  /// Relatively safe way to hold a raw pointer to an Attribute / Node.
  /// The wrapped pointer is automatically set to null once the object is deleted.
  ///
  /// This is not thread-safe nor it will work properly if used inside its own
  /// destructor chain before reaching to ~Attribute().
  template <typename T>
  class WeakAttributePtrT
  {
  public:
    WeakAttributePtrT(T * attr = nullptr)
    {
      reset(attr);
    }

    template <typename Y>
    WeakAttributePtrT(Y * attr)
    {
      reset(attr);
    }

    ~WeakAttributePtrT()
    {
      reset();
    }

    WeakAttributePtrT(const WeakAttributePtrT<T> & o)
    {
      reset(o.m_attr);
    }

    template <typename Y>
    WeakAttributePtrT(const WeakAttributePtrT<Y> & o)
    {
      reset(o.m_attr);
    }

    WeakAttributePtrT(WeakAttributePtrT<T> && o)
    {
      reset(o.m_attr);
    }

    template <typename Y>
    WeakAttributePtrT(WeakAttributePtrT<Y> && o)
    {
      reset(o.m_attr);
    }

    WeakAttributePtrT<T> & operator=(const WeakAttributePtrT<T> & o)
    {
      reset(o.m_attr);
      return *this;
    }

    template <typename Y>
    WeakAttributePtrT<T> & operator=(const WeakAttributePtrT<Y> & o)
    {
      reset(o.m_attr);
      return *this;
    }

    WeakAttributePtrT<T> & operator=(WeakAttributePtrT<T> && o)
    {
      reset(o.m_attr);
      return *this;
    }

    template <typename Y>
    WeakAttributePtrT<T> & operator=(WeakAttributePtrT<Y> && o)
    {
      reset(o.m_attr);
      return *this;
    }

    void reset(T * attr = nullptr)
    {
      if (m_attr && m_listenerId >= 0) {
        m_attr->removeListener(m_listenerId);
        m_listenerId = -1;
      }

      m_attr = attr;

      if (m_attr) {
        m_listenerId = m_attr->addListener([this] {
          m_attr = nullptr;
        }, Attribute::DELETE_ROLE);
      }
    }

    /// Returns the wrapped attribute, or null if has been deleted already
    T * operator*() const
    {
      return m_attr;
    }

  private:
    T * m_attr = nullptr;
    long m_listenerId = -1;
  };
}
