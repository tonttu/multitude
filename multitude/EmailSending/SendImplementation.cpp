#include "SendImplementation.hpp"

#include <QMimeDatabase>

namespace Email
{

  SmtpClient::ConnectionType connectionType(const Sender::SMTPSettings& settings)
  {
    switch (settings.encryption) {
      case Sender::EncryptionType::NONE:
        return SmtpClient::TcpConnection;
      case Sender::EncryptionType::SSL:
        return SmtpClient::SslConnection;
      case Sender::EncryptionType::TLS:
        return SmtpClient::TlsConnection;
      case Sender::EncryptionType::AUTO:
      default:
        if (settings.port == 465)
          return SmtpClient::SslConnection;
        return SmtpClient::TlsConnection;
    }
  }

  int connectionPort(const Sender::SMTPSettings & settings)
  {
    if (settings.port == 0) {
      return settings.encryption == Sender::EncryptionType::SSL ? 465 : 25;
    } else {
      return settings.port;
    }
  }

  MimeMultiPart::MultiPartType toSmtpType(Message::MultiPartType t)
  {
    switch (t) {
      case Message::MultiPartType::Mixed:
        return MimeMultiPart::Mixed;
      case Message::MultiPartType::Digest:
        return MimeMultiPart::Digest;
      case Message::MultiPartType::Alternative:
        return MimeMultiPart::Alternative;
      case Message::MultiPartType::Related:
        return MimeMultiPart::Related;
      case Message::MultiPartType::Report:
        return MimeMultiPart::Report;
      case Message::MultiPartType::Signed:
        return MimeMultiPart::Signed;
      case Message::MultiPartType::Encrypted:
        return MimeMultiPart::Encrypted;
    }
    Radiant::error("Unknown Message::MultiPartType %d", (int)t);
    return MimeMultiPart::Related;
  }

  std::unique_ptr<MimeMessage> createMimeMessage(const Message& message)
  {
    std::unique_ptr<MimeMessage> mimeMessage(new MimeMessage(false));
    mimeMessage->setContent(new MimeMultiPart(toSmtpType(message.multiPartType())));

    // Set sender and subject
    mimeMessage->setSender(new ::EmailAddress(message.sender().address, message.sender().name));
    mimeMessage->setSubject(message.subject());

    // Add recipients
    Q_FOREACH(const Address & address, message.recipients(Message::RecipientType::To))
      mimeMessage->addRecipient(new ::EmailAddress(address.address, address.name), MimeMessage::To);

    Q_FOREACH(const Address & address, message.recipients(Message::RecipientType::Cc))
      mimeMessage->addRecipient(new ::EmailAddress(address.address, address.name), MimeMessage::Cc);

    Q_FOREACH(const Address & address, message.recipients(Message::RecipientType::Bcc))
      mimeMessage->addRecipient(new ::EmailAddress(address.address, address.name), MimeMessage::Bcc);

    // Add text content
    auto text = new MimeText(message.content());
    mimeMessage->addPart(text);

    // Include attachments
    const auto & list = message.attachments();
    for(auto & attachment : list) {
      if(attachment.device->isOpen() || attachment.device->open(QIODevice::ReadOnly)) {
        QByteArray payload = attachment.device->readAll();

        auto a = new MimeFile(payload, attachment.filename);
        a->addHeaderLine(QString("Content-disposition: %1").arg(attachment.contentDisposition));

        if (attachment.contentType.isEmpty()) {
          a->setContentType(QMimeDatabase().mimeTypeForFileNameAndData(attachment.filename, payload).name());
        } else {
          a->setContentType(attachment.contentType);
        }

        mimeMessage->addPart(a);
        attachment.device->close();
      }
    }

    return mimeMessage;
  }

  static QStringList s_smtpErrorTexts = QStringList() << "ConnectionTimeoutError"
                                                      << "ResponseTimeoutError"
                                                      << "SendDataTimeoutError"
                                                      << "AuthenticationFailedError"
                                                      << "ServerError"
                                                      << "ClientError";

  // --------------------------------------------------------------------------

  WorkerThread::WorkerThread(SendImplementation& host)
    : QThread()
    , m_host(host)
    , m_keepRunning(true)
  {
    qRegisterMetaType<SmtpClient::SmtpError>("SmtpClient::SmtpError");
    qRegisterMetaType<QList<QSslError>>("QList<QSslError>");
  }

