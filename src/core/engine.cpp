#include "core/chart_selection.h"
#include "core/default_view.h"
#include "core/engine.h"

#include "core/logging.h"
#include "data/discovery.h"
#include "data/s57/reader.h"
#include "geo/mercator_projection.h"
#include "portrayal/engine.h"
#include "render/backend.h"
#include "render/chart_scene_builder.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace navscene {
namespace {

struct EngineState {
  FrameStats last_frame_stats{};
  std::vector<data::s57::DatasetInfo> catalog_datasets;
  std::unordered_map<std::string, data::s57::DatasetInfo> source_datasets;
  std::unordered_map<std::string, LayerDescriptor> layers;
  render::ChartScene scene;
  portrayal::PortrayalScene portrayal_scene;
  Viewport viewport{};
  DisplayOptions display_options{};
  bool viewport_initialized = false;
  bool viewport_user_controlled = false;
};

const char* ToString(GraphicsBackend backend) {
  switch (backend) {
    case GraphicsBackend::kAuto:
      return "Auto";
    case GraphicsBackend::kVulkan:
      return "Vulkan";
    case GraphicsBackend::kDirect3D12:
      return "Direct3D12";
    case GraphicsBackend::kMetal:
      return "Metal";
    case GraphicsBackend::kOpenGL:
      return "OpenGL";
    case GraphicsBackend::kSoftware:
      return "Software";
    case GraphicsBackend::kNull:
      return "Null";
  }
  return "Unknown";
}

Status OkStatus() { return {}; }

Status ErrorStatus(StatusCode code, std::string message) {
  return Status{code, std::move(message)};
}

void MergeDatasets(std::vector<DatasetDescriptor>* out,
                   std::unordered_set<std::string>* seen_paths,
                   const std::vector<data::s57::DatasetInfo>& datasets) {
  for (const auto& dataset : datasets) {
    if (seen_paths->insert(dataset.descriptor.path).second) {
      out->push_back(dataset.descriptor);
    }
  }
}

void MergeDatasets(std::vector<DatasetDescriptor>* out,
                   std::unordered_set<std::string>* seen_paths,
                   const std::unordered_map<std::string, data::s57::DatasetInfo>& datasets) {
  for (const auto& [_, dataset] : datasets) {
    if (seen_paths->insert(dataset.descriptor.path).second) {
      out->push_back(dataset.descriptor);
    }
  }
}

std::vector<DatasetDescriptor> BuildKnownDatasetSnapshot(const EngineState* state) {
  if (state == nullptr) {
    return {};
  }

  std::vector<DatasetDescriptor> snapshot;
  snapshot.reserve(state->catalog_datasets.size() + state->source_datasets.size());

  std::unordered_set<std::string> seen_paths;
  MergeDatasets(&snapshot, &seen_paths, state->catalog_datasets);
  MergeDatasets(&snapshot, &seen_paths, state->source_datasets);
  return snapshot;
}

std::vector<const data::s57::DatasetInfo*> BuildVisibleDatasetInfos(const EngineState* state) {
  std::vector<const data::s57::DatasetInfo*> visible_datasets;
  if (state == nullptr) {
    return visible_datasets;
  }

  std::unordered_set<std::string> seen_paths;
  auto append_dataset = [&](const data::s57::DatasetInfo& dataset) {
    if (seen_paths.insert(dataset.descriptor.path).second) {
      visible_datasets.push_back(&dataset);
    }
  };

  if (state->layers.empty()) {
    for (const auto& dataset : state->catalog_datasets) {
      append_dataset(dataset);
    }
    for (const auto& [_, dataset] : state->source_datasets) {
      append_dataset(dataset);
    }
    return visible_datasets;
  }

  std::vector<const LayerDescriptor*> ordered_layers;
  ordered_layers.reserve(state->layers.size());
  for (const auto& [_, layer] : state->layers) {
    if (layer.visible) {
      ordered_layers.push_back(&layer);
    }
  }

  std::sort(ordered_layers.begin(),
            ordered_layers.end(),
            [](const LayerDescriptor* lhs, const LayerDescriptor* rhs) {
              if (lhs->z_order != rhs->z_order) {
                return lhs->z_order < rhs->z_order;
              }
              return lhs->id < rhs->id;
            });

  for (const auto* layer : ordered_layers) {
    if (layer == nullptr) {
      continue;
    }

    if (layer->type == LayerType::kChart && layer->source_ids.empty()) {
      for (const auto& dataset : state->catalog_datasets) {
        append_dataset(dataset);
      }
    }

    for (const auto& source_id : layer->source_ids) {
      const auto source_it = state->source_datasets.find(source_id);
      if (source_it != state->source_datasets.end()) {
        append_dataset(source_it->second);
      }
    }
  }

  return visible_datasets;
}

std::vector<DatasetDescriptor> BuildVisibleDatasetSnapshot(const EngineState* state) {
  std::vector<DatasetDescriptor> snapshot;
  for (const auto* dataset : BuildVisibleDatasetInfos(state)) {
    if (dataset != nullptr) {
      snapshot.push_back(dataset->descriptor);
    }
  }
  return snapshot;
}

uint64_t CountVisibleDatasets(const EngineState* state) {
  return static_cast<uint64_t>(BuildVisibleDatasetInfos(state).size());
}

bool HasValidCoverage(const GeoBox& coverage) {
  return geo::HasValidCoverage(coverage);
}

GeoBox BuildMergedCoverage(const EngineState* state) {
  GeoBox coverage{};
  bool has_coverage = false;

  auto merge_dataset = [&](const data::s57::DatasetInfo& dataset) {
    if (!HasValidCoverage(dataset.descriptor.coverage)) {
      return;
    }

    if (!has_coverage) {
      coverage = dataset.descriptor.coverage;
      has_coverage = true;
      return;
    }

    coverage.min_lat = std::min(coverage.min_lat, dataset.descriptor.coverage.min_lat);
    coverage.min_lon = std::min(coverage.min_lon, dataset.descriptor.coverage.min_lon);
    coverage.max_lat = std::max(coverage.max_lat, dataset.descriptor.coverage.max_lat);
    coverage.max_lon = std::max(coverage.max_lon, dataset.descriptor.coverage.max_lon);
  };

  if (state != nullptr) {
    for (const auto* dataset : BuildVisibleDatasetInfos(state)) {
      if (dataset != nullptr) {
        merge_dataset(*dataset);
      }
    }
  }

  return coverage;
}

void FitViewportToVisibleData(uint32_t width, uint32_t height, EngineState* state) {
  if (state == nullptr) {
    return;
  }

  const auto visible_datasets = BuildVisibleDatasetInfos(state);
  if (visible_datasets.size() == 1 && visible_datasets.front() != nullptr) {
    state->viewport = core::MakePreferredViewport(visible_datasets.front()->descriptor, width, height);
    state->viewport_initialized = true;
    return;
  }

  const GeoBox coverage = BuildMergedCoverage(state);
  if (!HasValidCoverage(coverage)) {
    return;
  }

  state->viewport = geo::MakeFittedViewport(coverage, width, height);
  state->viewport_initialized = true;
}

Status FitViewportToCurrentData(EngineState* state) {
  if (state == nullptr) {
    return ErrorStatus(StatusCode::kInternalError, "Viewport state is not available.");
  }

  const GeoBox coverage = BuildMergedCoverage(state);
  if (!HasValidCoverage(coverage)) {
    return ErrorStatus(StatusCode::kNotFound,
                       "No valid chart coverage is available for fit.");
  }
  if (state->viewport.width == 0 || state->viewport.height == 0) {
    return ErrorStatus(StatusCode::kNotInitialized,
                       "Cannot fit viewport before a render surface size is known.");
  }

  FitViewportToVisibleData(state->viewport.width, state->viewport.height, state);
  state->viewport_user_controlled = false;
  return OkStatus();
}

void EnsureViewportReady(EngineState* state, const NativeSurfaceDesc& surface) {
  if (state == nullptr) {
    return;
  }

  const GeoBox coverage = BuildMergedCoverage(state);
  if (!HasValidCoverage(coverage)) {
    return;
  }

  if (state->viewport.width != surface.width || state->viewport.height != surface.height) {
    state->viewport.width = surface.width;
    state->viewport.height = surface.height;
  }

  if (!state->viewport_initialized || !state->viewport_user_controlled) {
    FitViewportToVisibleData(surface.width, surface.height, state);
  }
}

double ComputeViewportPixelsPerMeter(const EngineState* state) {
  const GeoBox coverage = BuildMergedCoverage(state);
  if (state == nullptr || !HasValidCoverage(coverage) || state->viewport.width == 0 ||
      state->viewport.height == 0) {
    return 0.0;
  }

  return geo::BuildViewportProjection(coverage, state->viewport, 48).pixels_per_meter;
}

void RefreshScene(EngineState* state) {
  if (state == nullptr) {
    return;
  }

  const auto visible_datasets = BuildVisibleDatasetInfos(state);
  const auto selected = core::SelectChartsForViewport(visible_datasets,
                                                      BuildMergedCoverage(state),
                                                      state->viewport);
  render::ChartScene scene;
  for (const auto* dataset : selected.selected_datasets) {
    if (dataset != nullptr) {
      render::AppendDatasetToChartScene(*dataset, &scene);
    }
  }

  auto portrayal_settings = portrayal::MakeDisplaySettings(state->display_options);
  portrayal_settings.estimated_display_scale = selected.estimated_display_scale;
  state->scene = std::move(scene);
  state->portrayal_scene =
      portrayal::BuildPortrayalScene(state->scene, portrayal_settings);
}

Status LoadS57Dataset(const std::filesystem::path& path,
                      SourceType source_type,
                      data::s57::DatasetInfo* out) {
  const auto status =
      data::s57::LoadDataset(data::s57::ReadRequest{path, source_type}, out);
  if (!status.ok()) {
    return status;
  }

  internal::Log(internal::LogLevel::kInfo,
                std::string("Loaded S-57 dataset via reader '") +
                    out->reader_name + "': " + out->descriptor.path);
  return status;
}

class Catalog final : public ICatalog {
 public:
  explicit Catalog(EngineState* state) : state_(state) {}

