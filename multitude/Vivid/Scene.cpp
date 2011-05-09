#include "Scene.hpp"
#include "DrawUtils.hpp"
#include "TextureManager.hpp"

#include <Luminous/Luminous.hpp>
#include <Luminous/Texture.hpp>
#include <Luminous/Image.hpp>

#include <fbxsdk.h>

#include <cmath>
#include <vector>

namespace Vivid
{

static double g_orthoCameraScale = 178.0;


#include <fbxfilesdk/kfbxplugins/kfbxtexture.h>


//////////
//////////

Scene::Scene(KFbxSdkManager * sdk)
  : m_time(0), m_currentLayer(0)
{
  m_manager = sdk;
  m_scene = KFbxScene::Create(sdk, "");
}

Scene::~Scene()
{
  m_scene->Destroy();
}

bool Scene::import(const std::string &filename)
{
  KFbxImporter * importer = KFbxImporter::Create(m_manager, "");

  bool status = importer->Initialize(filename.c_str(), -1, m_manager->GetIOSettings());
  if(!status) {
    Radiant::error("Scene::import # failed to initialize import for '%s' : %s", filename.c_str(), importer->GetLastErrorString());
    importer->Destroy();
    return false;
  }

  status = importer->Import(m_scene);

  if(!status) {
    Radiant::error("Vivid::import # failed to import scene '%s' : %s", filename.c_str(), importer->GetLastErrorString());
    importer->Destroy();
    return false;
  }

  importer->Destroy();

  return true;
}

KFbxCamera * Scene::getCurrentCamera()
{
  KFbxGlobalCameraSettings & cameraSettings = m_scene->GlobalCameraSettings();

  KString currentCameraName = cameraSettings.GetDefaultCamera();

  KFbxCamera * ret = 0;

  if (currentCameraName == PRODUCER_PERSPECTIVE)
    ret = cameraSettings.GetCameraProducerPerspective();
  else if (currentCameraName == PRODUCER_TOP)
    ret = cameraSettings.GetCameraProducerTop();
  else if (currentCameraName == PRODUCER_BOTTOM)
    ret = cameraSettings.GetCameraProducerBottom();
  else if (currentCameraName == PRODUCER_FRONT)
    ret = cameraSettings.GetCameraProducerFront();
  else if (currentCameraName == PRODUCER_BACK)
    ret = cameraSettings.GetCameraProducerBack();
  else if (currentCameraName == PRODUCER_RIGHT)
    ret = cameraSettings.GetCameraProducerRight();
  else if (currentCameraName == PRODUCER_LEFT)
    ret = cameraSettings.GetCameraProducerLeft();

  return ret;
}

KFbxXMatrix Scene::getGlobalPosition(KFbxNode *node, KFbxXMatrix* pParentGlobalPosition)
{
  return node->GetScene()->GetEvaluator()->GetNodeGlobalTransform(node, m_time);
}

KFbxXMatrix Scene::getGlobalPosition(KFbxNode * pNode, KFbxPose * pPose, KFbxXMatrix* pParentGlobalPosition)
{
  KFbxXMatrix lGlobalPosition;
  bool        lPositionFound = false;

  if (pPose)
  {
      int lNodeIndex = pPose->Find(pNode);

      if (lNodeIndex > -1)
      {
          // The bind pose is always a global matrix.
          // If we have a rest pose, we need to check if it is
          // stored in global or local space.
          if (pPose->IsBindPose() || !pPose->IsLocalMatrix(lNodeIndex))
          {
              lGlobalPosition = getPoseMatrix(pPose, lNodeIndex);
          }
          else
          {
              // We have a local matrix, we need to convert it to
              // a global space matrix.
              KFbxXMatrix lParentGlobalPosition;

              if (pParentGlobalPosition)
              {
                  lParentGlobalPosition = *pParentGlobalPosition;
              }
              else
              {
                  if (pNode->GetParent())
                  {
                      lParentGlobalPosition = getGlobalPosition(pNode->GetParent(), pPose);
                  }
              }

              KFbxXMatrix lLocalPosition = getPoseMatrix(pPose, lNodeIndex);
              lGlobalPosition = lParentGlobalPosition * lLocalPosition;
          }

          lPositionFound = true;
      }
  }

  if (!lPositionFound)
  {
      // There is no pose entry for that node, get the current global position instead
      lGlobalPosition = getGlobalPosition(pNode, pParentGlobalPosition);
  }

  return lGlobalPosition;
}

KFbxXMatrix Scene::getPoseMatrix(KFbxPose * pPose, int pNodeIndex)
{
  KFbxXMatrix lPoseMatrix;
  KFbxMatrix lMatrix = pPose->GetMatrix(pNodeIndex);

  memcpy((double*)lPoseMatrix, (double*)lMatrix, sizeof(lMatrix.mData));

  return lPoseMatrix;
}

void Scene::setCameraTransform(KFbxCamera *camera)
{
  KFbxNode * cameraNode = camera ? camera->GetNode() : 0;

  // Compute the camera position and direction.
  KFbxVector4 eye(0,0,1);
  KFbxVector4 center(0,0,0);
  KFbxVector4 up(0,1,0);
  KFbxVector4 forward, right;

  if(camera) {
    eye = camera->Position.Get();
    up = camera->UpVector.Get();
  }

  if(cameraNode && cameraNode->GetTarget()) {
    center = getGlobalPosition(cameraNode->GetTarget()).GetT();
  } else {

    if(!cameraNode || isProducerCamera(camera)) {

      if(camera)
        center = camera->InterestPosition.Get();

    } else {

      // Get direction
      KFbxXMatrix globalRotation;
      KFbxVector4 rotationVector(getGlobalPosition(cameraNode).GetR());
      globalRotation.SetR(rotationVector);

      // Get length
      KFbxVector4 interestPosition(camera->InterestPosition.Get());
      KFbxVector4 cameraGlobalPosition(getGlobalPosition(cameraNode).GetT());
      double len = (KFbxVector4(interestPosition - cameraGlobalPosition).Length());

      // Set the center
      rotationVector = KFbxVector4(1, 0, 0);
      center = globalRotation.MultT(rotationVector);
      center *= len;
      center += eye;

      // Update up vector
      rotationVector += KFbxVector4(0, 1, 0);
      up = globalRotation.MultT(rotationVector);
    }
  }

  // Align the up vector
  forward = center - eye;
  forward.Normalize();

  right = forward.CrossProduct(up);
  right.Normalize();

  up = right.CrossProduct(forward);
  up.Normalize();

  // Rotate up vector (roll)
  double radians = 0;
  if(camera)
    radians = M_PI * camera->Roll.Get() / 180.0;

  up *= cos(radians);
  right *= sin(radians);
  up = up + right;

  // Get clipping planes
  double nearPlane = 0.01;
  double farPlane = 1000.0;

  if(camera) {
    nearPlane = camera->GetNearPlane();
    farPlane = camera->GetFarPlane();
  }

  // Setup perspective projection
  if(camera && camera->ProjectionType.Get() == KFbxCamera::ePERSPECTIVE) {
    double fovY = 0.0;
    double aspect = camera->GetApertureWidth() * camera->GetSqueezeRatio() / camera->GetApertureHeight();

    if(camera->GetApertureMode() == KFbxCamera::eHORIZONTAL || camera->GetApertureMode() == KFbxCamera::eVERTICAL) {
      fovY = camera->FieldOfView.Get();

      if(camera->GetApertureMode() == KFbxCamera::eHORIZONTAL)
        fovY /= aspect;
    } else if(camera->GetApertureMode() == KFbxCamera::eFOCAL_LENGTH) {
      fovY = camera->ComputeFieldOfView(camera->FocalLength.Get());
      fovY /= aspect;
    } else if(camera->GetApertureMode() == KFbxCamera::eHORIZONTAL_AND_VERTICAL) {
      fovY = camera->FieldOfViewY.Get();
    }

    // Set perspective projection
    DrawUtils::setupPerspective(fovY, aspect, nearPlane, farPlane, eye, center, up);

  } else {
    // Setup orthogonal projection

    // Need window size to compute this correctly
    double pixelRatio = 1.0;

    if(camera)
      pixelRatio = camera->GetPixelRatio();

    /// @todo need viewport size for this to render properly
    int width = 640;
    int height = 480;

    double leftPlane, rightPlane, bottomPlane, topPlane;

    if(width < height) {
      leftPlane   = -g_orthoCameraScale * pixelRatio;
      rightPlane  =  g_orthoCameraScale * pixelRatio;
      bottomPlane = -g_orthoCameraScale * height / width;
      topPlane    =  g_orthoCameraScale * height / width;
    } else {
      width *= (int)pixelRatio;
      leftPlane   = -g_orthoCameraScale * width / height;
      rightPlane  =  g_orthoCameraScale * width / height;
      bottomPlane = -g_orthoCameraScale;
      topPlane    =  g_orthoCameraScale;
    }

    DrawUtils::setupOrthogonal(leftPlane, rightPlane, topPlane, bottomPlane, nearPlane, farPlane, eye, center, up);
  }
}

bool Scene::isProducerCamera(KFbxCamera *camera) const
{
  KFbxGlobalCameraSettings & cameraSettings = m_scene->GlobalCameraSettings();

  if(camera == cameraSettings.GetCameraProducerPerspective())
    return true;

  if(camera == cameraSettings.GetCameraProducerTop())
    return true;

  if(camera == cameraSettings.GetCameraProducerBottom())
    return true;

  if(camera == cameraSettings.GetCameraProducerFront())
    return true;

  if(camera == cameraSettings.GetCameraProducerBack())
    return true;

  if(camera == cameraSettings.GetCameraProducerRight())
    return true;

  if(camera == cameraSettings.GetCameraProducerLeft())
    return true;

  return false;
}

void Scene::draw()
{
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  // Apply camera
  KFbxCamera * camera = getCurrentCamera();
  setCameraTransform(camera);

  static float z = 0.0f;
  z += 0.01;
  // Draw scene
  KFbxXMatrix dummy;
  dummy.SetR(KFbxVector4(0, z, 0, 0));

  for(int i = 0; i < m_scene->GetRootNode()->GetChildCount(); i++)
    drawRecursive(m_scene->GetRootNode()->GetChild(i), dummy);

  // Draw grid
  DrawUtils::drawGrid(dummy);

  glDisable(GL_DEPTH_TEST);
}

void Scene::drawRecursive(KFbxNode *node, KFbxXMatrix &parentGlobalPosition)
{
  // Compute position
  KFbxXMatrix globalPosition = getGlobalPosition(node);

  // Geometry offset (not inherited by children)
  KFbxXMatrix geometryOffset = getGeometryDeformation(node);
  KFbxXMatrix globalOffsetPosition = globalPosition * geometryOffset;

  drawNode(node, parentGlobalPosition, globalOffsetPosition);

  for(int i = 0; i < node->GetChildCount(); i++)
    drawRecursive(node->GetChild(i), globalPosition);
}

KFbxXMatrix Scene::getGeometryDeformation(KFbxNode *node)
{
  KFbxVector4 t = node->GetGeometricTranslation(KFbxNode::eSOURCE_SET);
  KFbxVector4 r = node->GetGeometricRotation(KFbxNode::eSOURCE_SET);
  KFbxVector4 s = node->GetGeometricScaling(KFbxNode::eSOURCE_SET);

  KFbxXMatrix result;
  result.SetT(t);
  result.SetR(r);
  result.SetS(s);

  return result;
}

void Scene::drawNode(KFbxNode *node, KFbxXMatrix &parentGlobalPosition, KFbxXMatrix &globalOffsetPosition, KFbxPose * pose)
{
  KFbxNodeAttribute * attr = node->GetNodeAttribute();


  if(!attr)
    return;

  KFbxNodeAttribute::EAttributeType type = attr->GetAttributeType();

  if (type == KFbxNodeAttribute::eMARKER) {

    // DrawMarker(pGlobalPosition);
  } else if (type == KFbxNodeAttribute::eSKELETON) {
    drawSkeleton(node, parentGlobalPosition, globalOffsetPosition);
  } else if(type == KFbxNodeAttribute::eMESH) {
    //drawMesh(node, parentGlobalPosition, pose);
  } else if (type == KFbxNodeAttribute::eNURB) {
    Radiant::info("nurb");
    // Not supported yet.
    // Should have been converted into a mesh in function ConvertNurbsAndPatch().
  } else if (type == KFbxNodeAttribute::ePATCH) {
    Radiant::info("patch");
    // Not supported yet.
    // Should have been converted into a mesh in function ConvertNurbsAndPatch().
  } else if (type == KFbxNodeAttribute::eCAMERA) {
    Radiant::info("cam");
    // DrawCamera(pNode, pTime, pGlobalPosition);
  } else if (type == KFbxNodeAttribute::eLIGHT) {
    Radiant::info("light");
    // DrawLight(pNode, pTime, pGlobalPosition);
  } else if (type == KFbxNodeAttribute::eNULL) {
    Radiant::info("null");
    // DrawNull(pGlobalPosition);
  }
}

template<typename T>
void insertStuff(KFbxMesh* mesh, T* normals, std::vector<Nimble::Vector3>& output, int vertexCount)
{
  if (!normals)
    return;

  if (normals->GetMappingMode() == KFbxLayerElementNormal::eBY_CONTROL_POINT) {
    for (int i=0; i < vertexCount; ++i) {
      int idx=0;
      if (normals->GetReferenceMode() == KFbxLayerElement::eDIRECT) {
        idx = i;
      } else if (normals->GetReferenceMode() == KFbxLayerElement::eINDEX_TO_DIRECT) {
        idx = normals->GetIndexArray().GetAt(i);
      }
      KFbxVector4 normal = normals->GetDirectArray().GetAt(idx);
      output.push_back(Nimble::Vector3(normal[0], normal[1], normal[2]));
    }
  } else if(normals->GetMappingMode() == KFbxLayerElement::eBY_POLYGON_VERTEX) {
    int lIndexByPolygonVertex = 0;
    //Let's get normals of each polygon, since the mapping mode of normal element is by polygon-vertex.
    for(int lPolygonIndex = 0; lPolygonIndex < mesh->GetPolygonCount(); lPolygonIndex++)
    {
      //get polygon size, you know how many vertices in current polygon.
      int lPolygonSize = mesh->GetPolygonSize(lPolygonIndex);
      //retrieve each vertex of current polygon.
      for(int i = 0; i < lPolygonSize; i++)
      {
        int lNormalIndex = 0;
        //reference mode is direct, the normal index is same as lIndexByPolygonVertex.
        if( normals->GetReferenceMode() == KFbxLayerElement::eDIRECT )
          lNormalIndex = lIndexByPolygonVertex;

        //reference mode is index-to-direct, get normals by the index-to-direct
        if(normals->GetReferenceMode() == KFbxLayerElement::eINDEX_TO_DIRECT)
          lNormalIndex = normals->GetIndexArray().GetAt(lIndexByPolygonVertex);

        //Got normals of each polygon-vertex.
        KFbxVector4 lNormal = normals->GetDirectArray().GetAt(lNormalIndex);

        output.push_back(Nimble::Vector3(lNormal[0], lNormal[1], lNormal[2]));

        lIndexByPolygonVertex++;
      }//end for i //lPolygonSize
    }//end for lPolygonIndex //PolygonCount

  }//end eBY_POLYGON_VERTEX
}

Mesh* Scene::findMesh(const std::string& name)
{
  KFbxNode * node = m_scene->FindNodeByName(name.c_str());
  if (!node)
    return 0;

  KFbxXMatrix dummy;
  Mesh* mesh = buildMesh(node, dummy, 0);
  return mesh;
}

Mesh* Scene::buildMesh(KFbxNode * node, KFbxXMatrix & globalPosition, KFbxPose * pose)
{
  Mesh* myMesh = new Mesh;

  KFbxMesh * mesh = (KFbxMesh*)node->GetNodeAttribute();

  if (!mesh->IsTriangleMesh()) {
    KFbxGeometryConverter converter(m_manager);
    converter.TriangulateInPlace(node);
    mesh = (KFbxMesh*)node->GetNodeAttribute();
  }

  int vertexCount = mesh->GetControlPointsCount();

  if(!vertexCount)
    return 0;

  // Copy deformed vertices
  KFbxVector4 * vertexArray = new KFbxVector4[vertexCount];
  memcpy(vertexArray, mesh->GetControlPoints(), vertexCount * sizeof(KFbxVector4));

  // Active vertex deformer?
  if (mesh->GetDeformerCount(KFbxDeformer::eVERTEX_CACHE) &&
      (dynamic_cast<KFbxVertexCacheDeformer*>(mesh->GetDeformer(0, KFbxDeformer::eVERTEX_CACHE)))->IsActive()) {
    abort();
  } else {
    if (mesh->GetShapeCount()) {
      ComputeShapeDeformation(node, mesh, vertexArray);
    }
    int skinCount = mesh->GetDeformerCount(KFbxDeformer::eSKIN);
    int clusterCount = 0;
    for (int i=0; i < skinCount; ++i) {
      clusterCount += static_cast<KFbxSkin*>(mesh->GetDeformer(i, KFbxDeformer::eSKIN))->GetClusterCount();
    }
    if (clusterCount) {
      ComputeClusterDeformation(globalPosition, mesh, vertexArray, pose);
    }
  }

  /*
  GlDrawMesh(globalPosition,
      mesh,
      vertexArray,
      DRAW_MODE_TEXTURED);
*/
  myMesh->m_name = node->GetName();
  myMesh->m_vertices.resize(vertexCount);
  myMesh->m_indices.resize(mesh->GetPolygonVertexCount());

  KFbxLayer* layer = mesh->GetLayer(0);
  if (layer) {

    KFbxLayerElementNormal* normals = layer->GetNormals();
    KFbxLayerElementTangent* tangents = layer->GetTangents();
    KFbxLayerElementBinormal* binormals = layer->GetBinormals();

    insertStuff(mesh, normals, myMesh->m_normals, vertexCount);
    insertStuff(mesh, tangents, myMesh->m_tangents, vertexCount);
    insertStuff(mesh, binormals, myMesh->m_bitangents, vertexCount);
    Radiant::info("normals %d tangs %d bitangs %d", myMesh->m_normals.size(),
                  myMesh->m_tangents.size(), myMesh->m_bitangents.size());
  }

  int layers = mesh->GetLayerCount();
  for (int i=0; i < layers; ++i) {
    KFbxLayer* layer = mesh->GetLayer(i);
    int idx;
    FOR_EACH_TEXTURE(idx) {
      KFbxLayerElement::ELayerElementType lTextureType = TEXTURE_TYPE(idx);
        KFbxLayerElementUV const* uvs = layer->GetUVs(lTextureType);
        if(!uvs)
            continue;

        // only support mapping mode eBY_POLYGON_VERTEX and eBY_CONTROL_POINT
        if( uvs->GetMappingMode() != KFbxLayerElement::eBY_CONTROL_POINT)
            abort();

        //direct array, where holds the actual uv data
        const int lDataCount = uvs->GetDirectArray().GetCount();

        //index array, where holds the index referenced to the uv data
        const bool lUseIndex = uvs->GetReferenceMode() != KFbxLayerElement::eDIRECT;
        const int lIndexCount= (lUseIndex) ? uvs->GetIndexArray().GetCount() : 0;

        //iterating through the data by polygon
        const int lPolyCount = mesh->GetPolygonCount();

        if (uvs->GetMappingMode() == KFbxLayerElementNormal::eBY_CONTROL_POINT) {
          myMesh->m_textureCoordinates.resize(vertexCount);
          for (int i=0; i < vertexCount; ++i) {
            int idx=0;
            if (uvs->GetReferenceMode() == KFbxLayerElement::eDIRECT) {
              idx = i;
            } else if (uvs->GetReferenceMode() == KFbxLayerElement::eINDEX_TO_DIRECT) {
              idx = uvs->GetIndexArray().GetAt(i);
            }
            KFbxVector2 uv = uvs->GetDirectArray().GetAt(idx);
            myMesh->m_textureCoordinates[i] = Nimble::Vector2(uv[0], uv[1]);
          }
        } else {
          abort();
        }
    }

    int materials = node->GetSrcObjectCount(KFbxSurfaceMaterial::ClassId);
    KFbxSurfaceMaterial* material = KFbxCast<KFbxSurfaceMaterial>(node->GetSrcObject(KFbxSurfaceMaterial::ClassId, 0));

    const char* properties[] = {
      KFbxSurfaceMaterial::sDiffuse,
      KFbxSurfaceMaterial::sEmissive,
      KFbxSurfaceMaterial::sAmbient,
      KFbxSurfaceMaterial::sSpecular,
      KFbxSurfaceMaterial::sSpecularFactor,
      KFbxSurfaceMaterial::sBump,
      KFbxSurfaceMaterial::sNormalMap,
      NULL
    };

    if (material) {
      KFbxPropertyString shadingModel = material->GetShadingModel();
      if (shadingModel.IsValid())
        myMesh->m_material.m_shadingModel = shadingModel.Get().Buffer();


      for (int i=0; properties[i]; ++i) {
        KFbxProperty prop = material->FindProperty(properties[i]);

        if (!prop.IsValid())
          continue;

        KFbxTexture* texture = KFbxCast<KFbxTexture>(prop.GetSrcObject(KFbxTexture::ClassId, 0));
        if (!texture)
          continue;

        myMesh->m_material.m_textures[properties[i]] = TextureManager::instance().load(texture->GetRelativeFileName());
      }
    }
  }

  for (int i=0; i < vertexCount; ++i) {
    myMesh->m_vertices[i] = Nimble::Vector3(vertexArray[i][0], vertexArray[i][1], vertexArray[i][2]);
  }
  /*
        eDIFFUSE_TEXTURES,
        eDIFFUSE_FACTOR_TEXTURES,
                eEMISSIVE_TEXTURES,
                eEMISSIVE_FACTOR_TEXTURES,
                eAMBIENT_TEXTURES,
                eAMBIENT_FACTOR_TEXTURES,
                eSPECULAR_TEXTURES,
        eSPECULAR_FACTOR_TEXTURES,
        eSHININESS_TEXTURES,
                eNORMALMAP_TEXTURES,
                eBUMP_TEXTURES,
                eTRANSPARENT_TEXTURES,
                eTRANSPARENCY_FACTOR_TEXTURES,
                eREFLECTION_TEXTURES,
                eREFLECTION_FACTOR_TEXTURES,
        eDISPLACEMENT_TEXTURES,
        */

  for (int i=0; i < myMesh->m_indices.size(); ++i) {
    myMesh->m_indices[i] = mesh->GetPolygonVertices()[i];
  }


  delete[] vertexArray;

  return myMesh;
}

void Scene::drawSkeleton(KFbxNode * pNode, KFbxXMatrix & pParentGlobalPosition, KFbxXMatrix & pGlobalPosition) {
  KFbxSkeleton* lSkeleton = (KFbxSkeleton*) pNode->GetNodeAttribute();

  // Only draw the skeleton if it's a limb node and if
  // the parent also has an attribute of type skeleton.
  if (lSkeleton->GetSkeletonType() == KFbxSkeleton::eLIMB_NODE &&
      pNode->GetParent() &&
      pNode->GetParent()->GetNodeAttribute() &&
      pNode->GetParent()->GetNodeAttribute()->GetAttributeType() == KFbxNodeAttribute::eSKELETON)
  {

    glColor3f(1.0, 0.0, 0.0);
    glLineWidth(2.0);

    glBegin(GL_LINES);

    glVertex3dv((GLdouble *)pParentGlobalPosition.GetT());
    glVertex3dv((GLdouble *)pGlobalPosition.GetT());

    glEnd();
  }
}

void Scene::ComputeShapeDeformation(KFbxNode* node,
                             KFbxMesh* mesh,
                             KFbxVector4* vertexArray)
{
  int i, j;
  int shapeCount = mesh->GetShapeCount();
  int vertexCount = mesh->GetControlPointsCount();

  KFbxVector4* srcVertexArray = vertexArray;
  KFbxVector4* dstVertexArray = new KFbxVector4[vertexCount];

  memcpy(dstVertexArray, vertexArray, vertexCount * sizeof(KFbxVector4));

  for (i = 0; i < shapeCount; i++)
  {
    KFbxShape* shape = mesh->GetShape(i);

    // Get the percentage of influence of the shape.
    KFbxAnimCurve* curve = mesh->GetShapeChannel(i, m_currentLayer);
    if (!curve) continue;
    double weight = curve->Evaluate(m_time) / 100.0;

    for (j = 0; j < vertexCount; j++)
    {
      // Add the influence of the shape vertex to the mesh vertex.
      KFbxVector4 influence = (shape->GetControlPoints()[j] - srcVertexArray[j]) * weight;
      dstVertexArray[j] += influence;
    }
  }

  memcpy(vertexArray, dstVertexArray, vertexCount * sizeof(KFbxVector4));
  delete [] dstVertexArray;
}

void Scene::ComputeClusterDeformation(KFbxXMatrix& pGlobalPosition,
                               KFbxMesh* pMesh,
                               KFbxVector4* pVertexArray,
                               KFbxPose* pPose)
{
// All the links must have the same link mode.
   KFbxCluster::ELinkMode lClusterMode = ((KFbxSkin*)pMesh->GetDeformer(0, KFbxDeformer::eSKIN))->GetCluster(0)->GetLinkMode();

   int i, j;
   int lClusterCount=0;

   int lVertexCount = pMesh->GetControlPointsCount();
   int lSkinCount = pMesh->GetDeformerCount(KFbxDeformer::eSKIN);

   KFbxXMatrix* lClusterDeformation = new KFbxXMatrix[lVertexCount];
   memset(lClusterDeformation, 0, lVertexCount * sizeof(KFbxXMatrix));
   double* lClusterWeight = new double[lVertexCount];
   memset(lClusterWeight, 0, lVertexCount * sizeof(double));

   if (lClusterMode == KFbxCluster::eADDITIVE)
   {
       for (i = 0; i < lVertexCount; i++)
       {
           lClusterDeformation[i].SetIdentity();
       }
   }

   for ( i=0; i<lSkinCount; ++i)
   {
       lClusterCount =( (KFbxSkin *)pMesh->GetDeformer(i, KFbxDeformer::eSKIN))->GetClusterCount();
       for (j=0; j<lClusterCount; ++j)
       {
           KFbxCluster* lCluster =((KFbxSkin *) pMesh->GetDeformer(i, KFbxDeformer::eSKIN))->GetCluster(j);
           if (!lCluster->GetLink())
               continue;
           KFbxXMatrix lReferenceGlobalInitPosition;
           KFbxXMatrix lReferenceGlobalCurrentPosition;
           KFbxXMatrix lClusterGlobalInitPosition;
           KFbxXMatrix lClusterGlobalCurrentPosition;
           KFbxXMatrix lReferenceGeometry;
           KFbxXMatrix lClusterGeometry;

           KFbxXMatrix lClusterRelativeInitPosition;
           KFbxXMatrix lClusterRelativeCurrentPositionInverse;
           KFbxXMatrix lVertexTransformMatrix;

           if (lClusterMode == KFbxLink::eADDITIVE && lCluster->GetAssociateModel())
           {
               lCluster->GetTransformAssociateModelMatrix(lReferenceGlobalInitPosition);
               //lReferenceGlobalCurrentPosition = GetGlobalPosition(lCluster->GetAssociateModel(), m_time, pPose);
               lReferenceGlobalCurrentPosition = getGlobalPosition(lCluster->GetAssociateModel());
                   getGlobalPosition(lCluster->GetAssociateModel(), pPose);

               // Geometric transform of the model
               lReferenceGeometry = getGeometryDeformation(lCluster->GetAssociateModel());
               lReferenceGlobalCurrentPosition *= lReferenceGeometry;
           }
           else
           {
               lCluster->GetTransformMatrix(lReferenceGlobalInitPosition);
               lReferenceGlobalCurrentPosition = pGlobalPosition;
               // Multiply lReferenceGlobalInitPosition by Geometric Transformation
               lReferenceGeometry = getGeometryDeformation(pMesh->GetNode());
               lReferenceGlobalInitPosition *= lReferenceGeometry;
           }
           // Get the link initial global position and the link current global position.
           lCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);
           lClusterGlobalCurrentPosition = getGlobalPosition(lCluster->GetLink(), pPose);

           // Compute the initial position of the link relative to the reference.
           lClusterRelativeInitPosition = lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;

           // Compute the current position of the link relative to the reference.
           lClusterRelativeCurrentPositionInverse = lReferenceGlobalCurrentPosition.Inverse() * lClusterGlobalCurrentPosition;

           // Compute the shift of the link relative to the reference.
           lVertexTransformMatrix = lClusterRelativeCurrentPositionInverse * lClusterRelativeInitPosition;

           int k;
           int lVertexIndexCount = lCluster->GetControlPointIndicesCount();

           for (k = 0; k < lVertexIndexCount; ++k)
           {
               int lIndex = lCluster->GetControlPointIndices()[k];

               // Sometimes, the mesh can have less points than at the time of the skinning
               // because a smooth operator was active when skinning but has been deactivated during export.
               if (lIndex >= lVertexCount)
                   continue;

               double lWeight = lCluster->GetControlPointWeights()[k];

               if (lWeight == 0.0)
               {
                   continue;
               }

               // Compute the influence of the link on the vertex.
               KFbxXMatrix lInfluence = lVertexTransformMatrix;

               //MatrixScale(lInfluence, lWeight);
               lInfluence *= lWeight;

               if (lClusterMode == KFbxCluster::eADDITIVE)
               {
                   // Multiply with to the product of the deformations on the vertex.
                 for (int i=0; i < 4; ++i) {
                   lInfluence[i][i] = 1.0 - lWeight;
                 }
                   //MatrixAddToDiagonal(lInfluence, 1.0 - lWeight);
                   lClusterDeformation[lIndex] = lInfluence * lClusterDeformation[lIndex];

                   // Set the link to 1.0 just to know this vertex is influenced by a link.
                   lClusterWeight[lIndex] = 1.0;
               }
               else // lLinkMode == KFbxLink::eNORMALIZE || lLinkMode == KFbxLink::eTOTAL1
               {
                   // Add to the sum of the deformations on the vertex.
                   for (int i=0; i < 4; ++i) {
                     for (int j=0; j < 4; ++j) {
                       lClusterDeformation[lIndex][i][j] += lInfluence[i][j];
                     }
                   }

                  //MatrixAdd(lClusterDeformation[lIndex], lInfluence);

                   // Add to the sum of weights to either normalize or complete the vertex.
                   lClusterWeight[lIndex] += lWeight;
               }

           }
       }
   }

   for (i = 0; i < lVertexCount; i++)
   {
       KFbxVector4 lSrcVertex = pVertexArray[i];
       KFbxVector4& lDstVertex = pVertexArray[i];
       double lWeight = lClusterWeight[i];

       // Deform the vertex if there was at least a link with an influence on the vertex,
       if (lWeight != 0.0)
       {
           lDstVertex = lClusterDeformation[i].MultT(lSrcVertex);

           if (lClusterMode == KFbxCluster::eNORMALIZE)
           {
               // In the normalized link mode, a vertex is always totally influenced by the links.
               lDstVertex /= lWeight;
           }
           else if (lClusterMode == KFbxCluster::eTOTAL1)
           {
               // In the total 1 link mode, a vertex can be partially influenced by the links.
               lSrcVertex *= (1.0 - lWeight);
               lDstVertex += lSrcVertex;
           }
       }
   }

   delete [] lClusterDeformation;
   delete [] lClusterWeight;
}

}
