/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_VM1_HPP
#define LUMINOUS_VM1_HPP

/// @cond

#include "Export.hpp"

#include <Radiant/SerialPort.hpp>
#include <Radiant/Mutex.hpp>

#include <QMap>

#include <memory>

namespace Luminous
{
  class ColorCorrection;

  // This class is internal to MultiTouch Ltd. Do not use this class. It will
  // be removed in future revisions.
  class LUMINOUS_API VM1
  {
  public:
    VM1();
    ~VM1();

    bool detected() const;
    void setColorCorrection(const ColorCorrection & cc);
    void setLCDPower(bool enable);
    void setLogoTimeout(int timeout);
    void setVideoAutoselect();
    void setVideoInput(int input);
    void setVideoInputPriority(int input);
    void enableGamma(bool state);

    QString info();

    static QMap<QString, QString> parseInfo(const QString & info);

  private:
    class D;
    std::shared_ptr<D> m_d;
  };

}

/// @endcond

#endif // VM1_HPP