  Status AddChartDirectory(std::string_view path) override {
    if (path.empty()) {
      return ErrorStatus(StatusCode::kInvalidArgument,
                         "Chart directory path must not be empty.");
    }

    const std::filesystem::path directory{std::string(path)};
    if (!data::PathExists(directory)) {
      internal::Log(internal::LogLevel::kError,
                    "Chart directory registration failed: path does not exist.");
      return ErrorStatus(StatusCode::kNotFound,
                         "Chart directory path does not exist.");
    }
    if (!data::IsDirectory(directory)) {
      internal::Log(internal::LogLevel::kError,
                    "Chart directory registration failed: path is not a directory.");
      return ErrorStatus(StatusCode::kInvalidArgument,
                         "Chart directory path must point to a directory.");
    }

    const std::string normalized = data::NormalizePathString(directory.string());
    if (std::find(chart_directories_.begin(), chart_directories_.end(), normalized) !=
        chart_directories_.end()) {
      internal::Log(internal::LogLevel::kInfo,
                    std::string("Chart directory already registered: ") + normalized);
      return OkStatus();
    }

    chart_directories_.push_back(normalized);
    internal::Log(internal::LogLevel::kInfo,
                  std::string("Chart directory registered: ") + normalized);
    return OkStatus();
  }

  Status RemoveChartDirectory(std::string_view path) override {
    const auto it = std::remove(chart_directories_.begin(),
                                chart_directories_.end(),
                                data::NormalizePathString(path));
    if (it == chart_directories_.end()) {
      return ErrorStatus(StatusCode::kNotFound,
                         "Chart directory was not registered.");
    }

    chart_directories_.erase(it, chart_directories_.end());
    return RefreshCatalogSnapshot(false);
  }

