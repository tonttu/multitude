/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef VALUABLE_SIMPLEEXPRESSIONLINK_HPP
#define VALUABLE_SIMPLEEXPRESSIONLINK_HPP

#include "Attribute.hpp"
#include "SimpleExpression.hpp"

namespace Valuable
{
  class VALUABLE_API SimpleExpressionLink
  {
  public:
    SimpleExpressionLink();
    ~SimpleExpressionLink();

    void setExpression(const SimpleExpression & expr);
    const SimpleExpression & expression() const;

    void setOutput(Attribute * attr, Attribute::Layer layer);
    void setInput(SimpleExpression::Param p, Attribute * attr);
    void setDefaultInput(SimpleExpression::Param p, float v);

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
} // namespace Valuable

#endif // VALUABLE_SIMPLEEXPRESSIONLINK_HPP
