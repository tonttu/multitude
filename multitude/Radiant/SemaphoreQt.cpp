/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Semaphore.hpp"

#include <QSemaphore>

namespace Radiant
{

    class Semaphore::D : public QSemaphore {
    public:
        D(int n) : QSemaphore(n) {}
    };

    Semaphore::Semaphore(int d)
        : m_d(new D(d))
    {
    }

    Semaphore::~Semaphore()
    {
        delete m_d;
    }

    void Semaphore::acquire(int n)
    {
        m_d->acquire(n);
    }

    void Semaphore::release(int n)
    {
        m_d->release(n);
    }

    bool Semaphore::tryAcquire(int n)
    {
        return m_d->tryAcquire(n);
    }

    bool Semaphore::tryAcquire(int n, int timeoutMs)
    {
        return m_d->tryAcquire(n, timeoutMs);
    }


}
