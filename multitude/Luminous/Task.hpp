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
#ifndef LUMINOUS_TASK_HPP
#define LUMINOUS_TASK_HPP

#include <Luminous/Export.hpp>

#include <Patterns/NotCopyable.hpp>

#include <Radiant/TimeStamp.hpp>

namespace Radiant {
  class Mutex;
}

namespace Luminous
{
  class BGThread;

  /// Priority for the tasks
  typedef float Priority;

  /// Task is a base class for tasks that can be executed within BGThread. 
  class LUMINOUS_API Task : Patterns::NotCopyable
  {
  public:
    /// Standard priorities for tasks
    enum {
      PRIORITY_LOW = 1,
      PRIORITY_NORMAL = 500,
      PRIORITY_HIGH = 1000,
      PRIORITY_URGENT = 1500,
      PRIORITY_OFFSET_BIT_HIGHER = 1,
      PRIORITY_OFFSET_BIT_LOWER = -1
    };

      /// Constructs a task with the given priority
      Task(Priority p = PRIORITY_NORMAL);
      virtual ~Task();

      /// State of the task
      enum State
      {
        WAITING,      ///< Task is waiting in queue to be processed
        RUNNING,      ///< Task is currently running
        DONE          ///< Task has finished
      };

      /// Get the current priority of the task
      Priority priority() const { return m_priority; }

      /// Get the current state of the task
      State state() const { return m_state; }

      /// The actual work the task does should be implemented in here. Override
      /// in the derived class
      virtual void doTask() = 0;
  
      /// Return a timestamp for the next execution of the task
      Radiant::TimeStamp scheduled() const { return m_scheduled; }
      /// Schedule the next execution time for this task
      void schedule(Radiant::TimeStamp ts) { m_scheduled = ts; }
      /// Schedule the next execution time for this task
      void scheduleFromNow(Radiant::TimeStamp wait) 
      { m_scheduled = Radiant::TimeStamp::getTime() + wait; }
      /// @copydoc scheduleFromNow
      void scheduleFromNowSecs(double seconds)
      { m_scheduled = Radiant::TimeStamp::getTime() + 
          Radiant::TimeStamp::createSecondsD(seconds); }

    Radiant::Mutex * generalMutex();

    protected:
       /// Initialize the task. Called by BGThread before the task is processed
      virtual void initialize();
      /// Finished the task. Called by BGThread after the task has been
      /// processed
      virtual void finished();

      /// Sets the task state
      void setState(State s) { m_state = s; }

      /// State of the task
      State m_state;
      /// Priority of the task
      Priority m_priority;

      /// When is the task scheduled to run
      Radiant::TimeStamp m_scheduled;

      /// The background thread where this task is executed
      BGThread * m_host;
    
      friend class BGThread;
  };

}

#endif