  Status Rescan() override { return RefreshCatalogSnapshot(true); }

  std::vector<DatasetDescriptor> Snapshot() const override {
    return BuildKnownDatasetSnapshot(state_);
  }

 private:
  Status RefreshCatalogSnapshot(bool empty_is_error) {
    if (state_ == nullptr) {
      return ErrorStatus(StatusCode::kInternalError,
                         "Catalog state is not available.");
    }

    const auto report = data::DiscoverS57Datasets(chart_directories_);
    for (const auto& directory : report.missing_directories) {
      internal::Log(internal::LogLevel::kWarning,
                    std::string("Skipping missing chart directory: ") + directory);
    }
    for (const auto& directory : report.failed_directories) {
      internal::Log(internal::LogLevel::kWarning,
                    std::string("Failed to scan chart directory: ") + directory);
    }

    std::vector<data::s57::DatasetInfo> loaded_datasets;
    loaded_datasets.reserve(report.datasets.size());
    for (const auto& descriptor : report.datasets) {
      data::s57::DatasetInfo dataset_info;
      const auto load_status =
          LoadS57Dataset(std::filesystem::path(descriptor.path),
                         descriptor.source_type,
                         &dataset_info);
      if (!load_status.ok()) {
        internal::Log(internal::LogLevel::kWarning,
                      std::string("Failed to load discovered S-57 dataset: ") +
                          descriptor.path + ". " + load_status.message);
        continue;
      }
      loaded_datasets.push_back(std::move(dataset_info));
    }

    state_->catalog_datasets = std::move(loaded_datasets);
    RefreshScene(state_);
    if (state_->catalog_datasets.empty()) {
      internal::Log(internal::LogLevel::kWarning,
                    "Catalog rescan completed without recognized S-57 datasets.");
      if (empty_is_error) {
        return ErrorStatus(
            StatusCode::kNotFound,
            "No S-57 datasets were found in the registered chart directories.");
      }
      return OkStatus();
    }

    internal::Log(
        internal::LogLevel::kInfo,
        std::string("Catalog rescan recognized ") +
            std::to_string(state_->catalog_datasets.size()) + " dataset(s).");
    return OkStatus();
  }

