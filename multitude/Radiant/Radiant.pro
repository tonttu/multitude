include(../setup.pri)

HEADERS += TempFailureRetry.hpp \
    ThreadChecks.hpp \
    TraceDuplicateFilter.hpp \
    TraceStdFilter.hpp \
    TraceSeverityFilter.hpp \
    TraceQIODeviceFilter.hpp \
    SystemCpuTime.hpp \
    ReentrantVector.hpp \
    TemporaryDir.hpp
HEADERS += BlockRingBuffer.hpp
HEADERS += ArrayMap.hpp
HEADERS += ObjectPool.hpp
HEADERS += CommandLineArguments.hpp
HEADERS += SerialPortHelpers.hpp \
    SynchronizedMultiQueue.hpp \
    ProcessRunner.hpp
HEADERS += ArraySet.hpp
HEADERS += Flags.hpp
HEADERS += FutureBool.hpp
HEADERS += DropEvent.hpp
HEADERS += PenEvent.hpp
HEADERS += BGThread.hpp
HEADERS += MovingAverage.hpp
HEADERS += Mime.hpp
HEADERS += Timer.hpp
HEADERS += SynchronizedQueue.hpp
HEADERS += CameraDriver.hpp
HEADERS += Defines.hpp
HEADERS += ThreadPool.hpp
HEADERS += CSVDocument.hpp
HEADERS += UDPSocket.hpp
HEADERS += BinaryData.hpp
HEADERS += BinaryStream.hpp
HEADERS += Color.hpp
HEADERS += ColorUtils.hpp
HEADERS += Condition.hpp
HEADERS += ConfigReader.hpp
HEADERS += ConfigReaderTmpl.hpp
HEADERS += cycle.h
HEADERS += CycleRecord.hpp
HEADERS += DateTime.hpp
HEADERS += Directory.hpp
HEADERS += Export.hpp
HEADERS += FileUtils.hpp
HEADERS += Grid.hpp
HEADERS += ImageConversion.hpp
HEADERS += IODefs.hpp
HEADERS += KeyEvent.hpp
HEADERS += LockFile.hpp
HEADERS += Log.hpp
HEADERS += MemCheck.hpp
HEADERS += CallStack.hpp
HEADERS += Memory.hpp
HEADERS += Allocators.hpp
HEADERS += Mutex.hpp
HEADERS += Platform.hpp
HEADERS += PlatformUtils.hpp
HEADERS += Radiant.hpp
HEADERS += IntrusivePtr.hpp
HEADERS += Task.hpp
HEADERS += RingBuffer.hpp
HEADERS += SafeBool.hpp
HEADERS += Semaphore.hpp
HEADERS += SerialPort.hpp
HEADERS += Sleep.hpp
HEADERS += StringUtils.hpp
HEADERS += SocketUtilPosix.hpp
HEADERS += TCPServerSocket.hpp
HEADERS += TCPSocket.hpp
HEADERS += Thread.hpp
HEADERS += TimeStamp.hpp
HEADERS += Trace.hpp
HEADERS += Types.hpp
HEADERS += TouchEvent.hpp
HEADERS += VideoImage.hpp
HEADERS += VideoInput.hpp
HEADERS += WatchDog.hpp
HEADERS += VideoCamera.hpp
HEADERS += SocketWrapper.hpp
HEADERS += Singleton.hpp
HEADERS += VideoCamera1394.hpp
HEADERS += DeviceMonitor.hpp
HEADERS += SymbolRegistry.hpp
HEADERS += CrashHandler.hpp
HEADERS += Version.hpp
HEADERS += VersionGenerated.hpp
HEADERS += VersionString.hpp
HEADERS += TraceCrashHandlerFilter.hpp
HEADERS += fast_atof.h
HEADERS += VectorAllocator.hpp
HEADERS += TimeTracker.hpp
HEADERS += CacheManager.hpp

SOURCES += Mime.cpp \
    ThreadChecks.cpp \
    TraceDuplicateFilter.cpp \
    TraceStdFilter.cpp \
    TraceSeverityFilter.cpp \
    TraceQIODeviceFilter.cpp \
    TemporaryDir.cpp
SOURCES += ObjectPool.cpp
SOURCES += CommandLineArguments.cpp
SOURCES += SynchronizedMultiQueue.cpp \
    ProcessRunner.cpp
