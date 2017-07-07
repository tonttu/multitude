/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "ImageCodecCS.hpp"
#include "Image.hpp"

#include <Radiant/Directory.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/BinaryData.hpp>

#include <QFile>

#include <lz4.h>

namespace Luminous
{
  const char * s_magic = "cornerstone img";

  bool ImageCodecCS::canRead(QFile & file)
  {
    ImageInfo info;
    return ping(info, file);
  }

  QString ImageCodecCS::extensions() const
  {
    return "csimg";
  }

  QString ImageCodecCS::name() const
  {
    return "CS";
  }

  bool ImageCodecCS::ping(ImageInfo & info, QFile & file)
  {
    Radiant::BinaryData bd;
    auto originalPos = file.pos();

    int32_t headerSize = 0;
    file.read((char*)&headerSize, sizeof(headerSize));

    QByteArray binaryDataMagic = file.peek(4);

    // Hide warning messages from QByteArray constructor and bd.readString by
    // checking for string marker manually
    if (binaryDataMagic.size() != 4 || *reinterpret_cast<int32_t*>(binaryDataMagic.data()) !=
        Radiant::BinaryData::STRING_MARKER) {
      file.seek(originalPos);
      return false;
    }

    QByteArray buffer = file.read(headerSize);
    bd.linkTo(buffer.data(), buffer.size());
    bd.setTotal(buffer.size());

    file.seek(originalPos);

    QString hdr;
    if (bd.readString(hdr) && hdr != s_magic) {
      Radiant::warning("ImageCodecCS::ping # header error: '%s'", hdr.toUtf8().data());
      return false;
    }

    bool ok = true;

    int32_t version = bd.readInt32();
    Compression compression = (Compression)bd.readInt32();
    if (compression > COMPRESSION_LZ4) return false;
    int32_t width = bd.readInt32();
    int32_t height = bd.readInt32();
    int32_t layout = bd.readInt32();
    int32_t type = bd.readInt32();
    int32_t dataSize = bd.readInt32(&ok);
    (void)dataSize;

    Luminous::PixelFormat::Compression pfCompression = Luminous::PixelFormat::COMPRESSION_NONE;
    Flags flags = NO_FLAGS;

    if (version >= 1) {
      pfCompression = (Luminous::PixelFormat::Compression)bd.readInt32();
      flags = (Flags)bd.readInt32(&ok);
    }

    if(!ok) return false;

    info.height = height;
    info.width = width;

    if (pfCompression != Luminous::PixelFormat::COMPRESSION_NONE) {
      info.pf = Luminous::PixelFormat(pfCompression, flags & FLAG_PREMULTIPLIED_ALPHA);
    } else {
      info.pf = Luminous::PixelFormat(Luminous::PixelFormat::ChannelLayout(layout),
                                      Luminous::PixelFormat::ChannelType(type),
                                      flags & FLAG_PREMULTIPLIED_ALPHA);
    }

    return true;
  }

