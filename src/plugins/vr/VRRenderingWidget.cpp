///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <gui/GUI.h>
#include <core/animation/AnimationSettings.h>
#include <core/dataset/DataSet.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/viewport/ViewportSettings.h>
#include <core/scene/SceneRoot.h>
#include "VRWindow.h"

namespace VRPlugin {

/******************************************************************************
* Constructor.
******************************************************************************/
VRRenderingWidget::VRRenderingWidget(QWidget *parent, DataSet* dataset) : QOpenGLWidget(parent)
{
    _sceneRenderer = new VRSceneRenderer(dataset);

    // Create a settings object, or use stored one if there is one.
    _settings = dataset->findGlobalObject<VRSettingsObject>();
    if(!_settings) {
        _settings = new VRSettingsObject(dataset);
        dataset->addGlobalObject(_settings);
        _settings->recenter();
    }    

    // Initialize VR headset.
    vr::EVRInitError eError = vr::VRInitError_None;
    _hmd = vr::VR_Init(&eError, vr::VRApplication_Scene);
	if(eError != vr::VRInitError_None)
        dataset->throwException(tr("Cannot start virtual reality headset. OpenVR initialization error: %1").arg(vr::VR_GetVRInitErrorAsEnglishDescription(eError)));
    
    // Get the proper rendering resolution of the hmd.
    _hmd->GetRecommendedRenderTargetSize(&_hmdRenderWidth, &_hmdRenderHeight);

    // Initialize the compositor.
    vr::IVRCompositor* compositor = vr::VRCompositor();
	if(!compositor)
        dataset->throwException(tr("OpenVR Compositor initialization failed."));
    
    // Get dimensions of play area.
    vr::IVRChaperone* chaperone = vr::VRChaperone();
    chaperone->GetPlayAreaRect(&_playAreaRect);
    _playAreaMesh.setVertexCount(4);
    _playAreaMesh.setFaceCount(2);
    for(int i = 0; i < 4; i++) {
        _playAreaMesh.vertices()[i].x() = _playAreaRect.vCorners[i].v[0];
        _playAreaMesh.vertices()[i].y() = _playAreaRect.vCorners[i].v[1];
        _playAreaMesh.vertices()[i].z() = _playAreaRect.vCorners[i].v[2];
    }
    _playAreaMesh.faces()[0].setVertices(0,1,2);
    _playAreaMesh.faces()[1].setVertices(0,2,3);    
}

/******************************************************************************
* Destructor.
******************************************************************************/
VRRenderingWidget::~VRRenderingWidget()
{
    cleanup();

    // Shutdown VR headset.
    if(_hmd) {
        vr::VR_Shutdown();
    }
}

/******************************************************************************
* Called when the GL context is destroyed.
******************************************************************************/
void VRRenderingWidget::cleanup()
{
    makeCurrent();
    _floorMesh.reset();
    _eyeBuffer.reset();
    doneCurrent();
}

/******************************************************************************
* Called when the GL context is initialized.
******************************************************************************/
void VRRenderingWidget::initializeGL()
{    
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &VRRenderingWidget::cleanup);
    initializeOpenGLFunctions();
}

/******************************************************************************
* Computes the projection and transformation matrices for each of the two eyes.
******************************************************************************/
ViewProjectionParameters VRRenderingWidget::projectionParameters(int eye, FloatType aspectRatio, const AffineTransformation& bodyToWorldTM, const Box3& sceneBoundingBox)
{
	OVITO_ASSERT(aspectRatio > FLOATTYPE_EPSILON);
	OVITO_ASSERT(!sceneBoundingBox.isEmpty());

    AffineTransformation headToBodyTM = fromOpenVRMatrix(_trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);
    AffineTransformation eyeToHeadTM = fromOpenVRMatrix(_hmd->GetEyeToHeadTransform(eye ? vr::Eye_Right : vr::Eye_Left));
    
	ViewProjectionParameters params;
	params.aspectRatio = aspectRatio;
	params.validityInterval.setInfinite();
	params.boundingBox = sceneBoundingBox;
    params.inverseViewMatrix = bodyToWorldTM * headToBodyTM * eyeToHeadTM;
    params.viewMatrix = params.inverseViewMatrix.inverse();
    params.fieldOfView = 0;
    params.isPerspective = true;

	// Transform scene bounding box to camera space.
	Box3 bb = sceneBoundingBox.transformed(params.viewMatrix).centerScale(FloatType(1.01));

	// Compute near/far plane distances.
    if(bb.minc.z() < 0) {
        params.zfar = -bb.minc.z();
        params.znear = std::max(-bb.maxc.z(), params.zfar * FloatType(1e-4));
    }
    else {
        params.zfar = std::max(sceneBoundingBox.size().length(), FloatType(1));
        params.znear = params.zfar * FloatType(1e-4);
    }
    params.zfar = std::max(params.zfar, params.znear * FloatType(1.01));

	// Compute projection matrix.
    params.projectionMatrix = fromOpenVRMatrix(_hmd->GetProjectionMatrix(eye ? vr::Eye_Right : vr::Eye_Left, params.znear, params.zfar));
	params.inverseProjectionMatrix = params.projectionMatrix.inverse();

	return params;
}

