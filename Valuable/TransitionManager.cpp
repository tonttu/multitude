/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

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
