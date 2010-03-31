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

#ifndef RADIANT_THREAD_DATA_HPP
#define RADIANT_THREAD_DATA_HPP

#include <Radiant/Mutex.hpp>
#include <Radiant/Thread.hpp>

#include <map>

namespace Radiant {

  /** Thread-specific data holder. This class is used to hold
      thread-specific data. Any thread can place data to the holder. The
      holder has a list of data objects, with corresponding thread
      ids. Thus a caller only gets the data that is specific to that
      particular thread. 

      Keep in mind that one must be careful with how the data is
      organized. The data type must have a default constructor and copy
      operator. If you wish to use data objects that do not have these
      features, then your best bet is to store only pointers to the data
      here.

      This class is completely thread-safe. It uses an internal mutex to
      prevent simultaneous modifications from multiple threads.

      @author Tommi Ilmonen
  */

  template <class T> RADIANT_API class ThreadData
  {
  public:

    ThreadData() {}
    ~ThreadData() {}

    /** Access operator. You can always call this method. If the data
	element did not exist before this call , then it is created
	on-demand. This method does mutex locking to guarantee that
	other threads do not corrupt the table while the operation is
	taking place. */
    inline T & operator * ()
    {
      m_mutex.lock();
      Thread::id_t id = Thread::myThreadId();
      T& ret =  m_table[id];
      m_mutex.unlock();
      return ret;
    }

    /// Const access operator.
    inline const T & operator * () const
    { return * ((ThreadData *) this); }

    /// Superfast access operation with no safety locks.
    inline T & getFast()
    {return m_table[Thread::myThreadId()]; }

    /** This method deletes the data element that is specific to this
	thread. Use with care. */
    void clearMyData()
    {
      m_mutex.lock();
      Thread::id_t id = Thread::myThreadId();
      typename std::map<Thread::id_t, T>::iterator t = m_table.find(id);
      if(t != m_table.end()) m_table.erase(t);
      m_mutex.unlock();
    }

    /** Deletes all objects. Use with \b great care. */
    void clear()
    { m_mutex.lock(); m_table.clear(); m_mutex.unlock(); }

    /** Returns a pointer to the first object. If there are no objects,
	then NULL is returned. */
    T * first() 
    { 
      Guard g(&m_mutex);
      if(!m_table.size()) 
	return 0; 
      return & (*m_table.begin()).second; 
    }

  private:
    MutexAuto m_mutex;

    std::map<Thread::id_t, T> m_table;
  
    /// Disabled
    ThreadData & operator = (const ThreadData &) { return * this; }
    /// Disabled
    ThreadData(const ThreadData &) {}
  };

}

#endif
