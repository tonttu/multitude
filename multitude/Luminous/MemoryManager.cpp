#include "MemoryManager.hpp"

#include <Radiant/PlatformUtils.hpp>

#include <QGuiApplication>
#include <QMetaEnum>
#include <QTimer>

#include <array>

namespace Luminous
{
  class MemoryManager::D : public QObject
  {
    Q_OBJECT

  public:
    D(MemoryManager & host);
    void scheduleCheck();
    void updateProfile();

  public slots:
    void applicationStateChanged(Qt::ApplicationState state);
    void check();

  public:
    MemoryManager & m_host;

    MemoryManager::Profile m_currentProfile = MemoryManager::ProfileNormal;
    std::array<MemoryManager::ProfileSettings, MemoryManager::ProfileCount> m_profileSettings;

    Qt::ApplicationState m_state = Qt::ApplicationActive;
    bool m_isMinimized = false;

    QTimer m_timer;

    uint64_t m_overAllocatedBytes = 0;
  };

  MemoryManager::D::D(MemoryManager & host)
    : m_host(host)
  {
  }

  void MemoryManager::D::scheduleCheck()
  {
    QMetaObject::invokeMethod(this, "check", Qt::QueuedConnection);
    m_timer.start(m_profileSettings[m_currentProfile].pollingInterval);
  }

  void MemoryManager::D::updateProfile()
  {
    MemoryManager::Profile profile = MemoryManager::ProfileNormal;

    if (m_isMinimized || m_state == Qt::ApplicationSuspended || m_state == Qt::ApplicationHidden) {
      profile = MemoryManager::ProfileHidden;
    } else if (m_state == Qt::ApplicationInactive) {
      profile = MemoryManager::ProfileInactive;
    }

    if (m_currentProfile != profile) {
      const char * profileNames[MemoryManager::ProfileCount] =
      { "normal", "inactive", "hidden" };

      Radiant::debug("MemoryManager # Changing profile from %s to %s",
                     profileNames[m_currentProfile], profileNames[profile]);
      m_currentProfile = profile;
      scheduleCheck();
    }
  }

  void MemoryManager::D::check()
  {
    Radiant::PlatformUtils::MemInfo info = Radiant::PlatformUtils::memInfo();

    ProfileSettings settings = m_profileSettings[m_currentProfile];

    uint64_t overallocatedBytes = 0;

    if (info.memAvailableKb < settings.minAvailableMemoryMB * 1024) {
      overallocatedBytes = (settings.minAvailableMemoryMB * 1024 - info.memAvailableKb) * 1024;
    }

    const float memoryUsage = 1.0 - double(info.memAvailableKb) / info.memTotalKb;

    if (memoryUsage > settings.maxMemoryUsage) {
      overallocatedBytes = uint64_t((memoryUsage - settings.maxMemoryUsage) * info.memTotalKb) * 1024;
    }

    m_overAllocatedBytes = overallocatedBytes;

    if (m_overAllocatedBytes) {
      m_host.eventSend("out-of-memory");
    }
  }

  void MemoryManager::D::applicationStateChanged(Qt::ApplicationState state)
  {
    QMetaEnum meta = QMetaEnum::fromType<Qt::ApplicationState>();
    Radiant::debug("MemoryManager::D::applicationStateChanged # Application state changed to %s", meta.valueToKey(state));
    m_state = state;
    updateProfile();
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  MemoryManager::MemoryManager()
    : m_d(new D(*this))
  {
    eventAddOut("out-of-memory");

    ProfileSettings settings;
    settings.maxMemoryUsage = 0.1f;
    m_d->m_profileSettings[ProfileHidden] = settings;

    if (auto qapp = dynamic_cast<QGuiApplication *>(QCoreApplication::instance())) {
      QObject::connect(qapp, SIGNAL(applicationStateChanged(Qt::ApplicationState)),
                       m_d.get(), SLOT(applicationStateChanged(Qt::ApplicationState)));

      m_d->m_state = qapp->applicationState();
    } else {
      // Not a GUI application, so always use "hidden" profile
      m_d->m_state = Qt::ApplicationHidden;
    }

    QObject::connect(&m_d->m_timer, SIGNAL(timeout()), m_d.get(), SLOT(check()));
    m_d->m_timer.start(currentProfileSettings().pollingInterval);

    m_d->updateProfile();
  }

  MemoryManager::~MemoryManager()
  {
  }

  uint64_t MemoryManager::overallocatedBytes() const
  {
    return m_d->m_overAllocatedBytes;
  }

  void MemoryManager::setProfileSettings(MemoryManager::Profile profile, MemoryManager::ProfileSettings settings)
  {
    m_d->m_profileSettings[profile] = settings;

    if (profile == currentProfile())
      m_d->scheduleCheck();
  }

  MemoryManager::ProfileSettings MemoryManager::profileSettings(MemoryManager::Profile profile) const
  {
    return m_d->m_profileSettings[profile];
  }

  MemoryManager::Profile MemoryManager::currentProfile() const
  {
    return m_d->m_currentProfile;
  }

  MemoryManager::ProfileSettings MemoryManager::currentProfileSettings() const
  {
    return profileSettings(currentProfile());
  }

  void MemoryManager::setMinimized(bool minimized)
  {
    if (m_d->m_isMinimized == minimized)
      return;

    m_d->m_isMinimized = minimized;
    m_d->updateProfile();
  }

  bool MemoryManager::isMinimized() const
  {
    return m_d->m_isMinimized;
  }

  DEFINE_SINGLETON(MemoryManager)


} // namespace Luminous

#include "MemoryManager.moc"
