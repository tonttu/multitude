/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RESONANT_SOURCEINFO_HPP
#define RESONANT_SOURCEINFO_HPP

#include <QString>

namespace Resonant
{
  struct SourceInfo
  {
    /// Source name, for example "<alsa_input.pci-0000_82_00.0.analog-stereo>"
    QString name;
    /// Alsa card number
    int alsaCard = -1;
    /// Card name, for example "VisionSC-HD4+ Digital Node:4"
    QString alsaName;
  };
}

#endif // RESONANT_SOURCEINFO_HPP
