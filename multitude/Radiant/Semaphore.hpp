/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_SEMAPHORE_HPP
#define RADIANT_SEMAPHORE_HPP

#include <Patterns/NotCopyable.hpp>
#include "Export.hpp"

namespace Radiant {

  /** Provides a general counting semaphore. */
  class RADIANT_API Semaphore : public Patterns::NotCopyable
  {
  public:
    /// Initialize the semaphore to guard n resources
    /// @param n number of resources to guard (default 0)
    Semaphore(int n = 0);
    /// Destructor
    ~Semaphore();

    /// Try to acquire resources. This will block until available() >= n.
    /// @param n number of resources to acquire
    void acquire(int n = 1);
    /// Release resources
    /// @param n number of resources to release
    void release(int n = 1);

    /// Try to acquire resources. Does not block.
    /// @param n number of resources to acquire
    /// @return true if requested resources were available, otherwise false
    bool tryAcquire(int n = 1);
    /// Try to acquire resources. This call will block for at most the given timeout if available() < n.
    /// @param n number of resources to acquire
    /// @param timeoutMs time to wait for resources if they are not avaible
    /// @return true if requested resources were acquired, otherwise false
    bool tryAcquire(int n, int timeoutMs);

  private:
    class D;
    D * m_d;
  };

}

#endif
