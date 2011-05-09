#include "Camera.hpp"

#include <Luminous/Utils.hpp>

#include <Radiant/Trace.hpp>

namespace Vivid
{

Camera::Camera()
  : Transform(),
    m_nearPlane(0.1f),
    m_farPlane(1000.f),
    m_fovY(45.f),
    m_projectionMatrixDirty(true)
{
  m_viewport[0] = 0;
  m_viewport[1] = 0;
  m_viewport[2] = 100;
  m_viewport[3] = 100;
}

void Camera::recomputeProjectionMatrix()
{
  const float aspect = static_cast<float> (m_viewport[2]) / static_cast<float> (m_viewport[3]);

  m_projectionMatrix = Nimble::Matrix4::perspectiveProjection(m_fovY, aspect, m_nearPlane, m_farPlane);

  m_projectionMatrixDirty = false;
}

Nimble::Matrix4 & Camera::projectionMatrix()
{
  if(m_projectionMatrixDirty)
    recomputeProjectionMatrix();

  return m_projectionMatrix;
}

Nimble::Vector3 Camera::unproject(const Nimble::Vector3 &viewportCoord)
{
  bool ok = true;
  Nimble::Matrix4 tm = transform().inverse(&ok);

  if (!ok) {
    Radiant::error("Camera::unproject # failed to invert transform");
    return Nimble::Vector3(0.f, 0.f, 0.f);
  }

  Nimble::Matrix4 m = projectionMatrix() * tm;

  ok = true;
  m = m.inverse(&ok);

  if(!ok) {
    Radiant::error("Camera::unproject # failed to invert projection * view");
    return Nimble::Vector3(0.f, 0.f, 0.f);
  }

  Nimble::Vector4 p(2 * ((viewportCoord.x - m_viewport[0]) / m_viewport[2]) - 1,
                    2 * ((m_viewport[3] - viewportCoord.y - m_viewport[1]) / m_viewport[3]) - 1,
                    2 * viewportCoord.z - 1,
                    1.f);

  p = m * p;


  assert(p.w != 0.f);

  return (1.0f/p.w) * Nimble::Vector3(p.x, p.y, p.z);
}

void Camera::generateRay(float x, float y, Nimble::Vector3 &rayOrigin, Nimble::Vector3 &rayDir)
{
  Nimble::Vector3 nearPoint = unproject(Nimble::Vector3(x, y, 0.f));
  Nimble::Vector3 farPoint = unproject(Nimble::Vector3(x, y, 1.f));

  //Radiant::info("Camera::generateRay # near (%f,%f,%f) far (%f,%f,%f)", nearPoint.x, nearPoint.y, nearPoint.z, farPoint.x, farPoint.y, farPoint.z);

  rayDir = farPoint - nearPoint;
  rayDir.normalize();

  rayOrigin = transform().getTranslation();
}

}
