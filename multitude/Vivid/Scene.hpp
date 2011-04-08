#ifndef SCENE_HPP
#define SCENE_HPP

#include "Mesh.hpp"

#include <Nimble/Vector3.hpp>

#include <fbxsdk.h>

#include <string>
#include <vector>
#include <map>

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

    Mesh* findMesh(const std::string& name);

private:
    /// Get the global transformation matrix of a node
    KFbxXMatrix getGlobalPosition(KFbxNode * node, KFbxXMatrix* pParentGlobalPosition=0);
    KFbxXMatrix getGlobalPosition(KFbxNode * node, KFbxPose * pose, KFbxXMatrix* pParentGlobalPosition=0);
    KFbxXMatrix getPoseMatrix(KFbxPose * pPose, int pNodeIndex);
    KFbxXMatrix getGeometryDeformation(KFbxNode * node);

    /// Is the given camera a built-in camera (e.g. front,side,top,perspective)
    bool isProducerCamera(KFbxCamera * camera) const;

    /// Draw the given node and its children recursively
    void drawRecursive(KFbxNode * node, KFbxXMatrix & parentGlobalPosition);

    void drawNode(KFbxNode * node, KFbxXMatrix & parentGlobalPosition, KFbxXMatrix & globalOffsetPosition, KFbxPose * pose=0);
    Mesh* buildMesh(KFbxNode * node, KFbxXMatrix & globalPosition, KFbxPose * pose);
    void drawSkeleton(KFbxNode * pNode, KFbxXMatrix & pParentGlobalPosition, KFbxXMatrix & pGlobalPosition);

    void readVertexCacheData(KFbxMesh * mesh, KFbxVector4 * vertexArray);

    void ComputeShapeDeformation(KFbxNode* node,
                                 KFbxMesh* mesh,
                                 KFbxVector4* vertexArray);

    void ComputeClusterDeformation(KFbxXMatrix& globalPosition,
                                   KFbxMesh* mesh,
                                   KFbxVector4* vertexArray,
                                   KFbxPose* pose);
    /// Scene that is wrapped
    KFbxScene * m_scene;
    /// Current time the scene is evaluated at (for animations, etc)
    KTime m_time;
    KFbxAnimLayer* m_currentLayer;
    KFbxSdkManager * m_manager;
};

}

#endif // SCENE_HPP
