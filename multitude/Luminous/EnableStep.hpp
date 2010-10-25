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


#ifndef LUMINOUS_ENABLE_STEP_HPP
#define LUMINOUS_ENABLE_STEP_HPP

#include <Patterns/NotCopyable.hpp>

#include <Luminous/Luminous.hpp>

namespace Luminous {

  /** Enables some OpenGL feature while this object exists.
      In OpenGL applications it is commong that some special feature needs
      to be enabled for the duration of one function. This class can
      be used o make sure that the feature is disabled, as the function is
      finished.

      Example:

      @code
      void myrender()
      {
        Luminous::EnableStep clip5(GL_CLIP_PLANE5);

        drawThings();

        if(isEnough())
          return; // GL_CLIP_PLANE5 is automatically disabled

        drawMoreThings();
      }
      @endcode
    */

  /// @todo The state management needs fixing anyhow
  class EnableStep : public Patterns::NotCopyable
  {
  public:
    /// Enables the given feature for the lifetime of this object
    EnableStep(GLenum feature) : m_feature(feature) { glEnable(feature); }
    ~EnableStep() { glDisable(m_feature); }
  private:
    GLenum m_feature;
  };

}

#endif
