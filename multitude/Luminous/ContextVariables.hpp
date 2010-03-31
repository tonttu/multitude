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

#ifndef LUMINOUS_CONTEXTVARIABLES_HPP
#define LUMINOUS_CONTEXTVARIABLES_HPP

#include <Luminous/ContextVariable.hpp>
#include <Luminous/ContextVariableImpl.hpp>
#include <Luminous/Texture.hpp>
#include <Luminous/GLSLProgramObject.hpp>

namespace Luminous {

  typedef ContextVariableT<Texture1D> ContextTexture1D;
  typedef ContextVariableT<Texture2D> ContextTexture2D;
  typedef ContextVariableT<Texture3D> ContextTexture3D;
  typedef ContextVariableT<GLSLProgramObject> ContextGLSLProgramObject;

}

#endif // CONTEXTVARIABLES_HPP
