/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include <Luminous/BGThread.hpp>

#include <Nimble/Math.hpp>

#include <Radiant/Sleep.hpp>
#include <Radiant/Trace.hpp>

#include <typeinfo>
#include <cassert>


namespace Luminous
{

  BGThread * BGThread::m_instance = 0;

  BGThread::BGThread():
    m_continue(true)
  {
    if(m_instance == 0)
      m_instance = this;
  }
/*  
  static void g_deletePred1(Task * x)
  {
    delete x;
  }

  static void g_deletePred2(BGThread::contained c)
  {
    delete c.second;
  }
*/
  BGThread::~BGThread()
  {
    if(m_instance == this)
      m_instance = 0;
    stop();

    /// @todo Free resources, should we do this?
    // for_each(m_taskQueue.begin(), m_taskQueue.end(), g_deletePred2);
  }

  void BGThread::addTask(Task * task)
  {
//    Radiant::trace("BGThread::addTask #");
    assert(task);
    task->m_host = this;

    m_mutex.lock();
    m_taskQueue.insert(contained(task->priority(), task));
    m_mutex.unlock();

    m_wait.wakeAll();
  }
/*
  void BGThread::markForDeletion(Task * task)
  {
    assert(task);

    m_mutex.lock();
    task->m_canDelete = true;
    m_mutex.unlock();

    m_wait.wakeAll();
  }
*/
  void BGThread::stop()
  {
    m_continue = false;

    if(isRunning()) 
    {
      m_wait.wakeAll();
      waitEnd();
    }
  }

  void BGThread::setPriority(Task * task, Priority p)
  {
    m_mutex.lock();

    // Find tasks with the given priority
    container::iterator beg = m_taskQueue.find(task->priority());
    container::iterator end = m_taskQueue.upper_bound(task->priority());

    // Find the actual requested task
    container::iterator it;
    for(it = beg; it != end; it++) {
      if(it->second == task) break;
    }
    
    if(it != end) {
      // Move the task in the queue and update its priority
      m_taskQueue.erase(it);

      task->m_priority = p;
      m_taskQueue.insert(contained(p, task));
    } else {
      Radiant::error("BGThread::setPriority # requested task was not found");
    }

    m_mutex.unlock();
  }

   BGThread * BGThread::instance()
   {
       if(!m_instance) {
           m_instance = new BGThread();
	   m_instance->run();
	}

     return m_instance;
   }

/*
  static bool g_deleteMarkedPred(Task * x)
  {
    if(x->canBeDeleted()) {
      delete x;
      return true;
    }
   
    return false;
  }
*/

  unsigned BGThread::taskCount() 
  {
    Radiant::Guard guard(&m_mutex);
    return m_taskQueue.size();
  }

  Radiant::Mutex * BGThread::generalMutex()
  {
    return & m_generalMutex;
  }

/*
 * For TimeStamps: use Condition::wait(mutex, timeout)
 *
 * compute the next possible runtime from tasks, set as timeout
 * can't use m_taskQueue.empty() anymore
*/

  void BGThread::childLoop()
  {
    while(m_continue) {
//      Radiant::trace("BGThread::childLoop # running");

      // Pick a task to run
      Radiant::TimeStamp toWait;
      Task * task = pickNextTask(toWait);

      // Run the task
      if(task) {

	/* Radiant::trace("Picked a task %s with priority %f",
           typeid(*task).name(), task->priority());*/
//        Radiant::trace("FOO");
        bool first = (task->state() == Task::WAITING);

        if(first) {
          task->initialize();
          task->m_state = Task::RUNNING;
        }

        task->doTask();

        // Did the task complete?
        if(task->state() == Task::DONE) {
//          Radiant::trace("BGThread::childLoop # TASK DONE %s", typeid(*task).name());
          task->finished();
        } else {
          // If we are still running, push the task to the back of the given
          // priority range so that other tasks with the same priority will be 
          // executed in round-robin
          m_mutex.lock();
          m_taskQueue.insert(contained(task->priority(), task));
          m_mutex.unlock();
        }
      } else if(!m_taskQueue.empty()) {
        // There was nothing to run, wait until the next task can be run (or we
        // get interrupted)
//        Radiant::trace("BGTHREAD: NOTHING TO RUN; WAITING %d MSECS", (int)(toWait.secondsD() * 1000.0));
//        Radiant::trace("\t%d TASKS IN QUEUE", (int)m_taskQueue.size());
        m_mutexWait.lock();
        m_wait.wait(m_mutexWait, (int)(toWait.secondsD() * 1000.0));
        m_mutexWait.unlock();         
      }

      // Remove all tasks marked for deletion
      //m_mutex.lock();
      //m_taskQueue.remove_if(g_deleteMarkedPred);
      //m_mutex.unlock();


      // Wait for new tasks to appear
      while(m_taskQueue.empty() && m_continue) {
//        Radiant::trace("BGTHREAD: QUEUE EMPTY; WAITING FOR MORE");
        m_mutexWait.lock();
        m_wait.wait(m_mutexWait);
        m_mutexWait.unlock();
      }
    }

    // Down use all the cpu
    Radiant::Sleep::sleepS(1);
  }

  Task * BGThread::pickNextTask(Radiant::TimeStamp & wait)
  { 
//    Radiant::trace("BGThread::pickNextTask #");
    Radiant::Guard guard(&m_mutex);

    if(m_taskQueue.empty()) { 
      return 0;
    }

    const Radiant::TimeStamp now = Radiant::TimeStamp::getTime();

    assert(!m_taskQueue.empty());
    wait = m_taskQueue.begin()->second->scheduled() - now;

    for(container::iterator it = m_taskQueue.begin(); it != m_taskQueue.end(); it++) {
      Task * task = it->second;      

//      Radiant::trace("\tTASK SCHEDULED %s; NOW %s", task->scheduled().asString().c_str(), now.asString().c_str());

      // Should the task be run now?
      if(task->scheduled() <= now) {
        wait = 0;
        m_taskQueue.erase(it);
        return task;
      } else {
        Radiant::TimeStamp next = task->scheduled() - now;
        wait = Nimble::Math::Min(wait, next);
      }
    }

    return 0;
  }

}
