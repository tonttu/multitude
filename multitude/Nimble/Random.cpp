/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Random.hpp"

#include <chrono>

namespace Nimble {
  RandomUniform  RandomUniform::m_instance;

  uint64_t RandomUniform::randomSeed()
  {
    static std::random_device rnd;
    auto now = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    return rnd() ^ (static_cast<uint64_t>(rnd()) << uint64_t(32)) ^ ns;
  }

}
