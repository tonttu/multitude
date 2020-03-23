#include "Nvml.hpp"

#include <Radiant/Trace.hpp>

#include <nvml/nvml.h>

#include <QLibrary>
#include <QMutex>

#include <thread>

#define STR(x) #x
#define NVML_FUNC(name) decltype(::name) * name = nullptr
#define RESOLVE_FUNC(name) name = reinterpret_cast<decltype(name)>(m_nvmlLib.resolve(STR(name)))

namespace Luminous
{
  class Nvml::DeviceQuery::D
  {
  public:
    D(std::shared_ptr<Nvml> && nvml, nvmlDevice_t dev, int openglIndex);
    void run();
    QStringList state() const;

  public:
    std::shared_ptr<Nvml> m_nvml;
    nvmlDevice_t m_dev = nullptr;
    int m_openglIndex = -1;

    mutable QMutex m_sampleMutex;
    Nvml::DeviceQuery::Sample m_mergedSample;
    bool m_mergeSample = false;
    std::thread m_pollThread;
    std::atomic<bool> m_running{true};
  };

  class Nvml::D
  {
  public:
    D();

  public:
    NVML_FUNC(nvmlInit);
    NVML_FUNC(nvmlShutdown);
    NVML_FUNC(nvmlDeviceGetCount);
    NVML_FUNC(nvmlDeviceGetHandleByPciBusId);
    NVML_FUNC(nvmlDeviceGetPcieThroughput);
    NVML_FUNC(nvmlDeviceGetPciInfo);
    NVML_FUNC(nvmlDeviceGetUtilizationRates);
    NVML_FUNC(nvmlEventSetCreate);
    NVML_FUNC(nvmlDeviceRegisterEvents);
    NVML_FUNC(nvmlEventSetWait);
    NVML_FUNC(nvmlEventSetFree);

    NVML_FUNC(nvmlDeviceGetBoardPartNumber);
    NVML_FUNC(nvmlDeviceGetFanSpeed);
    NVML_FUNC(nvmlDeviceGetUUID);
    NVML_FUNC(nvmlDeviceGetTemperature);
    NVML_FUNC(nvmlDeviceGetSerial);
    NVML_FUNC(nvmlDeviceGetName);
    NVML_FUNC(nvmlDeviceGetIndex);

  public:
    QLibrary m_nvmlLib;
  };

  Nvml::DeviceQuery::D::D(std::shared_ptr<Nvml> && nvml, nvmlDevice_t dev, int openglIndex)
    : m_nvml(std::move(nvml))
    , m_dev(dev)
    , m_openglIndex(openglIndex)
    , m_pollThread([this] { run(); })
  {
  }

  void Nvml::DeviceQuery::D::run()
  {
    char buffer[256]{};
    QString name = QString("GPU %1").arg(m_openglIndex);

    QStringList specs;
    if (m_nvml->m_d->nvmlDeviceGetSerial(m_dev, buffer, sizeof(buffer)) == NVML_SUCCESS)
      specs << QString("SN %1").arg(buffer);

    if (m_nvml->m_d->nvmlDeviceGetBoardPartNumber(m_dev, buffer, sizeof(buffer)) == NVML_SUCCESS)
      specs << QString("Board part number %1").arg(buffer);

    if (m_nvml->m_d->nvmlDeviceGetUUID(m_dev, buffer, sizeof(buffer)) == NVML_SUCCESS)
      specs << QString("UUID %1").arg(buffer);

    auto tmp = specs + state();

    QString info = name;
    if (!tmp.isEmpty())
      info += QString(": %1").arg(tmp.join(", "));

    Radiant::info("%s", info.toUtf8().data());

    nvmlEventSet_t set = nullptr;
    m_nvml->m_d->nvmlEventSetCreate(&set);
    nvmlReturn_t error = m_nvml->m_d->nvmlDeviceRegisterEvents(m_dev, nvmlEventTypeXidCriticalError, set);
    if (error != NVML_SUCCESS && error != NVML_ERROR_NOT_SUPPORTED) {
      Radiant::error("Failed to monitor Xid errors on %s [error code %d]", name.toUtf8().data(), error);
      m_nvml->m_d->nvmlEventSetFree(set);
      set = nullptr;
    }

    while (m_running) {
      Nvml::DeviceQuery::Sample sample;
      error = m_nvml->m_d->nvmlDeviceGetPcieThroughput(m_dev, NVML_PCIE_UTIL_RX_BYTES, &sample.pcieRxThroughputKBs);
      if (error != NVML_SUCCESS) {
        Radiant::error("Failed to monitor PCIe throughput on %s [error code %d]",
                       name.toUtf8().data(), error);
        break;
      }

      nvmlUtilization_t utilization {0, 0};
      m_nvml->m_d->nvmlDeviceGetUtilizationRates(m_dev, &utilization);
      sample.gpuUtilization = utilization.gpu / 100.0f;
      sample.memUtilization = utilization.memory / 100.0f;
      {
        QMutexLocker g(&m_sampleMutex);
        if (!m_mergeSample) {
          m_mergedSample = sample;
          m_mergeSample = true;
        } else {
          m_mergedSample.gpuUtilization = std::max(m_mergedSample.gpuUtilization, sample.gpuUtilization);
          m_mergedSample.memUtilization = std::max(m_mergedSample.memUtilization, sample.memUtilization);
          m_mergedSample.pcieRxThroughputKBs = std::max(m_mergedSample.pcieRxThroughputKBs, sample.pcieRxThroughputKBs);
        }
      }

      nvmlEventData_t data;
      if (set && m_nvml->m_d->nvmlEventSetWait(set, &data, 0) == NVML_SUCCESS) {
        QStringList tmp;
        tmp << name;
        tmp += specs + state();
        /// Print the whole ID string including serial number, part number and
        /// uuid so that we can quickly identify the GPU that has errors even
        /// if we don't have the beginning of the application log available
        /// anymore. Also print fan speed and temperature, since Xid errors are
        /// often related to overheating.
        Radiant::error("Nvidia GPU Xid error %llu on %s", data.eventData, tmp.join(", ").toUtf8().data());
      }
    }
    m_nvml->m_d->nvmlEventSetFree(set);
  }

