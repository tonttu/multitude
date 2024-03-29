/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RADIANT_EMAIL_HPP
#define RADIANT_EMAIL_HPP

#include "Export.hpp"

#include <QIODevice>
#include <QString>

#include <list>
#include <memory>
#include <optional>

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
    Attachment(Attachment &&) = default;
    Attachment & operator=(Attachment &&) = default;

    /// Filename that appears in the email
    QString filename;
    /// Device to read attachment data from
    std::unique_ptr<QIODevice> device;
    /// Content type for attachment, e.g. "application/octet-stream"
    QString contentType;
    /// Content disposition for the attachment, typically either "inline" or "attachment"
    QString contentDisposition = "attachment";
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

    /// Multi-part type for the message
    enum class MultiPartType {
      Mixed           = 0,            ///< RFC 2046, section 5.1.3
      Digest          = 1,            ///< RFC 2046, section 5.1.5
      Alternative     = 2,            ///< RFC 2046, section 5.1.4
      Related         = 3,            ///< RFC 2387
      Report          = 4,            ///< RFC 6522
      Signed          = 5,            ///< RFC 1847, section 2.1
      Encrypted       = 6             ///< RFC 1847, section 2.2
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

    /// Set address to reply to
    /// @param address to send replies to
    void setReplyTo(const Address& address);

    /// An address to which replies should be sent.
    /// Can be used when sender cannot receive replies.
    /// @note RFC5322 section 3.6.2 specifies the "Reply-To" field as optional
    /// list of one or more addresses. The method assumes only single address,
    /// due to inernal class that eventually constructs the final message
    /// taking only single address for reply-to.
    const std::optional<Address> replyTo() const { return m_replyTo; }

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

    /// Set multi-part type. The default is Related, but for inline attachments
    /// Mixed is better. For instance Gmail shows inline html attachment inline
    /// only in Mixed multi-part message.
    void setMultiPartType(MultiPartType type);
    MultiPartType multiPartType() const;

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
    std::optional<Address> m_replyTo;
    QString m_subject;
    QString m_content;
    QList<Address> m_recipientsTo;
    QList<Address> m_recipientsCc;
    QList<Address> m_recipientsBcc;
    std::list<Attachment> m_attachments;
    MultiPartType m_multiPartType = MultiPartType::Related;
  };

}

#endif // RADIANT_EMAIL_HPP
