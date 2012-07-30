#include "ImageTex2.hpp"

#include "Luminous/Image.hpp"
#include "Luminous/Texture2.hpp"

namespace Luminous
{
  class ImageTex2::D
  {
  public:
    typedef std::pair<Luminous::Image, Luminous::Texture> ImgTex;
    std::shared_ptr<ImgTex> loadShared(const QString & filename);

  public:
    std::shared_ptr<ImgTex> m_image;
  };

  std::shared_ptr<ImageTex2::D::ImgTex> ImageTex2::D::loadShared(const QString & filename)
  {
    // C++11 says: [stmt.dcl 4]
    // If control enters the declaration concurrently while the variable is
    // being initialized, the concurrent execution shall wait for completion
    // of the initialization
    static Radiant::Mutex s_mutex;
    static std::map<QString, std::weak_ptr<ImgTex>> s_images;

    Radiant::Guard g(s_mutex);

    std::weak_ptr<ImgTex> & weak = s_images[filename];
    std::shared_ptr<ImgTex> image = weak.lock();
    if(image)
      return image;

    image = std::make_shared<ImgTex>();
    image->first.read(filename.toUtf8().data());
    image->second.setData(image->first.width(), image->first.height(),
                          image->first.pixelFormat(), image->first.data());
    weak = image;
    return image;
  }

  /////////////////////////////////////////////////////////////////////////////

  ImageTex2::ImageTex2()
    : m_d(new D())
  {}

  ImageTex2::~ImageTex2()
  {
    delete m_d;
  }

  bool ImageTex2::load(const QString & filename)
  {
    std::shared_ptr<ImageTex2::D::ImgTex> image = m_d->loadShared(filename);
    if(!image) return false;

    m_d->m_image = std::move(image);
    return true;
  }

  Texture & ImageTex2::tex()
  {
    return m_d->m_image->second;
  }
}
