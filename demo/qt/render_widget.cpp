#include "render_widget.h"

#include <QHideEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QWheelEvent>

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

namespace navscene_demo {

RenderWidget::RenderWidget(navscene::IEngine* engine, QWidget* parent)
    : QWidget(parent), engine_(engine) {
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_NoSystemBackground);
  setAttribute(Qt::WA_OpaquePaintEvent);
  setAttribute(Qt::WA_PaintOnScreen);
  setAutoFillBackground(false);
}

RenderWidget::~RenderWidget() {
  if (engine_ != nullptr && surface_attached_) {
    engine_->RenderSurface().DetachSurface();
  }
}

void RenderWidget::EnsureSurfaceReady() {
  if (engine_ == nullptr) {
    return;
  }

  winId();
  AttachSurfaceIfNeeded();
  if (surface_attached_ && width() > 0 && height() > 0) {
    last_render_status_ =
        engine_->RenderSurface().Resize(static_cast<uint32_t>(width()),
                                        static_cast<uint32_t>(height()));
  }
}

void RenderWidget::RequestRenderFrame() {
  RequestRender();
}

void RenderWidget::SetRenderingEnabled(bool enabled) {
  rendering_enabled_ = enabled;
  if (rendering_enabled_) {
    RequestRender();
  }
}

QPaintEngine* RenderWidget::paintEngine() const {
  return nullptr;
}

bool RenderWidget::event(QEvent* event) {
  if (event != nullptr && event->type() == QEvent::Paint) {
    RenderNow();
    event->accept();
    return true;
  }

  return QWidget::event(event);
}

bool RenderWidget::nativeEvent(const QByteArray& eventType, void* message, long* result) {
#if defined(Q_OS_WIN)
  if (message != nullptr) {
    const auto* native_message = static_cast<MSG*>(message);
    if (native_message->message == WM_ERASEBKGND) {
      if (result != nullptr) {
        *result = 1;
      }
      return true;
    }
  }
#else
  Q_UNUSED(eventType);
  Q_UNUSED(message);
  Q_UNUSED(result);
#endif

  return QWidget::nativeEvent(eventType, message, result);
}

void RenderWidget::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  EnsureSurfaceReady();
  RequestRender();
}

void RenderWidget::hideEvent(QHideEvent* event) {
  QWidget::hideEvent(event);
}

void RenderWidget::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  if (engine_ != nullptr && surface_attached_) {
    last_render_status_ =
        engine_->RenderSurface().Resize(static_cast<uint32_t>(width()),
                                        static_cast<uint32_t>(height()));
  }
  RequestRender();
}

void RenderWidget::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    dragging_ = true;
    last_drag_pos_ = event->pos();
    event->accept();
    return;
  }

  QWidget::mousePressEvent(event);
}

void RenderWidget::mouseMoveEvent(QMouseEvent* event) {
  if (dragging_ && engine_ != nullptr) {
    const QPoint delta = event->pos() - last_drag_pos_;
    last_drag_pos_ = event->pos();
    last_render_status_ =
        engine_->Input().PanPixels(static_cast<double>(delta.x()),
                                   static_cast<double>(delta.y()));
    RenderNow();
    event->accept();
    return;
  }

  QWidget::mouseMoveEvent(event);
}

void RenderWidget::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    dragging_ = false;
    event->accept();
    return;
  }

  QWidget::mouseReleaseEvent(event);
}

void RenderWidget::wheelEvent(QWheelEvent* event) {
  if (engine_ == nullptr) {
    QWidget::wheelEvent(event);
    return;
  }

  const QPoint num_degrees = event->angleDelta();
  if (num_degrees.y() == 0) {
    QWidget::wheelEvent(event);
    return;
  }

  const double delta = static_cast<double>(num_degrees.y()) / 120.0;
  last_render_status_ =
      engine_->Input().ZoomAt(event->position().x(), event->position().y(), delta);
  RenderNow();
  event->accept();
}

navscene::NativeSurfaceDesc RenderWidget::BuildSurfaceDesc() const {
  navscene::NativeSurfaceDesc desc;
  desc.type = navscene::SurfaceType::kWindow;
  desc.platform = navscene::NativePlatform::kWin32;
  desc.window_handle = reinterpret_cast<void*>(static_cast<uintptr_t>(winId()));
  desc.width = static_cast<uint32_t>(width());
  desc.height = static_cast<uint32_t>(height());
  desc.dpi_scale = static_cast<float>(devicePixelRatioF());
  return desc;
}

void RenderWidget::AttachSurfaceIfNeeded() {
  if (engine_ == nullptr || surface_attached_) {
    return;
  }

  const auto status = engine_->RenderSurface().AttachSurface(BuildSurfaceDesc());
  if (status.ok()) {
    surface_attached_ = true;
    last_render_status_ = status;
  } else {
    last_render_status_ = status;
  }
}

void RenderWidget::RequestRender() {
  if (!rendering_enabled_) {
    return;
  }

  if (render_requested_) {
    return;
  }

  render_requested_ = true;
  QTimer::singleShot(0, this, [this]() { RenderNow(); });
}

void RenderWidget::RenderNow() {
  render_requested_ = false;
  if (!rendering_enabled_ || engine_ == nullptr || !isVisible()) {
    return;
  }

  EnsureSurfaceReady();
  if (!surface_attached_ || width() <= 0 || height() <= 0) {
    return;
  }

  last_render_status_ = engine_->RenderSurface().RenderFrame();
}

}  // namespace navscene_demo
