#include "portrayal/catalog.h"

#include <filesystem>
#include <iostream>
#include <string_view>

namespace fs = std::filesystem;

namespace {

bool Expect(bool condition, std::string_view message) {
  if (condition) {
    return true;
  }

  std::cerr << "[navscene-portrayal-catalog] " << message << '\n';
  return false;
}

fs::path ResolveCatalogPath() {
  return fs::path(__FILE__).parent_path().parent_path() / "data" / "portrayal" /
         "s57_catalog.txt";
}

}  // namespace

int main() {
  navscene::portrayal::S57PortrayalCatalog catalog;
  const fs::path catalog_path = ResolveCatalogPath();
  const auto load_status =
      navscene::portrayal::LoadS57PortrayalCatalogFromFile(catalog_path.string(), &catalog);
  if (!Expect(load_status.ok(), "External catalog should load successfully.")) {
    return 1;
  }

  std::string validation_details;
  const auto validation_status = catalog.Validate(&validation_details);
  if (!Expect(validation_status.ok(), "External catalog should validate successfully.")) {
    std::cerr << "[navscene-portrayal-catalog] validation details: " << validation_details
              << '\n';
    return 1;
  }

  if (!Expect(catalog.profile_id() == navscene::portrayal::PortrayalProfileId::kS57,
              "Catalog profile id should be S-57.")) {
    return 1;
  }
  if (!Expect(catalog.FindColor("DEPDW") != nullptr,
              "Catalog should expose the DEPDW palette color entry.")) {
    return 1;
  }
  if (!Expect(catalog.FindColor("NODTA") != nullptr &&
                  catalog.FindColor("CHCOR") != nullptr &&
                  catalog.FindColor("UINFB") != nullptr,
              "Catalog should expose imported OpenCPN/S-52 palette entries beyond the MVP set.")) {
    return 1;
  }
  if (!Expect(catalog.FindAreaStyle("sea_area") != nullptr,
              "Catalog should expose the sea_area style.")) {
    return 1;
  }
  if (!Expect(catalog.FindAreaStyle("river_area") != nullptr,
              "Catalog should expose the river_area style.")) {
    return 1;
  }
  if (!Expect(catalog.FindAreaStyle("inland_water_area") != nullptr &&
                  catalog.FindAreaStyle("brown_feature_area") != nullptr &&
                  catalog.FindAreaStyle("label_only_area") != nullptr,
              "Catalog should expose the added inland-water, brown-feature, and label-only area styles.")) {
    return 1;
  }
  if (!Expect(catalog.FindAreaStyle("airare_area") != nullptr &&
                  catalog.FindAreaStyle("bridge_area") != nullptr &&
                  catalog.FindAreaStyle("cable_area") != nullptr &&
                  catalog.FindAreaStyle("admin_boundary_area") != nullptr,
              "Catalog should expose the added air-area, bridge, cable, and administrative boundary styles.")) {
    return 1;
  }
  if (!Expect(catalog.FindAreaStyle("obstruction_area") != nullptr,
              "Catalog should expose the obstruction_area style.")) {
    return 1;
  }
  if (!Expect(catalog.FindLineStyle("plain_boundary") != nullptr,
              "Catalog should expose the plain_boundary line style.")) {
    return 1;
  }
  if (!Expect(catalog.FindLineStyle("river_edge") != nullptr,
              "Catalog should expose the river_edge line style.")) {
    return 1;
  }
  if (!Expect(catalog.FindLineStyle("obstruction_line") != nullptr,
              "Catalog should expose the obstruction_line line style.")) {
    return 1;
  }
  if (!Expect(catalog.FindLineStyle("route_centerline") != nullptr,
              "Catalog should expose the dedicated route_centerline line style.")) {
    return 1;
  }
  if (!Expect(catalog.FindLineStyle("land_elevation") != nullptr,
              "Catalog should expose the dedicated land_elevation line style.")) {
    return 1;
  }
  if (!Expect(catalog.FindLineStyle("bridge_line") != nullptr,
              "Catalog should expose the dedicated bridge line style.")) {
    return 1;
  }
  if (!Expect(catalog.FindPointStyle("light") != nullptr,
              "Catalog should expose the light point style.")) {
    return 1;
  }
  if (!Expect(catalog.FindPointStyle("beacon_special") != nullptr,
              "Catalog should expose the dedicated special beacon style.")) {
    return 1;
  }
  if (!Expect(catalog.FindPointStyle("rock") != nullptr,
              "Catalog should expose the underwater rock point style.")) {
    return 1;
  }
  if (!Expect(catalog.FindTextRole("important_label") != nullptr,
              "Catalog should expose the important_label text role.")) {
    return 1;
  }
  const auto* route_centerline = catalog.FindLineStyle("route_centerline");
  if (!Expect(route_centerline != nullptr &&
                  route_centerline->stroke.palette_color_id == "CHGRD" &&
                  route_centerline->stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kDash,
              "Route centerline style should preserve the dashed CHGRD stroke pattern.")) {
    return 1;
  }
  const auto* production_area = catalog.FindAreaStyle("production_area");
  if (!Expect(production_area != nullptr &&
                  production_area->fill.palette_color_id == "CHBRN" &&
                  production_area->stroke.palette_color_id == "CSTLN" &&
                  production_area->stroke.width_px == 4 &&
                  production_area->stroke.pattern ==
                      navscene::portrayal::StrokePatternKind::kSolid,
              "Production area style should preserve the palette-driven solid boundary.")) {
    return 1;
  }
  const auto* inland_water = catalog.FindAreaStyle("inland_water_area");
  if (!Expect(inland_water != nullptr &&
                  inland_water->fill.palette_color_id == "DEPVS" &&
                  inland_water->stroke.palette_color_id == "CHBLK",
              "Inland-water style should preserve the S-52 blue fill with black outline.")) {
    return 1;
  }
  const navscene::AttributeList no_attributes;
  const auto land_lookup = catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
      .lookup_key =
          navscene::portrayal::LookupKey{
              .geometry_kind = navscene::portrayal::GeometryKind::kArea,
              .object_class_acronym = "LNDARE",
              .display_category = navscene::DisplayCategory::kBase,
          },
      .attributes = &no_attributes,
      .settings = nullptr,
  });
  const auto canal_area_lookup = catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
      .lookup_key =
          navscene::portrayal::LookupKey{
              .geometry_kind = navscene::portrayal::GeometryKind::kArea,
              .object_class_acronym = "CANALS",
              .display_category = navscene::DisplayCategory::kBase,
          },
      .attributes = &no_attributes,
      .settings = nullptr,
  });
  if (!Expect(land_lookup.rule != nullptr && canal_area_lookup.rule != nullptr &&
                  land_lookup.rule->priority < canal_area_lookup.rule->priority,
              "External catalog should keep LNDARE below inland-water priorities so canals remain visible.")) {
    return 1;
  }

  const auto* depdw_day = catalog.FindColor("DEPDW", navscene::ColorScheme::kDay);
  const auto* depdw_dusk = catalog.FindColor("DEPDW", navscene::ColorScheme::kDusk);
  const auto* depdw_night = catalog.FindColor("DEPDW", navscene::ColorScheme::kNight);
  if (!Expect(depdw_day != nullptr && depdw_dusk != nullptr && depdw_night != nullptr &&
                  depdw_day->r == 212 && depdw_day->g == 234 && depdw_day->b == 238 &&
                  depdw_dusk->r == 7 && depdw_dusk->g == 7 && depdw_dusk->b == 7 &&
                  depdw_night->r == 7 && depdw_night->g == 7 && depdw_night->b == 7,
              "Palette colors should resolve different scheme values through the catalog.")) {
    return 1;
  }
  const auto* nodta_day = catalog.FindColor("NODTA", navscene::ColorScheme::kDay);
  const auto* nodta_dusk = catalog.FindColor("NODTA", navscene::ColorScheme::kDusk);
  const auto* nodta_night = catalog.FindColor("NODTA", navscene::ColorScheme::kNight);
  if (!Expect(nodta_day != nullptr && nodta_dusk != nullptr && nodta_night != nullptr &&
                  nodta_day->r == 163 && nodta_day->g == 180 && nodta_day->b == 183 &&
                  nodta_dusk->r == 41 && nodta_dusk->g == 46 && nodta_dusk->b == 46 &&
                  nodta_night->r == 7 && nodta_night->g == 7 && nodta_night->b == 7,
              "Imported OpenCPN/S-52 colors should preserve day/dusk/night palette values.")) {
    return 1;
  }

  const navscene::AttributeList light_attributes = {{"CATLIT", "1"}, {"LITCHR", "Fl"}};
  const auto light_lookup = catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
      .lookup_key =
          navscene::portrayal::LookupKey{
              .geometry_kind = navscene::portrayal::GeometryKind::kPoint,
              .object_class_acronym = "LIGHTS",
              .display_category = navscene::DisplayCategory::kStandard,
          },
      .attributes = &light_attributes,
      .settings = nullptr,
  });
  if (!Expect(light_lookup.rule != nullptr,
              "Attribute-specific LIGHTS lookup should resolve a rule.")) {
    return 1;
  }
  if (!Expect(light_lookup.rule->important_label,
              "Attribute-specific LIGHTS rule should select the important label variant.")) {
    return 1;
  }
  if (!Expect(light_lookup.text_role != nullptr &&
                  light_lookup.text_role->text.role ==
                      navscene::portrayal::FontRole::kImportant,
              "Attribute-specific LIGHTS rule should resolve the important text role.")) {
    return 1;
  }

  const navscene::AttributeList empty_attributes;
  const auto fallback_lookup = catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
      .lookup_key =
          navscene::portrayal::LookupKey{
              .geometry_kind = navscene::portrayal::GeometryKind::kPoint,
              .object_class_acronym = "UNKNWN",
              .display_category = navscene::DisplayCategory::kBase,
          },
      .attributes = &empty_attributes,
      .settings = nullptr,
  });
  if (!Expect(fallback_lookup.rule != nullptr &&
                  fallback_lookup.style_id == "default_point",
              "Unknown point classes should resolve to the default point rule.")) {
    return 1;
  }

  const auto rock_lookup = catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
      .lookup_key =
          navscene::portrayal::LookupKey{
              .geometry_kind = navscene::portrayal::GeometryKind::kPoint,
              .object_class_acronym = "UWTROC",
              .display_category = navscene::DisplayCategory::kStandard,
          },
      .attributes = &empty_attributes,
      .settings = nullptr,
  });
  if (!Expect(rock_lookup.rule != nullptr && rock_lookup.style_id == "rock",
              "UWTROC should resolve to the dedicated underwater rock point style.")) {
    return 1;
  }

  const auto obstruction_area_lookup =
      catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
          .lookup_key =
              navscene::portrayal::LookupKey{
                  .geometry_kind = navscene::portrayal::GeometryKind::kArea,
                  .object_class_acronym = "OBSTRN",
                  .display_category = navscene::DisplayCategory::kStandard,
              },
          .attributes = &empty_attributes,
          .settings = nullptr,
      });
  if (!Expect(obstruction_area_lookup.rule != nullptr &&
                  obstruction_area_lookup.style_id == "obstruction_area",
              "OBSTRN area portrayal should resolve to the dedicated obstruction area style.")) {
    return 1;
  }

  const auto canal_lookup = catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
      .lookup_key =
          navscene::portrayal::LookupKey{
              .geometry_kind = navscene::portrayal::GeometryKind::kArea,
              .object_class_acronym = "CANALS",
              .display_category = navscene::DisplayCategory::kBase,
          },
      .attributes = &empty_attributes,
      .settings = nullptr,
  });
  if (!Expect(canal_area_lookup.rule != nullptr &&
                  canal_area_lookup.style_id == "inland_water_area",
              "CANALS should resolve to the dedicated inland-water area rule.")) {
    return 1;
  }
  const auto sea_lookup = catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
      .lookup_key =
          navscene::portrayal::LookupKey{
              .geometry_kind = navscene::portrayal::GeometryKind::kArea,
              .object_class_acronym = "SEAARE",
              .display_category = navscene::DisplayCategory::kBase,
          },
      .attributes = &empty_attributes,
      .settings = nullptr,
  });
  if (!Expect(sea_lookup.rule != nullptr && sea_lookup.rule->generate_label,
              "SEAARE should now resolve a label-generating area rule.")) {
    return 1;
  }
  const auto bridge_area_lookup =
      catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
          .lookup_key =
              navscene::portrayal::LookupKey{
                  .geometry_kind = navscene::portrayal::GeometryKind::kArea,
                  .object_class_acronym = "BRIDGE",
                  .display_category = navscene::DisplayCategory::kBase,
              },
          .attributes = &empty_attributes,
          .settings = nullptr,
      });
  if (!Expect(bridge_area_lookup.rule != nullptr &&
                  bridge_area_lookup.style_id == "bridge_area",
              "BRIDGE areas should resolve to the dedicated emphasized bridge style.")) {
    return 1;
  }
  const auto cable_area_lookup = catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
      .lookup_key =
          navscene::portrayal::LookupKey{
              .geometry_kind = navscene::portrayal::GeometryKind::kArea,
              .object_class_acronym = "CBLARE",
              .display_category = navscene::DisplayCategory::kStandard,
          },
      .attributes = &empty_attributes,
      .settings = nullptr,
  });
  if (!Expect(cable_area_lookup.rule != nullptr &&
                  cable_area_lookup.style_id == "cable_area",
              "CBLARE areas should resolve to the dedicated cable-area boundary style.")) {
    return 1;
  }
  const auto tsezne_lookup = catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
      .lookup_key =
          navscene::portrayal::LookupKey{
              .geometry_kind = navscene::portrayal::GeometryKind::kArea,
              .object_class_acronym = "TSEZNE",
              .display_category = navscene::DisplayCategory::kBase,
          },
      .attributes = &empty_attributes,
      .settings = nullptr,
  });
  if (!Expect(tsezne_lookup.rule != nullptr &&
                  tsezne_lookup.style_id == "traffic_fill_area",
              "TSEZNE areas should resolve to the dedicated traffic-separation fill style.")) {
    return 1;
  }

  const navscene::AttributeList slope_attributes = {{"CATSLO", "6"}};
  const auto slope_lookup = catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
      .lookup_key =
          navscene::portrayal::LookupKey{
              .geometry_kind = navscene::portrayal::GeometryKind::kArea,
              .object_class_acronym = "SLOGRD",
              .display_category = navscene::DisplayCategory::kStandard,
          },
      .attributes = &slope_attributes,
      .settings = nullptr,
  });
  if (!Expect(slope_lookup.rule != nullptr && slope_lookup.style_id == "gray_feature_area",
              "SLOGRD CATSLO=6 should resolve to the gray highlighted slope rule.")) {
    return 1;
  }

  const auto rectrc_lookup = catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
      .lookup_key =
          navscene::portrayal::LookupKey{
              .geometry_kind = navscene::portrayal::GeometryKind::kLine,
              .object_class_acronym = "RECTRC",
              .display_category = navscene::DisplayCategory::kStandard,
          },
      .attributes = &empty_attributes,
      .settings = nullptr,
  });
  if (!Expect(rectrc_lookup.rule != nullptr && rectrc_lookup.style_id == "route_centerline",
              "RECTRC line portrayal should resolve to the dedicated route centerline style.")) {
    return 1;
  }

  const auto rivers_area_lookup =
      catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
          .lookup_key =
              navscene::portrayal::LookupKey{
                  .geometry_kind = navscene::portrayal::GeometryKind::kArea,
                  .object_class_acronym = "RIVERS",
                  .display_category = navscene::DisplayCategory::kBase,
              },
          .attributes = &empty_attributes,
          .settings = nullptr,
      });
  if (!Expect(rivers_area_lookup.rule != nullptr && rivers_area_lookup.style_id == "river_area",
              "RIVERS area portrayal should resolve to the dedicated river area style.")) {
    return 1;
  }

  const auto lndelv_lookup = catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
      .lookup_key =
          navscene::portrayal::LookupKey{
              .geometry_kind = navscene::portrayal::GeometryKind::kLine,
              .object_class_acronym = "LNDELV",
              .display_category = navscene::DisplayCategory::kStandard,
          },
      .attributes = &empty_attributes,
      .settings = nullptr,
  });
  if (!Expect(lndelv_lookup.rule != nullptr && lndelv_lookup.style_id == "land_elevation",
              "LNDELV should resolve to the dedicated land-elevation line style.")) {
    return 1;
  }

  const auto obstruction_line_lookup =
      catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
          .lookup_key =
              navscene::portrayal::LookupKey{
                  .geometry_kind = navscene::portrayal::GeometryKind::kLine,
                  .object_class_acronym = "OBSTRN",
                  .display_category = navscene::DisplayCategory::kStandard,
              },
          .attributes = &empty_attributes,
          .settings = nullptr,
      });
  if (!Expect(obstruction_line_lookup.rule != nullptr &&
                  obstruction_line_lookup.style_id == "obstruction_line",
              "OBSTRN line portrayal should resolve to the dedicated obstruction line style.")) {
    return 1;
  }

  const auto rivers_line_lookup =
      catalog.ResolveLookup(navscene::portrayal::FeaturePortrayalContext{
          .lookup_key =
              navscene::portrayal::LookupKey{
                  .geometry_kind = navscene::portrayal::GeometryKind::kLine,
                  .object_class_acronym = "RIVERS",
                  .display_category = navscene::DisplayCategory::kBase,
              },
          .attributes = &empty_attributes,
          .settings = nullptr,
      });
  if (!Expect(rivers_line_lookup.rule != nullptr &&
                  rivers_line_lookup.style_id == "river_edge",
              "RIVERS line portrayal should resolve to the dedicated river edge style.")) {
    return 1;
  }

  return 0;
}