/******************************************************************************
* Called when the VR window contents are rendered.
******************************************************************************/
void VRRenderingWidget::paintGL()
{
    // Queue up another repaint event.
    update();

    // Clear background of VR window.
   	glViewport(0, 0, _windowWidth, _windowHeight);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vr::IVRCompositor* compositor = vr::VRCompositor();
    if(!compositor) return;

    // Choose between seated and standing mode.
    compositor->SetTrackingSpace(settings()->flyingMode() ? vr::TrackingUniverseSeated : vr::TrackingUniverseStanding);

    // Request camera position from VR headset.
    compositor->WaitGetPoses(_trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
    if(!_trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
        return;

    // Measure time since last frame.
    FloatType elapsedTime = _time.elapsed();
    _time.restart();

    bool accelerating = false;
    bool playAnimation = false;

    // Process SteamVR controller state.
    std::vector<AffineTransformation> controllerTMs;
    for(vr::TrackedDeviceIndex_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++) {
        if(!_hmd->IsTrackedDeviceConnected(unTrackedDevice))
            continue;
        if(_hmd->GetTrackedDeviceClass(unTrackedDevice) != vr::TrackedDeviceClass_Controller)
            continue;
        if(!_trackedDevicePose[unTrackedDevice].bPoseIsValid)
            continue;
        
        AffineTransformation controllerTM = fromOpenVRMatrix(_trackedDevicePose[unTrackedDevice].mDeviceToAbsoluteTracking);
        controllerTMs.push_back(controllerTM);
        
        vr::VRControllerState_t state;
        if(_hmd->GetControllerState(unTrackedDevice, &state, sizeof(state))) {
            if(state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad)) {
                _currentSpeed += elapsedTime * FloatType(1e-6);
                _currentSpeed = std::min(_currentSpeed, FloatType(0.001));
                accelerating = true;
                if(settings()->flyingMode()) {
                    AffineTransformation tm = settings()->viewerTM();
                    FloatType factor = _currentSpeed * settings()->movementSpeed() * elapsedTime;
                    tm.translation() += controllerTM * Vector3(state.rAxis[0].x * factor, 0, -state.rAxis[0].y * factor);
                    settings()->setViewerTM(tm);
                }
                else {
                    if(std::abs(state.rAxis[0].x) > std::abs(state.rAxis[0].y)) {
                        settings()->setRotationZ(settings()->rotationZ() + state.rAxis[0].x * elapsedTime * _currentSpeed * 2);
                    }
                    else {
                        AffineTransformation tm = AffineTransformation::rotationX(FLOATTYPE_PI/2) * settings()->viewerTM() * controllerTM;
                        Vector3 dir = tm.column(2);
                        Vector3 translation = settings()->translation();
                        if(dir.z()*dir.z() >= dir.x()*dir.x() + dir.y()*dir.y()) {
                            translation.z() -= FloatType(0.5) * state.rAxis[0].y * elapsedTime * std::copysign(_currentSpeed, dir.z()) * settings()->movementSpeed();
                        }
                        else {
                            translation -= (FloatType(0.5) * state.rAxis[0].y * elapsedTime * _currentSpeed * settings()->movementSpeed()) * Vector3(dir.x(), dir.y(), 0).normalized();
                        }
                        settings()->setTranslation(translation);
                    }                    
                }
            }

            if(state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)) {
                playAnimation = true;
            }            
        }
    }
    if(!accelerating) 
        _currentSpeed = 0;

    dataset()->animationSettings()->setAnimationPlayback(playAnimation);

    // Compute model transformation.
    AffineTransformation transformationTM = 
        AffineTransformation::translation(settings()->translation()) * 
        AffineTransformation::scaling(std::max(settings()->scaleFactor(), FloatType(1e-6))) *
        AffineTransformation::rotationZ(settings()->rotationZ()) *
        AffineTransformation::translation(-settings()->modelCenter());

    // Compute viewer position in scene space.
    AffineTransformation bodyToWorldTM = 
        ViewportSettings::getSettings().coordinateSystemOrientation() * 
        transformationTM.inverse() *
        AffineTransformation::rotationX(FLOATTYPE_PI/2) *
        settings()->viewerTM();

    // Allocate framebuffers for both eyes.
    _renderResolution = QSize(_hmdRenderWidth, _hmdRenderHeight) * (settings()->supersamplingEnabled() ? 2 : 1);
    if(!_eyeBuffer || _eyeBuffer->size() != _renderResolution)
        _eyeBuffer.reset(new QOpenGLFramebufferObject(_renderResolution, QOpenGLFramebufferObject::Depth));

    FloatType aspectRatio = (FloatType)_renderResolution.height() / _renderResolution.width();
    try {
        for(int eye = 0; eye < 2; eye++) {

            // Set up the renderer.
            TimePoint time = dataset()->animationSettings()->time();
            _sceneRenderer->startRender(dataset(), dataset()->renderSettings());

            // Render to an offscreen buffer, one for each eye.
            if(!_eyeBuffer->bind())
                dataset()->throwException(tr("Failed to bind OpenGL framebuffer object for offscreen rendering."));

            // Request scene bounding box.
            Box3 boundingBox = _sceneRenderer->sceneBoundingBox(time);

            // Add ground geometry to bounding box.
            AffineTransformation bodyToFloorTM;
            if(settings()->showFloor()) {
                if(settings()->flyingMode())
                    bodyToFloorTM = bodyToWorldTM * fromOpenVRMatrix(_hmd->GetSeatedZeroPoseToStandingAbsoluteTrackingPose()).inverse();
                else
                    bodyToFloorTM = bodyToWorldTM;
                boundingBox.addBox(_playAreaMesh.boundingBox().transformed(bodyToFloorTM));
            }
            
            // Add controller geometry to bounding box.
            for(const AffineTransformation& controllerTM : controllerTMs)
                boundingBox.addBox((bodyToWorldTM * controllerTM) * Box3(Point3::Origin(), _controllerSize));
            
            // Set up projection.
            ViewProjectionParameters projParams = projectionParameters(eye, aspectRatio, bodyToWorldTM, boundingBox);

            // Set up the renderer.
            _sceneRenderer->beginFrame(time, projParams, nullptr);
            _sceneRenderer->setRenderingViewport(0, 0, _renderResolution.width(), _renderResolution.height());
            
            // Call the viewport renderer to render the scene objects.
            _sceneRenderer->renderFrame(nullptr, SceneRenderer::NonStereoscopic, dataset()->container()->taskManager());

            // Render floor rectangle.
            if(settings()->showFloor()) {
                if(!_floorMesh || !_floorMesh->isValid(_sceneRenderer)) {
                    _floorMesh = _sceneRenderer->createMeshPrimitive();
                    _floorMesh->setMesh(_playAreaMesh, ColorA(1.0f,1.0f,0.7f,0.8f));
                    _floorMesh->setCullFaces(false);
                }
                _sceneRenderer->setWorldTransform(bodyToFloorTM);
                _floorMesh->render(_sceneRenderer);
            }

            // Render VR controllers.
            for(const AffineTransformation& controllerTM : controllerTMs) {
                if(!_controllerGeometry || !_controllerGeometry->isValid(_sceneRenderer)) {
                    _controllerGeometry = _sceneRenderer->createArrowPrimitive(ArrowPrimitive::ArrowShape, ArrowPrimitive::NormalShading, ArrowPrimitive::HighQuality);
                    _controllerGeometry->startSetElements(1);
                    _controllerGeometry->setElement(0, Point3(0,0,_controllerSize), Vector3(0,0,-_controllerSize), ColorA(1.0f, 0.0f, 0.0f, 1.0f), 0.02f);
                    _controllerGeometry->endSetElements();
                }
                _sceneRenderer->setWorldTransform(bodyToWorldTM * controllerTM);
                _controllerGeometry->render(_sceneRenderer);
            }

            // Cleanup
            _sceneRenderer->endFrame(true);
            _sceneRenderer->endRender();

            // Submit rendered image to VR compositor.
            const vr::Texture_t tex = { reinterpret_cast<void*>(intptr_t(_eyeBuffer->texture())), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
            compositor->Submit(vr::EVREye(eye), &tex);
        }
    }
    catch(const Exception& ex) {
        ex.logError();
        ex.reportError();
    }

    // Tell the compositor to begin work immediately instead of waiting for the next WaitGetPoses() call
    compositor->PostPresentHandoff();

    // Switch back to the screen framebuffer.
    if(!QOpenGLFramebufferObject::bindDefault())
        dataset()->throwException(tr("Failed to release OpenGL framebuffer object after offscreen rendering."));    

    // Mirror right eye on screen.
    FloatType windowAspectRatio = (FloatType)_windowHeight / _windowWidth;
    int blitWidth, blitHeight;
    if(aspectRatio > windowAspectRatio) {
        blitWidth = _renderResolution.width();
        blitHeight = _renderResolution.height() * (windowAspectRatio / aspectRatio);
    }
    else {
        blitWidth = _renderResolution.width() * (aspectRatio / windowAspectRatio);
        blitHeight = _renderResolution.height();
    }
    QOpenGLFramebufferObject::blitFramebuffer(
                    nullptr, QRect(0,0,_windowWidth,_windowHeight),
                    _eyeBuffer.get(), 
                    QRect((_renderResolution.width() - blitWidth)/2, (_renderResolution.height() - blitHeight)/2, blitWidth, blitHeight),
                    GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

/******************************************************************************
* Called when the VR display window is resized.
******************************************************************************/
void VRRenderingWidget::resizeGL(int w, int h)
{
    _windowWidth = w;
    _windowHeight = h;
}

};

