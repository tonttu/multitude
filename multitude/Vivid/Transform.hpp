#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <Nimble/Matrix4.hpp>

namespace Vivid
{

class Transform
{
public:
  Transform();

  void setTransform(const Nimble::Matrix4 & t) { m_transform = t; }

  Nimble::Matrix4 & transform() { return m_transform; }
  const Nimble::Matrix4 & transform() const { return m_transform; }

private:
  Nimble::Matrix4 m_transform;
};

}

#endif // TRANSFORM_HPP
