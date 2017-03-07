#include <android/log.h>
#include <assert.h>
#include <stdlib.h>
#include <cmath>
#include <random>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <jni.h>

#include <memory>
#include <string>
#include <thread>  // NOLINT
#include <vector>

#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_audio.h"
#include "vr/gvr/capi/include/gvr_controller.h"
#include "vr/gvr/capi/include/gvr_types.h"

#include "modules/vr/VRWebGLCommand.h"
#include "modules/vr/VRWebGLCommandProcessor.h"
#include "modules/vr/VRWebGLMath.h"
#include "modules/vr/VRWebGLPose.h"
#include "modules/vr/VRWebGLEyeParameters.h"

class VRWebGL_GVRMobileSDK {
 public:
  /**
   * Create a VRWebGL_GVRMobileSDK using a given |gvr_context|.
   *
   * @param gvr_api The (non-owned) gvr_context.
   * @param gvr_audio_api The (owned) gvr::AudioApi context.
   */
  VRWebGL_GVRMobileSDK(gvr_context* gvr_context);

  /**
   * Destructor.
   */
  ~VRWebGL_GVRMobileSDK();

  /**
   * Initialize any GL-related objects. This should be called on the rendering
   * thread with a valid GL context.
   */
  void InitializeGl();

  /**
   * Draw the TreasureHunt scene. This should be called on the rendering thread.
   */
  void DrawFrame();

  /**
   * Pause head tracking.
   */
  void OnPause();

  /**
   * Resume head tracking, refreshing viewer parameters if necessary.
   */
  void OnResume();

 private:
  /*
   * Prepares the GvrApi framebuffer for rendering, resizing if needed.
   */
  void PrepareFramebuffer();

  /**
   * Draws all world-space objects for one eye.
   *
   * @param view_matrix View transformation for the current eye.
   * @param viewport The buffer viewport for which we are rendering.
   */
  void DrawWorld(const gvr::Mat4f& view_matrix,
                 const gvr::BufferViewport& viewport);

  std::unique_ptr<gvr::GvrApi> gvr_api_;
  std::unique_ptr<gvr::BufferViewportList> viewport_list_;
  std::unique_ptr<gvr::SwapChain> swapchain_;
  gvr::BufferViewport scratch_viewport_;

  gvr::Mat4f head_view_;
  gvr::Mat4f projectionMatrixTransposed;
  gvr::Mat4f viewMatrixTransposed;
  gvr::Sizei render_size_;

  gvr::ViewerType gvr_viewer_type_;
};

#define LOG_TAG "VRWebGL_GVRMobileSDK"
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define CHECK(condition)                                                   \
  if (!(condition)) {                                                      \
    LOGE("*** CHECK FAILED at %s:%d: %s", __FILE__, __LINE__, #condition); \
    abort();                                                               \
  }

namespace {
static const float kZNear = 0.001f;
static const float kZFar = 100000.0f;

static const int kCoordsPerVertex = 3;

static const uint64_t kPredictionTimeWithoutVsyncNanos = 50000000;

static std::array<float, 16> MatrixToGLArray(const gvr::Mat4f& matrix) {
  // Note that this performs a *tranpose* to a column-major matrix array, as
  // expected by GL.
  std::array<float, 16> result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result[j * 4 + i] = matrix.m[i][j];
    }
  }
  return result;
}

static std::array<float, 4> MatrixVectorMul(const gvr::Mat4f& matrix,
                                            const std::array<float, 4>& vec) {
  std::array<float, 4> result;
  for (int i = 0; i < 4; ++i) {
    result[i] = 0;
    for (int k = 0; k < 4; ++k) {
      result[i] += matrix.m[i][k] * vec[k];
    }
  }
  return result;
}

static gvr::Mat4f MatrixMul(const gvr::Mat4f& matrix1,
                            const gvr::Mat4f& matrix2) {
  gvr::Mat4f result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = 0.0f;
      for (int k = 0; k < 4; ++k) {
        result.m[i][j] += matrix1.m[i][k] * matrix2.m[k][j];
      }
    }
  }
  return result;
}

