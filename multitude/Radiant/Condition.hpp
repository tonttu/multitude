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

#ifndef RADIANT_CONDITION_HPP
#define RADIANT_CONDITION_HPP

#include "Export.hpp"
#include "Mutex.hpp"

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
		Condition();
		~Condition();

    /** Waits on the wait condition. The mutex must be locked by the calling thread and is released. If the mutex is not locked the function will return immediately. */
		int wait(Mutex &mutex);
    /** Waits on the wait condition for at most the given time. The mutex must be locked by the calling thread and is released. If the mutex is not locked the function will return immediately. */
		int wait(Mutex &mutex, int millsecs);

    /// Wakes all threads waiting on the condition
		int wakeAll();
    /// Wakes all threads waiting on the condition
		int wakeAll(Mutex &mutex);

    /// Wakes one thread waiting on the condition (the woken thread can not be controlled or predicted)
		int wakeOne();
    /// Wakes one thread waiting on the condition (the woken thread can not be controlled or predicted)
		int wakeOne(Mutex &mutex);

	private:
		class D;
		D * m_d;
	};

}

#endif
