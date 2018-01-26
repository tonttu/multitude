#include "Email.hpp"

#include <cassert>

namespace Email
{

  Attachment::Attachment()
  {}

  // ----------------------------------------------------------------------

  Message::Message()
  {
  }

  Message::~Message()
  {
  }

  void Message::setSender(const Address& address)
  {
    m_sender = address;
  }

  void Message::setSubject(const QString& subject)
  {
    m_subject = subject;
  }

  void Message::setContent(const QString & content)
  {
    m_content = content;
  }

  void Message::addContent(const QString & content)
  {
    m_content += content;
  }

  void Message::addRecipient(const Address& recipient, Message::RecipientType type)
  {
    switch(type) {
      case RecipientType::To:
        m_recipientsTo.append(recipient);
        break;
      case RecipientType::Cc:
        m_recipientsCc.append(recipient);
        break;
      case RecipientType::Bcc:
        m_recipientsBcc.append(recipient);
        break;
      default:
        assert(false);
        break;
    }
  }

  const QList<Address> Message::recipients(Message::RecipientType type) const
  {
    return recipientListByType(type);
  }

  void Message::addAttachment(const QString& filename, std::unique_ptr<QIODevice> data, const QString& contentType)
  {
    Attachment attachment;
    attachment.filename = filename;
    attachment.device = std::move(data);
    attachment.contentType = contentType;
    addAttachment(std::move(attachment));
  }

  void Message::addAttachment(Attachment &&attachment)
  {
    m_attachments.push_back(std::move(attachment));
  }

  QList<Address> Message::recipientListByType(Message::RecipientType type) const
  {
    QList<Address> result;

    switch(type) {
      case RecipientType::To:
        result = m_recipientsTo;
        break;
      case RecipientType::Cc:
        result = m_recipientsCc;
        break;
      case RecipientType::Bcc:
        result = m_recipientsBcc;
        break;
      default:
        assert(false);
    }

    return result;
  }

}
