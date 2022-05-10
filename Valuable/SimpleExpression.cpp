#include "SimpleExpression.hpp"

#include <Radiant/Trace.hpp>

#include <QtGlobal>

#include <array>

#include <cassert>

namespace Valuable
{
  float evalOp(float a, SimpleExpression::Tag op, float b)
  {
    switch (op) {
    case SimpleExpression::OP_PLUS:
      return a + b;
    case SimpleExpression::OP_MINUS:
      return a - b;
    case SimpleExpression::OP_MUL:
      return a * b;
    default:
      assert(op == SimpleExpression::OP_DIV);
      return a / b;
    }
  }

  class SimpleExpression::D
  {
  public:
    struct Token
    {
      Token() {}
      Token(Tag t) : tag(t) {}
      Token(float f) : tag(TOKEN_FLOAT), f(f) {}
      Token(SimpleExpression::Param p) : tag(TOKEN_PARAM), paramIndex(p.index()) {}

      bool operator==(Token t) const
      {
        if (tag != t.tag)
          return false;
        if (tag == TOKEN_FLOAT)
          return qFuzzyCompare(f, t.f);
        if (tag == TOKEN_PARAM)
          return paramIndex == t.paramIndex;
        return true;
      }

      Tag tag;
      union {
        float f;
        int paramIndex;
      };
    };

  public:
    D() : m_paramCount(0) {}
    void replace(Tag op, Token t);

  public:
    std::vector<Token> m_tokens;
    int m_paramCount;
  };

