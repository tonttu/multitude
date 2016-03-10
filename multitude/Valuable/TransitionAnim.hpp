#ifndef VALUABLE_TRANSITION_ANIM_HPP
#define VALUABLE_TRANSITION_ANIM_HPP

#include <cassert>

namespace Valuable
{
  /// Simple class to store parameters of the transition curve, such as
  /// duration, delay and timing function?
  class TransitionCurve
  {
  public:
    TransitionCurve(float durationSeconds=-1.f, float delaySeconds=0.f)
      : duration(durationSeconds),
        delay(delaySeconds)
    {}

    bool isValid() const { return duration > 0.f; }

    // Both are in seconds
    float duration;
    float delay;

    /// @todo timing function
  };

  // ---------------------------------------------------------------------------

  template <typename T> class AttributeBaseT;

  class TransitionTypeData
  {
  public:
    TransitionTypeData()
      : transitionUpdated(false),
        deleted(false)
    {}

    bool transitionUpdated;
    bool deleted;
  };

  template <typename T>
  class TransitionTypeDataT : public TransitionTypeData
  {
  public:
    TransitionTypeDataT()
      : TransitionTypeData(),
        attr(nullptr)
    {}

    T src;
    T target;
    AttributeBaseT<T>* attr;
  };

  // ------------------------------------------------------------------------

  class TransitionManager;
  template <typename T> class TransitionManagerT;


  /// Defines attribute transition animation and its state
  class TransitionAnim
  {
  public:
    /// Creates new transition animation
    /// @param t attribute that this object animates, or null if this a null animation
    inline TransitionAnim(TransitionManager* manager=nullptr)
      : m_pos(1.f),
        m_delaySeconds(0),
        m_speed(1),
        m_typeData(nullptr),
        m_manager(manager)
    {}

    /// This should only be deleted from TransitionManager
    inline ~TransitionAnim()
    {
    }

    inline void setParameters(TransitionCurve curve)
    {
      if(curve.duration <= 0.f) {
        m_typeData->deleted = true;
      } else {
        setDuration(curve.duration);
        setDelay(curve.delay);
      }

    }

    inline void setDuration(float durationSeconds)
    {
      assert(durationSeconds > 0.f);
      m_speed = 1.0f / durationSeconds;
    }

    inline void setDelay(float delaySeconds)
    {
      m_delaySeconds = delaySeconds;
    }

    inline float normalizedPos() const
    {
      return m_pos;
    }

    inline void updatePosition(float dt)
    {
      m_pos += dt * m_speed;
    }

    inline void updateTargetAttributePointer();

    inline void remove();

    /// Checks if this animation is deleted
    inline bool isNull() const { return m_typeData == nullptr; }

    /// Makes this animation null
    inline void setNull()
    {
      delete m_typeData;
      m_typeData = nullptr;
    }

    inline TransitionTypeData* typeData()
    {
      return m_typeData;
    }

    inline void setTypeData(TransitionTypeData* data)
    {
      if(m_typeData) {
        delete m_typeData;
      }
      m_typeData = data;
    }

    /// Update the animation, update the value to m_attr
    /// This should only be called if the transition animation is active
    inline void update(float dt);

  private:
    /// Transition position, 0 == beginning, 1 == at the end.
    /// Note that the value itself isn't clamped
    float m_pos;

    /// Transition delay, in seconds
    float m_delaySeconds;

    /// Transition timing function
    /* m_func */

    /// Transition speed (1 / duration)
    float m_speed;

    TransitionTypeData * m_typeData;

    TransitionManager * m_manager;

    friend class TransitionManager;
    template <typename T> friend class TransitionManagerT;
  };

  // ------------------------------------------------------------------------
}

#endif // VALUABLE_TRANSITION_ANIM_HPP
