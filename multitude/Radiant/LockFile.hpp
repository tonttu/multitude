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

#if !defined (RADIANT_LOCKFILE_HPP)
#define RADIANT_LOCKFILE_HPP

#include "Radiant/Export.hpp"

namespace Radiant
{
	class RADIANT_API LockFile
	{
	public:
		LockFile(const char * filename);
		~LockFile();

    bool isLocked() const;

	private:
		class LockFile_Impl * m_impl;
	};
}
#endif // RADIANT_LOCKFILE_HPP
