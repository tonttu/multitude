#ifndef RADIANT_EMAILSENDER_HPP
#define RADIANT_EMAILSENDER_HPP

// Include order is important to avoid conflicts with Qt
#include <folly/futures/Future.h>

#include "Email.hpp"

#include <Valuable/AttributeBool.hpp>
#include <Valuable/AttributeEnum.hpp>
#include <Valuable/AttributeFloat.hpp>
#include <Valuable/AttributeInt.hpp>
#include <Valuable/AttributeString.hpp>
#include <Valuable/Node.hpp>

#include <QSslError>
#include <QNetworkProxy>

#include <memory>

namespace Email
{

  class EMAIL_API Sender : public Valuable::Node
  {
  public:
    /// Status of a single email send operation
    struct SendStatus
    {
      /// True if email was sent successfully; otherwise false
      bool ok = true;
      /// Empty if email was sent succesfully; otherwise contains error message
      QString errorMessage;
    };

    enum class EncryptionType {
      NONE, /// "none", No encryption, plain TCP-connection
      SSL,  /// "ssl", SSL encryption
      TLS,  /// "tls", TLS encryption
      AUTO  /// "auto", Deduce the encryption based on port number
    };

    /// Settings for connecting to SMTP server
    struct SMTPSettings
    {
      QString host;
      int port;
      EncryptionType encryption;
      QString username;
      QString password;
      bool ignoreSslErrors;
      QList<QSslError> sslErrorsToIgnore;
      std::optional<QNetworkProxy> proxy;

      float connectionTimeout;
      float responseTimeout;
      float sendTimeout;
    };


    Sender();
    virtual ~Sender();

    /// Adds given message to send queue
    /// @param message Message to be sent. Note that the attachments are not
    ///                safe to destroy before the operation has finished or failed
    /// @return Future that is fulfilled when the operation is finished or failed
    folly::Future<SendStatus> queueMessage(Message&& message);

    /// Returns template message specified using attributes "sender-name",
    /// "sender-address", "subject" and "template"
    Message templateMessage() const;

    SMTPSettings settings() const;

    // Attributes controlling server, and template messages

    /// Username of the SMTP-server used for sending emails
    QString smtpUsername() const;
    void setSmtpUsername(const QString& username);

    /// Password of the SMTP-server used for sending emails
    QString smtpPassword() const;
    void setSmtpPassword(const QString& password);

    /// Host address of the SMTP-server used for sending emails
    QString smtpHost() const;
    void setSmtpHost(const QString& host);

    /// Port of the SMTP-server used for sending emails
    int smtpPort() const;
    void setSmtpPort(int port);

    /// Encryption type to be used when connecting to the SMTP-server
    EncryptionType encryptionType() const;
    void setEncryptionType(EncryptionType type);

    /// Should SSL-errors be ignored when sending emails. These occur if the
    /// certificates in use have not been signed by authority (they can be
    /// self-signed or forged)
    bool ignoreSslErrors() const;
    void setIgnoreSslErrors(bool ignore);

    /// Sets specific SSL errors to ignore
    /// @note if list is not empty the Sender::ignoreSslErrors is ignored
    /// @note if QSslError object doesn't have certificate then error is
    /// ignored based on QSslError::SslError value.
    QList<QSslError> ignoredSslErrors() const;
    void setIgnoredSslErrors(const QList<QSslError> &errors);

    std::optional<QNetworkProxy> networkProxy() const;
    void setNetworkProxy(const QNetworkProxy & proxy);

    QString emailSenderName() const;
    void setEmailSenderName(const QString& senderName);

    QString emailSenderAddress() const;
    void setEmailSenderAddress(const QString& address);

    QString emailSubject() const;
    void setEmailSubject(const QString& subject);

    QString emailTemplate() const;
    void setEmailTemplate(const QString& message);


    float connectionTimeout() const;
    void setConnectionTimeout(float timeout);

    float responseTimeout() const;
    void setResponseTimeout(float timeout);

    float sendMessageTimeout() const;
    void setSendMessageTimeout(float timeout);

    /// Converts successful SendStatus to folly::Unit and
    /// failed SendStatus to std::runtime_error
    static folly::Future<folly::Unit> toFuture(SendStatus status);

  private:
    // Implementation details (backend etc)
    class D;
    std::unique_ptr<D> m_d;

    SMTPSettings m_smtpSettings;

    // SMTP settings
    Valuable::AttributeString m_smtpUsername;
    Valuable::AttributeString m_smtpPassword;
    Valuable::AttributeString m_smtpHost;
    Valuable::AttributeInt m_smtpPort;
    Valuable::AttributeT<EncryptionType> m_encryptionType;
    Valuable::AttributeBool m_ignoreSslErrors;
    QList<QSslError> m_sslErrorsToIgnore;
    std::optional<QNetworkProxy> m_proxy;

    // Email settings
    Valuable::AttributeString m_emailSenderName;
    Valuable::AttributeString m_emailSenderAddress;
    Valuable::AttributeString m_emailSubject;
    Valuable::AttributeString m_emailTemplate;

    // Timeout settings, in seconds
    Valuable::AttributeFloat m_connectionTimeout;
    Valuable::AttributeFloat m_responseTimeout;
    Valuable::AttributeFloat m_sendMessageTimeout;
  };

}

#endif // RADIANT_EMAILSENDER_HPP
