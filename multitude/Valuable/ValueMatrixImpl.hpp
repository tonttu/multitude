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

#ifndef VALUABLE_Matrix_SPECIALIZATION_HPP
#define VALUABLE_Matrix_SPECIALIZATION_HPP

#include "ValueMatrix.hpp"
#include "DOMElement.hpp"

#include <Radiant/StringUtils.hpp>

#include <stdlib.h>
#include <string.h>

namespace Valuable
{

  template <class T, typename S, int N>
  ValueMatrix<T,S,N>::~ValueMatrix()
  {}

  /*
  template <class T, typename S, int N>
  void ValueMatrix<T,S,N>::processMessage(const char * id,
                      Radiant::BinaryData & data)
  {
    if(id && strlen(id)) {
      int index = strtol(id, 0, 10);
      if(index >= N) {
        return;
      }

      bool ok = true;

      S v = data.read<S>(&ok);

      if(ok) {
        T tmp = Base::m_value;
        tmp.data()[index] = v;
        (*this) = tmp;
      }
    }
    else {

      bool ok = true;

      T v = data.read<T>(&ok);

      if(ok)
        (*this) = v;
    }
  }
  */

  template<class MatrixType, typename ElementType, int N>
  const char *  ValueMatrix<MatrixType, ElementType, N>::type() const { return "Matrix"; }


  template<class MatrixType, typename ElementType, int N>
  bool ValueMatrix<MatrixType, ElementType, N>::deserialize(ArchiveElement & element) {
    std::stringstream in(element.get());

    for(int i = 0; i < N; i++)
      in >> Base::m_value.data()[i];

    this->emitChange();

    return true;
  }

  template<class MatrixType, typename ElementType, int N>
  std::string ValueMatrix<MatrixType, ElementType, N>::asString(bool * const ok) const {
    if(ok) *ok = true;

    std::string r = Radiant::StringUtils::stringify(Base::m_value.data()[0]);

    for(int i = 1; i < N; i++)
      r += std::string(" ") + Radiant::StringUtils::stringify(Base::m_value.data()[i]);

    return r;
  }

  template<class MatrixType, typename ElementType, int N>
  bool ValueMatrix<MatrixType, ElementType, N>::set(const MatrixType & v)
  {
    Base::m_value = v;
    this->emitChange();
    return true;
  }

}

#endif
