#ifndef SENDIMPLEMENTATION_HPP
#define SENDIMPLEMENTATION_HPP

#include "Email.hpp"
#include "Sender.hpp"

#include <condition_variable>
#include <mutex>

#include <QThread>

#include <folly/futures/Future.h>

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

  class WorkerThread : public QThread
  {
  Q_OBJECT

  public:
    WorkerThread(SendImplementation& host);

    void run() override;

    void stop();

  private slots:
    void smtpError(SmtpClient::SmtpError error);

  private:
    void sendMessage(SendJob& job);

    SendImplementation& m_host;

    std::mutex m_waitMutex;

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
