/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RESONANT_PULSE_AUDIO_CONTEXT_HPP
#define RESONANT_PULSE_AUDIO_CONTEXT_HPP

#include "Export.hpp"

#include <Radiant/Condition.hpp>

#include <Valuable/Node.hpp>

/// @cond

struct pa_operation;
struct pa_source_info;
struct pa_context;
struct pa_threaded_mainloop;

namespace Resonant
{
  class RESONANT_API PulseAudioContext : public Valuable::Node
  {
  public:
    class Lock
    {
    public:
      Lock(PulseAudioContext & context);
      Lock(pa_threaded_mainloop * mainloop);
      ~Lock();

      Lock(const Lock &) = delete;
      Lock& operator=(const Lock &) = delete;

      Lock(Lock &&);
      Lock& operator=(Lock &&);

    private:
      pa_threaded_mainloop * m_mainloop;
    };

    class PaOperation
    {
    public:
      PaOperation(PulseAudioContext & context);
      PaOperation(pa_threaded_mainloop * loop);
      virtual ~PaOperation();

      PaOperation(PaOperation &&) = delete;
      PaOperation(const PaOperation &) = delete;

      PaOperation & operator=(PaOperation &&) = delete;
      PaOperation & operator=(const PaOperation &) = delete;

      bool isRunning() const;
      void cancel();

      void setPaOperation(pa_operation * op);

      bool waitForFinished(double timeoutSecs);
      void setFinished();

    private:
      pa_threaded_mainloop * m_mainloop = nullptr;
      pa_operation* m_op = nullptr;
      bool m_finished = false;
      Radiant::Condition m_finishedCond;
      Radiant::Mutex m_finishedCondMutex;
    };
    typedef std::shared_ptr<PaOperation> PaOperationPtr;

  public:
    static std::shared_ptr<PulseAudioContext> create(const QByteArray & clientName);
    virtual ~PulseAudioContext();

    void start();
    void stop();

    void addOperation(PaOperationPtr op);

    bool waitForReady(double timeoutSecs);
    /// func is called every time the context is (re)initialized
    long onReady(std::function<void()> func);
    void removeOnReadyListener(long id);

    pa_context * paContext() const;

    PaOperationPtr listSources(std::function<void(const pa_source_info *, bool)> cb);

  private:
    PulseAudioContext();

  private:
    class D;
    std::shared_ptr<D> m_d;
  };
  typedef std::shared_ptr<PulseAudioContext> PulseAudioContextPtr;
}

/// @endcond

#endif // RESONANT_PULSE_AUDIO_CONTEXT_HPP
