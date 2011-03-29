#ifndef RADIANT_SYNCHRONIZED_QUEUE_HPP
#define RADIANT_SYNCHRONIZED_QUEUE_HPP

#include "Mutex.hpp"
#include "Condition.hpp"

#include <QQueue>

namespace Radiant
{

  /** This class provides a thread-safe queue */
  template<class T>
  class SynchronizedQueue
  {
  public:
    /// Constructs an empty queue
    SynchronizedQueue()
    {}

    /// Constructs a copy of another queue
    /// @param c queue to copy
    SynchronizedQueue(const SynchronizedQueue & c)
    {
      Guard g(c.m_mutex);

      m_data = c.m_data;
    }

    /// Removes the head item in the queue and returns it. If the queue is
    /// empty, the function will block until something is pushed into it.
    /// @return the current head of the queue
    T pop()
    {
      Guard g(m_mutex);

      while(m_data.isEmpty())
        m_cond.wait(m_mutex);

      return m_data.dequeue();
    }

    /// Non-blocking call that returns false if the queue was empty. Otherwise pops the head and assigns it to result.
    /// @param result head of the queue
    /// @return returns true if the queue is was not empty and result was assigned, otherwise false
    bool testAndPop(T & result)
    {
      Radiant::Guard g(m_mutex);

      if(!m_data.isEmpty()) {
        result = m_data.dequeue();
        return true;
      }

      return false;
    }

    /// Adds an element to the end of the queue
    void push(const T & t)
    {
      Guard g(m_mutex);

      m_data.enqueue(t);

      m_cond.wakeAll();
    }

    /// Returns true if the queue is empty
    bool empty() const
    {
      Guard g(m_mutex);
      return m_data.isEmpty();
    }

    /// Return a reference to the head item in the queue. If the queue is
    /// empty, this function will block until something is pushed into it.
    T & head()
    {
      Guard g(m_mutex);

      while(m_data.isEmpty())
        m_cond.wait(m_mutex);

      return m_data.head();
    }

    /// @copydoc head()
    const T & head() const
    {
      Guard g(m_mutex);

      while(m_data.isEmpty())
        m_cond.wait(m_mutex);

      return m_data.head();
    }

    /// Copies a queue
    SynchronizedQueue<T> & operator = (const SynchronizedQueue & c)
    {
      Radiant::Guard g1(m_mutex);
      Radiant::Guard g2(c.m_mutex);

      m_data = c.m_data;

      return *this;
    }

  private:
    mutable MutexAuto m_mutex;
    mutable Condition m_cond;

    QQueue<T> m_data;
  };

}

#endif