static gvr::Mat4f PerspectiveMatrixFromView(const gvr::Rectf& fov, float z_near,
                                            float z_far) {
  gvr::Mat4f result;
  const float x_left = -std::tan(fov.left * M_PI / 180.0f) * z_near;
  const float x_right = std::tan(fov.right * M_PI / 180.0f) * z_near;
  const float y_bottom = -std::tan(fov.bottom * M_PI / 180.0f) * z_near;
  const float y_top = std::tan(fov.top * M_PI / 180.0f) * z_near;
  const float zero = 0.0f;

  assert(x_left < x_right && y_bottom < y_top && z_near < z_far &&
         z_near > zero && z_far > zero);
  const float X = (2 * z_near) / (x_right - x_left);
  const float Y = (2 * z_near) / (y_top - y_bottom);
  const float A = (x_right + x_left) / (x_right - x_left);
  const float B = (y_top + y_bottom) / (y_top - y_bottom);
  const float C = (z_near + z_far) / (z_near - z_far);
  const float D = (2 * z_near * z_far) / (z_near - z_far);

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = 0.0f;
    }
  }
  result.m[0][0] = X;
  result.m[0][2] = A;
  result.m[1][1] = Y;
  result.m[1][2] = B;
  result.m[2][2] = C;
  result.m[2][3] = D;
  result.m[3][2] = -1;

  return result;
}

static gvr::Rectf ModulateRect(const gvr::Rectf& rect, float width,
                               float height) {
  gvr::Rectf result = {rect.left * width, rect.right * width,
                       rect.bottom * height, rect.top * height};
  return result;
}

static gvr::Recti CalculatePixelSpaceRect(const gvr::Sizei& texture_size,
                                          const gvr::Rectf& texture_rect) {
  const float width = static_cast<float>(texture_size.width);
  const float height = static_cast<float>(texture_size.height);
  const gvr::Rectf rect = ModulateRect(texture_rect, width, height);
  const gvr::Recti result = {
      static_cast<int>(rect.left), static_cast<int>(rect.right),
      static_cast<int>(rect.bottom), static_cast<int>(rect.top)};
  return result;
}

// Generate a random floating point number between 0 and 1.
static float RandomUniformFloat() {
  static std::random_device random_device;
  static std::mt19937 random_generator(random_device());
  static std::uniform_real_distribution<float> random_distribution(0, 1);
  return random_distribution(random_generator);
}

static void CheckGLError(const char* label) {
  int gl_error = glGetError();
  if (gl_error != GL_NO_ERROR) {
    LOGW("GL error @ %s: %d", label, gl_error);
    // Crash immediately to make OpenGL errors obvious.
    // abort();
  }
}

static gvr::Sizei HalfPixelCount(const gvr::Sizei& in) {
  // Scale each dimension by sqrt(2)/2 ~= 7/10ths.
  gvr::Sizei out;
  out.width = (7 * in.width) / 10;
  out.height = (7 * in.height) / 10;
  return out;
}

static gvr::Mat4f ControllerQuatToMatrix(const gvr::ControllerQuat& quat) {
  gvr::Mat4f result;
  const float x = quat.qx;
  const float x2 = quat.qx * quat.qx;
  const float y = quat.qy;
  const float y2 = quat.qy * quat.qy;
  const float z = quat.qz;
  const float z2 = quat.qz * quat.qz;
  const float w = quat.qw;
  const float xy = quat.qx * quat.qy;
  const float xz = quat.qx * quat.qz;
  const float xw = quat.qx * quat.qw;
  const float yz = quat.qy * quat.qz;
  const float yw = quat.qy * quat.qw;
  const float zw = quat.qz * quat.qw;

  const float m11 = 1.0f - 2.0f * y2 - 2.0f * z2;
  const float m12 = 2.0f * (xy - zw);
  const float m13 = 2.0f * (xz + yw);
  const float m21 = 2.0f * (xy + zw);
  const float m22 = 1.0f - 2.0f * x2 - 2.0f * z2;
  const float m23 = 2.0f * (yz - xw);
  const float m31 = 2.0f * (xz - yw);
  const float m32 = 2.0f * (yz + xw);
  const float m33 = 1.0f - 2.0f * x2 - 2.0f * y2;

  return {{{m11, m12, m13, 0.0f},
           {m21, m22, m23, 0.0f},
           {m31, m32, m33, 0.0f},
           {0.0f, 0.0f, 0.0f, 1.0f}}};
}

static inline float VectorNorm(const std::array<float, 4>& vect) {
  return std::sqrt(vect[0] * vect[0] + vect[1] * vect[1] + vect[2] * vect[2]);
}

static float VectorInnerProduct(const std::array<float, 4>& vect1,
                                const std::array<float, 4>& vect2) {
  float product = 0;
  for (int i = 0; i < 3; i++) {
    product += vect1[i] * vect2[i];
  }
  return product;
}
}  // anonymous namespace

