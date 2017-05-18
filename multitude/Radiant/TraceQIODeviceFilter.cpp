#include "TraceQIODeviceFilter.hpp"

#include <QIODevice>

namespace Radiant
{
  namespace Trace
  {
    QIODeviceFilter::QIODeviceFilter(QIODevice * device)
      : Filter(ORDER_OUTPUT)
      , m_device(device)
    {
    }

    QIODeviceFilter::~QIODeviceFilter()
    {
      if (m_device && !m_device->parent())
        delete m_device;
    }

    bool QIODeviceFilter::trace(const Message & msg)
    {
      if (m_device && m_device->isOpen()) {
        if (msg.module.isEmpty()) {
          m_device->write(Radiant::Trace::severityText(msg.severity) + ": " + msg.text.toUtf8() + "\n");
        } else {
          m_device->write(Radiant::Trace::severityText(msg.severity) + ": " + msg.module + ": " + msg.text.toUtf8() + "\n");
        }
      }
      return false;
    }

  } // namespace Trace

} // namespace Radiant
