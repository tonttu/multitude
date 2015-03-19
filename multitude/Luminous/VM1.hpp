#ifndef LUMINOUS_VM1_HPP
#define LUMINOUS_VM1_HPP

#include "Export.hpp"

#include <Radiant/SerialPort.hpp>
#include <Radiant/Mutex.hpp>
#include <Radiant/RefPtr.hpp>

#include <QMap>

namespace Luminous
{
  class ColorCorrection;
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

    /// HACK - fix this when merging into taction-2.0!
    ///
    /// We need a single entry point for VM1 so we can ensure exclusive access
    /// (for example when writing a new edid). This is already impelmented in
    /// taction-2.0 and the edid writing code would ideally go there. But we also
    /// need this in the taction brach. So expose this internal mutex to enable
    /// exclusive access to VM1.
    static Radiant::Mutex & vm1Mutex();

  private:
    class D;
    std::shared_ptr<D> m_d;
  };

}
#endif // VM1_HPP
