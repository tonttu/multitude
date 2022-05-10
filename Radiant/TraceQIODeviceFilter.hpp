#ifndef RADIANT_TRACE_QIODEVICE_FILTER_HPP
#define RADIANT_TRACE_QIODEVICE_FILTER_HPP

#include "Trace.hpp"

class QIODevice;

namespace Radiant
{
  namespace Trace
  {
    /// Send all messages to given QIODevice instance.
    /// Example use (after creating QApplication or calling
    /// MultiWidgets::Application::init):
    /// @code
    /// QUdpSocket * socket = new QUdpSocket();
    /// socket->connectToHost("my-linux-box", 12345, QIODevice::WriteOnly);
    /// Radiant::Trace::addFilter(std::make_shared<Radiant::Trace::QIODeviceFilter>(socket));
    /// @endcode
    /// And then on my-linux-box:
    /// nc -l -u -p 12345
    class RADIANT_API QIODeviceFilter : public Filter
    {
    public:
      /// @param device already opened QIODevice. This class will take the
      /// ownership of the device if it doesn't have a parent QObject
      QIODeviceFilter(QIODevice * device);
      virtual ~QIODeviceFilter();

      bool trace(Message & msg) override;

    private:
      QIODevice * m_device;
    };
  } // namespace Trace
} // namespace Radiant

#endif // RADIANT_TRACE_QIODEVICE_FILTER_HPP
