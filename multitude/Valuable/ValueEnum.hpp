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

#ifndef VALUABLE_VALUE_ENUM_HPP
#define VALUABLE_VALUE_ENUM_HPP

/// @cond

#include <Valuable/ValueInt.hpp>

namespace Valuable
{

  /// @todo document, use properly, finish implementation

  /**
   * Valuable enum. Similar to AttributeFlags, but only one value can be enabled
   * at a time.
   *
   * @see AttributeFlags for more information and example
   */
  class VALUABLE_API AttributeEnum : public AttributeIntT<int32_t>
  {
  public:
    /// @copydoc Attribute::Attribute(HasValues *, const QString &, bool transit)
    AttributeEnum(Valuable::HasValues * host, const char * name,
              const char ** enumnames, int current);
    virtual ~AttributeEnum();

    virtual void processMessage(const char * id, Radiant::BinaryData & data);

  private:
    const char ** m_enumnames;
  };

}

/// @endcond

#endif // VALUEENUM_HPP