VRWebGL_GVRMobileSDK::VRWebGL_GVRMobileSDK(
    gvr_context* gvr_context)
    : gvr_api_(gvr::GvrApi::WrapNonOwned(gvr_context)),
      scratch_viewport_(gvr_api_->CreateBufferViewport()),
      gvr_viewer_type_(gvr_api_->GetViewerType()) {
  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_DAYDREAM) {
    LOGD("Viewer type: DAYDREAM");
  } else {
    LOGE("Unexpected viewer type.");
  }
}

VRWebGL_GVRMobileSDK::~VRWebGL_GVRMobileSDK() {
}

void VRWebGL_GVRMobileSDK::InitializeGl() {
  gvr_api_->InitializeGl();

  // Because we are using 2X MSAA, we can render to half as many pixels and
  // achieve similar quality.
  render_size_ =
      HalfPixelCount(gvr_api_->GetMaximumEffectiveRenderTargetSize());
  std::vector<gvr::BufferSpec> specs;

  specs.push_back(gvr_api_->CreateBufferSpec());
  specs[0].SetColorFormat(GVR_COLOR_FORMAT_RGBA_8888);
  specs[0].SetDepthStencilFormat(GVR_DEPTH_STENCIL_FORMAT_DEPTH_16);
  specs[0].SetSize(render_size_);
  specs[0].SetSamples(2);
  swapchain_.reset(new gvr::SwapChain(gvr_api_->CreateSwapChain(specs)));

  viewport_list_.reset(
      new gvr::BufferViewportList(gvr_api_->CreateEmptyBufferViewportList()));
}

void VRWebGL_GVRMobileSDK::DrawFrame() {
  VRWebGLCommandProcessor::getInstance()->update();

  // In order to reduce the flickering, when a synchronous command has been executed in the update call, do not render anything.
  // Not perfect, as the render get stalled, but at least it is better than rendering with an undefined state of the opengl context.
  if (/*!appThread->renderEnabled || */VRWebGLCommandProcessor::getInstance()->m_synchronousVRWebGLCommandBeenProcessedInUpdate())
  {
    VRWebGLCommandProcessor::getInstance()->renderFrame(false);
    return;
  }

  PrepareFramebuffer();
  gvr::Frame frame = swapchain_->AcquireFrame();

  // A client app does its rendering here.
  gvr::ClockTimePoint target_time = gvr::GvrApi::GetTimePointNow();
  target_time.monotonic_system_time_nanos += kPredictionTimeWithoutVsyncNanos;

  head_view_ = gvr_api_->GetHeadSpaceFromStartSpaceRotation(target_time);
  gvr::Mat4f left_eye_matrix = gvr_api_->GetEyeFromHeadMatrix(GVR_LEFT_EYE);
  gvr::Mat4f right_eye_matrix = gvr_api_->GetEyeFromHeadMatrix(GVR_RIGHT_EYE);
  gvr::Mat4f left_eye_view = MatrixMul(left_eye_matrix, head_view_);
  gvr::Mat4f right_eye_view = MatrixMul(right_eye_matrix, head_view_);

  viewport_list_->SetToRecommendedBufferViewports();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_BLEND);

  // Draw the world.
  frame.BindBuffer(0);
  glClearColor(0.1f, 0.1f, 0.1f, 0.5f);  // Dark background so text shows up.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  viewport_list_->GetBufferViewport(0, &scratch_viewport_);
  DrawWorld(left_eye_view, scratch_viewport_);
  viewport_list_->GetBufferViewport(1, &scratch_viewport_);
  DrawWorld(right_eye_view, scratch_viewport_);
  frame.Unbind();

  // Submit frame.
  frame.Submit(*viewport_list_, head_view_);

  // CheckGLError("onDrawFrame");
}

void VRWebGL_GVRMobileSDK::PrepareFramebuffer() {
  // Because we are using 2X MSAA, we can render to half as many pixels and
  // achieve similar quality.
  const gvr::Sizei recommended_size =
      HalfPixelCount(gvr_api_->GetMaximumEffectiveRenderTargetSize());
  if (render_size_.width != recommended_size.width ||
      render_size_.height != recommended_size.height) {
    // We need to resize the framebuffer.
    swapchain_->ResizeBuffer(0, recommended_size);
    render_size_ = recommended_size;
  }
}

void VRWebGL_GVRMobileSDK::OnPause() {
  gvr_api_->PauseTracking();
}

