/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "RGBCube.hpp"

#include <Luminous/PixelFormat.hpp>
#include <Luminous/ColorCorrection.hpp>

#include <Valuable/AttributeContainer.hpp>
#include <Valuable/Node.hpp>

namespace Luminous
{
  class RGBCube::D
  {
  public:
    D(RGBCube & cube)
      : m_rgbs(&cube, "rgb-table")
      , m_division(&cube, "division", 0)
      , m_dimension(&cube, "dimension", 32)
      , m_cube(cube)
    {
      m_texture.setWrap(Texture::WRAP_CLAMP, Texture::WRAP_CLAMP, Texture::WRAP_CLAMP);
      m_generation = m_texture.generation();

      // Invalidate cached texture if either of these are edited
      m_division.addListener([this] { invalidate(); });
      m_dimension.addListener([this] { invalidate(); });
    }

    void setIndex(size_t index, Nimble::Vector3 rgb);
    Nimble::Vector3 getIndex(size_t index) const;

    void setRGB(int rindex, int gindex, int bindex, Nimble::Vector3 rgb);
    Nimble::Vector3 getRGB(int rindex, int gindex, int bindex) const;

    void setError(size_t index, float error);

    size_t rgbCount() const;

    /// The index is relative (0-1)
    Nimble::Vector3 interpolateRGB(Nimble::Vector3 relindex) const;
    void upSample(RGBCube & dest) const;

    void invalidate();

    void fill3DTexture(uint8_t * rgbvals, int npixels) const;
    const Luminous::Texture & asTexture() const;

  public:
    // The fourth vector element is the error, if relevant
    typedef Valuable::AttributeContainer<std::vector<Nimble::Vector4> > Rgbs;
    Rgbs m_rgbs;

    Valuable::AttributeInt m_division;
    Valuable::AttributeInt m_dimension;

  private:
    int m_generation;

    mutable Luminous::Texture m_texture;
    mutable std::vector<uint8_t> m_textureData;

    RGBCube & m_cube;
  };

  void RGBCube::D::setIndex(size_t index, Nimble::Vector3 rgb)
  {
    m_rgbs->at(index).make(rgb.x, rgb.y, rgb.z, -1);
    invalidate();
  }

  Nimble::Vector3 RGBCube::D::getIndex(size_t index) const
  {
    return m_rgbs->at(index).vector3();
  }

  void RGBCube::D::setRGB(int rindex, int gindex, int bindex, Nimble::Vector3 rgb)
  {
    int index = rindex + gindex * m_division + bindex * m_division * m_division;
    setIndex(index, rgb);
  }

  Nimble::Vector3 RGBCube::D::getRGB(int rindex, int gindex, int bindex) const
  {
    int index = rindex + gindex * m_division + bindex * m_division * m_division;
    return getIndex(index);
  }

  void RGBCube::D::setError(size_t index, float error)
  {
    m_rgbs->at(index).w = error;
    invalidate();
  }

  size_t RGBCube::D::rgbCount() const
  {
    return m_rgbs->size();
  }

