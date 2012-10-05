/* COPYRIGHT
 *
 * This file is part of Resonant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Resonant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "Module.hpp"

#include <Radiant/Trace.hpp>

#include <string.h>

namespace Resonant {

  using Radiant::error;

  Module::Module()
  {
  }

  Module::~Module()
  {}

  bool Module::prepare(int &, int &)
  {
    return true;
  }

  void Module::processMessage(const QByteArray &, Radiant::BinaryData &)
  {}

  bool Module::stop()
  {
    return true;
  }

  void Module::setId(const QString & id)
  {
    m_id = id;
  }

}