  EngineState* state_ = nullptr;
  std::vector<std::string> chart_directories_;
};

class SourceManager final : public ISourceManager {
 public:
  explicit SourceManager(EngineState* state) : state_(state) {}

  Status RegisterSource(const SourceDescriptor& descriptor) override {
    if (descriptor.id.empty()) {
      return ErrorStatus(StatusCode::kInvalidArgument,
                         "Source id must not be empty.");
    }

    SourceDescriptor normalized = descriptor;
    if (descriptor.type == SourceType::kChartDataset) {
      std::string normalized_uri;
      const auto validation =
          data::ValidateChartDatasetSource(descriptor, &normalized_uri);
      if (!validation.ok()) {
        internal::Log(
            internal::LogLevel::kError,
            std::string("Chart dataset registration failed for source '") +
                descriptor.id + "': " + validation.message);
        return validation;
      }

      normalized.uri = std::move(normalized_uri);
      if (normalized.format == DatasetFormat::kUnknown) {
        normalized.format = DatasetFormat::kS57;
      }
    }

    if (state_ != nullptr) {
      if (normalized.type == SourceType::kChartDataset) {
        data::s57::DatasetInfo dataset_info;
        const auto load_status =
            LoadS57Dataset(std::filesystem::path(normalized.uri),
                           normalized.type,
                           &dataset_info);
        if (!load_status.ok()) {
          internal::Log(internal::LogLevel::kError,
                        std::string("Chart dataset load failed for source '") +
                            normalized.id + "': " + load_status.message);
          return load_status;
        }
        state_->source_datasets[normalized.id] = std::move(dataset_info);
        RefreshScene(state_);
      } else {
        state_->source_datasets.erase(normalized.id);
        RefreshScene(state_);
      }
    }

    sources_[normalized.id] = normalized;
    internal::Log(internal::LogLevel::kInfo,
                  std::string("Source registered: ") + normalized.id);
    return OkStatus();
  }

  Status UnregisterSource(std::string_view source_id) override {
    if (sources_.erase(std::string(source_id)) == 0) {
      return ErrorStatus(StatusCode::kNotFound, "Source not found.");
    }

    if (state_ != nullptr) {
      state_->source_datasets.erase(std::string(source_id));
      RefreshScene(state_);
    }
    internal::Log(internal::LogLevel::kInfo,
                  std::string("Source unregistered: ") + std::string(source_id));
    return OkStatus();
  }

  std::vector<SourceDescriptor> Snapshot() const override {
    std::vector<SourceDescriptor> snapshot;
    snapshot.reserve(sources_.size());
    for (const auto& [_, descriptor] : sources_) {
      snapshot.push_back(descriptor);
    }
    return snapshot;
  }

 private:
  EngineState* state_ = nullptr;
  std::unordered_map<std::string, SourceDescriptor> sources_;
};

class MapView final : public IMapView {
 public:
  explicit MapView(EngineState* state) : state_(state) {}

  Status SetViewport(const Viewport& viewport) override {
    if (state_ == nullptr) {
      return ErrorStatus(StatusCode::kInternalError, "Viewport state is not available.");
    }

    state_->viewport = viewport;
    state_->viewport_initialized = true;
    state_->viewport_user_controlled = true;
    return OkStatus();
  }

