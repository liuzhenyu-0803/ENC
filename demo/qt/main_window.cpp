#include "main_window.h"

#include "core/default_view.h"

#include <QActionGroup>
#include <QCoreApplication>
#include <QFileDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTimer>

#include "render_widget.h"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace navscene_demo {
namespace {

namespace fs = std::filesystem;

QString BackendName(navscene::GraphicsBackend backend) {
  switch (backend) {
    case navscene::GraphicsBackend::kAuto:
      return "Auto";
    case navscene::GraphicsBackend::kVulkan:
      return "Vulkan";
    case navscene::GraphicsBackend::kDirect3D12:
      return "Direct3D12";
    case navscene::GraphicsBackend::kMetal:
      return "Metal";
    case navscene::GraphicsBackend::kOpenGL:
      return "OpenGL";
    case navscene::GraphicsBackend::kSoftware:
      return "Software";
    case navscene::GraphicsBackend::kNull:
      return "Null";
  }
  return "Unknown";
}

QString StatusText(const navscene::Status& status) {
  if (status.ok()) {
    return "OK";
  }
  return QString::fromStdString(status.message);
}

QString LoadSummary(navscene::IEngine* engine) {
  if (engine == nullptr) {
    return "Engine unavailable.";
  }

  const auto source_count = engine->Sources().Snapshot().size();
  const auto dataset_count = engine->Catalog().Snapshot().size();
  return QString("Sources: %1, datasets: %2")
      .arg(static_cast<qulonglong>(source_count))
      .arg(static_cast<qulonglong>(dataset_count));
}

std::string ToLowerAscii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::optional<fs::path> FindBundledDataRoot() {
  fs::path current = QCoreApplication::applicationDirPath().toStdString();
  for (int depth = 0; depth < 8; ++depth) {
    const fs::path candidate = current / "data";
    std::error_code ec;
    if (fs::exists(candidate, ec) && fs::is_directory(candidate, ec)) {
      return candidate;
    }

    if (!current.has_parent_path()) {
      break;
    }
    current = current.parent_path();
  }

  return std::nullopt;
}

std::optional<fs::path> FindFirstEncDataset(const fs::path& root) {
  std::vector<fs::path> datasets;
  std::error_code ec;
  fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
  if (ec) {
    return std::nullopt;
  }

  for (const auto& entry : it) {
    std::error_code file_ec;
    if (!entry.is_regular_file(file_ec) || file_ec) {
      continue;
    }

    if (ToLowerAscii(entry.path().extension().string()) == ".000") {
      datasets.push_back(entry.path());
    }
  }

  if (datasets.empty()) {
    return std::nullopt;
  }

  std::sort(datasets.begin(), datasets.end());
  return datasets.front();
}

std::optional<fs::path> FindReferenceEncDataset(const fs::path& root) {
  const fs::path preferred = root / "s57" / "GB4X0000.000";
  std::error_code ec;
  if (fs::exists(preferred, ec) && fs::is_regular_file(preferred, ec)) {
    return preferred;
  }
  return std::nullopt;
}

std::vector<fs::path> BundledDemoSamplePaths(const fs::path& root) {
  const std::vector<fs::path> preferred_paths = {
      root / "US" / "US4NY1AQ" / "US4NY1AQ.000",
      root / "US" / "US5NY1BE" / "US5NY1BE.000",
      root / "US" / "US5NY1BF" / "US5NY1BF.000",
      root / "US" / "US5NY1CE" / "US5NY1CE.000",
      root / "US" / "US5NY1CF" / "US5NY1CF.000",
  };

  std::vector<fs::path> existing_paths;
  existing_paths.reserve(preferred_paths.size());
  for (const auto& path : preferred_paths) {
    std::error_code ec;
    if (fs::exists(path, ec) && fs::is_regular_file(path, ec)) {
      existing_paths.push_back(path);
    }
  }
  return existing_paths;
}

}  // namespace

