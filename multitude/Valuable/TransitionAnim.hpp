#ifndef VALUABLE_TRANSITION_ANIM_HPP
#define VALUABLE_TRANSITION_ANIM_HPP

namespace Valuable
{
  template <typename T> class AttributeBaseT;
  template <typename T> class TransitionManagerT;

  /// Defines attribute transition animation and its state
  template <typename T>
  class TransitionAnimT
  {
  public:
    /// Creates new transition animation
    /// @param t attribute that this object animates, or null if this a null animation
    inline TransitionAnimT(AttributeBaseT<T> * t = nullptr);

    /// This should only be deleted from TransitionManagerT
    inline ~TransitionAnimT() {}

    /// Starts new transition animation to newTarget
    /// @param newTarget target value
    inline void setTarget(T newTarget);

    inline void setDuration(float durationSeconds)
    {
      m_speed = 1.0f / durationSeconds;
    }

    inline void setDelay(float delaySeconds)
    {
      m_delaySeconds = delaySeconds;
    }

  private:
    /// Checks if this animation is null, i.e. it can be reused
    inline bool isNull() const { return m_attr == nullptr; }

    /// Is this animation still alive, i.e. should update() be called
    inline bool isActive() const { return m_active; }

    /// Makes this animation null
    inline void setNull()
    {
      m_active = false;
      m_attr = nullptr;
    }

    /// Update the animation, update the value to m_attr
    /// This should only be called if the transition animation is active
    inline void update(float dt);

  private:
    /// Is the animation still running
    bool m_active;

    /// Transition position, 0 == beginning, 1 == at the end.
    /// Note that the value itself isn't clamped
    float m_pos;

    /// Transition delay, in seconds
    float m_delaySeconds;

    /// Transition timing function
    /* m_func */

    /// Transition speed (1 / duration)
    float m_speed;

    /// Animate from
    T m_src;

    /// Animate to
    T m_target;

    /// Attribute that this object animates, or null if this is a null animation
    AttributeBaseT<T> * m_attr;

    friend class TransitionManagerT<T>;
  };
}

#endif // VALUABLE_TRANSITION_ANIM_HPP
