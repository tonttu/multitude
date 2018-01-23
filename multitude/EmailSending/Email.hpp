#ifndef RADIANT_EMAIL_HPP
#define RADIANT_EMAIL_HPP

#include "Export.hpp"

#include <QIODevice>
#include <QString>

#include <list>
#include <memory>

namespace Email
{

  /// Email address
  struct EMAIL_API Address
  {
    /// Recipient address, e.g. john.doe@gmail.com
    QString address;
    /// Recipient name, e.g. "John Doe"
    QString name;
  };

  /// Email attachment containing a file
  struct EMAIL_API Attachment
  {
    Attachment();
    Attachment(Attachment && o) = default;

    /// Filename that appears in the email
    QString filename;
    /// Device to read attachment data from
    std::unique_ptr<QIODevice> device;
    /// Content type for attachment, e.g. "application/octet-stream"
    QString contentType;
  };

  /// Email message
  class EMAIL_API Message
  {
  public:
    /// Recipient type for emails
    enum class RecipientType {
      /// To
      To,
      /// Carbon copy
      Cc,
      /// Blind carbon copy
      Bcc
    };

    Message();
    Message(Message && other) = default;

    ~Message();

    Message& operator=(Message&& other) = default;

    /// Set sender email address
    /// @param address address of the email
    void setSender(const Address& address);

    /// Address of the email sender
    const Address& sender() const { return m_sender; }

    /// Set email subject
    /// @param subject subject of the email
    void setSubject(const QString& subject);

    /// Subject of the email
    const QString& subject() const { return m_subject; }

    /// Set email content text
    void setContent(const QString & content);

    /// Add text to email content
    void addContent(const QString & content);

    /// Text content of the email
    const QString & content() const { return m_content; }

    /// Add a recipient to email
    /// @param recipient email address of the recipient
    /// @param type recipient type
    void addRecipient(const Address& recipient, RecipientType type = RecipientType::To);

    /// List of email recipients of given type
    /// @param type type of recipients to get
    /// @return list of email addresses of recipients
    const QList<Address> recipients(RecipientType type = RecipientType::To) const;

    /// Add file attachment to email
    /// @param filename filename of the attachment that appears in the email
    /// @param data data to attach
    /// @param contentType content type for the attachment
    void addAttachment(const QString& filename, std::unique_ptr<QIODevice> data, const QString& contentType = "application/octet-stream");
    void addAttachment(Attachment&& attachment);

    const std::list<Attachment>& attachments() const { return m_attachments; }

  private:
    QList<Address> recipientListByType(RecipientType type) const;

    Address m_sender;
    QString m_subject;
    QString m_content;
    QList<Address> m_recipientsTo;
    QList<Address> m_recipientsCc;
    QList<Address> m_recipientsBcc;
    std::list<Attachment> m_attachments;
  };

}

#endif // RADIANT_EMAIL_HPP
