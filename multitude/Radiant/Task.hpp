/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#ifndef RADIANT_TASK_HPP
#define RADIANT_TASK_HPP

#include <Radiant/Export.hpp>

#include <Patterns/NotCopyable.hpp>

#include <Radiant/MemCheck.hpp>
#include <Radiant/Timer.hpp>

#include <functional>

namespace Radiant {
  class Mutex;
}

namespace Radiant
{
  class BGThread;
  class TaskDeleter;

  /// Priority for the tasks
  typedef float Priority;

  /// Task is an interface for tasks that can be executed within BGThread.
  /** The purpose of #Task is to make it easy to move time-consuing operations
      away from the main thread of the application. Tasks are placed in the
      #Radiant::BGThread, that schedules and runs them as specified.

      Typical operations that can be implemented with tasks are:

      <UL>
      <LI>Loading data from disk</LI>
      <LI>Creating new widgets, before inserting them into the scene</LI>
      <LI>Checking for changes in the application configuration files</LI>
      </UL>

      <B>Please note</B> that Tasks are expected to execute fast. In other words,
      a task should not perform long, blocking operations. Such operations (for
      example database queries, or network file transfers), are best handled by
      launching a separate thread for them. For this purpose,
      see #Radiant::Thread.

      If you implement tasks that take a long time to execute, you should check
      the task state periodically inside you Task::doTask function and return from
      the method if the task is set to Task::DONE state. Otherwise your application
      may stall for a while when closing down because the application will wait
      for any tasks to finish before it stops the BGThread running them.
    */
  class RADIANT_API Task : Patterns::NotCopyable, public std::enable_shared_from_this<Task>
  {
    MEMCHECKED
  public:
    /// Standard priorities for tasks
    enum {
      PRIORITY_LOW = 1,      ///< Low priority
      PRIORITY_NORMAL = 500, ///< Normal priority
      PRIORITY_HIGH = 1000,  ///< High priority
      PRIORITY_URGENT = 1500 ///< Urgent priority
    };

    /// State of the task
    enum State
    {
      WAITING,      ///< Task is waiting in queue to be processed
      RUNNING,      ///< Task is currently running
      DONE,         ///< Task has finished
    };
  public:
    /// Constructs a task with the given priority
    /// @param p Priority for task
    Task(Priority p = PRIORITY_NORMAL);

    /// Destructor
    virtual ~Task();

    /// Sets priority for task
    /// @param priority Priority of the task
    void setPriority( Priority priority ) { m_priority = priority; }

    /// Get the current priority of the task
    /// @return Priority of the task
    Priority priority() const { return m_priority; }

    /// Get the current state of the task
    /// @return State of the task
    State state() const { return m_state; }

    /// The actual work the task does should be implemented in here. Override
    /// in the derived class. When the task is finished it is important to set the
    /// @ref State of the task to State::DONE (f.ex. calling @ref setFinished)
    /// so that @ref BGThread can properly release the task. If the state of the task
    /// is not set to State::DONE after call to this function this task is scheduled to
    /// be executed later.
    virtual void doTask() = 0;

    /// Marks the task as canceled, so it will be removed.
    void setCanceled() { m_canceled = true; }

    /// Returns whether a task has been canceled
    /// @return True if the task has been canceled, false otherwise
    bool isCanceled() const { return m_canceled; }

    /// Returns time until next execution of task
    /// @return seconds when the task is next executed. Might be negative.
    double secondsUntilScheduled() const;

    /// @copybrief scheduleFromNow
    /// @param seconds number of seconds before next execution
    void scheduleFromNowSecs(double seconds);

    /// Marks the task as finished, so it will be removed.
    void setFinished() { setState(DONE); }

    /// If the task isn't already finished, runs the task immediately in the
    /// calling thread. If the task is running in a background thread, waits
    /// until the task is released. The task will be removed from the background
    /// thread, even if it is not finished.
    ///
    /// Will call initialize() and finished() -functions if necessary.
    /// It's fine to call this function either before or after the task is
    /// added to BGThread, but this shouldn't be called at the same time as
    /// BGThread::addTask.
    /// @param finish run doTask until the task is in DONE state, otherwise
    ///               just run the task once.
    void runNow(bool finish);

    /// Enables or disables slow task debugging. Setting this to a positive value
    /// enables the feature and sets the threshold for slow tasks to the given
    /// time in seconds. For instance setSlowTaskDebuggingThreshold(0.050)
    /// logs all tasks that take 50 ms or longer to run. Setting this to zero or
    /// negative value disables the feature.
    static void setSlowTaskDebuggingThreshold(float timeS);

    /// @see setSlowTaskDebuggingThreshold
    static float slowTaskDebuggingThreshold();

  protected:
     /// Initialize the task. Called by BGThread before the task is processed
    virtual void initialize();

    /// Cancel the task. Called by BGThread after the task has been
    /// canceled
    virtual void canceled();

    /// Finished the task. Called by BGThread after the task has been
    /// processed
    virtual void finished();

    /// Sets the task state
    /// @param s State to set
    void setState(State s) { m_state = s; }

  private:
    /// State of the task
    State m_state;

    /// Whether or not the state has been canceled
    bool m_canceled;

    /// Priority of the task
    Priority m_priority;

    /// When is the task scheduled to run
    Radiant::Timer m_scheduled;

    /// The background thread where this task is executed
    BGThread * m_host;

    /// Callstack of the task creation, for debugging, only enabled if
    /// slowTaskDebuggingThreshold is enabled.
    std::unique_ptr<CallStack> m_createStack;

    friend class BGThread;
  };
  /// Shared pointer to a Task
  typedef std::shared_ptr<Task> TaskPtr;

  /// This class executes the given function within BGThread.
  /// Task is given as a parameter to the function that is being executed
  /// in task because function needs to finish task explicitly ie. call
  /// @ref Task::setFinished when everything is ready. The following example
  /// shows how to create and use FunctionTask:
  ///
  /// @code
  ///  std::function<void (Task &)> taskFunction = [] (Radiant::Task& t) {
  ///    // This will do some costly computing. It is called repeatedly as
  ///    // long as task is alive.
  ///    if(ready)
  ///      t.setFinished();
  ///  };
  ///
  ///  Radiant::TaskPtr task = std::make_shared<Radiant::FunctionTask>(taskFunction);
  ///  Radiant::BGThread::instance()->addTask(task);
  ///
  /// @endcode
  class RADIANT_API FunctionTask : public Task
  {
  public:
    /// Construct a new FunctionTask
    /// @param func function to execute
    FunctionTask(std::function<void (Task &)> func);

    virtual void doTask() OVERRIDE;

    static void executeInBGThread(std::function<void (Task &)> func);

  private:
    std::function<void (Task &)> m_func;
  };

  /// Same as FunctionTask, but executes the function only once
  class RADIANT_API SingleShotTask : public Task
  {
  public:
    /// Construct a new FunctionTask
    /// @param func function to execute
    SingleShotTask(std::function<void ()> func);

    virtual void doTask() OVERRIDE;

    /// Executes the given function exactly once in BGThread
    static void run(std::function<void ()> func) { run(0, func); }
    /// Executes the given function exactly once in BGThread after given delay
    /// @param delayS delay in seconds
    static void run(double delayS, std::function<void ()> func);

  private:
    std::function<void ()> m_func;
  };

}

#endif