  Status FitToData() override { return FitViewportToCurrentData(state_); }

  Viewport GetViewport() const override {
    return state_ == nullptr ? Viewport{} : state_->viewport;
  }

  Status SetDisplayOptions(const DisplayOptions& options) override {
    if (state_ == nullptr) {
      return ErrorStatus(StatusCode::kInternalError,
                         "Display option state is not available.");
    }
    state_->display_options = options;
    RefreshScene(state_);
    return OkStatus();
  }

  DisplayOptions GetDisplayOptions() const override {
    return state_ == nullptr ? DisplayOptions{} : state_->display_options;
  }

 private:
  EngineState* state_ = nullptr;
};

class SceneController final : public ISceneController {
 public:
  Status SetSceneMode(SceneMode mode) override {
    scene_mode_ = mode;
    return OkStatus();
  }

  SceneMode GetSceneMode() const override { return scene_mode_; }

  Status SetCameraState(const CameraState& camera) override {
    camera_ = camera;
    return OkStatus();
  }

  CameraState GetCameraState() const override { return camera_; }

 private:
  SceneMode scene_mode_ = SceneMode::k2D;
  CameraState camera_{};
};

class LayerManager final : public ILayerManager {
 public:
  explicit LayerManager(EngineState* state) : state_(state) {}

  Status CreateLayer(const LayerDescriptor& descriptor) override {
    if (descriptor.id.empty()) {
      return ErrorStatus(StatusCode::kInvalidArgument,
                         "Layer id must not be empty.");
    }

    if (state_ == nullptr) {
      return ErrorStatus(StatusCode::kInternalError, "Layer state is not available.");
    }

    state_->layers[descriptor.id] = descriptor;
    RefreshScene(state_);
    return OkStatus();
  }

  Status DestroyLayer(std::string_view layer_id) override {
    if (state_ == nullptr) {
      return ErrorStatus(StatusCode::kInternalError, "Layer state is not available.");
    }

    if (state_->layers.erase(std::string(layer_id)) == 0) {
      return ErrorStatus(StatusCode::kNotFound, "Layer not found.");
    }
    RefreshScene(state_);
    return OkStatus();
  }

  Status SetLayerVisible(std::string_view layer_id, bool visible) override {
    auto* layer = FindLayer(layer_id);
    if (layer == nullptr) {
      return ErrorStatus(StatusCode::kNotFound, "Layer not found.");
    }
    layer->visible = visible;
    RefreshScene(state_);
    return OkStatus();
  }

  Status SetLayerOpacity(std::string_view layer_id, float opacity) override {
    auto* layer = FindLayer(layer_id);
    if (layer == nullptr) {
      return ErrorStatus(StatusCode::kNotFound, "Layer not found.");
    }
    layer->opacity = opacity;
    RefreshScene(state_);
    return OkStatus();
  }

  Status SetLayerOrder(std::string_view layer_id, int z_order) override {
    auto* layer = FindLayer(layer_id);
    if (layer == nullptr) {
      return ErrorStatus(StatusCode::kNotFound, "Layer not found.");
    }
    layer->z_order = z_order;
    RefreshScene(state_);
    return OkStatus();
  }

  Status AttachSource(std::string_view layer_id,
                      std::string_view source_id) override {
    auto* layer = FindLayer(layer_id);
    if (layer == nullptr) {
      return ErrorStatus(StatusCode::kNotFound, "Layer not found.");
    }
    if (std::find(layer->source_ids.begin(), layer->source_ids.end(), source_id) ==
        layer->source_ids.end()) {
      layer->source_ids.emplace_back(source_id);
      RefreshScene(state_);
    }
    return OkStatus();
  }

  Status DetachSource(std::string_view layer_id,
                      std::string_view source_id) override {
    auto* layer = FindLayer(layer_id);
    if (layer == nullptr) {
      return ErrorStatus(StatusCode::kNotFound, "Layer not found.");
    }
    const auto it = std::remove(layer->source_ids.begin(),
                                layer->source_ids.end(),
                                std::string(source_id));
    if (it == layer->source_ids.end()) {
      return ErrorStatus(StatusCode::kNotFound, "Source not attached.");
    }
    layer->source_ids.erase(it, layer->source_ids.end());
    RefreshScene(state_);
    return OkStatus();
  }

