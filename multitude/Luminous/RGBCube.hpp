#ifndef RGBCUBE_HPP
#define RGBCUBE_HPP

#include <Luminous/Collectable.hpp>
#include <Luminous/Texture2.hpp>

#include <Nimble/Vector3.hpp>

#include <Valuable/AttributeContainer.hpp>
#include <Valuable/Node.hpp>

namespace Luminous
{
  class RGBCube : public Valuable::Node
  {
  public:
    RGBCube(Node * host = 0, const QByteArray & name = "rgbcube");
    virtual ~RGBCube();

    Nimble::Vector3 getIndex(size_t index) const { return m_rgbs->at(index).vector3(); }

    Nimble::Vector3 getRGB(int rindex, int gindex, int bindex) const;

    void setIndex(size_t index, Nimble::Vector3 rgb)
    { m_rgbs->at(index).make(rgb.x, rgb.y, rgb.z, -1); m_generation++; }
    void setError(size_t index, float error)
    { m_rgbs->at(index).w = error; }
    void setRGB(int rindex, int gindex, int bindex, Nimble::Vector3 rgb);

    /// The index is relative (0-1)
    Nimble::Vector3 interpolateRGB(Nimble::Vector3 index) const;


    int division() const { return m_division; }
    void setDivision(int division)
    {
      m_division = division;
      m_rgbs->resize(division * division * division + division - 1);
    }

    void setAll(Nimble::Vector3 rgb)
    {
      Nimble::Vector4 tmp(rgb.x, rgb.y, rgb.z, 0.0f);
      for(size_t i = 0; i < m_rgbs->size(); i++)
        m_rgbs->at(i) = tmp;
    }

    void createDefault(int division);

    void fill3DTexture(uint8_t * rgbvals, int npixels) const;
    void updateTexture();

    const Luminous::Texture & asTexture() const;

    bool isDefined() const { return !m_rgbs->empty(); }

    Nimble::Vector3 white() const
    {
      if(!isDefined())
        return Nimble::Vector3(0,0,0);
      return m_rgbs->at(m_division * m_division * m_division - 1).vector3();
    }

    int findClosestRGBIndex(Nimble::Vector3 color);

    void upSample(RGBCube & dest) const;

    size_t rgbCount() const { return m_rgbs->size(); }

  private:

    // The fourth vector element is the error, if relevant
    typedef Valuable::AttributeContainer<std::vector<Nimble::Vector4> > Rgbs;
    Valuable::AttributeInt m_division;
    Rgbs m_rgbs;
    size_t m_generation;
    Collectable m_key;
    Luminous::Texture m_texture;
  };
}
#endif // RGBCUBE_HPP