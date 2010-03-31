#ifndef IMAGECODECSVG_HPP
#define IMAGECODECSVG_HPP

#include <Luminous/ImageCodec.hpp>

class QSvgRenderer;

namespace Luminous {

class ImageCodecSVG : public Luminous::ImageCodec
{
public:
  ImageCodecSVG();
  virtual ~ImageCodecSVG();
  virtual bool canRead(FILE * file);

  virtual std::string extensions() const;
  virtual std::string name() const;
  virtual bool ping(ImageInfo & info, FILE * file);
  virtual bool read(Image & image, FILE * file);
  /// not supported
  virtual bool write(const Image & image, FILE * file);
private:
  QSvgRenderer * updateSVG(FILE * file);
};

} // namespace Luminous

#endif // IMAGECODECSVG_HPP