SOURCES += DropEvent.cpp
SOURCES += PenEvent.cpp
SOURCES += BGThread.cpp
SOURCES += CameraDriver.cpp
SOURCES += SocketUtilPosix.cpp
SOURCES += ThreadPoolQt.cpp
SOURCES += CSVDocument.cpp
SOURCES += BinaryData.cpp
SOURCES += VideoCamera.cpp
SOURCES += Color.cpp
SOURCES += ColorUtils.cpp
SOURCES += CycleRecord.cpp
SOURCES += MutexQt.cpp
SOURCES += ThreadQt.cpp
SOURCES += Task.cpp
SOURCES += ConfigReader.cpp
SOURCES += DateTime.cpp
SOURCES += DirectoryCommon.cpp
SOURCES += DirectoryQt.cpp
SOURCES += FileUtils.cpp
SOURCES += ImageConversion.cpp
SOURCES += KeyEvent.cpp
SOURCES += Log.cpp
SOURCES += MemCheck.cpp
SOURCES += Sleep.cpp
SOURCES += SemaphoreQt.cpp
SOURCES += StringUtils.cpp
SOURCES += TimeStamp.cpp
SOURCES += Trace.cpp
SOURCES += VideoImage.cpp
SOURCES += VideoInput.cpp
SOURCES += WatchDog.cpp
SOURCES += Singleton.cpp
SOURCES += TCPServerSocketPosix.cpp
SOURCES += TCPSocketPosix.cpp
SOURCES += UDPSocketPosix.cpp
SOURCES += PlatformUtilsLinux.cpp
SOURCES += PlatformUtilsOSX.cpp
SOURCES += SerialPortPosix.cpp
SOURCES += LockFilePosix.cpp
SOURCES += PlatformUtilsWin32.cpp
SOURCES += SerialPortWin32.cpp
SOURCES += LockFileWin32.cpp
SOURCES += CallStackW32.cpp
SOURCES += VideoCamera1394.cpp
SOURCES += IntrusivePtr.cpp
SOURCES += SymbolRegistry.cpp
SOURCES += SetupSearchPaths.cpp
SOURCES += Version.cpp
SOURCES += VersionString.cpp
SOURCES += CacheManager.cpp
!mobile:SOURCES += CrashHandlerCommon.cpp
!mobile:SOURCES += TraceCrashHandlerFilter.cpp

!mobile {
  linux*:SOURCES += ProcessRunnerPosix.cpp
  win32:SOURCES += ProcessRunnerWin32.cpp
}

# ios:OTHER_FILES += PlatformUtilsIOS.mm
ios {
  OBJECTIVE_SOURCES += PlatformUtilsIOS.mm

}
linux*:SOURCES += DeviceMonitor.cpp

enable-folly {
  HEADERS += ThreadPoolExecutor.hpp
  HEADERS += BGThreadExecutor.hpp
  SOURCES += ThreadPoolExecutor.cpp
  SOURCES += BGThreadExecutor.cpp
}

LIBS += $$LIB_NIMBLE $$LIB_PATTERNS
LIBS += $$LIB_FOLLY

linux-* {
  LIBS += -lX11
  PKGCONFIG += libudev

  HEADERS += TraceSyslogFilter.hpp
  SOURCES += TraceSyslogFilter.cpp

  SOURCES += SystemCpuTimeLinux.cpp

  # CrashHandler requirements
  SOURCES += CrashHandlerBreakpad.cpp

  arch_triple = x86_64-linux-gnu
  arm64 { arch_triple = aarch64-linux-gnu }
  INCLUDEPATH += $${ARM64_ROOTFS}/opt/multitaction-breakpad/include/breakpad
  QMAKE_LIBDIR += $${ARM64_ROOTFS}/opt/multitaction-breakpad/lib/$${arch_triple}
  LIBS += -lbreakpad -lbreakpad_client
}

macx {
  LIBS += -framework CoreFoundation

  SOURCES += SystemCpuTimeOSX.cpp

  SOURCES += CrashHandlerDummy.cpp
}

DEFINES += RADIANT_EXPORT

unix {
  !mobile:SOURCES += CallStackUnix.cpp
  mobile:SOURCES += CallStackDummy.cpp

  !mobile:LIBS += -lpthread
  LIBS +=  $$LIB_RT -ldl
  CONFIG += qt
  QT = core network gui
}

win32 {
    LIBS += Ws2_32.lib \
        ShLwApi.lib \
        shell32.lib \
        psapi.lib \
        Advapi32.lib \
        Ole32.lib \
        Winmm.lib \
        Setupapi.lib
    CONFIG += qt
    QT = core network opengl gui

  HEADERS += DeviceUtilsWin.hpp
  SOURCES += DeviceUtilsWin.cpp

  HEADERS += TraceWindowsDebugConsoleFilter.hpp
  SOURCES += TraceWindowsDebugConsoleFilter.cpp

  SOURCES += SystemCpuTimeWin32.cpp

  # CrashHandler requirements
  SOURCES += CrashHandlerCrashpad.cpp
  INCLUDEPATH += $$CORNERSTONE_DEPS_PATH/manual/crashpad/include
  INCLUDEPATH += $$CORNERSTONE_DEPS_PATH/manual/crashpad/include/third_party/mini_chromium/mini_chromium
  QMAKE_LIBDIR += $$CORNERSTONE_DEPS_PATH/manual/crashpad/lib

  CONFIG(release, debug|release) {
    LIBS += -lcrashpad
  } else {
    LIBS += -lcrashpadd
  }

  LIBS += -lAdvapi32 -lRpcrt4 -lShell32

  # PDB 'filename' was not found with 'object/library' or at 'path'; linking object as if no debug info
  QMAKE_LFLAGS += /ignore:4099

  DEFINES += _CRT_SECURE_NO_WARNINGS
}

QT += sql

DEFINES += MULTITACTION_DEPENDENCY_PATH=$$cat($$PWD/../../MULTITACTION_DEPS)

include(../setup-lib.pri)