MainWindow::MainWindow() : engine_(navscene::CreateEngine(navscene::EngineConfig{})) {
  setWindowTitle("navscene-sdk Qt Demo");
  resize(1280, 800);
  CreateMenus();

  render_widget_ = new RenderWidget(engine_.get(), this);
  setCentralWidget(render_widget_);

  if (engine_ != nullptr) {
    startup_load_pending_ = true;
    setUpdatesEnabled(false);
    render_widget_->SetRenderingEnabled(false);
    render_widget_->setUpdatesEnabled(false);

    navscene::DisplayOptions display_options = engine_->MapView().GetDisplayOptions();
    display_options.show_text = false;
    display_options.show_soundings = false;
    display_options.show_meta = false;
    display_options.show_quality_of_data = false;
    display_options.display_category = navscene::DisplayCategory::kStandard;
    display_options.simplified_points = true;
    display_options.symbolized_boundaries = false;
    display_options.safety_contour_m = 10.0;
    display_options.safety_depth_m = 7.0;
    display_options.shallow_contour_m = 5.0;
    display_options.deep_contour_m = 20.0;
    engine_->MapView().SetDisplayOptions(display_options);

    statusBar()->showMessage(
        QString("Engine ready. Active backend: %1")
            .arg(BackendName(engine_->RenderSurface().GetActiveBackend())));
    QTimer::singleShot(0, this, [this]() { AutoLoadBundledSample(); });
  } else {
    auto* fallback = new QLabel("Engine creation failed.", this);
    fallback->setAlignment(Qt::AlignCenter);
    setCentralWidget(fallback);
    statusBar()->showMessage("Engine creation failed.");
  }
}

MainWindow::~MainWindow() = default;

bool MainWindow::ApplyBundledReferenceViewport(const QString& path) {
  if (engine_ == nullptr || render_widget_ == nullptr || path.isEmpty()) {
    return false;
  }

  const std::string target_path = path.toStdString();
  const auto datasets = engine_->Catalog().Snapshot();
  const auto it = std::find_if(datasets.begin(),
                               datasets.end(),
                               [&](const navscene::DatasetDescriptor& descriptor) {
                                 return descriptor.path == target_path;
                               });
  if (it == datasets.end()) {
    return false;
  }

  render_widget_->EnsureSurfaceReady();
  const auto current_viewport = engine_->MapView().GetViewport();
  const uint32_t width = current_viewport.width > 0
                             ? current_viewport.width
                             : static_cast<uint32_t>(std::max(render_widget_->width(), 0));
  const uint32_t height = current_viewport.height > 0
                              ? current_viewport.height
                              : static_cast<uint32_t>(std::max(render_widget_->height(), 0));
  if (width == 0 || height == 0) {
    return false;
  }

  auto viewport = navscene::core::MakePreferredViewport(*it, width, height);
  if (fs::path(target_path).filename() == "GB4X0000.000") {
    viewport.center = navscene::GeoPoint{
        .lat = -32.4551,
        .lon = 60.99,
    };
  }

  return engine_->MapView().SetViewport(viewport).ok();
}

void MainWindow::AutoLoadBundledSample() {
  const auto bundled_root = FindBundledDataRoot();
  if (!bundled_root.has_value()) {
    statusBar()->showMessage("Bundled data directory was not found.");
    FinishStartupLoad(false);
    return;
  }

  const auto reference_dataset = FindReferenceEncDataset(*bundled_root);
  if (reference_dataset.has_value()) {
    if (LoadEncFilePath(QString::fromStdString(reference_dataset->string()), false)) {
      ApplyBundledReferenceViewport(QString::fromStdString(reference_dataset->string()));
      statusBar()->showMessage(
          QString("Auto-loaded reference ENC sample: %1. %2")
              .arg(QString::fromStdString(reference_dataset->filename().string()))
              .arg(LoadSummary(engine_.get())));
      FinishStartupLoad(true);
      return;
    }
  }

  const auto bundled_demo_samples = BundledDemoSamplePaths(*bundled_root);
  if (!bundled_demo_samples.empty()) {
    size_t loaded_count = 0;
    for (const auto& sample : bundled_demo_samples) {
      if (LoadEncFilePath(QString::fromStdString(sample.string()), false, false)) {
        ++loaded_count;
      }
    }

    if (loaded_count > 0) {
      FitChartsToView();
      statusBar()->showMessage(
          QString("Auto-loaded bundled ENC demo set (%1 cells). %2")
              .arg(static_cast<qulonglong>(loaded_count))
              .arg(LoadSummary(engine_.get())));
      FinishStartupLoad(true);
      return;
    }
  }

  const auto first_dataset = FindFirstEncDataset(*bundled_root);
  if (!first_dataset.has_value()) {
    statusBar()->showMessage(
        QString("Bundled data directory contains no usable .000 dataset: %1")
            .arg(QString::fromStdString(bundled_root->string())));
    FinishStartupLoad(false);
    return;
  }

  if (LoadEncFilePath(QString::fromStdString(first_dataset->string()), false)) {
    statusBar()->showMessage(
        QString("Auto-loaded ENC sample: %1. %2")
            .arg(QString::fromStdString(first_dataset->filename().string()))
            .arg(LoadSummary(engine_.get())));
    FinishStartupLoad(true);
    return;
  }

  FinishStartupLoad(false);
}

