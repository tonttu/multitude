#ifndef VALUABLE_SIMPLEEXPRESSION_HPP
#define VALUABLE_SIMPLEEXPRESSION_HPP

#include "Export.hpp"

#include <memory>
#include <vector>

#include <QByteArray>

namespace Valuable
{
  class VALUABLE_API SimpleExpression
  {
  public:
    class Param
    {
    public:
      Param(int index) : m_index(index) {}
      int index() const { return m_index; }

    private:
      const int m_index;
    };

    enum Tag
    {
      OP_PLUS,
      OP_MINUS,
      OP_MUL,
      OP_DIV,
      TOKEN_FLOAT,
      TOKEN_PARAM
    };

  public:
    SimpleExpression(float literalValue);
    ~SimpleExpression();

    SimpleExpression(const SimpleExpression & expr);
    SimpleExpression(SimpleExpression && expr);

    SimpleExpression & operator=(const SimpleExpression & expr);
    SimpleExpression & operator=(SimpleExpression && expr);

    bool operator==(const SimpleExpression & expr) const;

    bool isConstant() const;

    /// If the current expression is A, replace it to be ((A) op (expr))
    void replace(Tag op, const SimpleExpression & expr);
    void replace(Tag op, float literalValue);
    void replace(Tag op, Param p);

    float evaluate(const std::vector<float> & params) const;
    float evaluate(const float * params = nullptr, std::size_t numparams = 0) const;
    QByteArray toString() const;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
} // namespace Valuable

#endif // VALUABLE_SIMPLEEXPRESSION_HPP