void VRWebGL_GVRMobileSDK::OnResume() {
  gvr_api_->ResumeTracking();
}

void VRWebGL_GVRMobileSDK::DrawWorld(const gvr::Mat4f& view_matrix,
                                     const gvr::BufferViewport& viewport) {
  const gvr::Recti pixel_rect =
      CalculatePixelSpaceRect(render_size_, viewport.GetSourceUv());
  
  GLfloat viewportWidth = pixel_rect.right - pixel_rect.left;
  GLfloat viewportHeight = pixel_rect.top - pixel_rect.bottom;
  glViewport(pixel_rect.left, pixel_rect.bottom,
             viewportWidth, viewportHeight);

  // CheckGLError("World drawing setup");
  
  const gvr::Mat4f perspective =
      PerspectiveMatrixFromView(viewport.GetSourceFov(), kZNear, kZFar);
  
  // Transpose oculus projection and view matrices to be opengl compatible.
  VRWebGL_transposeMatrix4((const GLfloat*)perspective.m[0], (GLfloat*)projectionMatrixTransposed.m[0]);
  VRWebGL_transposeMatrix4((const GLfloat*)view_matrix.m[0], (GLfloat*)viewMatrixTransposed.m[0]);
  
  // Setup the information before rendering current eye's frame
  VRWebGLCommandProcessor::getInstance()->setViewAndProjectionMatrices((const GLfloat*)projectionMatrixTransposed.m[0], (const GLfloat*)viewMatrixTransposed.m[0]);  
  // VRWebGLCommandProcessor::getInstance()->setFramebuffer(ovrFramebuffer_GetCurrent(frameBuffer));
  VRWebGLCommandProcessor::getInstance()->setViewport(pixel_rect.left, pixel_rect.bottom, viewportWidth, viewportHeight);
  // Render current eye's frame
  VRWebGLCommandProcessor::getInstance()->renderFrame();
}

void VRWebGLCommandProcessor::getPose(VRWebGLPose& pose)
{
    // ==============================================
    // THIS CODE IS JUST FOR REFERENCE PURPOSES! BEGIN
    //            // Position and orientation together.
    //            typedef struct ovrPosef_
    //            {
    //                ovrQuatf  Orientation;
    //                ovrVector3f Position;
    //            } ovrPosef;
    //            typedef struct ovrRigidBodyPosef_
    //            {
    //                ovrPosef  Pose;
    //                ovrVector3f AngularVelocity;
    //                ovrVector3f LinearVelocity;
    //                ovrVector3f AngularAcceleration;
    //                ovrVector3f LinearAcceleration;
    //                double    TimeInSeconds;      // Absolute time of this pose.
    //                double    PredictionInSeconds;  // Seconds this pose was predicted ahead.
    //            } ovrRigidBodyPosef;
    //
    //            // Bit flags describing the current status of sensor tracking.
    //            typedef enum
    //            {
    //                VRAPI_TRACKING_STATUS_ORIENTATION_TRACKED = 0x0001, // Orientation is currently tracked.
    //                VRAPI_TRACKING_STATUS_POSITION_TRACKED    = 0x0002, // Position is currently tracked.
    //                VRAPI_TRACKING_STATUS_HMD_CONNECTED     = 0x0080  // HMD is available & connected.
    //            } ovrTrackingStatus;
    //
    //            // Tracking state at a given absolute time.
    //            typedef struct ovrTracking_
    //            {
    //                // Sensor status described by ovrTrackingStatus flags.
    //                unsigned int    Status;
    //                // Predicted head configuration at the requested absolute time.
    //                // The pose describes the head orientation and center eye position.
    //                ovrRigidBodyPosef HeadPose;
    //            } ovrTracking;
    // THIS CODE IS JUST FOR REFERENCE PURPOSES! END
    // ==============================================    
    // pthread_mutex_lock( &ovrApp::HeadTrackingInfoMutex );
    // pose.orientation[0] = ovrApp::Pose.Pose.Orientation.x;
    // pose.orientation[1] = ovrApp::Pose.Pose.Orientation.y;
    // pose.orientation[2] = ovrApp::Pose.Pose.Orientation.z;
    // pose.orientation[3] = ovrApp::Pose.Pose.Orientation.w;
    // pthread_mutex_unlock( &ovrApp::HeadTrackingInfoMutex );     
}

