#ifndef LUMINOUS_SCREENDETECTOR_HPP
#define LUMINOUS_SCREENDETECTOR_HPP

#include "Export.hpp"

#include <Nimble/Rect.hpp>

#include <QString>
#include <QList>

namespace Luminous
{

  class ScreenInfo
  {
  public:
    ScreenInfo() : m_logicalScreen(0) {}

    /// For example "GPU-0.DFP-3"
    QString id() const { return m_gpu + "." + m_connection; }

    /// Display name got from EDID
    const QString & name() const { return m_name; }
    void setName(const QString & name) { m_name = name; }

    /// For example "GPU-0" or "GPU-0,GPU-1"
    const QString & gpu() const { return m_gpu; }
    void setGpu(const QString & gpu) { m_gpu = gpu; }

    /// Display adapter name, for example "GeForce 9800 GT"
    const QString & gpuName() const { return m_gpuName; }
    void setGpuName(const QString & gpuName) { m_gpuName = gpuName; }

    /// For example "DFP-0"
    const QString & connection() const { return m_connection; }
    void setConnection(const QString & connection) { m_connection = connection; }

    /// With X11 this is the X screen number
    int logicalScreen() const { return m_logicalScreen; }
    void setLogicalScreen(int logicalScreen) { m_logicalScreen = logicalScreen; }

    /// Size and location relative to this logical screen
    const Nimble::Recti & geometry() const { return m_geometry; }
    void setGeometry(const Nimble::Recti & geometry) { m_geometry = geometry; }

  private:
    QString m_name;
    QString m_gpu;
    QString m_gpuName;
    QString m_connection;
    int m_logicalScreen;
    Nimble::Recti m_geometry;
  };

  class LUMINOUS_API ScreenDetector
  {
  public:
    void scan();
    inline const QList<ScreenInfo> & results() const { return m_results; }

  private:
    QList<ScreenInfo> m_results;
  };

}

#endif // LUMINOUS_SCREENDETECTOR_HPP
