/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_CONDITION_HPP
#define RADIANT_CONDITION_HPP

#include "Export.hpp"
#include "Mutex.hpp"

#include <limits>

#include <Patterns/NotCopyable.hpp>

namespace Radiant {

	/// Condition for threads.
	/** 
	Typical use pattern for thread that waits:

	<PRE>
	mutex.lock();

	while(needMoreData())
	condition.wait(mutex);

	mutex.unlock();
	</PRE>

	Typical use pattern for thread that informs its children:

	<PRE>
	mutex.lock();
	condition.wakeAll();
	mutex.unlock();
	</PRE>

	Or simply:

	<PRE>
	condition.wakeAll(mutex);
	</PRE>
	*/
	class RADIANT_API Condition : public Patterns::NotCopyable
	{
	public:
    /// Constructor
		Condition();
    /// Destructor
		~Condition();

    /// Waits on the wait condition for at most the given time. The mutex must
    /// be locked by the calling thread. The mutex is released. If the mutex is
    /// not locked the function will return immediately.
    /// @param mutex locked mutex
    /// @param millisecs timeout in milliseconds
    /// @return false if the wait timed out
    bool wait(Mutex &mutex, unsigned long millisecs = std::numeric_limits<unsigned long>::max());


    /// Waits on the wait condition for at most the given time. Decreases the
    /// parameter timeoutMs by amount of time waited. If the wait is timed out
    /// returns false; otherwise true.
    /// @param mutex locked mutex
    /// @param[in,out] timeoutMs timeout in milliseconds to wait
    /// @return false if the wait timed out; otherwise true
    bool wait2(Mutex & mutex, unsigned int & timeoutMs);

    /// Wakes all threads waiting on the condition
    void wakeAll();
    /// Wakes all threads waiting on the condition, while locking the given mutx
    /// @param mutex Mutex to lock
    void wakeAll(Mutex &mutex);

    /// Wakes one thread waiting on the condition (the woken thread can not be controlled or predicted)
    void wakeOne();
    /// Wakes one thread waiting on the condition (the woken thread can not be controlled or predicted)
    /// while locking the given mutex
    /// @param mutex Mutex to lock
    void wakeOne(Mutex &mutex);

	private:
		class D;
		D * m_d;
	};

}

#endif