  std::vector<LayerDescriptor> Snapshot() const override {
    std::vector<LayerDescriptor> snapshot;
    if (state_ != nullptr) {
      snapshot.reserve(state_->layers.size());
      for (const auto& [_, descriptor] : state_->layers) {
        snapshot.push_back(descriptor);
      }
    }
    return snapshot;
  }

 private:
  LayerDescriptor* FindLayer(std::string_view layer_id) {
    if (state_ == nullptr) {
      return nullptr;
    }

    const auto it = state_->layers.find(std::string(layer_id));
    return it == state_->layers.end() ? nullptr : &it->second;
  }

  EngineState* state_ = nullptr;
};

class TimeController final : public ITimeController {
 public:
  Status SetTimeState(const TimeState& state) override {
    state_ = state;
    return OkStatus();
  }

  TimeState GetTimeState() const override { return state_; }

 private:
  TimeState state_{};
};

class RenderSurface final : public IRenderSurface {
 public:
  explicit RenderSurface(EngineState* state, GraphicsBackend requested_backend)
      : state_(state),
        backend_(render::CreateRendererBackend(requested_backend)),
        active_backend_(backend_ == nullptr ? GraphicsBackend::kNull
                                            : backend_->backend_type()) {
    if (backend_ == nullptr) {
      backend_ = render::CreateRendererBackend(GraphicsBackend::kNull);
      active_backend_ = backend_ == nullptr ? GraphicsBackend::kNull
                                            : backend_->backend_type();
    }
  }

  Status AttachSurface(const NativeSurfaceDesc& desc) override {
    if (desc.type == SurfaceType::kWindow && desc.window_handle == nullptr) {
      internal::Log(internal::LogLevel::kError,
                    "Render surface attach failed: native window handle is null.");
      return ErrorStatus(StatusCode::kInvalidArgument,
                         "Native window handle must not be null.");
    }
    if (desc.width == 0 || desc.height == 0) {
      internal::Log(internal::LogLevel::kWarning,
                    "Render surface attached with zero-sized extent.");
    }

    const auto attach_status = backend_->AttachSurface(desc);
    if (!attach_status.ok()) {
      internal::Log(internal::LogLevel::kError,
                    std::string("Render backend attach failed. Backend=") +
                        ToString(active_backend_) + ". " + attach_status.message);
      return attach_status;
    }

    surface_ = desc;
    attached_ = true;
    EnsureViewportReady(state_, surface_);
    internal::Log(internal::LogLevel::kInfo,
                  std::string("Render surface attached. Backend=") +
                      ToString(active_backend_) + ".");
    return OkStatus();
  }

  Status DetachSurface() override {
    if (backend_ != nullptr) {
      const auto detach_status = backend_->DetachSurface();
      if (!detach_status.ok()) {
        internal::Log(internal::LogLevel::kWarning,
                      std::string("Render backend detach reported an error. Backend=") +
                          ToString(active_backend_) + ". " + detach_status.message);
      }
    }
    attached_ = false;
    surface_ = {};
    internal::Log(internal::LogLevel::kInfo, "Render surface detached.");
    return OkStatus();
  }

  Status Resize(uint32_t width, uint32_t height) override {
    if (!attached_) {
      internal::Log(internal::LogLevel::kError,
                    "Render surface resize requested before attach.");
      return ErrorStatus(StatusCode::kNotInitialized,
                         "Cannot resize before a surface is attached.");
    }
    surface_.width = width;
    surface_.height = height;

    const auto resize_status = backend_->Resize(surface_);
    if (!resize_status.ok()) {
      internal::Log(internal::LogLevel::kError,
                    std::string("Render backend resize failed. Backend=") +
                        ToString(active_backend_) + ". " + resize_status.message);
      return resize_status;
    }

    EnsureViewportReady(state_, surface_);
    internal::Log(internal::LogLevel::kInfo, "Render surface resized.");
    return OkStatus();
  }

