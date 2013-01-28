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
