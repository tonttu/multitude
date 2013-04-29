#include "FutureBool.hpp"
#include "Task.hpp"

namespace Radiant
{

  FutureBoolConjunction::FutureBoolConjunction(FutureBoolIPtr lhs, FutureBoolIPtr rhs)
    : m_lhs(std::move(lhs)),
      m_rhs(std::move(rhs))
  {
  }

  FutureBoolConjunction::~FutureBoolConjunction()
  {
  }

  bool FutureBoolConjunction::isReady() const
  {
    return m_lhs->isReady() && m_rhs->isReady();
  }

  TaskPtr FutureBoolConjunction::task() const
  {
    if(!m_lhs->isReady()) {
      return m_lhs->task();
    } else if(m_lhs->validate()) {
      return m_rhs->task();
    } else {
      return nullptr;
    }
  }

  bool FutureBoolConjunction::validate()
  {
    return m_lhs->validate() && m_rhs->validate();
  }

  FutureBoolIPtr FutureBoolConjunction::conjunction(FutureBoolI *lhs, FutureBoolI *rhs)
  {
    return FutureBoolIPtr(new FutureBoolConjunction(FutureBoolIPtr(lhs), FutureBoolIPtr(rhs)));
  }


} //namespace Radiant
