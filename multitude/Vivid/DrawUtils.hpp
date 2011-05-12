#ifndef DRAWUTILS_HPP
#define DRAWUTILS_HPP

#include <fbxsdk.h>

namespace Vivid
{

  namespace DrawUtils
  {

    /// Draw a grid (debug)
    void drawGrid(KFbxXMatrix & transform);

    /// Setup perspective OpenGL view projection
    void setupPerspective(double fovY, double aspect, double nearPlane, double farPlane, KFbxVector4 & eye, KFbxVector4 & center, KFbxVector4 & up);

    /// Setup orthogonal OpenGL view projection
    void setupOrthogonal(double leftPlane, double rightPlane, double bottomPlane, double topPlane, double nearPlane, double farPlane, KFbxVector4 & eye, KFbxVector4 & center, KFbxVector4 & up);

  }

}

#endif // DRAWUTILS_HPP
