#pragma once

#include <QByteArray>
#include <QEvent>
#include <QPaintEngine>
#include <QPoint>
#include <QWidget>

#include "navscene/navscene.h"

namespace navscene_demo {

class RenderWidget final : public QWidget {
 public:
  explicit RenderWidget(navscene::IEngine* engine, QWidget* parent = nullptr);
  ~RenderWidget() override;
  void EnsureSurfaceReady();
  void RequestRenderFrame();
  void SetRenderingEnabled(bool enabled);

 protected:
  QPaintEngine* paintEngine() const override;
  bool event(QEvent* event) override;
  bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

 private:
  navscene::NativeSurfaceDesc BuildSurfaceDesc() const;
  void AttachSurfaceIfNeeded();
  void RequestRender();
  void RenderNow();

  navscene::IEngine* engine_ = nullptr;
  bool surface_attached_ = false;
  bool dragging_ = false;
  bool rendering_enabled_ = true;
  bool render_requested_ = false;
  QPoint last_drag_pos_;
  navscene::Status last_render_status_{};
};

}  // namespace navscene_demo
