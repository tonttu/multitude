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

#ifndef VALUABLE_VALUE_BOOL_HPP
#define VALUABLE_VALUE_BOOL_HPP

#include "AttributeObject.hpp"

namespace Valuable
{

  /// A value object for boolean.
  class VALUABLE_API AttributeBool : public AttributeT<bool>
  {
  public:
    using AttributeT<bool>::operator =;

    /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
    /// @param value The value of this object
    AttributeBool(Node * host, const QString & name, bool value, bool transit = false);
    virtual ~AttributeBool();

    virtual const char * type() const OVERRIDE { return "bool"; }
    bool deserialize(const ArchiveElement & element) OVERRIDE;

    /// @cond
    virtual void processMessage(const QString &, Radiant::BinaryData & data) OVERRIDE;
    /// @endcond

    /// Boolean values can be set as integers in CSS files
    virtual bool set(int v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;
    virtual bool set(const StyleValue & v, Layer layer = USER) OVERRIDE;

    virtual int asInt(bool * const ok) const OVERRIDE;
    virtual QString asString(bool * const ok = 0) const OVERRIDE;
  };

}

#endif // VALUABLE_VALUE_BOOL_HPP
