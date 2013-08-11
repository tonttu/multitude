#ifndef VALUABLE_TRANSITION_MANAGER_HPP
#define VALUABLE_TRANSITION_MANAGER_HPP

#include "Export.hpp"

#include "TransitionAnim.hpp"

#include <list>
#include <vector>

namespace Valuable
{
  /// Base class for type-specific transition managers
  class VALUABLE_API TransitionManager
  {
  public:
    TransitionManager();
    virtual ~TransitionManager();

    static void updateAll(float dt);

  private:
    virtual void update(float dt) = 0;
  };

  /// Type-specific transition animation manager
  /// @param T type to animate, same type that is given as template
  ///          parameter to Valuable::AttributeT.
  template <typename T>
  class TransitionManagerT : public TransitionManager
  {
  public:
    /// Creates a new animation for given attribute
    /// @param attr attribute that the transition animation controls
    static TransitionAnimT<T> * create(AttributeBaseT<T> * attr);
    /// @param anim Transition animation to remove
    static void remove(TransitionAnimT<T> * anim);

  private:
    virtual void update(float dt) FINAL;

  private:
    /// Store all animations of this type in a list of vectors.
    /// These vectors are never resized, they are originally pre-allocated to
    /// contain null animations that can then later be used. Ideally only the
    /// first vector is actually used, but if it really runs out of animations,
    /// the next one is allocated, doubling the capacity of the previous one.
    std::list<std::vector<TransitionAnimT<T>>> m_storage;
  };

} // namespace Valuable

#endif // VALUABLE_TRANSITION_MANAGER_HPP
