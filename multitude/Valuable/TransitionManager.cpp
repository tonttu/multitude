#include "TransitionManager.hpp"
#include "TransitionAnim.hpp"
#include "TransitionImpl.hpp"

#include <algorithm>
#include <queue>
#include <list>
#include <utility>

#include <cassert>

namespace Valuable
{
  /// @todo Radiant::ReentrantVector here?
  static std::vector<TransitionManager*> s_managers;

  TransitionManager::TransitionManager()
  {
    s_managers.push_back(this);
  }

  TransitionManager::~TransitionManager()
  {
    s_managers.erase(std::find(s_managers.begin(), s_managers.end(), this));
  }

  std::size_t TransitionManager::activeTransitions()
  {
    std::size_t count = 0;
    for (TransitionManager * mgr: s_managers)
      count += mgr->countActiveTransitions();
    return count;
  }

  void TransitionManager::updateAll(float dt)
  {
    for (auto & mgr: s_managers)
      mgr->update(dt);
  }

  const std::vector<TransitionManager*> & TransitionManager::instances()
  {
    return s_managers;
  }
} // namespace Valuable