  bool ImageCodecCS::read(Image & image, QFile & file)
  {
    Radiant::BinaryData bd;
    int32_t headerSize = 0;
    file.read((char*)&headerSize, sizeof(headerSize));

    QByteArray buffer = file.read(headerSize);
    bd.linkTo(buffer.data(), buffer.size());
    bd.setTotal(buffer.size());

    QString hdr;
    if (bd.readString(hdr) && hdr != "cornerstone img") {
      Radiant::warning("ImageCodecCS::read # header error: '%s'", hdr.toUtf8().data());
      return false;
    }

    int32_t version = bd.readInt32();
    Compression compression = (Compression)bd.readInt32();
    int32_t width = bd.readInt32();
    int32_t height = bd.readInt32();
    int32_t layout = bd.readInt32();
    int32_t type = bd.readInt32();
    int32_t dataSize = bd.readInt32();

    Luminous::PixelFormat::Compression pfCompression = Luminous::PixelFormat::COMPRESSION_NONE;
    uint32_t flags = NO_FLAGS;

    if (version >= 1) {
      pfCompression = (Luminous::PixelFormat::Compression)bd.readInt32();
      flags = bd.readInt32();
    }

    if (pfCompression != Luminous::PixelFormat::COMPRESSION_NONE) {
      image.allocate(width, height, pfCompression);
    } else {
      image.allocate(width, height, Luminous::PixelFormat(Luminous::PixelFormat::ChannelLayout(layout),
                                                          Luminous::PixelFormat::ChannelType(type),
                                                          flags & FLAG_PREMULTIPLIED_ALPHA));
    }
    const int32_t rawDataSize = image.lineSize() * image.height();
    const int32_t imageDataOffset = headerSize + sizeof(headerSize);

    if (compression == NO_COMPRESSION) {
      file.seek(imageDataOffset);
      return rawDataSize == dataSize && rawDataSize == file.read((char*)image.data(), rawDataSize);
    } else if (compression == COMPRESSION_ZLIB) {
      file.seek(imageDataOffset);
      QByteArray data = qUncompress(file.read(dataSize));
      if (data.size() != rawDataSize) {
        Radiant::warning("ImageCodecCS::read # uncompressed data size: %d (should be %d)", data.size(), rawDataSize);
        return false;
      }
      memcpy(image.data(), data.data(), rawDataSize);
      return true;
    } else if (compression == COMPRESSION_LZ4) {
      /// @todo Why is mapping so much slower than copying the data? Maybe
      ///       should use this only with bigger images?
#if 0
      uchar * compressedData = file.map(imageDataOffset, dataSize, QFileDevice::NoOptions);
      if (compressedData) {
        int r = LZ4_decompress_safe((const char*)compressedData, (char*)image.data(), dataSize, rawDataSize);
        file.unmap(compressedData);
        return r == rawDataSize;
      }
      return false;
#else
      QByteArray compressedData = file.read(dataSize);
      int32_t r = LZ4_decompress_safe(compressedData.data(), (char*)image.data(), dataSize, rawDataSize);
      return r == rawDataSize;
#endif
    } else {
      return false;
    }
  }

  bool ImageCodecCS::write(const Image & image, QFile & file)
  {
    /// @todo should be configurable
    const Compression compression = COMPRESSION_LZ4;
    const int32_t fileFormatVersion = 1;

    Radiant::BinaryData bd;
    bd.write(s_magic);
    bd.writeInt32(fileFormatVersion);
    bd.writeInt32(compression);
    bd.writeInt32(image.width());
    bd.writeInt32(image.height());
    bd.writeInt32(image.pixelFormat().layout());
    bd.writeInt32(image.pixelFormat().type());

    int32_t dataSizePos = bd.pos();
    bd.writeInt32(0); /// placeholder for the data size

    bd.writeInt32(image.pixelFormat().compression());
    uint32_t flags = NO_FLAGS;
    if (image.pixelFormat().isPremultipliedAlpha())
      flags |= FLAG_PREMULTIPLIED_ALPHA;
    bd.writeInt32(flags);

    const int32_t headerSize = bd.pos();
    const int32_t rawDataSize = image.lineSize() * image.height();

    if (compression == NO_COMPRESSION) {
      bd.setPos(dataSizePos);
      bd.writeInt32(rawDataSize);

      file.write((const char*)&headerSize, sizeof(headerSize));
      qint64 totalWritten = file.write(bd.data(), headerSize);

      totalWritten += file.write((const char*)image.data(), rawDataSize);
      return totalWritten == headerSize + rawDataSize;
    } else if (compression == COMPRESSION_ZLIB) {
      QByteArray data = qCompress(image.data(), rawDataSize);

      bd.setPos(dataSizePos);
      bd.writeInt32(data.size());

      file.write((const char*)&headerSize, sizeof(headerSize));
      qint64 totalWritten = file.write(bd.data(), headerSize);

      totalWritten += file.write(data);
      return totalWritten == headerSize + data.size();
    } else if (compression == COMPRESSION_LZ4) {
      const int bufferSize = LZ4_compressBound(rawDataSize);
      std::unique_ptr<char[]> data(new char[bufferSize]);
#if LZ4_VERSION_MAJOR > 1 || (LZ4_VERSION_MAJOR == 1 && LZ4_VERSION_MINOR >= 7)
      const int32_t dataSize = LZ4_compress_default((const char*)image.data(), data.get(), rawDataSize, bufferSize);
#else
      const int32_t dataSize = LZ4_compress((const char*)image.data(), data.get(), rawDataSize);
#endif

      bd.setPos(dataSizePos);
      bd.writeInt32(dataSize);

      file.write((const char*)&headerSize, sizeof(headerSize));
      qint64 totalWritten = file.write(bd.data(), headerSize);

      totalWritten += file.write(data.get(), dataSize);
      return totalWritten == headerSize + dataSize;
    } else {
      return false;
    }
  }

}
