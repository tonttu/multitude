#include "ImageCodecDDS.hpp"
#include "Image.hpp"

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
    unsigned int    dwMagic;
    unsigned int    dwSize;
    unsigned int    dwFlags;
    unsigned int    dwHeight;
    unsigned int    dwWidth;
    unsigned int    dwPitchOrLinearSize;
    unsigned int    dwDepth;
    unsigned int    dwMipMapCount;
    unsigned int    dwReserved1[ 11 ];

    //  DDPIXELFORMAT
    struct {
      unsigned int    dwSize;
      unsigned int    dwFlags;
      unsigned int    dwFourCC;
      unsigned int    dwRGBBitCount;
      unsigned int    dwRBitMask;
      unsigned int    dwGBitMask;
      unsigned int    dwBBitMask;
      unsigned int    dwAlphaBitMask;
    }               sPixelFormat;

    //  DDCAPS2
    struct {
      unsigned int    dwCaps1;
      unsigned int    dwCaps2;
      unsigned int    dwDDSX;
      unsigned int    dwReserved;
    }               sCaps;
    unsigned int    dwReserved2;
  };
  char data[ 128 ];
};

bool parse(FILE * file, DDS_header & header, ImageInfo & info)
{
  long pos = ftell(file);
  int ok = fread(&header, sizeof(DDS_header), 1, file);
  fseek(file, pos, SEEK_SET);
  if(!ok) return false;

  /// @todo seek to the end of the file, check that dwPitchOrLinearSize is correct

  bool dxt1 = PF_IS_DXT1(header.sPixelFormat);
  bool dxt3 = PF_IS_DXT3(header.sPixelFormat);
  bool dxt5 = PF_IS_DXT5(header.sPixelFormat);

  if(header.dwMagic == DDS_MAGIC && header.dwSize == 124 && (dxt1 || dxt3 || dxt5)) {
    info.width = header.dwWidth;
    info.height = header.dwHeight;
    /// @todo RGB or RGBA?
    if(dxt1) info.pf = PixelFormat(PixelFormat::COMPRESSED_RGBA_DXT1);
    if(dxt3) info.pf = PixelFormat(PixelFormat::COMPRESSED_RGBA_DXT3);
    if(dxt5) info.pf = PixelFormat(PixelFormat::COMPRESSED_RGBA_DXT5);
    return true;
  }
  return false;
}

ImageCodecDDS::ImageCodecDDS()
{
}

bool ImageCodecDDS::canRead(FILE * file)
{
  DDS_header header;
  ImageInfo info;
  return parse(file, header, info);
}

std::string ImageCodecDDS::extensions() const
{
  return "dds";
}

std::string ImageCodecDDS::name() const
{
  return "dds";
}

bool ImageCodecDDS::ping(ImageInfo & info, FILE * file)
{
  DDS_header header;
  return parse(file, header, info);
}

bool ImageCodecDDS::read(Image &, FILE *)
{
  return false;
}

bool ImageCodecDDS::read(CompressedImage & image, FILE * file)
{
  DDS_header header;
  ImageInfo info;
  if(!parse(file, header, info)) return false;

  return image.loadImage(file, info, sizeof(header), header.dwPitchOrLinearSize);
}


bool ImageCodecDDS::write(const Image &, FILE *)
{
  return false;
}

}
