#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace navscene {

enum class StatusCode {
  kOk = 0,
  kInvalidArgument,
  kNotInitialized,
  kIoError,
  kUnsupported,
  kNotFound,
  kInternalError,
};

struct Status {
  StatusCode code = StatusCode::kOk;
  std::string message;

  [[nodiscard]] bool ok() const { return code == StatusCode::kOk; }
};

enum class DatasetFormat {
  kUnknown = 0,
  kS57,
  kS100,
};

enum class ProductFamily {
  kUnknown = 0,
  kEnc,
};

enum class SourceType {
  kUnknown = 0,
  kChartDataset,
  kRasterTiles,
  kSatelliteTiles,
  kVectorOverlay,
  kStreamingOverlay,
};

enum class LayerType {
  kUnknown = 0,
  kBasemap,
  kChart,
  kOverlay,
  kAnnotation,
  kTerrain,
};

enum class SceneMode {
  k2D = 0,
  k2_5D,
  k3D,
};

enum class ColorScheme {
  kDay = 0,
  kDusk,
  kNight,
};

enum class DisplayCategory {
  kBase = 0,
  kStandard,
  kAll,
};

enum class BlendMode {
  kNormal = 0,
  kAdditive,
  kMultiply,
};

enum class GraphicsBackend {
  kAuto = 0,
  kVulkan,
  kDirect3D12,
  kMetal,
  kOpenGL,
  kSoftware,
  kNull,
};

enum class RenderMode {
  kUnknown = 0,
  kGpuNativeGeometry,
  kGpuRasterUpload,
  kSoftwareRaster,
  kNull,
};

enum class BackendOwnership {
  kEngineOwned = 0,
  kHostOwned,
};

enum class NativePlatform {
  kUnknown = 0,
  kWin32,
  kAndroid,
  kXcb,
  kWayland,
  kMacOS,
  kIOS,
};

enum class SurfaceType {
  kWindow = 0,
  kOffscreen,
};

struct GeoPoint {
  double lat = 0.0;
  double lon = 0.0;
};

struct GeoBox {
  double min_lat = 0.0;
  double min_lon = 0.0;
  double max_lat = 0.0;
  double max_lon = 0.0;
};

struct Viewport {
  GeoPoint center;
  double scale_ppm = 1.0;
  double rotation_rad = 0.0;
  uint32_t width = 0;
  uint32_t height = 0;
};

struct CameraState {
  GeoPoint center;
  double scale_ppm = 1.0;
  double heading_rad = 0.0;
  double pitch_rad = 0.0;
  double roll_rad = 0.0;
  double camera_height_m = 0.0;
};

struct DisplayOptions {
  ColorScheme color_scheme = ColorScheme::kDay;
  DisplayCategory display_category = DisplayCategory::kStandard;
  bool show_text = true;
  bool show_soundings = true;
  bool show_lights = true;
  bool show_meta = false;
  bool show_quality_of_data = false;
  bool simplified_points = false;
  bool symbolized_boundaries = true;
  double safety_contour_m = 30.0;
  double safety_depth_m = 30.0;
  double shallow_contour_m = 2.0;
  double deep_contour_m = 30.0;
};

struct BackendConfig {
  GraphicsBackend preferred_backend = GraphicsBackend::kAuto;
  BackendOwnership ownership = BackendOwnership::kEngineOwned;
  bool allow_fallback = true;
};

struct EngineConfig {
  bool enable_validation = false;
  bool enable_debug_labels = false;
  BackendConfig backend;
  std::string cache_dir;
};

struct EngineCapabilities {
  bool supports_s57 = true;
  bool supports_s100 = false;
  bool supports_raster_tiles = false;
  bool supports_satellite_tiles = false;
  bool supports_vector_overlays = false;
  bool supports_3d = false;
  bool supports_multi_view = false;
  bool supports_time_controller = false;
  bool supports_offscreen_rendering = false;
  bool supports_host_owned_backend_context = false;
  std::vector<GraphicsBackend> supported_backends;
  std::vector<NativePlatform> supported_platforms;
};

struct DatasetDescriptor {
  std::string id;
  std::string path;
  DatasetFormat format = DatasetFormat::kUnknown;
  ProductFamily family = ProductFamily::kUnknown;
  SourceType source_type = SourceType::kUnknown;
  GeoBox coverage;
  GeoPoint default_view_center;
  bool has_default_view_center = false;
  int default_display_scale = 0;
  int compilation_scale = 0;
};

struct SourceDescriptor {
  std::string id;
  std::string uri;
  SourceType type = SourceType::kUnknown;
  DatasetFormat format = DatasetFormat::kUnknown;
};

struct LayerDescriptor {
  std::string id;
  LayerType type = LayerType::kUnknown;
  bool visible = true;
  float opacity = 1.0f;
  int z_order = 0;
  double min_scale_ppm = 0.0;
  double max_scale_ppm = 0.0;
  BlendMode blend_mode = BlendMode::kNormal;
  bool pickable = true;
  bool interactive = false;
  std::vector<std::string> source_ids;
};

