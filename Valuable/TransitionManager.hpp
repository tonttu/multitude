/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef VALUABLE_TRANSITION_MANAGER_HPP
#define VALUABLE_TRANSITION_MANAGER_HPP

#include "Export.hpp"
#include "TransitionAnim.hpp"

#include <Radiant/ReentrantVector.hpp>

namespace Valuable
{

  /// Base class for type-specific transition managers
  class VALUABLE_API TransitionManager
  {
  public:
    TransitionManager();
    virtual ~TransitionManager();

    static void updateAll(float dt);
    static std::size_t activeTransitions();

    static const std::vector<TransitionManager*> & instances();

  private:
    virtual void update(float dt) = 0;
    virtual std::size_t countActiveTransitions() const = 0;
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
    static TransitionAnimT<T> * create(AttributeBaseT<T> * attr, TransitionParameters curve);

    static TransitionManagerT<T> & instance();

  private:
    void update(float dt) override;
    std::size_t countActiveTransitions() const override;

  private:
    Radiant::ReentrantVector<TransitionAnimT<T>> m_transitions;
  };

} // namespace Valuable

#endif // VALUABLE_TRANSITION_MANAGER_HPP
