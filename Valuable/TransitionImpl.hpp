/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef VALUABLE_TRANSITION_IMPL_HPP
#define VALUABLE_TRANSITION_IMPL_HPP

#include "TransitionAnim.hpp"
#include "TransitionManager.hpp"

#include <Valuable/Attribute.hpp>

namespace Valuable
{
  /// hasCustomInterpolator<T>::value is true if there is a function:
  /// T AttributeT<T>::interpolate(T, T, float)
  template <typename T>
  struct hasCustomInterpolator
  {
    typedef T (*Func)(T, T, float);
    template <Func> struct Test {};
    template <class Y> static char test(Test<&Y::interpolate>*);
    template <class Y> static long test(...);
    static const bool value = sizeof(test<AttributeT<T>>(0)) == sizeof(char);
  };

  /// Dispatcher between the default trivial interpolation and custom
  /// interpolator implementation (AttributeT<T>::interpolate)
  template <typename T, bool = hasCustomInterpolator<T>::value>
  struct TransitionInterpolator
  {
    inline static T interpolate(T a, T b, float m)
    {
      return a * (1.0f - m) + b * m;
    }
  };

  template <typename T>
  struct TransitionInterpolator<T, true>
  {
    inline static T interpolate(T a, T b, float m)
    {
      return AttributeT<T>::interpolate(a, b, m);
    }
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <typename T>
  TransitionManagerT<T> & TransitionManagerT<T>::instance()
  {
    for (TransitionManager * mgr: TransitionManager::instances())
      if (auto t = dynamic_cast<TransitionManagerT<T>*>(mgr))
        return *t;
    return *(new TransitionManagerT<T>());
  }

  template <typename T>
  TransitionAnimT<T> * TransitionManagerT<T>::create(AttributeBaseT<T> * attr, TransitionParameters params)
  {
    static TransitionManagerT<T> & s_mgr = TransitionManagerT<T>::instance();

    assert(params.isValid());

    s_mgr.m_transitions.push_back(attr);
    TransitionAnimT<T> & anim = s_mgr.m_transitions.back();
    anim.setParameters(params);

    return &anim;
  }

  template <typename T>
  void TransitionManagerT<T>::update(float dt)
  {
    for (auto it = m_transitions.begin(), end = m_transitions.end(); it != end;) {
      TransitionAnimT<T> & transition = *it;
      while (transition.isNull()) {
        TransitionAnimT<T> & back = m_transitions.back();

        if (&transition == &back) {
          m_transitions.erase(m_transitions.size() - 1);
          return;
        } else {
          transition = std::move(back);
          m_transitions.pop_back();
        }
      }

      if (transition.isActive())
        transition.update(dt);
      ++it;
    }
  }

  template <typename T>
  std::size_t TransitionManagerT<T>::countActiveTransitions() const
  {
    std::size_t count = 0;
    for (const TransitionAnimT<T> & transition: m_transitions.unsafeVector())
      if (transition.isActive())
        ++count;

    return count;
  }

  /////////////////////////////////////////////////////////////////////////////

  template <typename T>
  TransitionAnimT<T>::TransitionAnimT(AttributeBaseT<T> * attr)
    : m_attr(attr),
      m_pos(1.f),
      m_src(attr->value()),
      m_target(attr->value()),
      m_speed(1)
  {
    if (m_attr)
      m_attr->updateTransitionPointer(this);
  }

  template <typename T>
  TransitionAnimT<T>::TransitionAnimT(TransitionAnimT<T> && anim)
    : m_attr(anim.m_attr)
    , m_pos(anim.m_pos)
    , m_params(anim.m_params)
    , m_src(anim.m_src)
    , m_target(anim.m_target)
    , m_speed(anim.m_speed)
  {
    anim.m_attr = nullptr;
    if (m_attr)
      m_attr->updateTransitionPointer(this);
  }

  template <typename T>
  TransitionAnimT<T> & TransitionAnimT<T>::operator=(TransitionAnimT<T> && anim)
  {
    if (m_attr)
      m_attr->updateTransitionPointer(nullptr);

    m_attr = anim.m_attr;
    m_pos = anim.m_pos;
    m_params = anim.m_params;
    m_src = anim.m_src;
    m_target = anim.m_target;
    m_speed = anim.m_speed;

    anim.m_attr = nullptr;

    if (m_attr)
      m_attr->updateTransitionPointer(this);

    return *this;
  }

  template <typename T>
  TransitionAnimT<T>::~TransitionAnimT()
  {
    if (m_attr)
      m_attr->updateTransitionPointer(nullptr);
  }


  template <typename T>
  void TransitionAnimT<T>::update(float dt)
  {
    assert(!isNull());

    m_pos += dt * m_speed;
    if (m_speed < 0 && m_pos <= 0.f) {
      m_attr->setAnimatedValue(m_src);
    } else if (m_speed > 0 && m_pos >= 1.f) {
      m_attr->setAnimatedValue(m_target);
    } else if (m_pos >= 0.f && m_pos <= 1.f) {
      float y = m_params.timingFunction.y(m_pos);
      T val = TransitionInterpolator<T>::interpolate(m_src, m_target, y);
      m_attr->setAnimatedValue(val);
    }
  }
}

#endif // VALUABLE_TRANSITION_IMPL_HPP
