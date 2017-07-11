#ifndef LUMINOUS_MEMORYMANAGER_HPP
#define LUMINOUS_MEMORYMANAGER_HPP

#include "Export.hpp"

#include <Radiant/Singleton.hpp>

#include <Valuable/Node.hpp>

namespace Luminous
{
  /// This singleton class monitors system memory usage and application state,
  /// and manages the application memory usage based on the profile settings.
  /// When the class detects that it's time to release some memory, it sends
  /// 'out-of-memory' event. Cache classes can then listen to the event and
  /// release up to overallocatedBytes of memory if they can.
  /// @event[out] out-of-memory
  class LUMINOUS_API MemoryManager : public Valuable::Node
  {
    DECLARE_SINGLETON(MemoryManager);

  public:
    /// Type of the profile
    enum Profile
    {
      /// Default running mode, application is in foreground and has focus
      ProfileNormal,
      /// Application is visible, but doesn't have focus
      ProfileInactive,
      /// Application is hidden or minimized
      ProfileHidden,

      ProfileCount
    };

    /// Settings per profile
    struct ProfileSettings
    {
      /// Maximum system memory usage we allow before starting to release memory.
      /// Value is between 0 and 1, relative to the maximum physical memory.
      float maxMemoryUsage = 0.85;
      /// Regardless of other settings, always try to keep at least this amount of
      /// available memory.
      uint64_t minAvailableMemoryMB = 1024;
      /// Polling interval in milliseconds. Check system memory usage on each
      /// iteration and might send out-of-memory event every time the usage is
      /// more than the profile settings allows.
      uint32_t pollingInterval = 1000;
    };

  public:
    virtual ~MemoryManager();

    /// Returns the amount of memory that should be released back to the operating system
    uint64_t overallocatedBytes() const;

    /// Sets profile settings for given profile
    void setProfileSettings(Profile profile, ProfileSettings settings);
    ProfileSettings profileSettings(Profile profile) const;

    Profile currentProfile() const;
    ProfileSettings currentProfileSettings() const;

    /// Set by MultiWidgets::Application.
    void setMinimized(bool minimized);
    bool isMinimized() const;

  private:
    MemoryManager();

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
  typedef std::shared_ptr<MemoryManager> MemoryManagerPtr;

} // namespace Luminous

#endif // LUMINOUS_MEMORYMANAGER_HPP
