include(../../common.cmake)

set(LIBRARY Radiant)

add_library(${LIBRARY} SHARED)

set(SOURCES
  Mime.cpp
  IniFile.cpp
  ThreadChecks.cpp
  TraceDuplicateFilter.cpp
  TraceStdFilter.cpp
  TraceSeverityFilter.cpp
  TraceQIODeviceFilter.cpp
  TemporaryDir.cpp
  ObjectPool.cpp
  CommandLineArguments.cpp
  SynchronizedMultiQueue.cpp
  ProcessRunner.cpp
  DropEvent.cpp
  PenEvent.cpp
  BGThread.cpp
  CameraDriver.cpp
  SocketUtilPosix.cpp
  ThreadPoolQt.cpp
  CSVDocument.cpp
  BinaryData.cpp
  VideoCamera.cpp
  Color.cpp
  ColorUtils.cpp
  CycleRecord.cpp
  MutexQt.cpp
  ThreadQt.cpp
  Task.cpp
  ConfigReader.cpp
  DateTime.cpp
  DirectoryCommon.cpp
  DirectoryQt.cpp
  FileUtils.cpp
  ImageConversion.cpp
  KeyEvent.cpp
  Log.cpp
  MemCheck.cpp
  Sleep.cpp
  SemaphoreQt.cpp
  StringUtils.cpp
  TimeStamp.cpp
  Trace.cpp
  VideoImage.cpp
  VideoInput.cpp
  WatchDog.cpp
  Singleton.cpp
  TCPServerSocketPosix.cpp
  TCPSocketPosix.cpp
  UDPSocketPosix.cpp
  PlatformUtilsLinux.cpp
  PlatformUtilsOSX.cpp
  SerialPortPosix.cpp
  LockFilePosix.cpp
  PlatformUtilsWin32.cpp
  SerialPortWin32.cpp
  LockFileWin32.cpp
  CallStackW32.cpp
  VideoCamera1394.cpp
  IntrusivePtr.cpp
  SymbolRegistry.cpp
  SetupSearchPaths.cpp
  Version.cpp
  VersionString.cpp
  CacheManager.cpp
  CrashHandlerCommon.cpp
  TraceCrashHandlerFilter.cpp
)

if (UNIX)
  list(APPEND SOURCES
    ProcessRunnerPosix.cpp
    DeviceMonitor.cpp
    TraceSyslogFilter.cpp
    SystemCpuTimeLinux.cpp
    CallStackUnix.cpp
  )

  # CrashHandler
  list(APPEND SOURCES CrashHandlerBreakpad.cpp)
  target_include_directories(${LIBRARY} PRIVATE ${BREAKPAD_ROOT}/include/breakpad)
  target_link_libraries(${LIBRARY} PRIVATE
    "/opt/multitaction-breakpad/lib/x86_64-linux-gnu/libbreakpad.a"
    "/opt/multitaction-breakpad/lib/x86_64-linux-gnu/libbreakpad_client.a"
  )

  find_package(PkgConfig REQUIRED)
  pkg_check_modules(UDEV REQUIRED libudev)

  target_link_libraries(${LIBRARY} PRIVATE ${UDEV_LIBRARIES} Nimble)
elseif(WIN32)
  list(APPEND SOURCES
    CrashHandlerCrashpad.cpp
    DeviceUtilsWin.cpp
    TraceWindowsDebugConsoleFilter.cpp
    SystemCpuTimeWin32.cpp
  )

  target_include_directories(${LIBRARY} PRIVATE ${MULTITACTION_DEPS_PATH}/manual/crashpad/include)
  target_include_directories(${LIBRARY} PRIVATE ${MULTITACTION_DEPS_PATH}/manual/crashpad/include/third_party/mini_chromium/mini_chromium)

  target_compile_definitions(${LIBRARY} PRIVATE -D_CRT_SECURE_NO_WARNINGS)

  find_package(Qt5 COMPONENTS Widgets PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
  target_link_libraries(${LIBRARY} PRIVATE Qt5::Widgets)

  target_link_libraries(${LIBRARY} PRIVATE Ws2_32.lib Winmm.lib ShLwApi.lib Setupapi.lib)

  target_link_libraries(${LIBRARY} PRIVATE ${MULTITACTION_DEPS_PATH}/manual/crashpad/lib/crashpad.lib)

endif()

if (ENABLE_FOLLY)
  list(APPEND SOURCES
    ThreadPoolExecutor.cpp
    BGThreadExecutor.cpp
    OnDemandExecutor.cpp
  )

  if(UNIX)
    pkg_check_modules(GLOG REQUIRED libglog)
    find_package(Folly REQUIRED folly PATHS ${FOLLY_ROOT})

    # Linking to Folly::folly does not work for some reason. Missing
    # dependencies to Threads and libevent and what not. Lets just get
    # the location of .so file from Folly target and link to that.
    get_target_property(FOLLY_SO Folly::folly IMPORTED_LOCATION_NOCONFIG)

    target_link_libraries(${LIBRARY}
      PRIVATE ${GLOG_LIBRARIES}
      PUBLIC  ${FOLLY_SO}
    )

    target_include_directories(${LIBRARY} PUBLIC ${FOLLY_INCLUDE_DIR})
  elseif(WIN32)
    find_package(folly CONFIG REQUIRED)
    find_package(ZLIB REQUIRED)

    target_link_libraries(${LIBRARY} PUBLIC Folly::folly Folly::folly_deps)

    #target_include_directories(${LIBRARY} PUBLIC ${VCPKG_ROOT}/include)
  endif()
endif()

if(ENABLE_SECRET_STORE)
  list(APPEND SOURCES SecretStoreDummy.cpp)

  if(UNIX)
    list(APPEND SOURCES SecretStoreLinux.cpp)

    pkg_check_modules(LIBSECRET REQUIRED IMPORTED_TARGET libsecret-1)
    target_link_libraries(${LIBRARY} PRIVATE PkgConfig::LIBSECRET)

    target_include_directories(${LIBRARY}
      PRIVATE ../ThirdParty/expected/include)
  endif()
endif()

set_target_properties(${LIBRARY} PROPERTIES SOURCES "${SOURCES}")

target_compile_definitions(${LIBRARY} PRIVATE -DRADIANT_EXPORT)

find_package(Qt5 COMPONENTS Network PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
find_package(Qt5 COMPONENTS Gui PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
find_package(Qt5 COMPONENTS Sql PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

target_link_libraries(${LIBRARY} PUBLIC Qt5::Network Qt5::Gui Qt5::Sql)