  Status RenderFrame() override {
    if (!attached_) {
      internal::Log(internal::LogLevel::kError,
                    "Render frame requested before surface attach.");
      return ErrorStatus(StatusCode::kNotInitialized,
                         "Cannot render before a surface is attached.");
    }

    const auto frame_begin = std::chrono::steady_clock::now();
    EnsureViewportReady(state_, surface_);
    RefreshScene(state_);

    if (state_ != nullptr) {
      const auto render_status =
          backend_->RenderFrame(state_->portrayal_scene,
                                BuildMergedCoverage(state_),
                                state_->viewport,
                                surface_);
      if (!render_status.ok()) {
        internal::Log(internal::LogLevel::kError,
                      std::string("Render backend frame failed. Backend=") +
                          ToString(active_backend_) + ". " + render_status.message);
        return render_status;
      }
    }

    const auto frame_end = std::chrono::steady_clock::now();
    if (state_ != nullptr) {
      state_->last_frame_stats.cpu_frame_ms =
          std::chrono::duration<double, std::milli>(frame_end - frame_begin)
              .count();
      state_->last_frame_stats.gpu_frame_ms = 0.0;
      state_->last_frame_stats.rendered_primitives =
          state_->portrayal_scene.stats.point_command_count +
          state_->portrayal_scene.stats.line_command_count +
          state_->portrayal_scene.stats.area_command_count;
      state_->last_frame_stats.visible_datasets = CountVisibleDatasets(state_);
      state_->last_frame_stats.render_mode = backend_->last_render_mode();
    }
    return OkStatus();
  }

  GraphicsBackend GetActiveBackend() const override { return active_backend_; }

  NativeSurfaceDesc GetAttachedSurface() const override { return surface_; }

 private:
  EngineState* state_ = nullptr;
  bool attached_ = false;
  std::unique_ptr<render::IRendererBackend> backend_;
  GraphicsBackend active_backend_ = GraphicsBackend::kNull;
  NativeSurfaceDesc surface_{};
};

class InputController final : public IInputController {
 public:
  explicit InputController(EngineState* state) : state_(state) {}

  Status PanPixels(double dx, double dy) override {
    if (state_ == nullptr) {
      return ErrorStatus(StatusCode::kInternalError, "Viewport state is not available.");
    }

    EnsureViewportReady(state_, NativeSurfaceDesc{
                                    .type = SurfaceType::kOffscreen,
                                    .width = state_->viewport.width,
                                    .height = state_->viewport.height,
                                });
    const double pixels_per_meter = ComputeViewportPixelsPerMeter(state_);
    if (pixels_per_meter <= 0.0) {
      return ErrorStatus(StatusCode::kNotInitialized, "Viewport is not ready for pan.");
    }

    const GeoBox coverage = BuildMergedCoverage(state_);
    const auto center_pixel =
        geo::GeoToPixel(coverage, state_->viewport, state_->viewport.center, 48);
    state_->viewport.center = geo::PixelToGeo(
        coverage, state_->viewport, center_pixel.first - dx, center_pixel.second - dy, 48);
    state_->viewport_user_controlled = true;
    return OkStatus();
  }

  Status ZoomAt(double x, double y, double delta) override {
    if (state_ == nullptr) {
      return ErrorStatus(StatusCode::kInternalError, "Viewport state is not available.");
    }

    if (!state_->viewport_initialized) {
      return ErrorStatus(StatusCode::kNotInitialized, "Viewport is not ready for zoom.");
    }

    const GeoBox coverage = BuildMergedCoverage(state_);
    const GeoPoint anchor_before = geo::PixelToGeo(coverage, state_->viewport, x, y, 48);
    const double zoom_factor = std::pow(1.2, delta);
    state_->viewport.scale_ppm =
        std::clamp(state_->viewport.scale_ppm * zoom_factor, 0.25, 256.0);
    const auto anchor_after_pixel =
        geo::GeoToPixel(coverage, state_->viewport, anchor_before, 48);
    const auto center_pixel =
        geo::GeoToPixel(coverage, state_->viewport, state_->viewport.center, 48);
    state_->viewport.center = geo::PixelToGeo(coverage,
                                              state_->viewport,
                                              center_pixel.first + (anchor_after_pixel.first - x),
                                              center_pixel.second + (anchor_after_pixel.second - y),
                                              48);
    state_->viewport_user_controlled = true;
    return OkStatus();
  }

 private:
  EngineState* state_ = nullptr;
};

class Diagnostics final : public IDiagnostics {
 public:
  explicit Diagnostics(const EngineState* state) : state_(state) {}

  FrameStats GetLastFrameStats() const override {
    return state_ == nullptr ? FrameStats{} : state_->last_frame_stats;
  }

