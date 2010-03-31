/* COPYRIGHT
 *
 * This file is part of Resonant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Resonant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef RESONANT_APPLICATION_HPP
#define RESONANT_APPLICATION_HPP

namespace Resonant {
  /** Abstract application base class for Resonant. The Application
      object is at the moment mostly unused, but it is included as it
      will be needed to deliver core information to the audio lugins
      (application time etc.). */
  class Application
  {
  public:
    Application();
    virtual ~Application();

  };

}


#endif

