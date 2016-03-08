#ifndef VALUABLE_TRANSITION_MANAGER_HPP
#define VALUABLE_TRANSITION_MANAGER_HPP

#include "Export.hpp"

#include <list>
#include <vector>

#include "TransitionAnim.hpp"

namespace Valuable
{

  /// Base class for type-specific transition managers
  class VALUABLE_API TransitionManager
  {
  public:
    TransitionManager();
    virtual ~TransitionManager();

    static void updateAll(float dt);

    virtual void update(TransitionAnim& anim, float dt) const = 0;
    virtual void remove(TransitionAnim * anim) = 0;
    virtual void updateTargetAttributePointer(TransitionAnim* anim) = 0;

    static std::size_t activeTransitions();

  protected:
    static TransitionAnim* createAnim(TransitionManager* manager);
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
    static TransitionAnim * create(AttributeBaseT<T> * attr, TransitionCurve curve);
    /// @param anim Transition animation to remove
    virtual void remove(TransitionAnim * anim) OVERRIDE;

    static void setTarget(TransitionAnim*, T target);

    virtual void updateTargetAttributePointer(TransitionAnim* anim) override;

  private:
    virtual void update(TransitionAnim& anim, float dt) const FINAL;
  };

} // namespace Valuable

#endif // VALUABLE_TRANSITION_MANAGER_HPP
