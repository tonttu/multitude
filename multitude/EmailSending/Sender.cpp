#include "Sender.hpp"

#include "SendImplementation.hpp"


namespace Email
{

  class Sender::D
  {
  public:
    D(Sender* host);
    ~D();

    folly::Future<Sender::SendStatus> queueMessage(Message&& message);

    Sender* m_host;
    SendImplementation m_implementation;
  };

  // ------------------------------------------------------------------------

  Sender::D::D(Sender *host)
    : m_host(host)
  {}

  Sender::D::~D()
  {}

  folly::Future<Sender::SendStatus> Sender::D::queueMessage(Message&& message)
  {
    return m_implementation.send(std::move(message), m_host->settings());
  }

  // ------------------------------------------------------------------------

  Valuable::EnumNames s_encryptionTypes[] = {
    {"none", (long)Sender::EncryptionType::NONE },
    {"ssl",  (long)Sender::EncryptionType::SSL  },
    {"tls",  (long)Sender::EncryptionType::TLS  },
    {"auto", (long)Sender::EncryptionType::AUTO },
    {0, 0}
  };

  Sender::Sender()
    : m_d(new D(this))
    , m_smtpUsername(this, "smtp-username")
    , m_smtpPassword(this, "smtp-password")
    , m_smtpHost(this, "smtp-host")
    , m_smtpPort(this, "smtp-port", 587)
    , m_encryptionType(this, "encryption-type", s_encryptionTypes, EncryptionType::NONE)
    , m_ignoreSslErrors(this, "ignore-ssl-errors", false)
    , m_emailSenderName(this, "sender-name")
    , m_emailSenderAddress(this, "sender-address")
    , m_emailSubject(this, "subject")
    , m_emailTemplate(this, "template")
    , m_connectionTimeout(this, "connection-timeout", 60)
    , m_responseTimeout(this, "response-timeout", 60)
    , m_sendMessageTimeout(this, "send-message-timeout", 120)
  {
  }

  Sender::~Sender()
  {}

  Sender::SMTPSettings Sender::settings() const
  {
    SMTPSettings s;
    s.host = m_smtpHost;
    s.port = m_smtpPort;
    s.encryption = m_encryptionType;
    s.username = m_smtpUsername;
    s.password = m_smtpPassword;
    s.ignoreSslErrors = m_ignoreSslErrors;
    s.sslErrorsToIgnore = m_sslErrorsToIgnore;
    s.connectionTimeout = m_connectionTimeout;
    s.responseTimeout = m_responseTimeout;
    s.sendTimeout = m_sendMessageTimeout;
    return s;
  }

  folly::Future<Sender::SendStatus> Sender::queueMessage(Message&& message)
  {
    return m_d->queueMessage(std::move(message));
  }

  Message Sender::templateMessage() const
  {
    Message message;
    Address sender = {m_emailSenderAddress, m_emailSenderName};

    message.setSender(sender);
    message.setSubject(m_emailSubject);
    message.setContent(m_emailTemplate);

    return message;
  }

  // Simple setters and getters

  QString Sender::smtpUsername() const
  {
    return m_smtpUsername;
  }

  void Sender::setSmtpUsername(const QString& username)
  {
    m_smtpUsername = username;
  }

  QString Sender::smtpPassword() const
  {
    return m_smtpPassword;
  }

  void Sender::setSmtpPassword(const QString& password)
  {
    m_smtpPassword = password;
  }

  QString Sender::smtpHost() const
  {
    return m_smtpHost;
  }

  void Sender::setSmtpHost(const QString& host)
  {
    m_smtpHost = host;
  }

  int Sender::smtpPort() const
  {
    return m_smtpPort;
  }

  void Sender::setSmtpPort(int port)
  {
    m_smtpPort = port;
  }

  Sender::EncryptionType Sender::encryptionType() const
  {
    return m_encryptionType;
  }

  void Sender::setEncryptionType(EncryptionType type)
  {
    m_encryptionType = type;
  }

  bool Sender::ignoreSslErrors() const
  {
    return m_ignoreSslErrors;
  }

  void Sender::setIgnoreSslErrors(bool ignore)
  {
    m_ignoreSslErrors = ignore;
  }

  QList<QSslError> Sender::ignoredSslErrors() const
  {
    return m_sslErrorsToIgnore;
  }

  void Sender::setIgnoredSslErrors(const QList<QSslError> & errors)
  {
    m_sslErrorsToIgnore = errors;
  }

  QString Sender::emailSenderName() const
  {
    return m_emailSenderName;
  }

  void Sender::setEmailSenderName(const QString &senderName)
  {
    m_emailSenderName = senderName;
  }

  QString Sender::emailSenderAddress() const
  {
    return m_emailSenderAddress;
  }

  void Sender::setEmailSenderAddress(const QString &address)
  {
    m_emailSenderAddress = address;
  }

  QString Sender::emailSubject() const
  {
    return m_emailSubject;
  }

  void Sender::setEmailSubject(const QString &subject)
  {
    m_emailSubject = subject;
  }

  QString Sender::emailTemplate() const
  {
    return m_emailTemplate;
  }

  void Sender::setEmailTemplate(const QString &message)
  {
    m_emailTemplate = message;
  }

  float Sender::connectionTimeout() const
  {
    return m_connectionTimeout;
  }

  void Sender::setConnectionTimeout(float timeout)
  {
    m_connectionTimeout = timeout;
  }

  float Sender::responseTimeout() const
  {
    return m_responseTimeout;
  }

  void Sender::setResponseTimeout(float timeout)
  {
    m_responseTimeout = timeout;
  }

  float Sender::sendMessageTimeout() const
  {
    return m_sendMessageTimeout;
  }

  void Sender::setSendMessageTimeout(float timeout)
  {
    m_sendMessageTimeout = timeout;
  }

  folly::Future<folly::Unit> Sender::toFuture(SendStatus status)
  {
    if (status.ok)
      return folly::Unit();
    else
      return std::runtime_error(status.errorMessage.toStdString());
  }

}
