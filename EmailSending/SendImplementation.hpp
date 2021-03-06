#ifndef SENDIMPLEMENTATION_HPP
#define SENDIMPLEMENTATION_HPP

#include "Sender.hpp"
#include "Email.hpp"

#include <condition_variable>
#include <mutex>

#include <Radiant/Thread.hpp>

#include <SMTPEmail/src/SmtpMime>

namespace Email
{

  struct SendJob
  {
    Message message;
    Sender::SMTPSettings settings;
    folly::Promise<Sender::SendStatus> statusPromise;
  };

  class SendImplementation;

  class WorkerThread : public Radiant::QThreadWrapper
  {
  Q_OBJECT

  public:
    WorkerThread(SendImplementation& host);

    void runImpl() override;

    void stop();

  private Q_SLOTS:
    void smtpError(SmtpClient::SmtpError error);

  private:
    void sendMessage(SendJob& job);

    SendImplementation& m_host;

    bool m_keepRunning;


    // Not ideal, but the signal/slot system requires us to keep track of the
    // current status on class level
    Sender::SendStatus m_activeStatus;
  };


  class SendImplementation
  {
  public:
    SendImplementation();
    ~SendImplementation();

    folly::Future<Sender::SendStatus>
    send(Message&& message, Sender::SMTPSettings settings);

    std::mutex messageQueueMutex;
    std::list<SendJob> messageQueue;

    std::condition_variable messageQueueCondition;
    WorkerThread workerThread;
  };

}

#endif // SENDIMPLEMENTATION_HPP
