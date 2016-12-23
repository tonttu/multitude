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

    void initWatcher();

    AttributeAsset& m_host;
    std::unique_ptr<Valuable::FileWatcher> m_watcher;
    bool m_wantToMonitor;
  };

  // ------------------------------------------------------------------------

  AttributeAsset::D::D(AttributeAsset &host)
    : m_host(host),
      m_watcher(nullptr),
      m_wantToMonitor(false)
  {
    m_host.addListener([this] { checkFileMonitor(); });
  }

  void AttributeAsset::D::initWatcher()
  {
    assert(!m_watcher);

    m_watcher.reset(new Valuable::FileWatcher());

    m_watcher->eventAddListener("file-created", [this] { m_host.emitChange(); });
    m_watcher->eventAddListener("file-changed", [this] { m_host.emitChange(); });
    m_watcher->eventAddListener("file-removed", [this] { m_host.emitChange(); });
  }

  void AttributeAsset::D::checkFileMonitor()
  {
    if(!m_wantToMonitor) {
      m_watcher.reset();
    } else {
      ensureMonitoring();
    }
  }

  void AttributeAsset::D::ensureMonitoring()
  {
    QString path = m_host.asString();
    if(path.isEmpty())
      return;

    if(!m_watcher) {
      initWatcher();
      m_watcher->addPath(path);
    } else {
      QStringList files = m_watcher->files();
      QFileInfo fi(path);
      if(!files.contains(fi.absoluteFilePath())) {
        m_watcher->clear();
        m_watcher->addPath(path);
      }
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
    return m_d->m_wantToMonitor;
  }

  void AttributeAsset::setToMonitorFile(bool monitor)
  {
    m_d->m_wantToMonitor = monitor;
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