  Nimble::Vector3 RGBCube::D::interpolateRGB(Nimble::Vector3 relindex) const
  {
    Nimble::Vector3f realindex = relindex * (float) (m_division - 1);
    Nimble::Vector3i realbase(realindex.x, realindex.y, realindex.z); // round down
    Nimble::Vector3i realnext;
    Nimble::Vector3f baseweight, nextweight;

    // Calculate weights and indices for trilinear interpolation
    for(int i = 0; i < 3; i++) {
      if(realbase[i] >= m_division - 1) {
        realbase[i] = m_division - 1;
        realnext[i] = m_division - 1;
        baseweight[i] = 1.0f;
        nextweight[i] = 0.0f;
      }
      else if(realbase[i] < 0 || realindex[i] < 0.0f) {
        realbase[i] = 0;
        realnext[i] = 0;
        baseweight[i] = 1.0f;
        nextweight[i] = 0.0f;
      }
      else {
        realnext[i] = realbase[i] + 1;
        nextweight[i] = realindex[i] - (float) realbase[i];
        baseweight[i] = 1.0f - nextweight[i];
      }
    }

    Nimble::Vector3 lll = getRGB(realbase.x, realbase.y, realbase.z);
    Nimble::Vector3 hll = getRGB(realnext.x, realbase.y, realbase.z);
    Nimble::Vector3 lhl = getRGB(realbase.x, realnext.y, realbase.z);
    Nimble::Vector3 hhl = getRGB(realnext.x, realnext.y, realbase.z);

    Nimble::Vector3 llh = getRGB(realbase.x, realbase.y, realnext.z);
    Nimble::Vector3 hlh = getRGB(realnext.x, realbase.y, realnext.z);
    Nimble::Vector3 lhh = getRGB(realbase.x, realnext.y, realnext.z);
    Nimble::Vector3 hhh = getRGB(realnext.x, realnext.y, realnext.z);

    float wlll = baseweight.x * baseweight.y * baseweight.z;
    float whll = nextweight.x * baseweight.y * baseweight.z;
    float wlhl = baseweight.x * nextweight.y * baseweight.z;
    float whhl = nextweight.x * nextweight.y * baseweight.z;

    float wllh = baseweight.x * baseweight.y * nextweight.z;
    float whlh = nextweight.x * baseweight.y * nextweight.z;
    float wlhh = baseweight.x * nextweight.y * nextweight.z;
    float whhh = nextweight.x * nextweight.y * nextweight.z;

    float wtotal = wlll + whll + wlhl + whhl +
        wllh + whlh + wlhh + whhh;

    Nimble::Vector3 r =  (lll * wlll + hll * whll + lhl * wlhl + hhl * whhl +
                          llh * wllh + hlh * whlh + lhh * wlhh + hhh * whhh) / wtotal;

    /* if(relindex.x > 0.99f && relindex.y > 0.99f && relindex.x > 0.99f)
      Radiant::info("RGBCube::interpolateRGB # FINAL %f %f %f TO %f %f %f",
                    hhh.x, hhh.y, hhh.z, r.x, r.y, r.z);
                    */
    return r;
  }

  /// @todo does this actually work?
  void RGBCube::D::upSample(RGBCube & dest) const
  {
    int updiv = m_division * 2 - 1;
    // int upend = updiv - 1;
    dest.setDivision(updiv);
    dest.setAll(Nimble::Vector3(-1, -1, -1));

    // Fill dest with interpolated values
    float step = 1.0f / (updiv - 1);

    for(int bi = 0; bi < updiv; bi++) {
      for(int gi = 0; gi < updiv; gi++) {
        for(int ri = 0; ri < updiv; ri++) {
          Nimble::Vector3 rgb = interpolateRGB( Nimble::Vector3(ri, gi, bi) * step);
          dest.setRGB(ri, gi, bi, rgb);
        }
      }
    }

    // Then set the diagonal values

    int mybase = m_division * m_division * m_division;
    for(int i = 0; i < m_division - 1; i++) {
      int di = i * 2 + 1;
      Nimble::Vector3 rgb = getIndex(mybase + i);
      dest.setRGB(di, di, di, rgb);
    }

    // Then expand around the diagonal values
    /*
  for(int i = 1; i < upend; i++) {

  }
  */

  }

  void RGBCube::D::invalidate()
  {
    m_generation++;
  }

  void RGBCube::D::fill3DTexture(uint8_t * rgbvals, int npixels) const
  {
    debugLuminous("RGBCube::fill3DTexture # %d %d %f",
                  (int) m_division, (int) m_rgbs->size(), m_rgbs->at(7).x);

    float inv = 1.0f / (npixels - 1.0f);

    for(int b = 0; b < npixels; b++) {
      for(int g = 0; g < npixels; g++) {
        for(int r = 0; r < npixels; r++) {

          Nimble::Vector3 rgb = interpolateRGB(Nimble::Vector3(r, g, b) * inv);
          rgb *= 255.5f;

          rgbvals[0] = Nimble::Math::Clamp((int) (rgb[0] + 0.5f), 0, 255);
          rgbvals[1] = Nimble::Math::Clamp((int) (rgb[1] + 0.5f), 0, 255);
          rgbvals[2] = Nimble::Math::Clamp((int) (rgb[2] + 0.5f), 0, 255);

          rgbvals += 3;

        }
      }
    }
  }

  const Luminous::Texture & RGBCube::D::asTexture() const
  {
    if(m_generation != m_texture.generation()) {

      debugLuminous("RGBCube # Updating texture");

      const RGBCube * cube = &m_cube;

      RGBCube tmp;
      if(m_rgbs->size() > size_t(m_division * m_division * m_division)) {
        debugLuminous("RGBCube::updateTexture # Upsampling");
        upSample(tmp);
        cube = &tmp;
      }

      m_textureData.resize(3 * m_dimension * m_dimension * m_dimension);
      cube->m_d->fill3DTexture(&m_textureData[0], m_dimension);

      m_texture.setData(m_dimension, m_dimension, m_dimension, Luminous::PixelFormat::rgbUByte(), &m_textureData[0]);
      m_texture.setGeneration(m_generation);
    }

    if(!m_texture.isValid()) {
      Radiant::warning("RGBCube # Texture is not valid! "
                       "The color correction configuration might be broken or missing.");
    }

    return m_texture;
  }

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

