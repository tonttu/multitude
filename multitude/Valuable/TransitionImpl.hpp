#ifndef VALUABLE_TRANSITION_IMPL_HPP
#define VALUABLE_TRANSITION_IMPL_HPP

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
  /// @todo make sure we don't use this version with ints
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
  TransitionAnimT<T>::TransitionAnimT(AttributeBaseT<T> * t)
    : m_active(false),
      m_pos(1.f),
      m_delaySeconds(0),
      m_speed(1),
      m_src(),
      m_target(),
      m_attr(t) {}

  template <typename T>
  void TransitionAnimT<T>::setTarget(T newTarget)
  {
    /// @todo handle animation interrupting right
    m_src = m_attr->m_currentValue;
    m_target = newTarget;
    m_pos = -m_delaySeconds * m_speed;
    m_active = true;
  }

  template <typename T>
  void TransitionAnimT<T>::update(float dt)
  {
    assert(m_attr);
    m_pos += dt * m_speed;
    if (m_pos >= 1.0f) {
      m_active = false;
      m_attr->setAnimatedValue(m_target);
    } else if (m_pos >= 0.0f) {
      m_attr->setAnimatedValue(TransitionInterpolator<T>::interpolate(m_src, m_target, m_pos));
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <typename T>
  TransitionAnimT<T> * TransitionManagerT<T>::create(AttributeBaseT<T> * attr)
  {
    static TransitionManagerT<T> s_mgr;
    for (auto & v: s_mgr.m_storage) {
      for (auto & n: v) {
        if (n.isNull()) {
          n = TransitionAnimT<T>(attr);
          return &n;
        }
      }
    }

    int size = s_mgr.m_storage.empty() ? 50 : s_mgr.m_storage.rbegin()->size() * 2;
    s_mgr.m_storage.push_back(std::vector<TransitionAnimT<T>>(size));
    s_mgr.m_storage.back()[0] = TransitionAnimT<T>(attr);
    return &s_mgr.m_storage.back()[0];
  }

  template <typename T>
  void TransitionManagerT<T>::remove(TransitionAnimT<T> * anim)
  {
    /// @todo check if m_storage could be optimized
    anim->setNull();
  }

  template <typename T>
  void TransitionManagerT<T>::update(float dt)
  {
    for (auto & v: m_storage)
      for (auto & n: v)
        if (n.isActive())
          n.update(dt);
  }
}

#endif // VALUABLE_TRANSITION_IMPL_HPP
