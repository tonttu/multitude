#ifndef VALUABLE_TRANSITION_ANIM_HPP
#define VALUABLE_TRANSITION_ANIM_HPP

#include <cassert>
#include <array>

#include <Nimble/Vector2.hpp>

namespace Valuable
{
  /// Bezier curve that has hard-coded first point at (0, 0) and last point
  /// at (1, 1).
  /// See https://drafts.csswg.org/css-timing-1/#cubic-bezier-timing-functions
  class BezierTimingFunction
  {
  public:
    float y(float x) const
    {
      const float t = solveT(x);
      return evalY(t);
    }

    float evalY(float t) const
    {
      const float n = 1.f - t;
      return 3.f*n*n*t*m_points[0].y + 3.f*n*t*t*m_points[1].y + t*t*t;
    }

    float evalX(float t) const
    {
      const float n = 1.f - t;
      return 3.f*n*n*t*m_points[0].x + 3.f*n*t*t*m_points[1].x + t*t*t;
    }

    float derivateX(float t) const
    {
      return -3.0f * (- m_points[0].x * (3.f*t*t - 4.f*t + 1.f) + t * (3.f*m_points[1].x*t - 2.f*m_points[1].x - t));
    }

    float solveT(float x) const
    {
      float t = x;

      for (int i = 0; i < 5; ++i) {
        const float v = evalX(t);
        const float error = x - v;
        if (std::abs(error) < 0.0001f)
          break;
        const float d = derivateX(t);
        t = Nimble::Math::Clamp(t + 0.9f * error / d, 0.f, 1.f);
      }

      return t;
    }

    BezierTimingFunction(Nimble::Vector2f p1, Nimble::Vector2f p2)
      : m_points{{p1, p2}} {}

    const std::array<Nimble::Vector2f, 2> & points() const { return m_points; }

  private:
    std::array<Nimble::Vector2f, 2> m_points;
  };

  /// Simple class to store parameters of the transition curve, such as
  /// duration, delay and timing function
  class TransitionParameters
  {
  public:
    TransitionParameters(float durationSeconds=-1.f, float delaySeconds=0.f)
      : duration(durationSeconds),
        delay(delaySeconds)
    {}

    bool isValid() const { return duration > 0.f; }

    // Both are in seconds
    float duration;
    float delay;

    BezierTimingFunction timingFunction{ {0, 0}, {1, 1} };
  };

  // ------------------------------------------------------------------------

  class TransitionManager;
  template <typename T> class TransitionManagerT;

  template <typename T> class AttributeBaseT;

  /// Defines attribute transition animation and its state
  template <typename T>
  class TransitionAnimT
  {
  public:
    /// Creates new transition animation
    /// @param attr attribute that this object animates
    inline TransitionAnimT(AttributeBaseT<T> * attr);

    TransitionAnimT(const TransitionAnimT<T> &) = delete;
    TransitionAnimT<T> & operator=(const TransitionAnimT<T> &) = delete;

    inline TransitionAnimT(TransitionAnimT<T> && anim);

    inline TransitionAnimT<T> & operator=(TransitionAnimT<T> && anim);

    /// This should only be deleted from TransitionManager
    inline ~TransitionAnimT();

    inline void setParameters(TransitionParameters params)
    {
      assert(params.isValid());
      m_params = params;
      m_speed = (m_speed >= 0 ? 1.0f : -1.0f) / params.duration;
    }

    inline const TransitionParameters & parameters() const
    {
      return m_params;
    }

    inline T target() const
    {
      return m_target;
    }

    inline bool isActive() const
    {
      return ((m_speed > 0 && m_pos < 1.f) || (m_speed < 0 && m_pos > 0.f)) && !isNull();
    }

    inline void setNull()
    {
      m_attr = nullptr;
    }

    /// Checks if this animation is deleted
    inline bool isNull() const
    {
      return m_attr == nullptr;
    }

    inline void setTarget(T src, T target)
    {
      if (isActive()) {
        if (m_speed > 0 && target == m_src) {
          m_speed = -1.0f / m_params.duration;
          return;
        } else if (m_speed < 0 && target == m_target) {
          m_speed = 1.0f / m_params.duration;
          return;
        }
      }
      m_src = src;
      m_target = target;
      m_speed = 1.0f / m_params.duration;
      m_pos = -m_params.delay * m_speed;
    }

    /// Update the animation, update the value to m_attr
    /// This should only be called if the transition animation is active
    inline void update(float dt);

  private:
    AttributeBaseT<T> * m_attr = nullptr;

    /// Transition position, 0 == beginning, 1 == at the end.
    /// Note that the value itself isn't clamped
    float m_pos;

    TransitionParameters m_params;

    T m_src;
    T m_target;

    /// Transition speed (1 / duration)
    float m_speed;

    friend class TransitionManager;
    template <typename Y> friend class TransitionManagerT;
  };

  // ------------------------------------------------------------------------
}

#endif // VALUABLE_TRANSITION_ANIM_HPP
