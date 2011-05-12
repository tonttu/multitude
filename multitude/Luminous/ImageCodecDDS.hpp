#ifndef IMAGECODECDDS_HPP
#define IMAGECODECDDS_HPP

#include "ImageCodec.hpp"

namespace Luminous {

class ImageCodecDDS : public ImageCodec
{
public:
  ImageCodecDDS();
  bool canRead(FILE * file);
  QString extensions() const;
  QString name() const;
  bool ping(ImageInfo & info, FILE * file);
  bool read(Image & image, FILE * file);
  bool write(const Image & image, FILE * file);
  bool read(CompressedImage & image, FILE * file, int level = 0);
};

}

#endif // IMAGECODECDDS_HPP
