/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ImageCodecDDS.hpp"
#include "Image.hpp"

#include <Radiant/Directory.hpp>
#include <Radiant/FileUtils.hpp>

#include <QFile>

namespace Luminous {

// original code by Jon Watte

//  little-endian, of course
#define DDS_MAGIC 0x20534444

//  DDS_header.dwFlags
#define DDSD_CAPS                   0x00000001
#define DDSD_HEIGHT                 0x00000002
#define DDSD_WIDTH                  0x00000004
#define DDSD_PITCH                  0x00000008
#define DDSD_PIXELFORMAT            0x00001000
#define DDSD_MIPMAPCOUNT            0x00020000
#define DDSD_LINEARSIZE             0x00080000
#define DDSD_DEPTH                  0x00800000

//  DDS_header.sPixelFormat.dwFlags
#define DDPF_ALPHAPIXELS            0x00000001
#define DDPF_FOURCC                 0x00000004
#define DDPF_INDEXED                0x00000020
#define DDPF_RGB                    0x00000040

//  DDS_header.sCaps.dwCaps1
#define DDSCAPS_COMPLEX             0x00000008
#define DDSCAPS_TEXTURE             0x00001000
#define DDSCAPS_MIPMAP              0x00400000

//  DDS_header.sCaps.dwCaps2
#define DDSCAPS2_CUBEMAP            0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX  0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY  0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY  0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ  0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ  0x00008000
#define DDSCAPS2_VOLUME             0x00200000

#define D3DFMT_DXT1     0x31545844 // 'DXT1'
#define D3DFMT_DXT3     0x33545844 // 'DXT3'
#define D3DFMT_DXT5     0x35545844 // 'DXT5'

#define PF_IS_DXT1(pf) \
  ((pf.dwFlags & DDPF_FOURCC) && \
   (pf.dwFourCC == D3DFMT_DXT1))

#define PF_IS_DXT3(pf) \
  ((pf.dwFlags & DDPF_FOURCC) && \
   (pf.dwFourCC == D3DFMT_DXT3))

#define PF_IS_DXT5(pf) \
  ((pf.dwFlags & DDPF_FOURCC) && \
   (pf.dwFourCC == D3DFMT_DXT5))

#define PF_IS_BGRA8(pf) \
  ((pf.dwFlags & DDPF_RGB) && \
   (pf.dwFlags & DDPF_ALPHAPIXELS) && \
   (pf.dwRGBBitCount == 32) && \
   (pf.dwRBitMask == 0xff0000) && \
   (pf.dwGBitMask == 0xff00) && \
   (pf.dwBBitMask == 0xff) && \
   (pf.dwAlphaBitMask == 0xff000000U))

#define PF_IS_BGR8(pf) \
  ((pf.dwFlags & DDPF_ALPHAPIXELS) && \
  !(pf.dwFlags & DDPF_ALPHAPIXELS) && \
   (pf.dwRGBBitCount == 24) && \
   (pf.dwRBitMask == 0xff0000) && \
   (pf.dwGBitMask == 0xff00) && \
   (pf.dwBBitMask == 0xff))

#define PF_IS_BGR5A1(pf) \
  ((pf.dwFlags & DDPF_RGB) && \
   (pf.dwFlags & DDPF_ALPHAPIXELS) && \
   (pf.dwRGBBitCount == 16) && \
   (pf.dwRBitMask == 0x00007c00) && \
   (pf.dwGBitMask == 0x000003e0) && \
   (pf.dwBBitMask == 0x0000001f) && \
   (pf.dwAlphaBitMask == 0x00008000))

#define PF_IS_BGR565(pf) \
  ((pf.dwFlags & DDPF_RGB) && \
  !(pf.dwFlags & DDPF_ALPHAPIXELS) && \
   (pf.dwRGBBitCount == 16) && \
   (pf.dwRBitMask == 0x0000f800) && \
   (pf.dwGBitMask == 0x000007e0) && \
   (pf.dwBBitMask == 0x0000001f))

#define PF_IS_INDEX8(pf) \
  ((pf.dwFlags & DDPF_INDEXED) && \
   (pf.dwRGBBitCount == 8))


union DDS_header {
  struct {
    uint32_t      dwMagic;
    uint32_t      dwSize;
    uint32_t      dwFlags;
    uint32_t      dwHeight;
    uint32_t      dwWidth;
    uint32_t      dwPitchOrLinearSize;
    uint32_t      dwDepth;
    uint32_t      dwMipMapCount;
    uint32_t      dwReserved1[ 11 ];

    //  DDPIXELFORMAT
    struct {
      uint32_t    dwSize;
      uint32_t    dwFlags;
      uint32_t    dwFourCC;
      uint32_t    dwRGBBitCount;
      uint32_t    dwRBitMask;
      uint32_t    dwGBitMask;
      uint32_t    dwBBitMask;
      uint32_t    dwAlphaBitMask;
    }             sPixelFormat;

