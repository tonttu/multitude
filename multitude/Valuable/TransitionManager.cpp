#include "TransitionManager.hpp"

#include <algorithm>

namespace Valuable
{
  static std::vector<TransitionManager*> s_managers;

  TransitionManager::TransitionManager()
  {
    s_managers.push_back(this);
  }

  TransitionManager::~TransitionManager()
  {
    s_managers.erase(std::find(s_managers.begin(), s_managers.end(), this));
  }

  void TransitionManager::updateAll(float dt)
  {
    for (auto b: s_managers)
      b->update(dt);
  }
} // namespace Valuable
