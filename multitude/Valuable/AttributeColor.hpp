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

#ifndef VALUABLE_VALUE_COLOR_HPP
#define VALUABLE_VALUE_COLOR_HPP

#include <Radiant/Color.hpp>

#include <Valuable/Export.hpp>
#include <Valuable/AttributeVector.hpp>

namespace Valuable
{

  /** A value object holding a #Radiant::Color value. */
  class AttributeColor : public AttributeVector<Nimble::Vector4f>
  {
  public:
    /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
    /// @param c The color value
    VALUABLE_API AttributeColor(Node * host, const QString & name, const Radiant::Color & c, bool transit = false);
    
    VALUABLE_API virtual ~AttributeColor();

    /// Copies a color
    AttributeColor & operator = (const Radiant::Color & c)
      { (* (AttributeVector<Nimble::Vector4f> *) this) = c; return *this; }
    
    const char * type() const OVERRIDE { return "color"; }
    
    /// Converts the value object to color
    Radiant::Color asColor() const { return asVector(); }
    
    /// Returns the red comoponent of the color (0-1).
    inline float red() const   { return get(0); }
    /// Returns the green comoponent of the color (0-1).
    inline float green() const { return get(1); }
    /// Returns the blue comoponent of the color (0-1).
    inline float blue() const  { return get(2); }
    /// Returns the alpha comoponent of the color (0-1).
    inline float alpha() const { return get(3); }
  };

}

#endif
