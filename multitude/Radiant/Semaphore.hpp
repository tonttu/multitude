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

#ifndef RADIANT_SEMAPHORE_HPP
#define RADIANT_SEMAPHORE_HPP

#include <Patterns/NotCopyable.hpp>

namespace Radiant {

    class Semaphore : public Patterns::NotCopyable
    {
    public:
        Semaphore(int n = 0);
        ~Semaphore();

        void acquire(int n = 1);
        void release(int n = 1);

        void tryAcquire(int n = 1);
        void tryAcquire(int n, int timeoutMs);

    private:
        class D;
        D * m_d;
    };

}

#endif
