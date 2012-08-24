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

#include "AttributeVector.hpp"
namespace Valuable
{
  template<> VALUABLE_API const char * AttributeVector<Nimble::Vector2i>::type() const { return "vec2i"; }
  template<> VALUABLE_API const char * AttributeVector<Nimble::Vector3i>::type() const { return "vec3i"; }
  template<> VALUABLE_API const char * AttributeVector<Nimble::Vector4i>::type() const { return "vec4i"; }
  template<> VALUABLE_API const char * AttributeVector<Nimble::Vector2f>::type() const { return "vec2f"; }
  template<> VALUABLE_API const char * AttributeVector<Nimble::Vector3f>::type() const { return "vec3f"; }
  template<> VALUABLE_API const char * AttributeVector<Nimble::Vector4f>::type() const { return "vec4f"; }
  template<> VALUABLE_API const char * AttributeVector<Radiant::Color>::type() const { return "color"; }
/*
  template<> VALUABLE_API const char * AttributeVector<Nimble::Vector2d>::type() const { return "vec2d"; }
  template<> VALUABLE_API const char * AttributeVector<Nimble::Vector3d>::type() const { return "vec3d"; }
  template<> VALUABLE_API const char * AttributeVector<Nimble::Vector4d>::type() const { return "vec4d"; }
*/
}
