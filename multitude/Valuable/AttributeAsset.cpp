#include "AttributeAsset.hpp"
#include "FileWatcher.hpp"

#include <QFileInfo>

namespace Valuable
{

  class AttributeAsset::D
  {
  public:
    D(AttributeAsset& host);
    void checkFileMonitor();

    void ensureMonitoring();

    AttributeAsset& m_host;
    Valuable::FileWatcher m_watcher;
    bool m_monitorFile;
  };

  // ------------------------------------------------------------------------

  AttributeAsset::D::D(AttributeAsset &host)
    : m_host(host),
      m_monitorFile(false)
  {
    m_watcher.eventAddListener("file-created", [this] { m_host.emitChange(); });
    m_watcher.eventAddListener("file-changed", [this] { m_host.emitChange(); });
    m_watcher.eventAddListener("file-removed", [this] { m_host.emitChange(); });

    m_host.addListener([this] { checkFileMonitor(); });
  }

  void AttributeAsset::D::checkFileMonitor()
  {
    if(!m_monitorFile) {
      m_watcher.clear();
    } else {
      ensureMonitoring();
    }
  }

  void AttributeAsset::D::ensureMonitoring()
  {
    QString path = m_host.asString();
    if(path.isEmpty())
      return;

    QStringList files = m_watcher.files();
    QFileInfo fi(path);

    bool isWatching = files.contains(fi.absoluteFilePath());
    if(!isWatching) {
      m_watcher.clear();
      m_watcher.addPath(path);
    }
  }

  // ------------------------------------------------------------------------

  AttributeAsset::AttributeAsset()
    : m_d(new D(*this))
  {}

  AttributeAsset::AttributeAsset(Node *host, const QByteArray &name,
                                 const QString &filePath)
    : AttributeString(host, name, filePath),
      m_d(new D(*this))
  {}

  AttributeAsset::~AttributeAsset()
  {
    delete m_d;
  }

  bool AttributeAsset::isMonitoringFile() const
  {
    return m_d->m_monitorFile;
  }

  void AttributeAsset::setToMonitorFile(bool monitor)
  {
    m_d->m_monitorFile = monitor;
    m_d->checkFileMonitor();
  }

  bool AttributeAsset::operator==(const QString& that) const
  {
    return QFileInfo(that).absoluteFilePath() == QFileInfo(*this).absoluteFilePath();
  }

  bool AttributeAsset::operator!=(const QString& that) const
  {
    return !(*this == that);
  }

}
