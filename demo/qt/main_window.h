#pragma once

#include <QMainWindow>

#include <memory>

#include "navscene/navscene.h"

namespace navscene_demo {

class RenderWidget;

class MainWindow final : public QMainWindow {
 public:
  MainWindow();
  ~MainWindow() override;

 private:
  void AutoLoadBundledSample();
  bool ApplyBundledReferenceViewport(const QString& path);
  void FinishStartupLoad(bool request_render);
  void CreateMenus();
  void ApplyDisplayOptions(const navscene::DisplayOptions& options, const QString& reason);
  bool LoadEncFilePath(const QString& path, bool show_errors, bool fit_to_view = true);
  void OpenEncFile();
  void OpenChartDirectory();
  void FitChartsToView();
  void ShowOperationError(const QString& title, const navscene::Status& status);

  std::unique_ptr<navscene::IEngine> engine_;
  RenderWidget* render_widget_ = nullptr;
  bool startup_load_pending_ = false;
};

}  // namespace navscene_demo
