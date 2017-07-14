#include "MemoryManager.hpp"

#include <Radiant/PlatformUtils.hpp>

#include <QGuiApplication>
#include <QMetaEnum>

#include <Radiant/BGThread.hpp>

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
    void check();

  public slots:
    void applicationStateChanged(Qt::ApplicationState state);

  public:
    MemoryManager & m_host;

    std::atomic<MemoryManager::Profile> m_currentProfile { MemoryManager::ProfileNormal };

    Radiant::Mutex m_profileSettingsMutex;
    std::array<MemoryManager::ProfileSettings, MemoryManager::ProfileCount> m_profileSettings;

    std::atomic<Qt::ApplicationState> m_state { Qt::ApplicationActive };
    std::atomic<bool> m_isMinimized { false };

    std::atomic<uint64_t> m_overAllocatedBytes { 0 };

    std::shared_ptr<Radiant::BGThread> m_bgThread = Radiant::BGThread::instance();
    Radiant::TaskPtr m_task;
  };

  MemoryManager::D::D(MemoryManager & host)
    : m_host(host)
  {
    m_task = std::make_shared<Radiant::FunctionTask>([this] (Radiant::Task &) { check(); });
    /// It's really important we run this with high priority.
    /// This is now one more than Mipmap default ping priority
    m_task->setPriority(Radiant::Task::PRIORITY_HIGH + 3);
  }

  void MemoryManager::D::scheduleCheck()
  {
    m_task->scheduleFromNowSecs(0);
    m_bgThread->reschedule(m_task);
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

    ProfileSettings settings;
    {
      Radiant::Guard g(m_profileSettingsMutex);
      settings = m_profileSettings[m_currentProfile];
    }

    uint64_t overallocatedBytes = 0;

    if (info.memAvailableKb < settings.minAvailableMemoryMB * 1024) {
      overallocatedBytes = (settings.minAvailableMemoryMB * 1024 - info.memAvailableKb) * 1024;
    }

    if (info.memTotalKb > 0) {
      const float memoryUsage = 1.0 - double(info.memAvailableKb) / info.memTotalKb;

      if (memoryUsage > settings.maxMemoryUsage) {
        overallocatedBytes = std::max(overallocatedBytes, uint64_t(
                                        (memoryUsage - settings.maxMemoryUsage) * info.memTotalKb) * 1024);
      }
    } else if (settings.maxMemoryUsage < 1.0f) {
      // If there is no information how much memory this computer has, we need
      // to play it safe and just release as much as possible
      overallocatedBytes = std::numeric_limits<uint64_t>::max();
    }

    m_overAllocatedBytes = overallocatedBytes;

    if (m_overAllocatedBytes) {
      m_host.eventSend("out-of-memory");
    }

    m_task->scheduleFromNowSecs(settings.pollingIntervalS);
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

    m_d->updateProfile();
    m_d->m_bgThread->addTask(m_d->m_task);
  }

  MemoryManager::~MemoryManager()
  {
    m_d->m_bgThread->removeTask(m_d->m_task, true, true);
  }

  uint64_t MemoryManager::overallocatedBytes() const
  {
    return m_d->m_overAllocatedBytes;
  }

  void MemoryManager::setProfileSettings(MemoryManager::Profile profile, MemoryManager::ProfileSettings settings)
  {
    {
      Radiant::Guard g(m_d->m_profileSettingsMutex);
      m_d->m_profileSettings[profile] = settings;
    }

    if (profile == currentProfile())
      m_d->scheduleCheck();
  }

  MemoryManager::ProfileSettings MemoryManager::profileSettings(MemoryManager::Profile profile) const
  {
    Radiant::Guard g(m_d->m_profileSettingsMutex);
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