using AttributeList = std::vector<std::pair<std::string, std::string>>;

struct FeatureInfo {
  std::string dataset_id;
  std::string feature_id;
  std::string feature_class;
  GeoPoint location;
  AttributeList attributes;
};

struct NativeSurfaceDesc {
  SurfaceType type = SurfaceType::kWindow;
  NativePlatform platform = NativePlatform::kUnknown;
  void* window_handle = nullptr;
  void* display_handle = nullptr;
  void* view_handle = nullptr;
  void* layer_handle = nullptr;
  uint32_t width = 0;
  uint32_t height = 0;
  float dpi_scale = 1.0f;
};

struct FrameStats {
  double cpu_frame_ms = 0.0;
  double gpu_frame_ms = 0.0;
  uint64_t rendered_primitives = 0;
  uint64_t visible_datasets = 0;
  RenderMode render_mode = RenderMode::kUnknown;
};

struct TimeState {
  int64_t unix_time_ms = 0;
  double playback_rate = 1.0;
  bool paused = true;
};

class ICatalog {
 public:
  virtual ~ICatalog() = default;

  virtual Status AddChartDirectory(std::string_view path) = 0;
  virtual Status RemoveChartDirectory(std::string_view path) = 0;
  virtual Status Rescan() = 0;
  virtual std::vector<DatasetDescriptor> Snapshot() const = 0;
};

class ISourceManager {
 public:
  virtual ~ISourceManager() = default;

  virtual Status RegisterSource(const SourceDescriptor& descriptor) = 0;
  virtual Status UnregisterSource(std::string_view source_id) = 0;
  virtual std::vector<SourceDescriptor> Snapshot() const = 0;
};

class IMapView {
 public:
  virtual ~IMapView() = default;

  virtual Status SetViewport(const Viewport& viewport) = 0;
  virtual Status FitToData() = 0;
  virtual Viewport GetViewport() const = 0;
  virtual Status SetDisplayOptions(const DisplayOptions& options) = 0;
  virtual DisplayOptions GetDisplayOptions() const = 0;
};

class ISceneController {
 public:
  virtual ~ISceneController() = default;

  virtual Status SetSceneMode(SceneMode mode) = 0;
  virtual SceneMode GetSceneMode() const = 0;
  virtual Status SetCameraState(const CameraState& camera) = 0;
  virtual CameraState GetCameraState() const = 0;
};

class ILayerManager {
 public:
  virtual ~ILayerManager() = default;

  virtual Status CreateLayer(const LayerDescriptor& descriptor) = 0;
  virtual Status DestroyLayer(std::string_view layer_id) = 0;
  virtual Status SetLayerVisible(std::string_view layer_id, bool visible) = 0;
  virtual Status SetLayerOpacity(std::string_view layer_id, float opacity) = 0;
  virtual Status SetLayerOrder(std::string_view layer_id, int z_order) = 0;
  virtual Status AttachSource(std::string_view layer_id,
                              std::string_view source_id) = 0;
  virtual Status DetachSource(std::string_view layer_id,
                              std::string_view source_id) = 0;
  virtual std::vector<LayerDescriptor> Snapshot() const = 0;
};

class ITimeController {
 public:
  virtual ~ITimeController() = default;

  virtual Status SetTimeState(const TimeState& state) = 0;
  virtual TimeState GetTimeState() const = 0;
};

class IRenderSurface {
 public:
  virtual ~IRenderSurface() = default;

  virtual Status AttachSurface(const NativeSurfaceDesc& desc) = 0;
  virtual Status DetachSurface() = 0;
  virtual Status Resize(uint32_t width, uint32_t height) = 0;
  virtual Status RenderFrame() = 0;
  virtual GraphicsBackend GetActiveBackend() const = 0;
  virtual NativeSurfaceDesc GetAttachedSurface() const = 0;
};

class IInputController {
 public:
  virtual ~IInputController() = default;

  virtual Status PanPixels(double dx, double dy) = 0;
  virtual Status ZoomAt(double x, double y, double delta) = 0;
};

class IDiagnostics {
 public:
  virtual ~IDiagnostics() = default;

  virtual FrameStats GetLastFrameStats() const = 0;
};

class IEngine {
 public:
  virtual ~IEngine() = default;

  virtual ICatalog& Catalog() = 0;
  virtual ISourceManager& Sources() = 0;
  virtual IMapView& MapView() = 0;
  virtual ISceneController& Scene() = 0;
  virtual ILayerManager& Layers() = 0;
  virtual ITimeController& Time() = 0;
  virtual IRenderSurface& RenderSurface() = 0;
  virtual IInputController& Input() = 0;
  virtual IDiagnostics& Diagnostics() = 0;

  virtual Status PickFeature(uint32_t x, uint32_t y, FeatureInfo* out) = 0;
  virtual std::vector<DatasetDescriptor> GetVisibleDatasets() const = 0;
  virtual EngineCapabilities GetCapabilities() const = 0;
};

std::unique_ptr<IEngine> CreateEngine(const EngineConfig& config);

}  // namespace navscene