void VRWebGLCommandProcessor::getEyeParameters(VRWebGLEyeParameters& eyeParameters)
{
    // pthread_mutex_lock( &ovrApp::HeadTrackingInfoMutex );
    // eyeParameters.xFOV = ovrApp::EyeParameters.xFOV;
    // eyeParameters.yFOV = ovrApp::EyeParameters.yFOV;
    // eyeParameters.width = ovrApp::EyeParameters.width;
    // eyeParameters.height = ovrApp::EyeParameters.height;
    // eyeParameters.interpupillaryDistance = ovrApp::EyeParameters.interpupillaryDistance;
    // pthread_mutex_unlock( &ovrApp::HeadTrackingInfoMutex );       
}

void VRWebGLCommandProcessor::setCameraProjectionMatrix(GLfloat* cameraProjectionMatrix)
{
  // GLfloat far = (cameraProjectionMatrix[14]) / (1 + cameraProjectionMatrix[10]);
  // GLfloat near = (far + far * cameraProjectionMatrix[10]) / (cameraProjectionMatrix[10] - 1);
  //   pthread_mutex_lock( &ovrRenderer::NewNearFarMutex );
  //   ovrRenderer::NewNear = near;
  //   ovrRenderer::NewFar = far;
  //   pthread_mutex_unlock( &ovrRenderer::NewNearFarMutex );      
}

