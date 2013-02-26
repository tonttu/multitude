/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_VM1_HPP
#define LUMINOUS_VM1_HPP

/// @cond

#include "Export.hpp"

#include <Radiant/SerialPort.hpp>
#include <Radiant/Mutex.hpp>

#include <QMap>

namespace Luminous
{
  class ColorCorrection;

  // This class is internal to MultiTouch Ltd. Do not use this class. It will
  // be removed in future revisions.
  class LUMINOUS_API VM1
  {
  public:
    VM1();

    bool detected() const;
    void setColorCorrection(const ColorCorrection & cc);

    QString info();

    static QMap<QString, QString> parseInfo(const QString & info);

    QByteArray takeData();
    Radiant::SerialPort & open(bool & ok);

  private:
    QByteArray m_data;
    Radiant::Mutex m_dataMutex;
    Radiant::SerialPort m_port;
  };

}

/// @endcond

#endif // VM1_HPP
