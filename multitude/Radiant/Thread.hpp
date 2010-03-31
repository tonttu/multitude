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

#ifndef RADIANT_THREAD_HPP
#define RADIANT_THREAD_HPP

#include <Radiant/Export.hpp>
#include <Patterns/NotCopyable.hpp>

#include <cstring>

class QThread;

namespace Radiant {

  class Mutex;

  /// Platform-independent threading
  /** This class is used by inheriting it and overriding the virtual
      method childLoop().

      At the moment there is only POSIX-threads -based implementation,
      that works on various UNIX-like systems.
      */
  class RADIANT_API Thread : public Patterns::NotCopyable
  {
  public:

    /// Thread id type.
    /** On most systems this is some kind of integer value. */
    typedef size_t id_t;

    /// The id of the calling thread
    static id_t myThreadId();

    /** Construct a thread structure. The thread is NOT activated by this
    method. */
    Thread();

    /// Destructor
    /** The thread must be stopped before this method is
    called. Thread cannot be terminated within the destructor, as
    the inheriting class that implements the virtual childLoop
    function does not exist any more (its desctructor is called
    before this function). */
    virtual ~Thread();

    /** Starts the thread in either system-level or process-level
    scope. If the OS fails to deliver the desired thread type the
    other kind is silently used instead. This method uses the other
    two "run" methods */
    bool run(bool prefer_system = true);

    /// Runs a system-level thread.
    /** System-level threads usually have separate process ids for
    each thread. On some platforms (such as Linux), this is the
    only kind of thread. On some platforms (such as IRIX) you need
    special privileges to run system-level threads.*/
    ///@todo remove
    bool runSystem();
    /// Runs a process-level thread.
    /** Process-level threads work inside one process id and
    usually. This thread type is supposed to offer very good
    performance. Some people (such as Linus Torvalds) do not share
    this view. */
    ///@todo remove
    bool runProcess();

    /** Waits until thread is finished. This method does nothing to
    kill the thread, it simply waits until the thread has run its
    course. */
    bool waitEnd();

    /** Kills the thread. A violent way to shut down a thread. You
    should only call this method in emergency situations. May result
    in application crash and other minor problems.*/
    void kill();

    /// Returns true if the thread is running.
    bool isRunning();

    /// Sets the real-time priority for the calling thread
    static bool setThreadRealTimePriority(int priority);

    /** Drive some self tests. */
    static void test();

    /// Access the internal QThread
    QThread * qtThread();

  protected:
    /// Exits the the calling thread.
    void threadExit();

    /// Calls childLoop.
    void mainLoop();

    /** The actual contents of the thread. You need to override this to
    add functionality to your software. */
    virtual void childLoop() = 0;

  private:
    static void *entry(void *);

    class D;
    D * m_d;

    int             m_state;

    static bool m_threadDebug;
    static bool m_threadWarnings;
  };

}

#endif
