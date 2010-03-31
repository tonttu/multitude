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

#ifndef VALUABLE_VECTOR_SPECIALIZATION_HPP
#define VALUABLE_VECTOR_SPECIALIZATION_HPP

#include "ValueVector.hpp"
#include "DOMElement.hpp"

namespace Valuable
{

  template <class T, typename S, int N>
  ValueVector<T,S,N>::~ValueVector()
  {}

  template <class T, typename S, int N>
  void ValueVector<T,S,N>::processMessage(const char *,
					  Radiant::BinaryData & data)
  {
    bool ok = true;

    T v = data.read<T>(&ok);

    if(ok)
      (*this) = v;
  }

  template<class VectorType, typename ElementType, int N>
  const char *  ValueVector<VectorType, ElementType, N>::type() const { return "vector"; }


  /// @todo Under WIN32 these specializations conflict with the class instantiation
  /// in ValueVector.hpp. Remove them and if possible modify the template function 
  /// above to handle all types.
/*
  template<>
  const char * const ValueVector<Nimble::Vector2f, float, 2>::type() const { return "vec2f"; }

  template<> 
  const char * const ValueVector<Nimble::Vector3f, float, 3>::type() const { return "vec3f"; }

  template<> 
  const char * const ValueVector<Nimble::Vector4f, float, 4>::type() const { return "vec4f"; }

  template<> 
  const char * const ValueVector<Nimble::Vector2i, int, 2>::type() const { return "vec2i"; }

  template<> 
  const char * const ValueVector<Nimble::Vector3i, int, 3>::type() const { return "vec3i"; }

  template<> 
  const char * const ValueVector<Nimble::Vector4i, int, 4>::type() const { return "vec4i"; }
*/

  template<class VectorType, typename ElementType, int N>
  bool ValueVector<VectorType, ElementType, N>::deserializeXML(DOMElement element) {
    std::stringstream in(element.getTextContent());

    for(int i = 0; i < N; i++)
      in >> m_value[i];

    return true;
  }

  template<class VectorType, typename ElementType, int N>
  std::string ValueVector<VectorType, ElementType, N>::asString(bool * const ok) const {
    if(ok) *ok = true;

    std::string r = Radiant::StringUtils::stringify(m_value[0]);

    for(int i = 1; i < N; i++) 
      r += std::string(" ") + Radiant::StringUtils::stringify(m_value[i]);

    return r;
  }

  template<class VectorType, typename ElementType, int N>
  bool ValueVector<VectorType, ElementType, N>::set(const VectorType & v)
  {
    m_value = v;
    return true;
  }
 
}

#endif
