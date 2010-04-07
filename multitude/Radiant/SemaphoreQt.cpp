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
