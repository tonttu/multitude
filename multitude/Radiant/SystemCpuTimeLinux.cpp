#include "SystemCpuTime.hpp"

#include <QFile>
#include <QTextStream>

namespace Radiant
{
  struct CpuTimes
  {
    uint64_t user = 0;
    uint64_t nice = 0;
    uint64_t system = 0;
    uint64_t idle = 0;
    uint64_t iowait = 0;
    uint64_t irq = 0;
    uint64_t softirq = 0;
    uint64_t steal = 0;
  };

  CpuTimes cpuTimes()
  {
    CpuTimes ret;
    QFile procStat("/proc/stat");
    if (procStat.open(QFile::ReadOnly)) {
      // atEnd doesn't work with /proc/stat
      while (true) {
        const QString line = QString::fromUtf8(file.readLine().trimmed());
        if (line.isEmpty()) break;

        QTextStream stream(&line);
        QString prefix;
        stream >> prefix;
        if (prefix == "cpu") {
          stream >> ret.user >> ret.nice >> ret.system >> ret.idle >> ret.iowait >> ret.irq >> ret.softirq >> ret.steal;
          return ret;
        }
      }
      Radiant::warning("SystemCpuTime # Failed to parse /proc/stat");
    } else {
      Radiant::warning("SystemCpuTime # Failed to open /proc/stat: %s",
          procStat.errorString().toUtf8().data());
    }
    return ret;
  }

  class SystemCpuTime::D
  {
  public:
    CpuTimes cpuTimes;
  };

  SystemCpuTime::SystemCpuTime()
    : m_d(new D())
  {
    reset();
  }

  SystemCpuTime::~SystemCpuTime()
  {
  }

  double SystemCpuTime::cpuLoad() const
  {
    const CpuTimes now = cpuTimes();
    // See https://github.com/Leo-G/DevopsWiki/wiki/How-Linux-CPU-Usage-Time-and-Percentage-is-calculated
    const double idle = (now.idle + now.iowait) - (m_d->cpuTimes.idle + m_d->cpuTimes.iowait);
    const double total = (now.user + now.nice + now.system + now.irq + now.softirq + now.steal) -
        (m_d->cpuTimes.user + m_d->cpuTimes.nice + m_d->cpuTimes.system + m_d->cpuTimes.irq +
          m_d->cpuTimes.softirq + m_d->cpuTimes.steal) + idle;
    return total > 0 ? 1.0 - idle / total: 0;
  }

  void SystemCpuTime::reset()
  {
    m_d->cpuTimes = cpuTimes();
  }
} // namespace Radiant