void VRWebGLCommandProcessor::setRenderEnabled(bool flag)
{
    // ovrMessage message;
    // ovrMessage_Init( &message, MESSAGE_ON_SET_RENDER_ENABLED, MQ_WAIT_RECEIVED );
    // ovrMessage_SetIntegerParm(&message, 0, (int)(flag ? 1 : 0));
    // ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

GLuint VRWebGLCommandProcessor::newVideoTexture()
{
  //   ovrMessage message;
  //   ovrMessage_Init( &message, MESSAGE_ON_NEW_VIDEO, MQ_WAIT_PROCESSED );
  //   GLuint videoTextureId = 0;
  //   message.Result = &videoTextureId;
  //   ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
  // return videoTextureId;
  return 0;
}

void VRWebGLCommandProcessor::deleteVideoTexture(GLuint videoTextureId)
{
    // ovrMessage message;
    // ovrMessage_Init( &message, MESSAGE_ON_NEW_VIDEO, MQ_WAIT_NONE );
    // ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    // ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VRWebGLCommandProcessor::setVideoSource(GLuint videoTextureId, const std::string& src)
{
    // ovrMessage message;
    // ovrMessage_Init( &message, MESSAGE_ON_SET_VIDEO_SRC, MQ_WAIT_NONE );
    // std::string* newSrc = new std::string(src);
    // ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    // ovrMessage_SetPointerParm(&message, 1, newSrc);
    // ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VRWebGLCommandProcessor::playVideo(GLuint videoTextureId, double volume, bool loop)
{
    // ovrMessage message;
    // ovrMessage_Init( &message, MESSAGE_ON_PLAY_VIDEO, MQ_WAIT_NONE );
    // ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    // ovrMessage_SetFloatParm(&message, 1, (float)volume);
    // ovrMessage_SetIntegerParm(&message, 2, (int)(loop ? 1 : 0));
    // ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VRWebGLCommandProcessor::pauseVideo(GLuint videoTextureId)
{
    // ovrMessage message;
    // ovrMessage_Init( &message, MESSAGE_ON_PAUSE_VIDEO, MQ_WAIT_NONE );
    // ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    // ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VRWebGLCommandProcessor::setVideoVolume(GLuint videoTextureId, double volume)
{
    // ovrMessage message;
    // ovrMessage_Init( &message, MESSAGE_ON_SET_VIDEO_VOLUME, MQ_WAIT_NONE );
    // ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    // ovrMessage_SetFloatParm(&message, 1, (float)volume);
    // ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VRWebGLCommandProcessor::setVideoLoop(GLuint videoTextureId, bool loop)
{
    // ovrMessage message;
    // ovrMessage_Init( &message, MESSAGE_ON_SET_VIDEO_LOOP, MQ_WAIT_NONE );
    // ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    // ovrMessage_SetIntegerParm(&message, 1, (int)(loop ? 1 : 0));
    // ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VRWebGLCommandProcessor::setVideoCurrentTime(GLuint videoTextureId, double currentTime)
{
    // ovrMessage message;
    // ovrMessage_Init( &message, MESSAGE_ON_SET_VIDEO_CURRENT_TIME, MQ_WAIT_NONE );
    // ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    // ovrMessage_SetIntegerParm(&message, 1, (int)(currentTime * 1000.0));
    // ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

double VRWebGLCommandProcessor::getVideoCurrentTime(GLuint videoTextureId)
{
  //   ovrMessage message;
  //   ovrMessage_Init( &message, MESSAGE_ON_GET_VIDEO_CURRENT_TIME, MQ_WAIT_PROCESSED );
  //   ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
  //   int currentTime = 0;
  //   message.Result = &currentTime;
  //   ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
  // return (double)currentTime / 1000.0;
  return 0;
}

double VRWebGLCommandProcessor::getVideoDuration(GLuint videoTextureId)
{
  //   ovrMessage message;
  //   ovrMessage_Init( &message, MESSAGE_ON_GET_VIDEO_DURATION, MQ_WAIT_PROCESSED );
  //   ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
  //   int duration = 0;
  //   message.Result = &duration;
  //   ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
  // return (double)duration / 1000.0;
  return 0;
}

int VRWebGLCommandProcessor::getVideoWidth(GLuint videoTextureId)
{
  //   ovrMessage message;
  //   ovrMessage_Init( &message, MESSAGE_ON_GET_VIDEO_WIDTH, MQ_WAIT_PROCESSED );
  //   ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
  //   int width = 0;
  //   message.Result = &width;
  //   ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
  // return width;
  return 0;
}

int VRWebGLCommandProcessor::getVideoHeight(GLuint videoTextureId)
{
  //   ovrMessage message;
  //   ovrMessage_Init( &message, MESSAGE_ON_GET_VIDEO_HEIGHT, MQ_WAIT_PROCESSED );
  //   ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
  //   int height = 0;
  //   message.Result = &height;
  //   ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
  // return height;
  return 0;
}

bool VRWebGLCommandProcessor::checkVideoPrepared(GLuint videoTextureId)
{
  //   ovrMessage message;
  //   ovrMessage_Init( &message, MESSAGE_ON_CHECK_VIDEO_PREPARED, MQ_WAIT_PROCESSED );
  //   ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
  //   bool prepared = false;
  //   message.Result = &prepared;
  //   ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
  // return prepared;
  return false;
}

bool VRWebGLCommandProcessor::checkVideoEnded(GLuint videoTextureId)
{
  //   ovrMessage message;
  //   ovrMessage_Init( &message, MESSAGE_ON_CHECK_VIDEO_ENDED, MQ_WAIT_PROCESSED );
  //   ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
  //   bool ended = false;
  //   message.Result = &ended;
  //   ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
  // return ended;
  return 0;
}
// ===============================================================================================

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_org_chromium_android_1webview_shell_AwShellActivity_##method_name

namespace {

inline jlong jptr(VRWebGL_GVRMobileSDK *handle) {
  return reinterpret_cast<intptr_t>(handle);
}

inline VRWebGL_GVRMobileSDK *native(jlong ptr) {
  return reinterpret_cast<VRWebGL_GVRMobileSDK *>(ptr);
}
}  // anonymous namespace

extern "C" {

JNI_METHOD(jlong, nativeOnCreate)
(JNIEnv *env, jclass clazz, jlong native_gvr_api) {
  return jptr(
      new VRWebGL_GVRMobileSDK(reinterpret_cast<gvr_context *>(native_gvr_api)));
}

JNI_METHOD(void, nativeOnStart)
(JNIEnv *env, jclass clazz, jlong handle) {
}

JNI_METHOD(void, nativeOnPause)
(JNIEnv *env, jobject obj, jlong handle) {
  native(handle)->OnPause();
}

JNI_METHOD(void, nativeOnResume)
(JNIEnv *env, jobject obj, jlong handle) {
  native(handle)->OnResume();
}

JNI_METHOD(void, nativeOnStop)
(JNIEnv *env, jobject obj, jlong handle) {
}

JNI_METHOD(void, nativeOnDestroy)
(JNIEnv *env, jclass clazz, jlong handle) {
  delete native(handle);
}

JNI_METHOD(void, nativeOnSurfaceCreated)
(JNIEnv *env, jobject obj, jlong handle) {
  native(handle)->InitializeGl();
}

JNI_METHOD(void, nativeOnDrawFrame)
(JNIEnv *env, jobject obj, jlong handle) {
  native(handle)->DrawFrame();
}

JNI_METHOD(void, nativeOnPageStarted)
(JNIEnv *env, jobject obj) {
  VRWebGLCommandProcessor::getInstance()->reset();
}


}  // extern "C"