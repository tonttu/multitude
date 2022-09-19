/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RADIANT_TRACE_DUPLICATE_FILTER_HPP
#define RADIANT_TRACE_DUPLICATE_FILTER_HPP

#include "Trace.hpp"
#include "Mutex.hpp"

namespace Radiant
{
  namespace Trace
  {
    /// Drops duplicate messages
    class RADIANT_API DuplicateFilter : public Filter
    {
    public:
      /// Creates a new filter using order ORDER_DEFAULT_FILTERS + 1, so that
      /// this filter is executed after the other default filters
      DuplicateFilter();

      bool trace(Message & message) override;

    private:
      Message m_prevMessage;
      Mutex m_msgMutex;
    };
  } // namespace Trace
} // namespace Radiant

#endif // RADIANT_TRACE_DUPLICATE_FILTER_HPP
