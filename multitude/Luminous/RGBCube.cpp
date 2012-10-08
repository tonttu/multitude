#include "RGBCube.hpp"

#include <Luminous/PixelFormat.hpp>

#include <Valuable/Node.hpp>

namespace Luminous
{
  RGBCube::RGBCube(Node * host, const QByteArray & name)
    : Node(host, name),
      m_division(this, "division", 0),
      m_rgbs(this, "rgb-table")
  {
    m_division.addListener(std::bind(&RGBCube::updateTexture, this));
    m_rgbs.addListener(std::bind(&RGBCube::updateTexture, this));
  }

  RGBCube::~RGBCube()
  {}

  Nimble::Vector3 RGBCube::getRGB(int rindex, int gindex, int bindex) const
  {
    int index = rindex + gindex * m_division + bindex * m_division * m_division;
    return getIndex(index);
  }

  void RGBCube::setRGB(int rindex, int gindex, int bindex, Nimble::Vector3 rgb)
  {
    int index = rindex + gindex * m_division + bindex * m_division * m_division;
    setIndex(index, rgb);
  }

  Nimble::Vector3 RGBCube::interpolateRGB(Nimble::Vector3 relindex) const
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

  void RGBCube::fill3DTexture(uint8_t * rgbvals, int npixels) const
  {
    Radiant::info("RGBCube::fill3DTexture # %d %d %f",
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

          // if(r == npixels - 1) {
          /*
        if(r == g && r == b && (r % 4) == 0) {
          printf("of %d [%.2d %.2d %.2d]=%.3d %.3d %.3d\n", npixels, r, g, b,
                 (int) rgbvals[0], (int) rgbvals[1], (int) rgbvals[2]);
          fflush(0);
        }
        */
          rgbvals += 3;

        }
      }
    }
  }

  void RGBCube::updateTexture()
  {
    const int dim = 32;

    std::vector<uint8_t> texrgb;
    texrgb.resize(3 * dim * dim * dim);

    fill3DTexture(& texrgb[0], dim);

    m_texture.setData(dim, dim, dim, Luminous::PixelFormat::rgbUByte(), &texrgb[0]);
  }

  const Luminous::Texture & RGBCube::asTexture() const
  {
    Radiant::info("is defined: %d", isDefined());
    return m_texture;
  }

  /*Texture3D * RGBCube::bind(GLResources * r, int texunit) const
{
  if(!m_division) {
    return 0;
  }

  GLRESOURCE_ENSURE(Texture3D, tex, & m_key, r);

  tex->bind(texunit);

  const int dim = 32;

  if(tex->generation() != m_generation || tex->width() != dim) {

    // Radiant::info("RGBCube::bind # Loading 3D color correction texture for %p", this);

    const RGBCube * cube = this;

    RGBCube tmp;
    if(m_rgbs->size() > m_division * m_division * m_division) {
      Radiant::info("RGBCube::bind # Upsampling");
      upSample(tmp);
      cube = & tmp;
    }


    std::vector<uint8_t> texrgb;
    texrgb.resize(3 * dim * dim * dim);
    cube->fill3DTexture(& texrgb[0], dim);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, dim, dim, dim, 0, GL_RGB, GL_UNSIGNED_BYTE, & texrgb[0]);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    tex->setGeneration(m_generation);
    tex->setWidth(dim);
    tex->setHeight(dim);
    tex->setDepth(dim);
    Luminous::Utils::glCheck("RGBCube::bind");
    // Radiant::info("RGBCube::bind # Loading 3D color correction texture for %p LOADED", this);
  }

  return tex;
}*/

  int RGBCube::findClosestRGBIndex(Nimble::Vector3 color)
  {
    float error = 1000.0f;

    int index = -1;

    for(size_t i = 0; i < m_rgbs->size(); i++) {
      float d = (color - m_rgbs->at(i).vector3()).length();
      if(error > d) {
        error = d;
        index = i;
      }
    }

    return index;
  }

  void RGBCube::upSample(RGBCube & dest) const
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

}
