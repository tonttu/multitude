/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "SimpleExpressionLink.hpp"

namespace Valuable
{
  class SimpleExpressionLink::D
  {
  public:
    D()
      : m_expr(0.0f),
        m_output(nullptr),
        m_outputLayer(Attribute::USER),
        m_outputListener(0)
    {}

    void evaluate();

  public:
    SimpleExpression m_expr;
    Attribute * m_output;
    Attribute::Layer m_outputLayer;
    long m_outputListener;

    struct AttrListener
    {
      Attribute * attr;
      long changeListener;
      long deleteListener;
      float defaultValue;
    };

    std::vector<AttrListener> m_input;
    std::vector<float> m_inputCache;
  };

  void SimpleExpressionLink::D::evaluate()
  {
    if (m_output)
      m_output->set(m_expr.evaluate(m_inputCache), m_outputLayer);
  }

  SimpleExpressionLink::SimpleExpressionLink()
    : m_d(new D())
  {}

  SimpleExpressionLink::~SimpleExpressionLink()
  {}

  void SimpleExpressionLink::setExpression(const SimpleExpression & expr)
  {
    m_d->m_expr = expr;
    m_d->evaluate();
  }

  const SimpleExpression & SimpleExpressionLink::expression() const
  {
    return m_d->m_expr;
  }

  void SimpleExpressionLink::setOutput(Attribute * attr, Attribute::Layer layer)
  {
    if (m_d->m_output)
      m_d->m_output->removeListener(m_d->m_outputListener);
    m_d->m_output = attr;
    m_d->m_outputLayer = layer;
    if (attr) {
      m_d->m_outputListener = attr->addListener([=] {
        m_d->m_outputListener = 0;
        m_d->m_output = nullptr;
      }, Attribute::DELETE_ROLE);
      m_d->evaluate();
    }
  }

  void SimpleExpressionLink::setDefaultInput(SimpleExpression::Param p, float v)
  {
    if (int(m_d->m_input.size()) <= p.index())
      m_d->m_input.resize(p.index()+1);
    if (int(m_d->m_inputCache.size()) <= p.index())
      m_d->m_inputCache.resize(p.index()+1);

    m_d->m_input[p.index()].defaultValue = v;
    if (!m_d->m_input[p.index()].attr) {
      m_d->m_inputCache[p.index()] = v;
      m_d->evaluate();
    }
  }

  void SimpleExpressionLink::setInput(SimpleExpression::Param p, Attribute * attr)
  {
    if (int(m_d->m_input.size()) <= p.index())
      m_d->m_input.resize(p.index()+1);
    if (int(m_d->m_inputCache.size()) <= p.index())
      m_d->m_inputCache.resize(p.index()+1);

    auto & input = m_d->m_input[p.index()];
    if (input.attr) {
      input.attr->removeListener(input.changeListener);
      input.attr->removeListener(input.deleteListener);
    }
    input.attr = attr;
    if (attr) {
      input.deleteListener = attr->addListener([=] {
          m_d->m_input[p.index()].attr = nullptr;
          m_d->m_input[p.index()].changeListener = 0;
          m_d->m_input[p.index()].deleteListener = 0;
      }, Attribute::DELETE_ROLE);
      input.changeListener = attr->addListener([=] {
          m_d->m_inputCache[p.index()] = attr->asFloat();
          m_d->evaluate();
        }, Attribute::CHANGE_ROLE);
      m_d->m_inputCache[p.index()] = attr->asFloat();
    } else {
      m_d->m_inputCache[p.index()] = input.defaultValue;
    }
    m_d->evaluate();
  }

} // namespace Valuable
