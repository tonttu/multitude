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


#ifndef LUMINOUS_MATRIX_STEP_HPP
#define LUMINOUS_MATRIX_STEP_HPP

#include <Luminous/Luminous.hpp>

namespace Luminous {

  /// Helper class for handling the OpenGL matrix stack
  class MatrixStep
  {
  public:
    MatrixStep()  { glPushMatrix(); }
    ~MatrixStep() { glPopMatrix(); }
  private:
    MatrixStep(const MatrixStep &) {}
  };

}

#endif