void MainWindow::FinishStartupLoad(bool request_render) {
  if (!startup_load_pending_) {
    if (request_render && render_widget_ != nullptr) {
      render_widget_->RequestRenderFrame();
    }
    return;
  }

  startup_load_pending_ = false;
  if (render_widget_ != nullptr) {
    render_widget_->SetRenderingEnabled(true);
    render_widget_->setUpdatesEnabled(true);
  }
  setUpdatesEnabled(true);
  update();

  if (request_render && render_widget_ != nullptr) {
    render_widget_->RequestRenderFrame();
  }
}

void MainWindow::CreateMenus() {
  auto* file_menu = menuBar()->addMenu("&File");

  auto* open_file_action = file_menu->addAction("Open ENC File...");
  connect(open_file_action, &QAction::triggered, this, [this]() { OpenEncFile(); });

  auto* open_dir_action = file_menu->addAction("Open Chart Directory...");
  connect(open_dir_action, &QAction::triggered, this,
          [this]() { OpenChartDirectory(); });

  file_menu->addSeparator();
  auto* quit_action = file_menu->addAction("Quit");
  connect(quit_action, &QAction::triggered, this, [this]() { close(); });

  auto* view_menu = menuBar()->addMenu("&View");
  auto* fit_action = view_menu->addAction("Fit Charts");
  connect(fit_action, &QAction::triggered, this, [this]() { FitChartsToView(); });

  auto* display_menu = menuBar()->addMenu("&Display");

  auto add_toggle = [this, display_menu](const QString& text,
                                         bool navscene::DisplayOptions::*member) {
    auto* action = display_menu->addAction(text);
    action->setCheckable(true);
    if (engine_ != nullptr) {
      action->setChecked(engine_->MapView().GetDisplayOptions().*member);
    }
    connect(action, &QAction::toggled, this, [this, member, text](bool checked) {
      if (engine_ == nullptr) {
        return;
      }
      auto options = engine_->MapView().GetDisplayOptions();
      options.*member = checked;
      ApplyDisplayOptions(options, text);
    });
  };

  add_toggle("Show Text", &navscene::DisplayOptions::show_text);
  add_toggle("Show Soundings", &navscene::DisplayOptions::show_soundings);
  add_toggle("Show Lights", &navscene::DisplayOptions::show_lights);
  add_toggle("Show Metadata", &navscene::DisplayOptions::show_meta);
  add_toggle("Show Quality Of Data", &navscene::DisplayOptions::show_quality_of_data);
  display_menu->addSeparator();
  add_toggle("Simplified Point Symbols", &navscene::DisplayOptions::simplified_points);
  add_toggle("Symbolized Boundaries", &navscene::DisplayOptions::symbolized_boundaries);

  display_menu->addSeparator();
  auto* display_category_menu = display_menu->addMenu("Display Category");
  auto* display_category_group = new QActionGroup(this);
  display_category_group->setExclusive(true);
  const auto add_display_category =
      [this, display_category_menu, display_category_group](const QString& text,
                                                            navscene::DisplayCategory category) {
        auto* action = display_category_menu->addAction(text);
        action->setCheckable(true);
        if (engine_ != nullptr) {
          action->setChecked(engine_->MapView().GetDisplayOptions().display_category == category);
        }
        display_category_group->addAction(action);
        connect(action, &QAction::triggered, this, [this, category, text]() {
          if (engine_ == nullptr) {
            return;
          }
          auto options = engine_->MapView().GetDisplayOptions();
          options.display_category = category;
          ApplyDisplayOptions(options, text);
        });
      };

  add_display_category("Base", navscene::DisplayCategory::kBase);
  add_display_category("Standard", navscene::DisplayCategory::kStandard);
  add_display_category("All", navscene::DisplayCategory::kAll);

  display_menu->addSeparator();
  auto* color_scheme_menu = display_menu->addMenu("Color Scheme");
  auto* color_scheme_group = new QActionGroup(this);
  color_scheme_group->setExclusive(true);
  const auto add_color_scheme =
      [this, color_scheme_menu, color_scheme_group](const QString& text,
                                                    navscene::ColorScheme color_scheme) {
        auto* action = color_scheme_menu->addAction(text);
        action->setCheckable(true);
        if (engine_ != nullptr) {
          action->setChecked(engine_->MapView().GetDisplayOptions().color_scheme ==
                             color_scheme);
        }
        color_scheme_group->addAction(action);
        connect(action, &QAction::triggered, this, [this, color_scheme, text]() {
          if (engine_ == nullptr) {
            return;
          }
          auto options = engine_->MapView().GetDisplayOptions();
          options.color_scheme = color_scheme;
          ApplyDisplayOptions(options, text);
        });
      };

  add_color_scheme("Day", navscene::ColorScheme::kDay);
  add_color_scheme("Dusk", navscene::ColorScheme::kDusk);
  add_color_scheme("Night", navscene::ColorScheme::kNight);
}