  void SimpleExpression::D::replace(Tag op, Token t)
  {
    if (t.tag == TOKEN_PARAM)
      m_paramCount = std::max(m_paramCount, t.paramIndex+1);

    /// only literals, optimize by calculating the value now
    if (t.tag == TOKEN_FLOAT && m_tokens.size() == 1 && m_tokens[0].tag == TOKEN_FLOAT) {
      m_tokens[0].f = evalOp(m_tokens[0].f, op, t.f);
      return;
    }

    if (op == OP_PLUS) {
      // optimize 0 + x == x
      if (m_tokens.size() == 1 && m_tokens[0] == Token(0.f)) {
        m_tokens[0] = t;
        return;
      }
    }

    if (op == OP_PLUS || op == OP_MINUS) {
      // optimize x +/- 0 == x
      if (t == Token(0.f))
        return;
    }

    if (op == OP_MUL) {
      // optimize x * 0 == 0
      if (t == Token(0.f)) {
        if (m_tokens.size() >= 2 && m_tokens[1].tag == TOKEN_PARAM)
          --m_paramCount;
        m_tokens.resize(1);
        m_tokens[0] = Token(0.f);
        return;
      }
      // optimize x * 1 == x
      if (t == Token(1.f))
        return;

      if (m_tokens.size() == 1) {
        // optimize 0 * x = 0
        if (m_tokens[0] == Token(0.f)) {
          if (t.tag == TOKEN_PARAM)
            --m_paramCount;
          return;
        }
        // optimize 1 * x = x
        if (m_tokens[0] == Token(1.f)) {
          m_tokens[0] = t;
          return;
        }
      }
    }

    if (op == OP_MINUS) {
      // optimize x - x == 0
      if (m_tokens.size() == 1 && m_tokens[0] == t) {
        m_tokens[0] = Token(0.f);
        return;
      }
    }

    m_tokens.push_back(t);
    m_tokens.push_back(Token(op));
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  SimpleExpression::SimpleExpression(float literalValue)
    : m_d(new D())
  {
    m_d->m_tokens.push_back(D::Token(literalValue));
  }

  SimpleExpression::~SimpleExpression()
  {}

  SimpleExpression::SimpleExpression(const SimpleExpression & expr)
    : m_d(new D(*expr.m_d))
  {}

  SimpleExpression::SimpleExpression(SimpleExpression && expr)
    : m_d(std::move(expr.m_d))
  {}

  SimpleExpression & SimpleExpression::operator=(const SimpleExpression & expr)
  {
    m_d->m_tokens = expr.m_d->m_tokens;
    m_d->m_paramCount = expr.m_d->m_paramCount;
    return *this;
  }

  SimpleExpression & SimpleExpression::operator=(SimpleExpression && expr)
  {
    std::swap(m_d, expr.m_d);
    return *this;
  }

  bool SimpleExpression::operator==(const SimpleExpression & expr) const
  {
    // no need to check m_paramCount, since it depends on m_tokens
    return m_d->m_tokens == expr.m_d->m_tokens;
  }

  bool SimpleExpression::isConstant() const
  {
    return m_d->m_tokens.size() == 1 && m_d->m_tokens[0].tag == TOKEN_FLOAT;
  }

  void SimpleExpression::replace(Tag op, const SimpleExpression & expr)
  {
    const std::vector<D::Token> & exprTokens = expr.m_d->m_tokens;
    if (exprTokens.size() == 1) {
      m_d->replace(op, exprTokens[0]);
      return;
    }

    if (op == OP_PLUS) {
      // optimize 0 + x == x
      if (m_d->m_tokens.size() == 1 && m_d->m_tokens[0] == D::Token(0.f)) {
        m_d->m_tokens = exprTokens;
        m_d->m_paramCount = expr.m_d->m_paramCount;
        return;
      }
    }

    if (op == OP_MUL) {
      if (m_d->m_tokens.size() == 1) {
        // optimize 0 * x = 0
        if (m_d->m_tokens[0] == D::Token(0.f))
          return;
        // optimize 1 * x = x
        if (m_d->m_tokens[0] == D::Token(1.f)) {
          m_d->m_tokens = exprTokens;
          m_d->m_paramCount = expr.m_d->m_paramCount;
          return;
        }
      }
    }

    m_d->m_paramCount = std::max(m_d->m_paramCount, expr.m_d->m_paramCount);
    m_d->m_tokens.insert(m_d->m_tokens.end(), exprTokens.begin(), exprTokens.end());
    m_d->m_tokens.push_back(D::Token(op));
  }

  void SimpleExpression::replace(Tag op, float literalValue)
  {
    m_d->replace(op, D::Token(literalValue));
  }

  void SimpleExpression::replace(Tag op, SimpleExpression::Param p)
  {
    m_d->replace(op, D::Token(p));
  }

  float SimpleExpression::evaluate(const std::vector<float> & params) const
  {
    return evaluate(params.data(), params.size());
  }

  float SimpleExpression::evaluate(const float * params, std::size_t numparams) const
  {
    if (m_d->m_paramCount > int(numparams)) {
      Radiant::error("SimpleExpression::evaluate # Expression uses %d params, but %d given",
                     m_d->m_paramCount, int(numparams));
      return 0;
    }
    // we could use std::stack<float, std::vector<float> > stack, but that allocates memory
    /// @todo boundary checking?
    std::array<float, 200> stack;
    int head = -1;
    for (D::Token t: m_d->m_tokens) {
      if (t.tag == TOKEN_FLOAT) {
        stack[++head] = t.f;
      } else if (t.tag == TOKEN_PARAM) {
        stack[++head] = params[t.paramIndex];
      } else {
        stack[head-1] = evalOp(stack[head-1], t.tag, stack[head]);
        --head;
      }
    }
    return stack[0];
  }

  QByteArray SimpleExpression::toString() const
  {
    std::array<QByteArray, 200> stack;
    int head = -1;
    for (D::Token t: m_d->m_tokens) {
      if (t.tag == TOKEN_FLOAT) {
        stack[++head] = QByteArray::number(t.f);
      } else if (t.tag == TOKEN_PARAM) {
        stack[++head] = "param"+QByteArray::number(t.paramIndex);
      } else {
        stack[head-1] = "(" + stack[head-1] + " " + "+-*/"[t.tag] + " " + stack[head] + ")";
        --head;
      }
    }
    return stack[0];
  }
} // namespace Valuable
