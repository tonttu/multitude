#pragma once

#include "Export.hpp"

#include <Radiant/Singleton.hpp>

#include <QByteArray>

typedef struct nvmlDevice_st * nvmlDevice_t;

namespace Luminous
{
  /// Wrapper for Nvidia Management Library
  class LUMINOUS_API Nvml
  {
    DECLARE_SINGLETON(Nvml);

  public:
    /// Separate thread that polls PCIe RX (data transfer from host to GPU)
    /// throughput, GPU utilization values and Xid errors roughly every 20 ms.
    class LUMINOUS_API DeviceQuery
    {
    public:
      struct Sample
      {
        uint32_t pcieRxThroughputKBs = 0;
        float gpuUtilization = 0;
        float memUtilization = 0;
      };

    public:
      DeviceQuery(std::shared_ptr<Nvml> nvml, nvmlDevice_t dev, int openglIndex);
      ~DeviceQuery();

      Sample latestSample() const;

    private:
      class D;
      std::unique_ptr<D> m_d;
    };

  public:
    ~Nvml();

    std::shared_ptr<DeviceQuery> createDeviceQueryThread(const QByteArray & busId, int openglIndex) const;

  private:
    Nvml();

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
} // namespace Luminous
