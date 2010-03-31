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
  class VALUABLE_API ValueRect : public ValueObject
  {
    public:
      ValueRect(HasValues * parent, const std::string & name, const Nimble::Rect & r, bool transit = false);

      ValueRect & operator = (const Nimble::Rect & r) { m_rect = r; VALUEMIT_STD_OP }

    const char * type() const { return "rect"; }

    ///@todo virtual void processMessage(const char * id, Radiant::BinaryData & data);
      std::string asString(bool * const ok = 0) const;

      bool deserializeXML(DOMElement element);

      Nimble::Rect asRect() const { return m_rect; }

      //virtual bool set(const Nimble::Vector4f & v);

    protected:
      Nimble::Rect m_rect;
  };

}

#undef VALUEMIT_STD_OP

#endif