  RGBCube::RGBCube(Node * host, const QByteArray & name)
    : Node(host, name)
    , m_d(new D(*this))
  {
  }

  RGBCube::~RGBCube()
  {
    delete m_d;
  }

  bool RGBCube::deserialize(const Valuable::ArchiveElement & element)
  {
    /// @todo actually check if contents of rgb-table changes
    bool ok = Node::deserialize(element);
    if(ok)
      m_d->invalidate();
    return ok;
  }

  int RGBCube::division() const
  {
    return m_d->m_division;
  }

  void RGBCube::setDivision(int division)
  {
    m_d->m_rgbs->resize(division * division * division + division - 1);
    m_d->m_division = division;
    m_d->invalidate();
  }

  void RGBCube::createDefault(int division)
  {
    setDivision(division);

    float step = 1.0f / (division - 1);

    for(int b = 0; b < division; b++) {
      for(int g = 0; g < division; g++) {
        for(int r = 0; r < division; r++) {
          setRGB(r, g, b, Nimble::Vector3(r * step, g * step, b * step));
        }
      }
    }

    int base = division * division * division;

    for(int i = 0; i < (division-1); i++) {
      float lum = (i + 0.5f) * step;
      setIndex(base + i, Nimble::Vector3(lum, lum, lum));
    }
  }

  bool RGBCube::isDefined() const
  {
    return !m_d->m_rgbs->empty();
  }

  size_t RGBCube::rgbCount() const
  {
    return m_d->rgbCount();
  }

  Nimble::Vector3 RGBCube::white() const
  {
    if(!isDefined())
      return Nimble::Vector3(0,0,0);
    return getIndex(m_d->m_division * m_d->m_division * m_d->m_division - 1);
  }

  const Luminous::Texture & RGBCube::asTexture() const
  {
    return m_d->asTexture();
  }

  void RGBCube::setAll(Nimble::Vector3 rgb)
  {
    Nimble::Vector4 tmp(rgb.x, rgb.y, rgb.z, 0.0f);
    for(size_t i = 0; i < m_d->m_rgbs->size(); i++)
      m_d->m_rgbs->at(i) = tmp;
    m_d->invalidate();
  }

  void RGBCube::setIndex(size_t index, Nimble::Vector3 rgb)
  {
    m_d->setIndex(index, rgb);
  }

  Nimble::Vector3 RGBCube::getIndex(size_t index) const
  {
    return m_d->getIndex(index);
  }

  void RGBCube::setRGB(int rindex, int gindex, int bindex, Nimble::Vector3 rgb)
  {
    m_d->setRGB(rindex, gindex, bindex, rgb);
  }

  Nimble::Vector3 RGBCube::getRGB(int rindex, int gindex, int bindex) const
  {
    return m_d->getRGB(rindex, gindex, bindex);
  }

  void RGBCube::setError(size_t index, float error)
  {
    m_d->setError(index, error);
  }

  int RGBCube::findClosestRGBIndex(Nimble::Vector3 color) const
  {
    float error = 1000.0f;

    int index = -1;

    for(size_t i = 0; i < m_d->rgbCount(); i++) {
      float d = (color - m_d->getIndex(i)).length();
      if(error > d) {
        error = d;
        index = static_cast<int>(i);
      }
    }

    return index;
  }

  void RGBCube::fromColorSplines(const Luminous::ColorCorrection & cc)
  {
    const int division = 3;

    setDivision(division);

    const float step = 1.0f / (division - 1);

    for(int b = 0; b < division; b++) {
      for(int g = 0; g < division; g++) {
        for(int r = 0; r < division; r++) {
          setRGB(r, g, b, Nimble::Vector3(cc.value(r * step, 0, true, true),
                                          cc.value(g * step, 1, true, true),
                                          cc.value(b * step, 2, true, true)));
        }
      }
    }

    int base = division * division * division;

    for(int i = 0; i < (division-1); i++) {
      float lum = (i + 0.5f) * step;
      setIndex(base + i, cc.valueRGB(lum, true, true));
    }
  }
}
