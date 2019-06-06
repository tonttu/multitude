#pragma once

#include "Export.hpp"
#include "Flags.hpp"
#include "Mutex.hpp"

#include <QString>

#include <vector>

#ifdef ENABLE_CI_BUILD
#define DEFAULT_MINIDUMP_URL "http://diagnostics.multitaction.com/crash-reports/upload-crash-dump"
#else
#define DEFAULT_MINIDUMP_URL QString()
#endif

namespace Radiant
{
  namespace CrashHandler
  {
    class AttachmentRingBuffer;

    enum AttachmentFlag
    {
      ATTACHMENT_NO_FLAGS    = 0,
      /// When truncating a file, take the end of the file instead of the beginning
      ATTACHMENT_TAIL        = 1 << 0,
      /// This attachment is a AttachmentRingBuffer with a 64 bit header (ring buffer write offset)
      ATTACHMENT_RING_BUFFER = 1 << 1,
    };
    typedef FlagsT<AttachmentFlag> AttachmentFlags;

    /// Metadata stored with each attachment
    struct AttachmentMetadata
    {
      /// Original filename, or empty if not applicable
      QString filename;

      AttachmentFlags flags = ATTACHMENT_NO_FLAGS;
    };

    RADIANT_API void init(const QString & application,
                          const QString & url, const QString & db = QString());

    /// Add a single key-value -pair annotation to the crash report. Key and
    /// value can be max 255 characters long, and the crash report includes
    /// max 64 entries.
    RADIANT_API void setAnnotation(const QByteArray & key, const QByteArray & value);

    /// Removes an annotation
    RADIANT_API void removeAnnotation(const QByteArray & key);

    /// Set attachment with a unique key to the crash report. File will be truncated to
    /// attachmentMaxSize() bytes. The file be read immediately.
    /// The function can be called several times with the same key, any old attachment
    /// with the same key will be overwritten.
    /// @param key non-empty unique key that is also used in the crash reporting server UI,
    ///            for instance "screen.xml"
    /// @param filename Actual filename, for instance "/tmp/my-screen.xml"
    /// @param flags attachment flags
    /// @returns false on IO error
    RADIANT_API bool setAttachmentFile(const QByteArray & key, const QString & filename,
                                       AttachmentFlags flags = ATTACHMENT_NO_FLAGS);

    /// Like setAttachmentFile, but with data buffer.
    /// @param data raw data that will be copied to the report. Please note that this data
    ///        will be truncated if it is longer than attachmentMaxSize()
    RADIANT_API void setAttachmentData(const QByteArray & key, const QByteArray & data,
                                       AttachmentMetadata metadata = AttachmentMetadata());

    /// Like setAttachentData, but instead of using QByteArray and making a copy of
    /// the data, this function passes the data pointer directly to breakpad / crashpad.
    /// @param data Data pointer that contains the attachment data. This will not be copied,
    ///        so the pointer needs to stay valid until the attachment is removed. The data
    ///        is also not truncated like with other functions.
    RADIANT_API void setAttachmentPtr(const QByteArray & key, void * data, size_t len,
                                      AttachmentMetadata metadata = AttachmentMetadata());

    /// Like setAttachmentPtr, but with a ring buffer object
    RADIANT_API void setAttachmentBuffer(const QByteArray & key, AttachmentRingBuffer & buffer);

    /// Removes an attachment
    RADIANT_API void removeAttachment(const QByteArray & key);

    /// Set maximum size of attachments in bytes. This won't affect attachments that
    /// have already been registered.
    RADIANT_API void setAttachmentMaxSize(size_t bytes);
    /// Attachment max size, 128 kB by default
    RADIANT_API size_t attachmentMaxSize();

    /// Create a minidump immediately without crashing
    /// @returns absolute file path to the dump file
    RADIANT_API QString makeDump();
    RADIANT_API void reloadSignalHandlers();
    /// Get default path to store minidump files
    RADIANT_API QString defaultMinidumpPath();


    /// Ring buffer that stores last N bytes of data in crash report.
    /// Register this with setAttachmentBuffer. The data contains a 64-bit header,
    /// which is included in the bufferSize.
    class RADIANT_API AttachmentRingBuffer
    {
    public:
      /// Creates a new ring buffer with a static buffer size
      AttachmentRingBuffer(size_t bufferSize = attachmentMaxSize());

      /// Pointer to the beginning of the buffer data (header)
      void * data();

      /// Size in bytes of the whole buffer, including the payload and header
      size_t bufferSize() const;

      /// Max payload size
      size_t maxDataSize() const;

      /// Write data to the end of the buffer, thread-safe
      void write(char * data, size_t len);

    private:
      /// This is the data that should be registered to the crash reporting system,
      /// including the 64 bit header and the actual payload
      std::vector<uint8_t> m_buffer;

      /// Locked when the header is modified
      Radiant::Mutex m_headerMutex;
    };
  }
}
