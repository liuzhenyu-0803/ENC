#include <ogrsf_frmts.h>
#include <iostream>

int main() {
  OGRRegisterAll();
  OGRPoint p(1, 2);
  const OGRGeometry& g = p;
  auto* gc = g.toGeometryCollection();
  std::cout << (gc == nullptr ? "null" : "nonnull") << "\n";
  return 0;
}