  QStringList Nvml::DeviceQuery::D::state() const
  {
    QStringList ret;
    unsigned int value = 0;
    if (m_nvml->m_d->nvmlDeviceGetFanSpeed(m_dev, &value) == NVML_SUCCESS)
      ret << QString("Fan %1%").arg(value);

    if (m_nvml->m_d->nvmlDeviceGetTemperature(m_dev, NVML_TEMPERATURE_GPU, &value) == NVML_SUCCESS)
      ret << QString("Temperature %1 C").arg(value);

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  Nvml::Nvml()
    : m_d(new D())
  {
    if (m_d->nvmlInit)
      m_d->nvmlInit();
  }

  Nvml::~Nvml()
  {
    if (m_d->nvmlShutdown)
      m_d->nvmlShutdown();
  }

  std::shared_ptr<Nvml::DeviceQuery> Nvml::createDeviceQueryThread(const QByteArray & busId, int openglIndex) const
  {
    nvmlDevice_t dev = nullptr;
    if (m_d->nvmlDeviceGetHandleByPciBusId &&
        m_d->nvmlDeviceGetHandleByPciBusId(busId.data(), &dev) == NVML_SUCCESS)
      return std::make_shared<DeviceQuery>(s_multiSingletonInstance.lock(), dev, openglIndex);
    return nullptr;
  }

  /////////////////////////////////////////////////////////////////////////////

  Nvml::DeviceQuery::DeviceQuery(std::shared_ptr<Nvml> nvml, nvmlDevice_t dev, int openglIndex)
    : m_d(new D(std::move(nvml), dev, openglIndex))
  {
  }

  Nvml::DeviceQuery::~DeviceQuery()
  {
    m_d->m_running = false;
    m_d->m_pollThread.join();
  }

  Nvml::DeviceQuery::Sample Nvml::DeviceQuery::takePeakSample()
  {
    QMutexLocker g(&m_d->m_sampleMutex);
    m_d->m_mergeSample = false;
    return m_d->m_mergedSample;
  }

  Nvml::D::D()
  {
#ifdef RADIANT_WINDOWS
    m_nvmlLib.setFileName(qgetenv("PROGRAMFILES") + "/NVIDIA Corporation/NVSMI/nvml.dll");
#elif defined(RADIANT_LINUX)
    m_nvmlLib.setFileName("libnvidia-ml");
#endif
    if (!m_nvmlLib.load()) {
      // This is not really an error, since this library only exists if you have nvidia drivers installed
      Radiant::debug("Failed to load %s: %s", m_nvmlLib.fileName().toUtf8().data(),
                     m_nvmlLib.errorString().toUtf8().data());
      return;
    }

    RESOLVE_FUNC(nvmlInit);
    RESOLVE_FUNC(nvmlShutdown);
    RESOLVE_FUNC(nvmlDeviceGetCount);
    RESOLVE_FUNC(nvmlDeviceGetHandleByPciBusId);
    RESOLVE_FUNC(nvmlDeviceGetPcieThroughput);
    RESOLVE_FUNC(nvmlDeviceGetPciInfo);
    RESOLVE_FUNC(nvmlDeviceGetUtilizationRates);
    RESOLVE_FUNC(nvmlEventSetCreate);
    RESOLVE_FUNC(nvmlDeviceRegisterEvents);
    RESOLVE_FUNC(nvmlEventSetWait);
    RESOLVE_FUNC(nvmlEventSetFree);
    RESOLVE_FUNC(nvmlDeviceGetBoardPartNumber);
    RESOLVE_FUNC(nvmlDeviceGetFanSpeed);
    RESOLVE_FUNC(nvmlDeviceGetUUID);
    RESOLVE_FUNC(nvmlDeviceGetTemperature);
    RESOLVE_FUNC(nvmlDeviceGetSerial);
    RESOLVE_FUNC(nvmlDeviceGetName);
    RESOLVE_FUNC(nvmlDeviceGetIndex);
  }

  DEFINE_SINGLETON(Nvml)
} // namespace Luminous
