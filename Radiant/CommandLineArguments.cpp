/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "CommandLineArguments.hpp"

namespace Radiant
{
  CommandLineArguments::CommandLineArguments(const QStringList & args)
    : m_argc(args.size())
  {
    std::vector<int> offsets;
    offsets.reserve(args.size());
    m_argv.reserve(args.size()+1);

    for (auto & str: args) {
      QByteArray arg = str.toUtf8();
      offsets.push_back(m_data.size());
      m_data += arg;
      m_data += '\0';
    }

    for (auto offset: offsets) {
      m_argv.push_back(m_data.data() + offset);
    }
    m_argv.push_back(nullptr);
  }

  CommandLineArguments::~CommandLineArguments()
  {
  }

} // namespace Radiant
