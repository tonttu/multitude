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

#include <Patterns/NotCopyable.hpp>

#include <Radiant/Export.hpp>
#include <Radiant/Mutex.hpp>

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

		int wait(Mutex &mutex);
		int wait(Mutex &mutex, int millsecs);

		int wakeAll();
		int wakeAll(Mutex &mutex);

		int wakeOne();
		int wakeOne(Mutex &mutex);

	private:
		class D;
		D * m_d;
	};

}

#endif
