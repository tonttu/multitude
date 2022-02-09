set(LIBRARY Radiant)

cornerstone_add_library(${LIBRARY} SHARED)

target_sources(${LIBRARY} PRIVATE
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

target_link_libraries(${LIBRARY} PRIVATE
    Nimble)

target_include_directories(${LIBRARY}
  PUBLIC
    ${Cornerstone_SOURCE_DIR}/multitude
    ${Cornerstone_BINARY_DIR}/multitude # Generated version header.
)

if (UNIX)
  target_sources(${LIBRARY} PRIVATE
    ProcessRunnerPosix.cpp
    DeviceMonitor.cpp
    TraceSyslogFilter.cpp
    SystemCpuTimeLinux.cpp
    CallStackUnix.cpp
    CrashHandlerBreakpad.cpp # CrashHandler
  )

  pkg_check_modules(Udev REQUIRED IMPORTED_TARGET libudev)

  target_link_libraries(${LIBRARY} PRIVATE
    PkgConfig::Udev PkgConfig::BreakPad)
elseif(WIN32)
  target_sources(${LIBRARY} PRIVATE
    CrashHandlerCrashpad.cpp
    DeviceUtilsWin.cpp
    TraceWindowsDebugConsoleFilter.cpp
    SystemCpuTimeWin32.cpp
  )

  target_include_directories(${LIBRARY} PRIVATE ${MULTITACTION_DEPS_PATH}/manual/crashpad/include)
  target_include_directories(${LIBRARY} PRIVATE ${MULTITACTION_DEPS_PATH}/manual/crashpad/include/third_party/mini_chromium/mini_chromium)

  target_compile_definitions(${LIBRARY} PRIVATE
    -D_CRT_SECURE_NO_WARNINGS
    -DMULTITACTION_DEPENDENCY_PATH=${MULTITACTION_DEPS_PATH}
  )

  target_link_libraries(${LIBRARY} PRIVATE Ws2_32.lib Winmm.lib ShLwApi.lib Setupapi.lib)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(${LIBRARY} PRIVATE ${MULTITACTION_DEPS_PATH}/manual/crashpad/lib/crashpadd.lib)
  else()
    target_link_libraries(${LIBRARY} PRIVATE ${MULTITACTION_DEPS_PATH}/manual/crashpad/lib/crashpad.lib)
  endif()

  cornerstone_add_qt_libs(${LIBRARY} PRIVATE "Widgets")
endif()

if (ENABLE_FOLLY)
  target_sources(${LIBRARY} PRIVATE
    ThreadPoolExecutor.cpp
    BGThreadExecutor.cpp
    OnDemandExecutor.cpp
  )

  cornerstone_add_folly_libs(${LIBRARY} PUBLIC)

  if(UNIX)
    pkg_check_modules(Glog REQUIRED IMPORTED_TARGET libglog)

    target_link_libraries(${LIBRARY} PUBLIC PkgConfig::Glog)
  elseif(WIN32)
    find_package(ZLIB REQUIRED)

    #target_include_directories(${LIBRARY} PUBLIC ${VCPKG_ROOT}/include)
  endif()
endif()

if(ENABLE_SECRET_STORE)
  target_sources(${LIBRARY} PRIVATE SecretStoreDummy.cpp)

  if(UNIX)
    target_sources(${LIBRARY} PRIVATE SecretStoreLinux.cpp)

    pkg_check_modules(LIBSECRET REQUIRED IMPORTED_TARGET libsecret-1)
    target_link_libraries(${LIBRARY} PRIVATE PkgConfig::LIBSECRET OurExpected)
  endif()
endif()

target_compile_definitions(${LIBRARY} PRIVATE -DRADIANT_EXPORT)

cornerstone_add_qt_libs(${LIBRARY} PUBLIC "Gui;Sql")

cornerstone_install_lib(${LIBRARY})
