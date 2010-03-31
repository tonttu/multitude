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

    void Semaphore::tryAcquire(int n)
    {
        m_d->tryAcquire(n);
    }

    void Semaphore::tryAcquire(int n, int timeoutMs)
    {
        m_d->tryAcquire(n, timeoutMs);
    }


}
