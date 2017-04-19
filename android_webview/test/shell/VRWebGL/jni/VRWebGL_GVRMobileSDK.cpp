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
// #include "modules/vr/VRWebGLGamepad.h"

#include "public/platform/WebGamepad.h"

class VRWebGL_GVRMobileSDK {
 public:

  static VRWebGL_GVRMobileSDK* instance;

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
   * Process the controller input.
   */
  void ProcessControllerInput();

  /*
   * Resume the controller api if needed.
   */
  void ResumeControllerApiAsNeeded();

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

  std::shared_ptr<blink::WebGamepad> getGamepadCopy();

  void getPose(VRWebGLPose& pose);

  void getEyeParameters(const std::string& eye, VRWebGLEyeParameters& eyeParameters);

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
                 const gvr::BufferViewport& viewport, gvr::Eye eye);

  std::unique_ptr<gvr::GvrApi> gvr_api_;
  std::unique_ptr<gvr::BufferViewportList> viewport_list_;
  std::unique_ptr<gvr::SwapChain> swapchain_;
  gvr::BufferViewport scratch_viewport_;

  gvr::Mat4f head_view_;
  GLfloat head_view_orientation_[4];
  gvr::Mat4f projectionMatrixTransposed_;
  gvr::Mat4f viewMatrixTransposed_;
  gvr::Sizei render_size_;
  gvr::Rectf fovLeft_;
  gvr::Rectf fovRight_;
  GLfloat viewportWidth_;
  GLfloat viewportHeight_;
  GLfloat interpupillaryDistance_;

  std::unique_ptr<gvr::ControllerApi> gvr_controller_api_;
  gvr::ControllerState gvr_controller_state_;
  std::shared_ptr<blink::WebGamepad> gamepad_;
  std::shared_ptr<blink::WebGamepad> gamepadCopy_;
  std::mutex gamepadMutex_;
  std::mutex poseMutex_;
  std::mutex eyeParametersMutex_;

  gvr::ViewerType gvr_viewer_type_;
};

VRWebGL_GVRMobileSDK* VRWebGL_GVRMobileSDK::instance = 0;


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
static float kZNear = 0.001f;
static float kZFar = 100000.0f;

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
      gvr_controller_api_(nullptr),
      gvr_viewer_type_(gvr_api_->GetViewerType()) {

  if (instance != 0)
  {
    LOGE("There can only be one.");
    abort();
  }
  instance = this;
  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_DAYDREAM) {
    LOGD("VRWebGL_GVRMobileSDK: DAYDREAM");
    gamepad_.reset(new blink::WebGamepad());
    // TODO: Fix this. We should be able to use the real 16 bit type for the id string.
    strcpy((char*)gamepad_->id, "Daydream Controller");
    gamepad_->buttonsLength = 2;
    gamepad_->axesLength = 2;
    gamepad_->pose.notNull = true;
    gamepad_->pose.orientation.notNull = true;
    gamepad_->pose.hasOrientation = true;
    gamepad_->pose.hasPosition = false;
  } else {
    LOGE("Unexpected viewer type.");
  }
}

