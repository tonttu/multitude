#include "CrashHandler.hpp"
#include "Trace.hpp"

#include <QFile>
#include <QFileInfo>

namespace Radiant
{
  namespace CrashHandler
  {
    static std::map<QByteArray, QByteArray> s_attachments;
    static size_t s_attachmentMaxSize = 128*1024;

    void setAttachmentPtrImpl(const QByteArray & key, void * data, size_t len);

    bool setAttachmentFile(const QByteArray & key, const QString & filename,
                           AttachmentFlags flags)
    {
      QFile file(filename);
      if (file.open(QFile::ReadOnly)) {
        QByteArray data;
        const qint64 maxSize = static_cast<qint64>(attachmentMaxSize());
        if (flags & ATTACHMENT_TAIL) {
          const qint64 fileSize = file.size();
          if (fileSize > maxSize) {
            file.seek(fileSize - maxSize);
          }
        }
        data = file.read(maxSize);
        AttachmentMetadata metadata;
        metadata.filename = QFileInfo(filename).absoluteFilePath();
        metadata.flags = flags;
        setAttachmentData(key, data, metadata);
        return true;
      } else {
        Radiant::error("CrashHandler::setAttachmentFile # Failed to open '%s': %s",
                       file.fileName().toUtf8().data(), file.errorString().toUtf8().data());
        return false;
      }
    }

    void setAttachmentData(const QByteArray & key, const QByteArray & data,
                           AttachmentMetadata metadata)
    {
      QByteArray & persistent = s_attachments[key];
      const int maxSize = static_cast<int>(attachmentMaxSize());
      if (data.size() > maxSize) {
        if (metadata.flags & ATTACHMENT_TAIL) {
          persistent = data.mid(data.size() - maxSize, maxSize);
        } else {
          persistent = data;
          persistent.truncate(maxSize);
        }
      } else {
        persistent = data;
      }

      setAttachmentPtr(key, persistent.data(), static_cast<size_t>(persistent.size()), metadata);
    }

    void setAttachmentBuffer(const QByteArray & key, AttachmentRingBuffer & buffer)
    {
      AttachmentMetadata metadata;
      metadata.flags |= ATTACHMENT_RING_BUFFER;
      setAttachmentPtr(key, buffer.data(), buffer.bufferSize(), metadata);
    }

    void setAttachmentMaxSize(size_t bytes)
    {
      s_attachmentMaxSize = bytes;
    }

    size_t attachmentMaxSize()
    {
      return s_attachmentMaxSize;
    }

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    AttachmentRingBuffer::AttachmentRingBuffer(size_t size)
      : m_buffer(size)
    {
    }

    void * AttachmentRingBuffer::data()
    {
      return m_buffer.data();
    }

    size_t AttachmentRingBuffer::bufferSize() const
    {
      return m_buffer.size();
    }

    size_t AttachmentRingBuffer::maxDataSize() const
    {
      return m_buffer.size() - sizeof(uint64_t);
    }

    void AttachmentRingBuffer::write(char * newData, size_t len)
    {
      if (len == 0)
        return;

      uint64_t * header = reinterpret_cast<uint64_t*>(m_buffer.data());
      uint8_t * data = reinterpret_cast<uint8_t*>(header + 1);
      const size_t dataSize = maxDataSize();

      if (len > dataSize) {
        newData += (len - dataSize);
        len = dataSize;
      }

      size_t base;
      {
        Guard g(m_headerMutex);
        base = *header % dataSize;
        *header += len;
      }

      size_t copy = std::min<size_t>(len, dataSize - base);
      memcpy(data + base, newData, copy);

      if (len > copy)
        memcpy(data, newData + copy, len - copy);
    }

    void setAttachmentPtr(const QByteArray & key, void * data, size_t len,
                          AttachmentMetadata metadata)
    {
      if (!metadata.filename.isEmpty())
        setAnnotation("attachment-filename:" + key, metadata.filename.toUtf8());
      else
        removeAnnotation("attachment-filename:" + key);

      if (metadata.flags)
        setAnnotation("attachment-flags:" + key, QString::number(metadata.flags.asInt(), 16).toUtf8());
      else
        removeAnnotation("attachment-flags:" + key);

      if (len)
        setAnnotation("attachment-addr:" + key, QString::number(uint64_t(data), 16).toUtf8());
      else
        removeAnnotation("attachment-addr:" + key);

      setAttachmentPtrImpl(key, data, len);
    }
  }
}

