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

#ifndef VALUABLE_VALUE_RECT_HPP
#define VALUABLE_VALUE_RECT_HPP

#include <Valuable/Export.hpp>
#include <Valuable/ValueObject.hpp>

#include <Nimble/Rect.hpp>

#define VALUEMIT_STD_OP this->emitChange(); return *this;

namespace Valuable
{

  /// A valuable object holding a Nimble::Rect object
  class VALUABLE_API ValueRect : public ValueObjectT<Nimble::Rect>
  {
    typedef ValueObjectT<Nimble::Rect> Base;
    public:
      /// @copydoc ValueObject::ValueObject(HasValues *, const std::string &, bool transit)
      ValueRect(HasValues * parent, const std::string & name, const Nimble::Rect & r, bool transit = false);

      /// Copies a rectangle
      ValueRect & operator = (const Nimble::Rect & r) { Base::m_value = r; VALUEMIT_STD_OP }

      const char * type() const { return "rect"; }

      std::string asString(bool * const ok = 0) const;

      bool deserialize(ArchiveElement & element);

      /// Converts the object to rectangle
      Nimble::Rect asRect() const { return Base::m_value; }
  };

}

#undef VALUEMIT_STD_OP

#endif
