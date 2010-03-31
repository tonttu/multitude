/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "Valuable.hpp"

#ifndef USE_QT45

#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_USE;

namespace Valuable
{

  void initialize()
  {
    XMLPlatformUtils::Initialize();
  }

  void terminate()
  {
    XMLPlatformUtils::Terminate();
  }

}

#else

namespace Valuable
{

  void initialize()
  {
  }

  void terminate()
  {
  }

}

#endif