VRWebGL_GVRMobileSDK::~VRWebGL_GVRMobileSDK() {
  instance = 0;
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

void VRWebGL_GVRMobileSDK::ResumeControllerApiAsNeeded() {
  switch (gvr_viewer_type_) {
    case GVR_VIEWER_TYPE_CARDBOARD:
      gvr_controller_api_.reset();
      break;
    case GVR_VIEWER_TYPE_DAYDREAM:
      if (!gvr_controller_api_) {
        // Initialized controller api.
        gvr_controller_api_.reset(new gvr::ControllerApi);
        CHECK(gvr_controller_api_);
        CHECK(gvr_controller_api_->Init(gvr::ControllerApi::DefaultOptions(),
                                        gvr_api_->cobj()));
      }
      gvr_controller_api_->Resume();
      break;
    default:
      LOGE("unexpected viewer type.");
      break;
  }
}

void VRWebGL_GVRMobileSDK::ProcessControllerInput() {
  const gvr::UserPrefs userPrefs = gvr_api_->GetUserPrefs();
  const int32_t hand = userPrefs.GetControllerHandedness();
  const int old_status = gvr_controller_state_.GetApiStatus();
  const int old_connection_state = gvr_controller_state_.GetConnectionState();

  // Read current controller state.
  gvr_controller_state_.Update(*gvr_controller_api_);

  // Print new API status and connection state, if they changed.
  if (gvr_controller_state_.GetApiStatus() != old_status ||
      gvr_controller_state_.GetConnectionState() != old_connection_state) {
    LOGD("VRWebGL_GVRMobileSDK: controller API status: %s, connection state: %s",
         gvr_controller_api_status_to_string(
             gvr_controller_state_.GetApiStatus()),
         gvr_controller_connection_state_to_string(
             gvr_controller_state_.GetConnectionState()));
  }

  gamepadMutex_.lock();
  {
    gvr::ControllerQuat controllerPose = gvr_controller_state_.GetOrientation();
    gamepad_->pose.orientation.x = controllerPose.qx;
    gamepad_->pose.orientation.y = controllerPose.qy;
    gamepad_->pose.orientation.z = controllerPose.qz;
    gamepad_->pose.orientation.w = controllerPose.qw;
    gamepad_->buttons[0].pressed = gvr_controller_state_.GetButtonState(GVR_CONTROLLER_BUTTON_CLICK);

    gamepad_->buttons[0].value = gamepad_->buttons[0].pressed ? 1 : 0;
    gamepad_->buttons[1].pressed = gvr_controller_state_.GetButtonState(GVR_CONTROLLER_BUTTON_APP);
    gamepad_->buttons[1].value = gamepad_->buttons[1].pressed ? 1 : 0;
    gamepad_->buttons[0].touched = gvr_controller_state_.IsTouching();
    gvr::ControllerVec2 controllerTouchPos = gvr_controller_state_.GetTouchPos();
    gamepad_->axes[0] = controllerTouchPos.x;
    gamepad_->axes[1] = controllerTouchPos.y;
    gamepad_->hand = hand == GVR_CONTROLLER_RIGHT_HANDED ? blink::GamepadHandRight : blink::GamepadHandLeft;
    // LOGD("gamepad_: pose(%f, %f, %f, %f), button0(%f, %s), button1(%f, %s), x(%f), y(%f)", gamepad_->pose.orientation.x, gamepad_->pose.orientation.y, gamepad_->pose.orientation.z, gamepad_->pose.orientation.w, gamepad_->buttons[0].value, (gamepad_->buttons[0].pressed ? "YES" : "NO"), gamepad_->buttons[1].value, (gamepad_->buttons[1].pressed ? "YES" : "NO"), gamepad_->axes[0], gamepad_->axes[1]);
  }
  gamepadMutex_.unlock();
}

std::shared_ptr<blink::WebGamepad> VRWebGL_GVRMobileSDK::getGamepadCopy()
{
  gamepadMutex_.lock();
  {
    if (gamepad_)
    {
      if (!gamepadCopy_)
      {
        gamepadCopy_.reset(new blink::WebGamepad(*gamepad_.get()));
      }
      else
      {
        *(gamepadCopy_.get()) = *(gamepad_.get());
      }
    }
  }
  gamepadMutex_.unlock();
  return gamepadCopy_;
}

void VRWebGL_GVRMobileSDK::getPose(VRWebGLPose& pose)
{
  poseMutex_.lock();
  {
    memcpy(pose.orientation, head_view_orientation_, sizeof(GLfloat) * 4);
  }
  poseMutex_.unlock();
}

void VRWebGL_GVRMobileSDK::getEyeParameters(const std::string& eye, VRWebGLEyeParameters& eyeParameters)
{
  eyeParametersMutex_.lock();
  {
    if (eye == "left")
    {
      eyeParameters.upDegrees = fovLeft_.top;
      eyeParameters.downDegrees = fovLeft_.bottom;
      eyeParameters.leftDegrees = fovLeft_.left;
      eyeParameters.rightDegrees = fovLeft_.right;
    }
    else if (eye == "right")
    {
      eyeParameters.upDegrees = fovRight_.top;
      eyeParameters.downDegrees = fovRight_.bottom;
      eyeParameters.leftDegrees = fovRight_.left;
      eyeParameters.rightDegrees = fovRight_.right;
    }
    eyeParameters.width = viewportWidth_;
    eyeParameters.height = viewportHeight_;
    eyeParameters.interpupillaryDistance = interpupillaryDistance_;
  }
  eyeParametersMutex_.unlock();
}

void VRWebGL_GVRMobileSDK::DrawFrame() {
  VRWebGLCommandProcessor::getInstance()->update();

  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_DAYDREAM) {
    ProcessControllerInput();
  }

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

  // Calculate the pose quaternion from the head matrix.
  poseMutex_.lock();
  {
    VRWebGL_quaternionFromMatrix4(&(head_view_.m[0][0]), head_view_orientation_);
  }
  poseMutex_.unlock();

  gvr::Mat4f left_eye_matrix = gvr_api_->GetEyeFromHeadMatrix(GVR_LEFT_EYE);
  gvr::Mat4f right_eye_matrix = gvr_api_->GetEyeFromHeadMatrix(GVR_RIGHT_EYE);

  eyeParametersMutex_.lock();
  {
    // Not ideal way of calculating the interpupillary distance as only X is taken into account.
    // LOGD("%s: %f, %f, %f", "left", left_eye_matrix.m[0][3], left_eye_matrix.m[1][3], left_eye_matrix.m[2][3]);
    // LOGD("%s: %f, %f, %f", "right", right_eye_matrix.m[0][3], right_eye_matrix.m[1][3], right_eye_matrix.m[2][3]);
    interpupillaryDistance_ = fabs(left_eye_matrix.m[0][3]) + fabs(right_eye_matrix.m[0][3]);
  }
  eyeParametersMutex_.unlock();

  gvr::Mat4f left_eye_view = MatrixMul(left_eye_matrix, head_view_);
  gvr::Mat4f right_eye_view = MatrixMul(right_eye_matrix, head_view_);

  viewport_list_->SetToRecommendedBufferViewports();

  // glEnable(GL_DEPTH_TEST);
  // glEnable(GL_CULL_FACE);
  // glDisable(GL_SCISSOR_TEST);
  // glDisable(GL_BLEND);

  // Draw the world.
  frame.BindBuffer(0);
  // glClearColor(0.1f, 0.1f, 0.1f, 0.5f);  // Dark background so text shows up.
  // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  viewport_list_->GetBufferViewport(0, &scratch_viewport_);
  DrawWorld(left_eye_view, scratch_viewport_, GVR_LEFT_EYE);
  viewport_list_->GetBufferViewport(1, &scratch_viewport_);
  DrawWorld(right_eye_view, scratch_viewport_, GVR_RIGHT_EYE);
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
  if (gvr_controller_api_) gvr_controller_api_->Pause();
}

