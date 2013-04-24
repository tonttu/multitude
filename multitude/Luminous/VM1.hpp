#ifndef LUMINOUS_VM1_HPP
#define LUMINOUS_VM1_HPP

#include "Export.hpp"

#include <Radiant/SerialPort.hpp>
#include <Radiant/Mutex.hpp>

#include <QMap>

namespace Luminous
{
  class ColorCorrection;
  class LUMINOUS_API VM1
  {
  public:
    VM1();

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

    QByteArray takeData();
    Radiant::SerialPort & open(bool & ok);

  private:
    void sendCommand(const QByteArray & ba);

    QByteArray m_data;
    Radiant::Mutex m_dataMutex;
    Radiant::SerialPort m_port;
  };

}
#endif // VM1_HPP
