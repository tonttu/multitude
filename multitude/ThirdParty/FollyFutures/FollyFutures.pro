include(../../multitude.pri)
include(../../library.pri)

INCLUDEPATH += .

CONFIG += c++11
QT -= gui

win32:QMAKE_CXXFLAGS += /bigobj -w44624

DEFINES += FOLLY_EXPORT

HEADERS += folly/ApplyTuple.h \
           folly/Baton.h \
           folly/CPortability.h \
           folly/ExceptionWrapper.h \
           folly/Executor.h \
           folly/Export.h \
           folly/Hash.h \
           folly/Likely.h \
           folly/Memory.h \
           folly/MicroSpinLock.h \
           folly/MoveWrapper.h \
           folly/Optional.h \
           folly/Portability.h \
           folly/Preprocessor.h \
           folly/ScopeGuard.h \
           folly/SpookyHashV1.h \
           folly/SpookyHashV2.h \
           folly/Traits.h \
           folly/detail/ExceptionWrapper.h \
           folly/detail/Futex.h \
           folly/detail/Sleeper.h \
           folly/detail/UncaughtExceptionCounter.h \
           folly/futures/Barrier.h \
           folly/futures/DrivableExecutor.h \
           folly/futures/Future-inl.h \
           folly/futures/Future-pre.h \
           folly/futures/Future.h \
           folly/futures/FutureException.h \
           folly/futures/helpers.h \
           folly/futures/InlineExecutor.h \
           folly/futures/ManualExecutor.h \
           folly/futures/OpaqueCallbackShunt.h \
           folly/futures/Promise-inl.h \
           folly/futures/Promise.h \
           folly/futures/QueuedImmediateExecutor.h \
           folly/futures/ScheduledExecutor.h \
           folly/futures/SharedPromise-inl.h \
           folly/futures/SharedPromise.h \
           folly/futures/ThreadedExecutor.h \
           folly/futures/Timekeeper.h \
           folly/futures/Try-inl.h \
           folly/futures/Try.h \
           folly/futures/Unit.h \
           folly/futures/detail/Core.h \
           folly/futures/detail/FSM.h \
           folly/futures/detail/SimpleTimekeeper.h \
           folly/futures/detail/Types.h

SOURCES += folly/SpookyHashV1.cpp \
           folly/SpookyHashV2.cpp \
           folly/detail/Futex.cpp \
           folly/futures/Barrier.cpp \
           folly/futures/Future.cpp \
           folly/futures/InlineExecutor.cpp \
           folly/futures/ManualExecutor.cpp \
           folly/futures/QueuedImmediateExecutor.cpp \
           folly/futures/ThreadedExecutor.cpp \
           folly/futures/detail/SimpleTimekeeper.cpp
