#include "Scene.hpp"
#include "DrawUtils.hpp"

#include <Luminous/Luminous.hpp>

#include <fbxsdk.h>

#include <cmath>

namespace Vivid
{

static double g_orthoCameraScale = 178.0;

//////////
//////////

Scene::Scene(KFbxSdkManager * sdk)
  : m_time(0)
{
  m_scene = KFbxScene::Create(sdk, "");
}

Scene::~Scene()
{
  m_scene->Destroy();
}

bool Scene::import(KFbxSdkManager * sdk, const std::string &filename)
{
  KFbxImporter * importer = KFbxImporter::Create(sdk, "");

  bool status = importer->Initialize(filename.c_str(), -1, sdk->GetIOSettings());
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

KFbxXMatrix Scene::getGlobalPosition(KFbxNode *node)
{
  return node->GetScene()->GetEvaluator()->GetNodeGlobalTransform(node, m_time);
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

  // Draw scene
  KFbxXMatrix dummy;

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

  //drawNode(node, parentGlobalPosition, globalOffsetPosition);

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

void Scene::drawNode(KFbxNode *node, KFbxXMatrix &parentGlobalPosition)
{
  KFbxNodeAttribute * attr = node->GetNodeAttribute();

  if(attr) {

    if(attr->GetAttributeType() == KFbxNodeAttribute::eMESH)
      drawMesh(node, parentGlobalPosition);

  }
}

void Scene::drawMesh(KFbxNode *node, KFbxXMatrix &parentGlobalPosition)
{
  KFbxMesh * mesh = (KFbxMesh*)node->GetNodeAttribute();
  int vertexCount = mesh->GetControlPointsCount();

  if(!vertexCount)
    return;

  // Copy deformed vertices
  KFbxVector4 * vertexArray = new KFbxVector4 [vertexCount];
  memcpy(vertexArray, mesh->GetControlPoints(), vertexCount * sizeof(KFbxVector4));

  // Active vertex deformer?
  if(mesh->GetDeformerCount(KFbxDeformer::eVERTEX_CACHE) && )


  delete[] vertexArray;
}

}
