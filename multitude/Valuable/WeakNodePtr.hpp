#pragma once

#include "Node.hpp"

namespace Valuable
{
  /// Relatively safe way to hold a raw pointer to a Node.
  /// The wrapped pointer is automatically set to null once the object is deleted.
  ///
  /// This is not thread-safe nor it will work properly if used inside its own
  /// destructor chain before reaching to ~Node().
  template <typename T>
  class WeakNodePtrT
  {
  public:
    WeakNodePtrT(T * node = nullptr)
      : m_node(node ? std::static_pointer_cast<T>(node->sharedPtr()) : nullptr)
    {}

    WeakNodePtrT(const WeakNodePtrT<T> & o) = default;
    WeakNodePtrT(WeakNodePtrT<T> && o) = default;
    WeakNodePtrT & operator=(const WeakNodePtrT<T> & o) = default;
    WeakNodePtrT & operator=(WeakNodePtrT<T> && o) = default;

    /// Returns the wrapped node, or null if has been deleted already
    T * operator*() const
    {
      T* node = m_node.lock().get();
#ifdef ENABLE_THREAD_CHECKS
      // While WeakNodePtr can be moved between threads, it is not safe to
      // access its content from thread different than its owner.
      if(node) {
        REQUIRE_THREAD(node->m_ownerThread);
      }
#endif
      return node;
    }

    void reset(T * node = nullptr)
    {
      if (node)
        m_node = std::static_pointer_cast<T>(node->sharedPtr());
      else
        m_node.reset();
    }

  private:
    std::weak_ptr<T> m_node;
  };
}
