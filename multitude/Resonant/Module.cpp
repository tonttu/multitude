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

  Module::Module(Application * app)
    : m_application(app)
  {
    m_id[0] = 0;
  }

  Module::~Module()
  {}

  bool Module::prepare(int &, int &)
  {
    return true;
  }

  void Module::processMessage(const char *, Radiant::BinaryData *)
  {}

  bool Module::stop()
  {
    return true;
  }

  void Module::setId(const std::string & id)
  {
    m_id = id;
  }

}