void VRWebGL_GVRMobileSDK::OnResume() {
  gvr_api_->ResumeTracking();
  gvr_viewer_type_ = gvr_api_->GetViewerType();
  ResumeControllerApiAsNeeded();
}

void VRWebGL_GVRMobileSDK::DrawWorld(const gvr::Mat4f& view_matrix,
                                     const gvr::BufferViewport& viewport, gvr::Eye eye) {
  const gvr::Recti pixel_rect =
      CalculatePixelSpaceRect(render_size_, viewport.GetSourceUv());
  
  GLfloat viewportWidth = pixel_rect.right - pixel_rect.left;
  GLfloat viewportHeight = pixel_rect.top - pixel_rect.bottom;
  glViewport(pixel_rect.left, pixel_rect.bottom,
             viewportWidth, viewportHeight);

  // CheckGLError("World drawing setup");

  gvr::Rectf fov = viewport.GetSourceFov();

  eyeParametersMutex_.lock();
  {
    if (eye == GVR_LEFT_EYE)
    {
      fovLeft_ = fov;
    }
    else if (eye == GVR_RIGHT_EYE)
    {
      fovRight_ = fov;
    }
    viewportWidth_ = viewportWidth;
    viewportHeight_ = viewportHeight_;
  }
  eyeParametersMutex_.unlock();
  
  const gvr::Mat4f perspective =
      PerspectiveMatrixFromView(fov, kZNear, kZFar);
  
  // Transpose oculus projection and view matrices to be opengl compatible.
  VRWebGL_transposeMatrix4((const GLfloat*)perspective.m[0], (GLfloat*)projectionMatrixTransposed_.m[0]);
  VRWebGL_transposeMatrix4((const GLfloat*)view_matrix.m[0], (GLfloat*)viewMatrixTransposed_.m[0]);
  
  // Setup the information before rendering current eye's frame
  VRWebGLCommandProcessor::getInstance()->setViewAndProjectionMatrices((const GLfloat*)projectionMatrixTransposed_.m[0], (const GLfloat*)viewMatrixTransposed_.m[0]);  
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

  if (VRWebGL_GVRMobileSDK::instance != 0)
  {
    VRWebGL_GVRMobileSDK::instance->getPose(pose);
  }
}

void VRWebGLCommandProcessor::getEyeParameters(const std::string& eye, VRWebGLEyeParameters& eyeParameters)
{
  if (VRWebGL_GVRMobileSDK::instance != 0)
  {
    VRWebGL_GVRMobileSDK::instance->getEyeParameters(eye, eyeParameters);
  }
}

void VRWebGLCommandProcessor::setCameraProjectionMatrix(GLfloat* cameraProjectionMatrix)
{
  // TODO: Use a mutex? We should but should we? I mean, near and far values do not usually change and even if they do, the visual artifact won't be noticeable.
  GLfloat far = (cameraProjectionMatrix[14]) / (1 + cameraProjectionMatrix[10]);
  GLfloat near = (far + far * cameraProjectionMatrix[10]) / (cameraProjectionMatrix[10] - 1);
  if (far != kZFar) kZFar = far;
  if (near != kZNear) kZNear = near;
}

void VRWebGLCommandProcessor::setRenderEnabled(bool flag)
{
    // ovrMessage message;
    // ovrMessage_Init( &message, MESSAGE_ON_SET_RENDER_ENABLED, MQ_WAIT_RECEIVED );
    // ovrMessage_SetIntegerParm(&message, 0, (int)(flag ? 1 : 0));
    // ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

std::shared_ptr<blink::WebGamepad> VRWebGLCommandProcessor::getGamepad()
{
  std::shared_ptr<blink::WebGamepad> gamepad;
  if (VRWebGL_GVRMobileSDK::instance != 0)
  {
    gamepad = VRWebGL_GVRMobileSDK::instance->getGamepadCopy();
  }
  return gamepad;
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
  VRWebGLCommandProcessor::getInstance()->setupJNI(env, obj);
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