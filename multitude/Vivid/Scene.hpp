#ifndef SCENE_HPP
#define SCENE_HPP

#include <fbxsdk.h>

#include <string>

namespace Vivid
{

class Scene
{
public:
    Scene(KFbxSdkManager * sdk);
    ~Scene();

    bool import(KFbxSdkManager * sdk, const std::string & filename);

    void setTime(KTime & time) { m_time = time; }

    KFbxCamera * getCurrentCamera();

    /// Setup OpenGL projection&transform to match given camera
    void setCameraTransform(KFbxCamera *camera);

    void draw();

private:
    /// Get the global transformation matrix of a node
    KFbxXMatrix getGlobalPosition(KFbxNode * node);
    KFbxXMatrix getGeometryDeformation(KFbxNode * node);

    /// Is the given camera a built-in camera (e.g. front,side,top,perspective)
    bool isProducerCamera(KFbxCamera * camera) const;

    /// Draw the given node and its children recursively
    void drawRecursive(KFbxNode * node, KFbxXMatrix & parentGlobalPosition);

    void drawNode(KFbxNode * node, KFbxXMatrix & parentGlobalPosition);
    void drawMesh(KFbxNode * node, KFbxXMatrix & parentGlobalPosition);

    void readVertexCacheData(KFbxMesh * mesh, KFbxVector4 * vertexArray);

    /// Scene that is wrapped
    KFbxScene * m_scene;
    /// Current time the scene is evaluated at (for animations, etc)
    KTime m_time;
};

}

#endif // SCENE_HPP