void MainWindow::ApplyDisplayOptions(const navscene::DisplayOptions& options,
                                     const QString& reason) {
  if (engine_ == nullptr || render_widget_ == nullptr) {
    return;
  }

  const auto status = engine_->MapView().SetDisplayOptions(options);
  if (!status.ok()) {
    ShowOperationError("Display Options", status);
    statusBar()->showMessage(QString("%1 failed: %2").arg(reason).arg(StatusText(status)));
    return;
  }

  statusBar()->showMessage(
      QString("%1 applied. %2").arg(reason).arg(LoadSummary(engine_.get())));
  render_widget_->RequestRenderFrame();
}

bool MainWindow::LoadEncFilePath(const QString& path, bool show_errors, bool fit_to_view) {
  if (engine_ == nullptr || path.isEmpty()) {
    return false;
  }

  navscene::SourceDescriptor descriptor;
  descriptor.id = path.toStdString();
  descriptor.uri = path.toStdString();
  descriptor.type = navscene::SourceType::kChartDataset;
  descriptor.format = navscene::DatasetFormat::kS57;

  const auto status = engine_->Sources().RegisterSource(descriptor);
  if (!status.ok()) {
    if (show_errors) {
      ShowOperationError("Open ENC File", status);
    }
    statusBar()->showMessage(
        QString("Register ENC file: %1. %2")
            .arg(StatusText(status))
            .arg(LoadSummary(engine_.get())));
    return false;
  }

  if (fit_to_view) {
    FitChartsToView();
  }
  statusBar()->showMessage(
      QString("Register ENC file: %1. %2")
          .arg(StatusText(status))
          .arg(LoadSummary(engine_.get())));
  return true;
}

void MainWindow::OpenEncFile() {
  if (engine_ == nullptr) {
    statusBar()->showMessage("Engine is not available.");
    return;
  }

  const QString path =
      QFileDialog::getOpenFileName(this, "Open ENC File", QString(), "ENC Files (*.000)");
  if (path.isEmpty()) {
    return;
  }

  LoadEncFilePath(path, true);
}

void MainWindow::OpenChartDirectory() {
  if (engine_ == nullptr) {
    statusBar()->showMessage("Engine is not available.");
    return;
  }

  const QString path =
      QFileDialog::getExistingDirectory(this, "Open Chart Directory", QString());
  if (path.isEmpty()) {
    return;
  }

  const auto add_status = engine_->Catalog().AddChartDirectory(path.toStdString());
  if (!add_status.ok()) {
    ShowOperationError("Open Chart Directory", add_status);
    statusBar()->showMessage(
        QString("Add chart directory failed: %1").arg(StatusText(add_status)));
    return;
  }

  const auto scan_status = engine_->Catalog().Rescan();
  if (!scan_status.ok()) {
    ShowOperationError("Scan Chart Directory", scan_status);
  } else {
    FitChartsToView();
  }
  statusBar()->showMessage(
      QString("Chart directory scan: %1. %2")
          .arg(StatusText(scan_status))
          .arg(LoadSummary(engine_.get())));
}

void MainWindow::FitChartsToView() {
  if (engine_ == nullptr || render_widget_ == nullptr) {
    return;
  }

  render_widget_->EnsureSurfaceReady();
  const auto status = engine_->MapView().FitToData();
  if (!status.ok()) {
    if (status.code == navscene::StatusCode::kNotFound) {
      QMessageBox::information(this,
                               "Fit Charts",
                               "No valid chart coverage is available yet.");
      statusBar()->showMessage("Fit charts skipped: no valid chart coverage is available.");
      return;
    }
    ShowOperationError("Fit Charts", status);
  }
  statusBar()->showMessage(
      QString("Fit charts: %1. %2").arg(StatusText(status)).arg(LoadSummary(engine_.get())));
  render_widget_->RequestRenderFrame();
}

void MainWindow::ShowOperationError(const QString& title, const navscene::Status& status) {
  QMessageBox::warning(this,
                       title,
                       QString("Operation failed.\n\n%1").arg(StatusText(status)));
}

}  // namespace navscene_demo