  void WorkerThread::run()
  {
    while(m_keepRunning) {

      std::unique_lock<std::mutex> waitLock(m_waitMutex);

      auto t = std::chrono::milliseconds(1000);

      m_host.messageQueueCondition.wait_for(waitLock, t, [this] {
        std::lock_guard<std::mutex> g(m_host.messageQueueMutex);
        return !m_host.messageQueue.empty() || !m_keepRunning;
      });

      std::unique_lock<std::mutex> queueLock(m_host.messageQueueMutex);

      // We might have woken up because stop was requested
      if(m_host.messageQueue.empty())
        continue;

      SendJob job = std::move(m_host.messageQueue.front());
      m_host.messageQueue.pop_front();

      queueLock.unlock();

      sendMessage(job);
    }
  }

  void WorkerThread::stop()
  {
    m_keepRunning = false;
  }

  void WorkerThread::smtpError(SmtpClient::SmtpError error)
  {
    m_activeStatus.ok = false;
    auto text = error < s_smtpErrorTexts.size() ? s_smtpErrorTexts.at(error) : "Unknown";
    m_activeStatus.errorMessage = QString("SMTP: smtp error: %3 : %4").arg(error).arg(text);
  }

  void WorkerThread::sendMessage(SendJob& job)
  {
    // Reset active status
    m_activeStatus = Sender::SendStatus();
    m_activeStatus.errorMessage = QString();

    SmtpClient::ConnectionType type = connectionType(job.settings);

    int port = connectionPort(job.settings);
    SmtpClient smtp(job.settings.host, port, type);

    connect(&smtp, SIGNAL(smtpError(SmtpClient::SmtpError)), this, SLOT(smtpError(SmtpClient::SmtpError)), Qt::DirectConnection);

    smtp.setUser(job.settings.username);
    smtp.setPassword(job.settings.password);

    // convert timeout values from seconds to milliseconds
    smtp.setConnectionTimeout(job.settings.connectionTimeout * 1000.f);
    smtp.setResponseTimeout(job.settings.responseTimeout * 1000.f);
    smtp.setSendMessageTimeout(job.settings.sendTimeout * 1000.f);

    if (auto ssl = dynamic_cast<QSslSocket*>(smtp.getSocket())) {
      auto ignore = job.settings.ignoreSslErrors;
      // Qt's signal handling syntax is horrible with overloaded functions
      connect(ssl, static_cast<void(QSslSocket::*)(const QList<QSslError> &)>(&QSslSocket::sslErrors),
              [ssl, ignore] (const QList<QSslError> &errors) {
        if (ignore) {
          ssl->ignoreSslErrors();
          for (const auto & error : errors)
            Radiant::warning("SMTP: Ignored an SSL error (%s)", error.errorString().toUtf8().data());
        }
        else {
          for (const auto & error : errors)
            Radiant::error("SMTP: SSL error (%s)", error.errorString().toUtf8().data());
        }
      });
    }

    m_activeStatus.ok = smtp.connectToHost();
    bool connected = m_activeStatus.ok;
    if(!m_activeStatus.ok) {
      m_activeStatus.errorMessage = QString("SMTP: failed to connect to %1:%2")
          .arg(job.settings.host).arg(port);
    }

    bool requireLogin = !job.settings.username.isEmpty() || !job.settings.password.isEmpty();
    if(m_activeStatus.ok && requireLogin) {
      m_activeStatus.ok = smtp.login();
      if(!m_activeStatus.ok) {
        m_activeStatus.errorMessage = QString("SMTP: failed to login to %1:%2 with username %3")
            .arg(job.settings.host).arg(port).arg(job.settings.username);
      }
    }

    if(m_activeStatus.ok) {
      std::unique_ptr<MimeMessage> email = createMimeMessage(job.message);
      m_activeStatus.ok = smtp.sendMail(*email.get());
      if(!m_activeStatus.ok) {
        if (m_activeStatus.errorMessage.isEmpty())
          m_activeStatus.errorMessage = QString("SMTP: failed to send message via %1:%2. Server response was '%3'")
              .arg(job.settings.host).arg(port).arg(smtp.getResponseText().toUtf8().data());
      }
    }

    if (connected)
      smtp.quit();

    job.statusPromise.setValue(m_activeStatus);
  }

  // --------------------------------------------------------------------------

  SendImplementation::SendImplementation()
    : workerThread(*this)
  {
    workerThread.start();
  }

  SendImplementation::~SendImplementation()
  {
    workerThread.stop();
    messageQueueCondition.notify_one();

    workerThread.wait();
  }

  folly::Future<Sender::SendStatus>
  SendImplementation::send(Message &&message, Sender::SMTPSettings settings)
  {
    SendJob job;
    job.message = std::move(message);
    job.settings = std::move(settings);

    auto future = job.statusPromise.getFuture();

    {
      std::lock_guard<std::mutex> g(messageQueueMutex);
      messageQueue.emplace_back(std::move(job));
    }
    messageQueueCondition.notify_one();
    return future;
  }

}