 private:
  const EngineState* state_ = nullptr;
};

class Engine final : public IEngine {
 public:
  explicit Engine(EngineConfig config)
      : config_(std::move(config)),
        catalog_(&state_),
        sources_(&state_),
        map_view_(&state_),
        render_surface_(&state_, SelectBackend(config_)) {
    capabilities_.supported_backends = SupportedBackends();
    capabilities_.supported_platforms = {
        NativePlatform::kWin32, NativePlatform::kXcb, NativePlatform::kWayland};
    capabilities_.supports_offscreen_rendering = true;
    capabilities_.supports_s57 = true;
    internal::Log(internal::LogLevel::kInfo,
                  std::string("Engine active backend: ") +
                      ToString(render_surface_.GetActiveBackend()) + ".");
    internal::Log(internal::LogLevel::kInfo,
                  std::string("Preferred S-57 reader: ") +
                      data::s57::PreferredReaderName() + ".");
    internal::Log(internal::LogLevel::kInfo,
                  "Engine created with stub services for phase 1.");
  }

  ICatalog& Catalog() override { return catalog_; }
  ISourceManager& Sources() override { return sources_; }
  IMapView& MapView() override { return map_view_; }
  ISceneController& Scene() override { return scene_; }
  ILayerManager& Layers() override { return layers_; }
  ITimeController& Time() override { return time_; }
  IRenderSurface& RenderSurface() override { return render_surface_; }
  IInputController& Input() override { return input_; }
  IDiagnostics& Diagnostics() override { return diagnostics_; }

  Status PickFeature(uint32_t, uint32_t, FeatureInfo* out) override {
    if (out == nullptr) {
      return ErrorStatus(StatusCode::kInvalidArgument,
                         "FeatureInfo output pointer must not be null.");
    }
    *out = {};
    return ErrorStatus(StatusCode::kUnsupported,
                       "Feature picking is not implemented in phase 1.");
  }

  std::vector<DatasetDescriptor> GetVisibleDatasets() const override {
    return BuildVisibleDatasetSnapshot(&state_);
  }

  EngineCapabilities GetCapabilities() const override { return capabilities_; }

 private:
  static std::vector<GraphicsBackend> SupportedBackends() {
    std::vector<GraphicsBackend> backends;
#if defined(NAVSCENE_HAS_VULKAN)
    backends.push_back(GraphicsBackend::kVulkan);
#endif
    backends.push_back(GraphicsBackend::kSoftware);
    backends.push_back(GraphicsBackend::kNull);
    return backends;
  }

  static bool IsSupportedBackend(GraphicsBackend backend) {
    const auto supported_backends = SupportedBackends();
    return std::find(supported_backends.begin(), supported_backends.end(), backend) !=
           supported_backends.end();
  }

  static GraphicsBackend DefaultAutoBackend() {
#if defined(NAVSCENE_HAS_VULKAN)
    return GraphicsBackend::kVulkan;
#else
    return GraphicsBackend::kSoftware;
#endif
  }

  static GraphicsBackend SelectBackend(const EngineConfig& config) {
    if (config.backend.preferred_backend == GraphicsBackend::kAuto) {
      return DefaultAutoBackend();
    }

    if (IsSupportedBackend(config.backend.preferred_backend)) {
      return config.backend.preferred_backend;
    }

    if (!config.backend.allow_fallback) {
      internal::Log(
          internal::LogLevel::kWarning,
          std::string("Preferred backend '") +
              ToString(config.backend.preferred_backend) +
              "' is not implemented and fallback is disabled. Using Null backend.");
      return GraphicsBackend::kNull;
    }

    internal::Log(
        internal::LogLevel::kWarning,
        std::string("Preferred backend '") +
            ToString(config.backend.preferred_backend) +
            "' is not implemented yet. Falling back to Software backend.");
    return GraphicsBackend::kSoftware;
  }

  EngineConfig config_;
  EngineState state_{};
  EngineCapabilities capabilities_{};
  class Catalog catalog_;
  class SourceManager sources_;
  class MapView map_view_;
  class SceneController scene_{};
  class LayerManager layers_{&state_};
  class TimeController time_{};
  class RenderSurface render_surface_;
  class InputController input_{&state_};
  class Diagnostics diagnostics_{&state_};
};

}  // namespace

namespace internal {

std::unique_ptr<IEngine> CreateEngineImpl(const EngineConfig& config) {
  return std::make_unique<Engine>(config);
}

}  // namespace internal

std::unique_ptr<IEngine> CreateEngine(const EngineConfig& config) {
  return internal::CreateEngineImpl(config);
}

}  // namespace navscene
