/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "TraceDuplicateFilter.hpp"

namespace Radiant
{
  namespace Trace
  {
    DuplicateFilter::DuplicateFilter()
      : Filter(ORDER_DEFAULT_FILTERS + 1.0f)
    {}

    bool DuplicateFilter::trace(Message & message)
    {
      Guard g(m_msgMutex);
      if (m_prevMessage == message) {
        return true;
      }

      m_prevMessage = message;
      return false;
    }

  } // namespace Trace
} // namespace Radiant
