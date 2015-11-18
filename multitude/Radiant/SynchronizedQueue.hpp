/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_SYNCHRONIZED_QUEUE_HPP
#define RADIANT_SYNCHRONIZED_QUEUE_HPP

#include "Mutex.hpp"
#include "Condition.hpp"

#include <QQueue>

namespace Radiant
{

  /// This class provides a thread-safe queue
  /// @tparam T Type of the values stored
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
    /// @param t Value to add to the queue
    void push(const T & t)
    {
      Guard g(m_mutex);

      m_data.enqueue(t);

      m_cond.wakeAll();
    }

    /// Returns true if the queue is empty
    /// @return True if queue is empty, true otherwise
    bool empty() const
    {
      Guard g(m_mutex);
      return m_data.isEmpty();
    }

    /// Return a reference to the head item in the queue. If the queue is
    /// empty, this function will block until something is pushed into it.
    /// @return current head
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
    /// @param c Queue to copy
    /// @return Reference to self
    SynchronizedQueue<T> & operator = (const SynchronizedQueue & c)
    {
      if(this == &c)
        return *this;

      auto firstMutex = std::min(&m_mutex, &c.m_mutex);
      auto secondMutex = std::max(&m_mutex, &c.m_mutex);

      Radiant::Guard g1(*firstMutex);
      Radiant::Guard g2(*secondMutex);

      m_data = c.m_data;

      return *this;
    }

    template <typename InputIterator>
    void push(InputIterator begin, InputIterator end)
    {
      Radiant::Guard g(m_mutex);
      while(begin != end) {
        m_data.enqueue(*begin);
        ++begin;
      }
      m_cond.wakeAll();
    }

    std::vector<T> popElements(int num, unsigned int millisecs=0)
    {
      std::vector<T> elems;
      Radiant::Guard g(m_mutex);

      for(int i = 0; i < num; ++i) {

        if(m_data.empty()) {

          bool hasValue = millisecs == 0 ? m_cond.wait(m_mutex) :
                                           m_cond.wait(m_mutex, millisecs);
          if(hasValue)
            break;
        }
        elems.push_back(m_data.takeFirst());
      }
      return elems;
    }

  private:
    mutable Mutex m_mutex;
    mutable Condition m_cond;

    QQueue<T> m_data;
  };

}

#endif
