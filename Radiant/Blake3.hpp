#pragma once

#include <blake3.h>

#include <QByteArray>
#include <QFile>

#include <boost/expected/expected.hpp>

namespace Radiant
{
  /// Calculates BLAKE3 hashes
  class Blake3
  {
  public:
    inline Blake3();

    /// Adds data to the hash, can be called multiple times
    inline void addData(const void * data, std::size_t len);
    inline void addData(const QByteArray & data);

    /// Adds data to the hash from the given device. Reads at most len bytes.
    /// Returns the number of bytes read or an IO error message.
    inline boost::expected<size_t, QString> addData(
        QIODevice & dev, size_t len = std::numeric_limits<size_t>::max());

    /// Returns the hash in binary form (32 bytes). Can be called multiple
    /// times, doesn't modify the internal state. It's fine to call this
    /// function, modify the hash by calling addData, and then call this
    /// function again.
    inline QByteArray result() const;

    /// Shorthand for easily hashing some data
    inline static QByteArray hashData(const QByteArray & data);

    /// Hash a file, return the hash or an error string on IO error.
    /// @param[out] fileSizeOut if not null, this is the number of bytes
    ///             processed.
    inline static boost::expected<QByteArray, QString> hashFile(
        const QString & filePath, size_t * fileSizeOut = nullptr);

  private:
    blake3_hasher m_hasher;
  };

  /////////////////////////////////////////////////////////////////////////////

  Blake3::Blake3()
  {
    blake3_hasher_init(&m_hasher);
  }

  void Blake3::addData(const void * data, size_t len)
  {
    blake3_hasher_update(&m_hasher, data, len);
  }

  void Blake3::addData(const QByteArray & data)
  {
    blake3_hasher_update(&m_hasher, data.data(), data.size());
  }

  boost::expected<size_t, QString> Blake3::addData(QIODevice & dev, size_t len)
  {
    QByteArray buffer(std::min<size_t>(len, 64 * 1024), Qt::Initialization::Uninitialized);
    size_t bytes = 0;
    while (bytes < len) {
      qint64 res = dev.read(buffer.data(), std::min<size_t>(len - bytes, buffer.size()));
      if (res < 0)
        return boost::make_unexpected(dev.errorString());

      if (res == 0)
        break;

      addData(buffer.data(), res);
      bytes += res;
    }

    return bytes;
  }

  QByteArray Blake3::result() const
  {
    QByteArray result(BLAKE3_OUT_LEN, Qt::Initialization::Uninitialized);
    blake3_hasher_finalize(&m_hasher, reinterpret_cast<uint8_t*>(result.data()), result.size());
    return result;
  }

  QByteArray Blake3::hashData(const QByteArray & data)
  {
    Blake3 blake3;
    blake3.addData(data);
    return blake3.result();
  }

  boost::expected<QByteArray, QString> Blake3::hashFile(const QString & filePath, size_t * fileSizeOut)
  {
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
      return boost::make_unexpected(file.errorString());

    Blake3 hasher;
    boost::expected<size_t, QString> r = hasher.addData(file);
    if (r) {
      if (fileSizeOut)
        *fileSizeOut = r.value();
      return hasher.result();
    } else {
      return boost::make_unexpected(r.error());
    }
  }
}
