/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#pragma once

#include <memory>

namespace Valuable
{
  /// Thread-safe wrapper for lazily-created dummy shared_ptr. This can be used
  /// with Valuable::Event class as a listener owner. It basically serves the
  /// same purpose as Valuable::ListenerHolder, but this one is lightweight.
  ///
  /// Example:
  ///
  /// struct MyClass
  /// {
  ///   Valuable::Reference listeners;
  ///   void init()
  ///   {
  ///     // this will get invalidated automatically when 'listeners' is destroyed.
  ///     onSomeEvent.addListener(listeners.weak(), callback);
  ///   }
  /// };
  class Reference
  {
  public:
    inline std::weak_ptr<void> weak();
    /// Invalidate the reference and all listeners bound to it, next call to
    /// weak() will recreate a new one.
    inline void reset();
    /// Returns false once weak() has been called, until reset() is called.
    inline bool isNull() const;

  private:
    std::shared_ptr<void> m_ref;
  };

  ///////////////////////////////////////////////////////////////////////////////

  std::weak_ptr<void> Reference::weak()
  {
    std::shared_ptr<void> ptr = std::atomic_load(&m_ref);
    if (ptr)
      return ptr;

    std::shared_ptr<void> newPtr(this, [] (void *) {});
    if (std::atomic_compare_exchange_strong(&m_ref, &ptr, newPtr)) {
      return newPtr;
    } else {
      return ptr;
    }
  }

  void Reference::reset()
  {
    std::atomic_store(&m_ref, std::shared_ptr<void>{});
  }

  bool Reference::isNull() const
  {
    return std::atomic_load(&m_ref) == nullptr;
  }
}
