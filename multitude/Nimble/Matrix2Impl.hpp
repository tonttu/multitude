/* COPYRIGHT
 *
 * This file is part of Nimble.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Nimble.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */


#ifndef NIMBLE_MATRIX2T_IMPL_HPP
#define NIMBLE_MATRIX2T_IMPL_HPP

#include <Nimble/Matrix2.hpp>

namespace Nimble {

  template <class T>
  const Matrix2T<T> Matrix2T<T>::IDENTITY(1, 0, 
					  0, 1);

}

#endif

