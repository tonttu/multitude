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
