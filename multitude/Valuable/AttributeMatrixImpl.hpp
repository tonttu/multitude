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

#include "AttributeMatrix.hpp"
#include "DOMElement.hpp"

#include <Radiant/StringUtils.hpp>

#include <stdlib.h>
#include <string.h>

namespace Valuable
{

  template <class T, typename S, int N>
  AttributeMatrix<T,S,N>::~AttributeMatrix()
  {}

  /*
  template <class T, typename S, int N>
  void AttributeMatrix<T,S,N>::processMessage(const char * id,
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
  const char *  AttributeMatrix<MatrixType, ElementType, N>::type() const { return "Matrix"; }


  template<class MatrixType, typename ElementType, int N>
  bool AttributeMatrix<MatrixType, ElementType, N>::deserialize(const ArchiveElement & element) {
    std::stringstream in(element.get().toStdString());

    MatrixType m;
    for(int i = 0; i < N; i++)
      in >> m.data()[i];

    *this = m;
    return true;
  }

  template<class MatrixType, typename ElementType, int N>
  QString AttributeMatrix<MatrixType, ElementType, N>::asString(bool * const ok) const {
    if(ok) *ok = true;

    const ElementType * buf = data();
    QString r = Radiant::StringUtils::stringify(buf[0]);

    for(int i = 1; i < N; i++)
      r += " " + Radiant::StringUtils::stringify(buf[i]);

    return r;
  }

}

#endif
