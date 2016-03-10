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
  void TransitionManagerT<T>::update(TransitionAnim& anim, float dt) const
  {
    TransitionTypeDataT<T>* tData = static_cast<TransitionTypeDataT<T>*>(anim.typeData());

    tData->transitionUpdated = true;

    if(tData->deleted) {
      return;
    }

    float pos = anim.m_pos + dt * anim.m_speed;
    if(pos >= 1.f) {
      tData->attr->setAnimatedValue(tData->target, dt);
    } else if(pos >= 0.f) {
      T val = TransitionInterpolator<T>::interpolate(tData->src, tData->target, pos);
      tData->attr->setAnimatedValue(val, dt);
    }
  }

  template <typename T>
  TransitionAnim * TransitionManagerT<T>::create(AttributeBaseT<T> * attr, TransitionCurve curve)
  {
    static TransitionManagerT<T> s_mgr;

    assert(curve.isValid());

    TransitionAnim* anim = TransitionManager::createAnim(&s_mgr);

    TransitionTypeDataT<T> * tData = new TransitionTypeDataT<T>();
    tData->attr = attr;
    anim->setTypeData(tData);
    anim->setParameters(curve);

    return anim;
  }

  template <typename T>
  void TransitionManagerT<T>::setTarget(TransitionAnim* anim, T target)
  {
    TransitionTypeDataT<T>* tData = static_cast<TransitionTypeDataT<T>*>(anim->typeData());
    if(tData->deleted)
      return;

    tData->transitionUpdated = false;
    tData->src = tData->attr->value();
    tData->target = target;
    anim->m_pos = -anim->m_delaySeconds * anim->m_speed;
  }

  template <typename T>
  void TransitionManagerT<T>::remove(TransitionAnim * anim)
  {
    TransitionTypeDataT<T>* tData = static_cast<TransitionTypeDataT<T>*>(anim->typeData());
    if(tData) {
      assert(tData->attr);
      tData->deleted = true;
      tData->attr->updateTransitionPointer(nullptr);
    }
  }

  template <typename T>
  void TransitionManagerT<T>::updateTargetAttributePointer(TransitionAnim * anim)
  {
    TransitionTypeDataT<T>* tData = static_cast<TransitionTypeDataT<T>*>(anim->typeData());
    if(tData) {
      assert(tData->attr);
      tData->attr->updateTransitionPointer(anim);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  void TransitionAnim::update(float dt)
  {
    m_manager->update(*this, dt);
  }

  void TransitionAnim::remove()
  {
    m_manager->remove(this);
  }

  void TransitionAnim::updateTargetAttributePointer()
  {
    m_manager->updateTargetAttributePointer(this);
  }
}

#endif // VALUABLE_TRANSITION_IMPL_HPP
