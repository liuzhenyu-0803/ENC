#pragma once

#include "portrayal/catalog.h"

namespace navscene::render {

using Rgb8 = portrayal::Rgb8;

struct AreaPaintStyle {
  Rgb8 fill;
  Rgb8 stroke;
  bool visible = true;
};

struct LinePaintStyle {
  Rgb8 stroke;
  int width = 1;
  portrayal::StrokePatternKind pattern = portrayal::StrokePatternKind::kSolid;
  bool visible = true;
};

struct PointPaintStyle {
  portrayal::PointSymbolKind kind = portrayal::PointSymbolKind::kCircle;
  Rgb8 fill;
  Rgb8 stroke;
  int radius = 2;
  bool visible = true;
};

AreaPaintStyle ResolveAreaPaintStyle(const PolygonPrimitive& polygon);
LinePaintStyle ResolveLinePaintStyle(const PolylinePrimitive& polyline);
PointPaintStyle ResolvePointPaintStyle(const PointPrimitive& point);

}  // namespace navscene::render
