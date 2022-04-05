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
  # With Folly
  $<$<BOOL:${ENABLE_FOLLY}>:ThreadPoolExecutor.cpp>
  $<$<BOOL:${ENABLE_FOLLY}>:BGThreadExecutor.cpp>
  $<$<BOOL:${ENABLE_FOLLY}>:OnDemandExecutor.cpp>
  # For Linux
  $<$<PLATFORM_ID:Linux>:ProcessRunnerPosix.cpp>
  $<$<PLATFORM_ID:Linux>:DeviceMonitor.cpp>
  $<$<PLATFORM_ID:Linux>:TraceSyslogFilter.cpp>
  $<$<PLATFORM_ID:Linux>:SystemCpuTimeLinux.cpp>
  $<$<PLATFORM_ID:Linux>:CallStackUnix.cpp>
  $<$<PLATFORM_ID:Linux>:CrashHandlerBreakpad.cpp>
  # For Windows
  $<$<PLATFORM_ID:Windows>:CrashHandlerCrashpad.cpp>
  $<$<PLATFORM_ID:Windows>:DeviceUtilsWin.cpp>
  $<$<PLATFORM_ID:Windows>:TraceWindowsDebugConsoleFilter.cpp>
  $<$<PLATFORM_ID:Windows>:SystemCpuTimeWin32.cpp>
)

target_link_libraries(${LIBRARY}
  PRIVATE
    Nimble
    "$<$<PLATFORM_ID:Linux>:PkgConfig::Udev;PkgConfig::BreakPad>"
    "$<$<PLATFORM_ID:Windows>:Ws2_32.lib;Winmm.lib;ShLwApi.lib;Setupapi.lib>"
    "$<$<PLATFORM_ID:Windows>:MultiCrashpad>"
  PUBLIC
    VersionHdr
)

target_include_directories(${LIBRARY}
  PUBLIC
    ${Cornerstone_SOURCE_DIR}/multitude
    ${Cornerstone_BINARY_DIR}/multitude # Generated version header.
)

if (ENABLE_FOLLY)
  cornerstone_add_folly_libs(${LIBRARY} PUBLIC)

  if(UNIX)
    target_link_libraries(${LIBRARY} PUBLIC PkgConfig::Glog)
  endif()
endif()

if(ENABLE_SECRET_STORE)
  target_sources(${LIBRARY} PRIVATE
    SecretStoreDummy.cpp
    $<$<PLATFORM_ID:Linux>:SecretStoreLinux.cpp>
    $<$<PLATFORM_ID:Windows>:SecretStoreWin32.cpp>
  )

  target_link_libraries(${LIBRARY} PRIVATE
    "$<$<PLATFORM_ID:Linux>:OurExpected;PkgConfig::LIBSECRET>"
    $<$<PLATFORM_ID:Windows>:Crypt32>
  )
endif()

target_compile_definitions(${LIBRARY} PRIVATE
  RADIANT_EXPORT
  $<$<BOOL:${ENABLE_CI_BUILD}>:ENABLE_CI_BUILD>
  $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
  $<$<PLATFORM_ID:Windows>:MULTITACTION_DEPENDENCY_PATH=${MULTITACTION_DEPS_PATH}>
)

cornerstone_add_qt_libs(${LIBRARY} PUBLIC "Gui;Sql")

if(WIN32)
  cornerstone_add_qt_libs(${LIBRARY} PRIVATE "Widgets")
endif()

cornerstone_install_lib(${LIBRARY})
