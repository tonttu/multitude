#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "Transform.hpp"

namespace Vivid
{

/// A pespective camera
class Camera : public Transform
{
public:
    Camera();

    void setClipPlanes(float nearPlane, float farPlane) { m_projectionMatrixDirty = true; m_nearPlane = nearPlane; m_farPlane = farPlane; }
    /// Sets the field-of-view in Y direction
    /// @param fovY field-of-view in degrees
    void setFieldOfView(float fovY) { m_projectionMatrixDirty = true; m_fovY = fovY; }

    void setViewport(int x, int y, int w, int h) { m_projectionMatrixDirty = true; m_viewport[0] = x; m_viewport[1] = y; m_viewport[2] = w; m_viewport[3] = h; }

    float nearPlane() const { return m_nearPlane; }
    float farPlane() const { return m_farPlane; }
    /// Returns the field-of-view in Y direction in degrees
    float fov() const { return m_fovY; }
    float aspect() const;

    /// Generate a ray from camera location towards the direction the camera
    /// looking at through the point (x,y) on the near plane
    void generateRay(float x, float y, Nimble::Vector3 & rayOrigin, Nimble::Vector3 & rayDir);

    /// Returns the projection matrix associated with the camera
    Nimble::Matrix4 & projectionMatrix();

    /// Map viewport coordinates to object coordinates
    /// @param viewportCoord viewport coordinate to unproject
    /// @returns object space coordinates
    Nimble::Vector3 unproject(const Nimble::Vector3 & viewportCoord);

private:
    void recomputeProjectionMatrix();

    /// Distance to near plane
    float m_nearPlane;
    /// Distance to far plane
    float m_farPlane;
    /// Field-of-view in Y direction in degrees
    float m_fovY;
    /// Viewport (x, y, width, height)
    int m_viewport[4];

    Nimble::Matrix4 m_projectionMatrix;

    // Dirty flag used for lazy projection matrix evaluation
    bool m_projectionMatrixDirty;
};

}

#endif // CAMERA_HPP
