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

#include <Valuable/ValueInt.hpp>

namespace Valuable
{

  ///@todo document, use properly
  class VALUABLE_API ValueEnum : public ValueIntT<int32_t>
  {
  public:
    ValueEnum(Valuable::HasValues * host, const char * name,
              const char ** enumnames, int current);
    virtual ~ValueEnum();

    virtual void processMessage(const char * id, Radiant::BinaryData & data);

  private:
    const char ** m_enumnames;
  };

}
#endif // VALUEENUM_HPP