    //  DDCAPS2
    struct {
      uint32_t    dwCaps1;
      uint32_t    dwCaps2;
      uint32_t    dwDDSX;
      uint32_t    dwReserved;
    }             sCaps;
    uint32_t      dwReserved2;
  };
  char data[ 128 ];
};

bool parse(QFile & file, DDS_header & header, ImageInfo & info)
{
  auto pos = file.pos();
  auto bytesRead = file.read(reinterpret_cast<char *>(&header), sizeof(DDS_header));
  file.seek(pos);
  if(bytesRead != sizeof(DDS_header)) return false;

  /// @todo seek to the end of the file, check that dwPitchOrLinearSize is correct

  bool dxt1 = PF_IS_DXT1(header.sPixelFormat);
  bool dxt3 = PF_IS_DXT3(header.sPixelFormat);
  bool dxt5 = PF_IS_DXT5(header.sPixelFormat);

  if(header.dwMagic == DDS_MAGIC && header.dwSize == 124 && (dxt1 || dxt3 || dxt5)) {
    info.width = header.dwWidth;
    info.height = header.dwHeight;
    /// @todo RGB or RGBA?
    if(dxt1) info.pf = PixelFormat(PixelFormat::COMPRESSED_RGB_DXT1);
    if(dxt3) info.pf = PixelFormat(PixelFormat::COMPRESSED_RGBA_DXT3);
    if(dxt5) info.pf = PixelFormat(PixelFormat::COMPRESSED_RGBA_DXT5);

    if(header.dwFlags & DDSD_MIPMAPCOUNT)
      info.mipmaps = header.dwMipMapCount;

    return true;
  }
  return false;
}

ImageCodecDDS::ImageCodecDDS()
{
}

bool ImageCodecDDS::canRead(QFile & file)
{
  DDS_header header;
  ImageInfo info;
  return parse(file, header, info);
}

QString ImageCodecDDS::extensions() const
{
  return "dds";
}

QString ImageCodecDDS::name() const
{
  return "dds";
}

bool ImageCodecDDS::ping(ImageInfo & info, QFile & file)
{
  DDS_header header;
  return parse(file, header, info);
}

bool ImageCodecDDS::read(Image &, QFile &)
{
  return false;
}

bool ImageCodecDDS::read(CompressedImage & image, QFile & file, int level)
{
  DDS_header header;
  ImageInfo info;
  if(!parse(file, header, info)) return false;

  if(level >= info.mipmaps) {
    Radiant::error("ImageCodecDDS::read # DDS file have %d mipmaps, tried to read mipmap level #%d", info.mipmaps, level);
    return false;
  }

  int size = linearSize(Nimble::Size(info.width, info.height),
                        info.pf.compression());

  if((header.dwFlags & DDSD_LINEARSIZE) && size != int(header.dwPitchOrLinearSize)) {
    Radiant::error("Invalid DDS file, level 0 calculated size %d doesn't match reported size %d",
                   size, header.dwPitchOrLinearSize);
    return false;
  }

  int offset = sizeof(header);

  if(level == 0) {
    file.seek(offset);
    return image.loadImage(file, info, size);
  }

  for(int l = 0; l <= level; ++l) {
    size = linearSize(Nimble::Size(info.width, info.height),
                      info.pf.compression());
    if(l == level) break;

    offset += size;
    info.width = std::max(1, info.width >> 1);
    info.height = std::max(1, info.height >> 1);
  }

  file.seek(offset);
  return image.loadImage(file, info, size);
}

bool ImageCodecDDS::writeMipmaps(const QString & filename, PixelFormat::Compression format,
                                 Nimble::Size size, int mipmaps,
                                 const std::vector<unsigned char> & dxt)
{
  DDS_header header;
  memset(&header, 0, sizeof(header));

  // see "Programming Guide for DDS" in MSDN
  header.dwMagic = DDS_MAGIC;
  header.dwSize = 124; // <- doesn't include magic
  header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT |
      DDSD_MIPMAPCOUNT | DDSD_LINEARSIZE;
  header.dwWidth = size.width();
  header.dwHeight = size.height();
  header.dwMipMapCount = mipmaps;
  header.dwPitchOrLinearSize = linearSize(size, format);

  header.sCaps.dwCaps1 = DDSCAPS_COMPLEX | DDSCAPS_MIPMAP | DDSCAPS_TEXTURE;

  header.sPixelFormat.dwSize = sizeof(header.sPixelFormat);
  header.sPixelFormat.dwFlags = DDPF_FOURCC;
  header.sPixelFormat.dwFourCC =
      format == PixelFormat::COMPRESSED_RGB_DXT1  ? D3DFMT_DXT1 :
      format == PixelFormat::COMPRESSED_RGBA_DXT1 ? D3DFMT_DXT1 :
      format == PixelFormat::COMPRESSED_RGBA_DXT3 ? D3DFMT_DXT3 :
      format == PixelFormat::COMPRESSED_RGBA_DXT5 ? D3DFMT_DXT5 : 0;

  if(header.sPixelFormat.dwFourCC == 0) {
    Radiant::error("ImageCodecDDS::writeMipmaps # Invalid format");
    return false;
  }

  Radiant::Directory::mkdirRecursive(Radiant::FileUtils::path(filename));
  QFile file(filename);
  if(file.open(QFile::WriteOnly)) {
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(&dxt[0]), dxt.size());
    return true;
  } else {
    Radiant::error("ImageCodecDDS::writeMipmaps # Failed to open target file %s", filename.toUtf8().data());
    return false;
  }
}

bool ImageCodecDDS::write(const Image &, QSaveFile &)
{
  return false;
}

Nimble::Size ImageCodecDDS::bufferSize(Nimble::Size size)
{
  // DXT is compressed in 4x4 blocks, this will round the size up to the
  // nearest size diviable by 4
  if(size.width() == 0 || (size.width() & 3))
    size.setWidth((size.width() & ~3) + 4);
  if(size.height() == 0 || (size.height() & 3))
    size.setHeight((size.height() & ~3) + 4);
  return size;
}

int ImageCodecDDS::linearSize(Nimble::Size size, PixelFormat::Compression format)
{
  bool dxt1 = format == PixelFormat::COMPRESSED_RGB_DXT1 ||
      format == PixelFormat::COMPRESSED_RGBA_DXT1;
  int channels = dxt1 ? 3 : 4;
  int factor = dxt1 ? 6 : 4;
  size = bufferSize(size);
  return size.width() * size.height() * channels / factor;
}
}
