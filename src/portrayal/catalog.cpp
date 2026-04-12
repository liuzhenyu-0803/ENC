#include "portrayal/catalog.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <unordered_set>

namespace navscene::portrayal {
namespace {

constexpr const char* kEmbeddedCatalogText = R"(catalog|s57|1
color|CHBLK|7,7,7|54,60,61|31,34,35
color|CHGRD|125,137,140|54,60,61|31,34,35
color|CHGRF|163,180,183|41,46,46|16,18,18
color|CHRED|241,84,105|80,28,35|59,17,10
color|CHGRN|104,228,86|35,76,29|22,34,7
color|CHYLW|244,218,72|81,73,24|41,33,10
color|CHMGD|197,69,195|74,58,81|52,18,52
color|CHMGF|211,166,233|58,20,57|52,18,52
color|CHBRN|177,145,57|54,44,17|15,13,5
color|CHWHT|212,234,238|71,78,79|37,41,41
color|CHCOR|235,125,54|75,38,19|52,28,12
color|LANDA|201,185,122|44,41,27|13,10,8
color|LANDF|139,102,31|76,56,17|23,17,5
color|CSTLN|82,90,92|54,60,61|37,41,41
color|SNDG1|125,137,140|41,46,46|31,34,35
color|SNDG2|7,7,7|71,78,79|43,48,48
color|DEPSC|82,90,92|54,60,61|37,41,41
color|DEPCN|125,137,140|41,46,46|31,34,35
color|DEPDW|212,234,238|7,7,7|7,7,7
color|DEPMD|186,213,225|12,14,15|7,7,7
color|DEPMS|152,197,242|21,27,33|3,4,19
color|DEPVS|115,182,239|22,35,47|3,4,19
color|DEPIT|131,178,149|21,37,31|8,11,9
color|RESBL|58,120,240|19,40,80|21,29,69
color|RESGR|125,137,140|41,46,46|16,18,18
color|RES01|163,180,183|41,46,46|7,7,7
color|RES02|163,180,183|41,46,46|7,7,7
color|RES03|163,180,183|41,46,46|7,7,7
color|ADINF|178,159,52|59,53,17|41,33,10
color|APLRT|235,125,54|75,38,19|52,28,12
color|ARPAT|63,165,111|26,69,47|12,31,21
color|BKAJ1|7,7,7|7,7,7|7,7,7
color|BKAJ2|35,39,40|11,13,13|7,8,8
color|CURSR|235,125,54|75,38,19|52,28,12
color|DNGHL|241,84,105|80,28,35|59,17,10
color|ISDNG|197,69,195|74,58,81|52,18,52
color|LITGN|104,228,86|35,76,29|22,34,7
color|LITRD|241,84,105|80,28,35|59,17,10
color|LITYW|244,218,72|81,73,24|41,33,10
color|NINFO|235,125,54|75,38,19|52,28,12
color|NODTA|163,180,183|41,46,46|7,7,7
color|OUTLL|201,185,122|44,41,27|13,10,8
color|OUTLW|7,7,7|7,7,7|7,7,7
color|PLRTE|220,64,37|73,21,12|66,19,11
color|PSTRK|7,7,7|71,78,79|37,41,41
color|RADHI|104,228,86|35,76,29|22,34,7
color|RADLO|63,138,52|21,46,17|10,16,3
color|SCLBR|235,125,54|75,38,19|52,28,12
color|SHIPS|7,7,7|71,78,79|37,41,41
color|SYTRK|125,137,140|41,46,46|31,34,35
color|TRFCD|197,69,195|74,58,81|58,20,58
color|TRFCF|211,166,233|58,20,57|52,18,52
color|UIAFD|115,182,239|22,35,47|3,4,19
color|UIAFF|201,185,122|44,41,27|13,10,8
color|UIBCK|212,234,238|7,7,7|7,7,7
color|UIBDR|125,137,140|54,60,61|31,34,35
color|UINFB|58,120,240|19,40,80|21,29,69
color|UINFD|7,7,7|71,78,79|43,48,48
color|UINFF|125,137,140|41,46,46|31,34,35
color|UINFG|104,228,86|35,76,29|22,34,7
color|UINFM|197,69,195|58,20,57|52,18,52
color|UINFO|235,125,54|75,38,19|52,28,12
color|UINFR|241,84,105|80,28,35|59,17,10
text_role|body|body|CHBLK|11|1
text_role|important_label|important|CHBLK|12|1
text_role|sounding_label|sounding|SNDG2|11|1
area_style|transparent_area|0,0,0|0|0,0,0|0|0
area_style|land_area|LANDA|1|0,0,0|0|solid|0
area_style|harbor_area|CHBRN|1|LANDF|1|solid|1
area_style|brown_feature_area|CHBRN|1|LANDF|1|solid|1
area_style|brown_structure_area|CHBRN|1|CSTLN|2|solid|1
area_style|brown_black_area|CHBRN|1|CHBLK|1|solid|1
area_style|airare_area|LANDA|1|LANDF|1|solid|1
area_style|bridge_area|0,0,0|0|CHGRD|4|solid|1
area_style|admin_boundary_area|0,0,0|0|CHGRF|2|dash|1
area_style|cable_area|0,0,0|0|CHMGD|2|dash|1
area_style|harbor_admin_area|0,0,0|0|CHGRD|2|dash|1
area_style|construction_area|CHBRN|1|CHBLK|1|solid|1
area_style|production_area|CHBRN|1|CSTLN|4|solid|1
area_style|route_area|0,0,0|0|CHGRD|1|dash|1
area_style|road_area|LANDA|1|LANDF|1|solid|1
area_style|sea_area|DEPDW|1|DEPSC|1|solid|0
area_style|inland_water_area|DEPVS|1|CHBLK|1|solid|1
area_style|depth_area_default|DEPMD|1|0,0,0|0|solid|0
area_style|depth_intertidal_area|DEPIT|1|0,0,0|0|solid|0
area_style|depth_shallow_area|DEPVS|1|0,0,0|0|solid|0
area_style|depth_safe_area|DEPMS|1|0,0,0|0|solid|0
area_style|depth_deep_area|DEPMD|1|0,0,0|0|solid|0
area_style|depth_very_deep_area|DEPDW|1|0,0,0|0|solid|0
area_style|river_area|DEPVS|1|CHBLK|1|solid|1
area_style|seabed_area|0,0,0|0|CHGRD|1|solid|1
area_style|traffic_fill_area|TRFCF|1|0,0,0|0|0
area_style|gray_feature_area|CHGRD|1|CHBLK|1|solid|1
area_style|label_only_area|0,0,0|0|0,0,0|0|0
area_style|restriction_area|0,0,0|0|CHMGD|2|dash|1
area_style|metadata_area|0,0,0|0|CHGRD|1|solid|1
area_style|default_area|0,0,0|0|0,0,0|0|0
line_style|coastline|CSTLN|2|solid|1
line_style|river_edge|CHBLK|1|solid|1
line_style|depth_contour|DEPCN|1|solid|1
line_style|depth_contour_safety|DEPSC|2|solid|1
line_style|submarine_cable|CHMGD|1|solid|1
line_style|fairway|CHMGD|2|solid|1
line_style|caution_line|CHMGD|2|solid|1
line_style|land_edge|LANDF|1|solid|1
line_style|slcons_edge|CHBLK|1|solid|1
line_style|land_elevation|LANDF|1|solid|1
line_style|bridge_line|CHGRD|4|solid|1
line_style|plain_boundary|CHGRD|1|solid|1
line_style|route_centerline|CHGRD|1|dash|1
line_style|railway|LANDF|2|solid|1
line_style|pipeline|CHGRD|1|dash|1
line_style|road_centerline|LANDF|1|solid|1
line_style|default_line|CHGRD|1|solid|1
point_style|sounding|circle|SNDG1|CHWHT|5|1
point_style|obstruction|diamond|CHBRN|CHWHT|7|1
point_style|land_point|circle|CHBRN|CHWHT|5|1
point_style|buoy|triangle|CHYLW|CHWHT|7|1
point_style|beacon|square|CHYLW|CHWHT|7|1
point_style|beacon_cardinal|diamond|CHYLW|CHBLK|8|1
point_style|beacon_isolated|diamond|CHRED|CHBLK|8|1
point_style|beacon_safe|square|CHWHT|CHRED|8|1
point_style|beacon_special|square|CHYLW|CHMGD|8|1
point_style|light|diamond|CHYLW|CHBRN|7|1
point_style|light_vessel|diamond|CHYLW|CHBLK|8|1
point_style|landmark|diamond|CHBRN|CHBLK|8|1
point_style|building|square|CHBRN|LANDF|8|1
point_style|fog_signal|square|CHMGD|CHBLK|8|1
point_style|pilot|circle|CHYLW|CHBLK|8|1
point_style|platform|square|CHGRD|CHBLK|9|1
point_style|rock|diamond|DEPMS|CSTLN|8|1
point_style|buoy_cardinal|triangle|CHYLW|CHBLK|8|1
point_style|buoy_isolated|diamond|CHRED|CHBLK|8|1
point_style|buoy_safe|circle|CHWHT|CHRED|8|1
point_style|buoy_special|diamond|CHYLW|CHMGD|8|1
point_style|wreck|diamond|CHWHT|CHBLK|8|1
point_style|topmark|triangle|CHMGD|CHBLK|7|1
point_style|default_point|circle|DEPSC|CHWHT|5|1
lookup_rule|area|LNDARE|base|land_area||4|0|0|
lookup_rule|area|SEAARE|base|transparent_area|body|5|1|0|
lookup_rule|area|DEPARE|base|depth_area_default||6|0|0|
lookup_rule|area|DRGARE|base|depth_area_default||7|0|0|
lookup_rule|area|UNSARE|base|depth_very_deep_area||7|0|0|
lookup_rule|area|TIDEWY|base|label_only_area|body|7|1|0|
lookup_rule|area|LAKARE|base|inland_water_area||7|0|0|
lookup_rule|area|RIVERS|base|river_area|body|7|1|0|
lookup_rule|area|ADMARE|base|admin_boundary_area|body|8|1|0|
lookup_rule|area|AIRARE|base|airare_area||8|0|0|
lookup_rule|area|BRIDGE|base|bridge_area|body|9|1|0|
lookup_rule|area|CANALS|base|inland_water_area||7|0|0|
lookup_rule|area|CBLARE|standard|cable_area||17|0|0|
lookup_rule|area|DOCARE|base|inland_water_area|body|7|1|0|
lookup_rule|area|LOKBSN|base|inland_water_area||7|0|0|
lookup_rule|area|BUAARE|standard|harbor_area||16|0|0|
lookup_rule|area|BUISGL|standard|brown_feature_area||16|0|0|
lookup_rule|area|HRBARE|standard|harbor_admin_area|body|16|1|0|
lookup_rule|area|CAUSWY|standard|brown_structure_area||16|0|0|
lookup_rule|area|CRANES|standard|brown_feature_area||16|0|0|
lookup_rule|area|DAMCON|standard|brown_feature_area||16|0|0|
lookup_rule|area|DYKCON|standard|brown_feature_area||16|0|0|
lookup_rule|area|FORSTC|standard|brown_feature_area||16|0|0|
lookup_rule|area|HULKES|standard|brown_structure_area||16|0|0|
lookup_rule|area|LNDMRK|standard|brown_feature_area|body|16|1|0|
lookup_rule|area|MORFAC|standard|brown_black_area||16|0|0|
lookup_rule|area|PONTON|standard|brown_structure_area||16|0|0|
lookup_rule|area|PRDARE|standard|production_area|body|16|1|0|
lookup_rule|area|RUNWAY|standard|brown_feature_area||16|0|0|
lookup_rule|area|SILTNK|standard|brown_feature_area||16|0|0|
lookup_rule|area|VEGATN|all|label_only_area||16|0|0|
lookup_rule|area|RECTRC|standard|route_area|body|18|1|0|
lookup_rule|area|ROADWY|standard|road_area|body|15|1|0|
lookup_rule|area|OFSPLF|standard|production_area|important_label|18|1|0|
lookup_rule|area|SLCONS|standard|construction_area|body|17|1|0|
lookup_rule|area|SBDARE|standard|label_only_area|body|18|1|0|
lookup_rule|area|SLOGRD|standard|gray_feature_area||17|0|0|CATSLO=6
lookup_rule|area|TSEZNE|base|traffic_fill_area||18|0|0|
lookup_rule|area|RESARE|all|restriction_area||20|0|0|
lookup_rule|area|ACHARE|all|restriction_area||20|0|0|
lookup_rule|area|PSSARE|all|restriction_area||20|0|0|
lookup_rule|area|M_COVR|all|metadata_area||30|0|0|
lookup_rule|area|M_QUAL|all|metadata_area||31|0|0|
lookup_rule|area||base|default_area||25|0|0|
lookup_rule|line|COALNE|base|coastline||50|0|0|
lookup_rule|line|RIVERS|base|river_edge|body|50|1|0|
lookup_rule|line|DEPCNT|base|depth_contour||40|0|0|
lookup_rule|line|CBLSUB|standard|submarine_cable||60|0|0|
lookup_rule|line|RECTRC|standard|route_centerline|body|66|1|0|
lookup_rule|line|ROADWY|standard|road_centerline|body|58|1|0|
lookup_rule|line|RAILWY|standard|railway|body|59|1|0|
lookup_rule|line|PIPSOL|standard|pipeline|body|59|1|0|
lookup_rule|line|BRIDGE|base|bridge_line||58|0|0|
lookup_rule|line|SLCONS|standard|slcons_edge||58|0|0|
lookup_rule|line|LNDELV|standard|land_elevation||57|0|0|
lookup_rule|line|FAIRWY|standard|fairway||65|0|0|
lookup_rule|line|NAVLNE|standard|fairway||65|0|0|
lookup_rule|line||base|default_line||55|0|0|
lookup_rule|point|SOUNDG|base|sounding|sounding_label|80|1|1|
lookup_rule|point|OBSTRN|standard|obstruction|important_label|85|1|1|
lookup_rule|point|UWTROC|standard|rock|important_label|85|1|1|
lookup_rule|point|LNDMRK|standard|landmark|body|86|1|0|
lookup_rule|point|BUISGL|standard|building|body|86|1|0|
lookup_rule|point|FOGSIG|standard|fog_signal|body|87|1|0|
lookup_rule|point|WRECKS|standard|wreck|important_label|88|1|1|
lookup_rule|point|OFSPLF|standard|platform|important_label|89|1|1|
lookup_rule|point|PILPNT|standard|pilot|body|89|1|0|
lookup_rule|point|TOPMAR|standard|topmark|body|89|1|0|
lookup_rule|point|LIGHTS|standard|light|important_label|92|1|1|CATLIT=1
lookup_rule|point|LIGHTS|standard|light|body|90|1|0|
lookup_rule|point|LITVES|standard|light_vessel|important_label|91|1|1|
lookup_rule|point|BCNCAR|standard|beacon_cardinal|body|90|1|0|
lookup_rule|point|BCNISD|standard|beacon_isolated|body|90|1|0|
lookup_rule|point|BCNLAT|standard|beacon|body|90|1|0|
lookup_rule|point|BCNSAW|standard|beacon_safe|body|90|1|0|
lookup_rule|point|BCNSPP|standard|beacon_special|body|90|1|0|
lookup_rule|point|BOYCAR|standard|buoy_cardinal|body|90|1|0|
lookup_rule|point|BOYISD|standard|buoy_isolated|body|90|1|0|
lookup_rule|point|BOYLAT|standard|buoy|body|90|1|0|
lookup_rule|point|BOYSAW|standard|buoy_safe|body|90|1|0|
lookup_rule|point|BOYSPP|standard|buoy_special|body|90|1|0|
lookup_rule|point||base|default_point|body|82|0|0|
)";

std::string Trim(std::string value) {
  auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
  while (!value.empty() && is_space(static_cast<unsigned char>(value.front()))) {
    value.erase(value.begin());
  }
  while (!value.empty() && is_space(static_cast<unsigned char>(value.back()))) {
    value.pop_back();
  }
  return value;
}

std::vector<std::string> Split(std::string_view text, char delimiter) {
  std::vector<std::string> parts;
  size_t begin = 0;
  while (begin <= text.size()) {
    const size_t end = text.find(delimiter, begin);
    if (end == std::string_view::npos) {
      parts.push_back(std::string(text.substr(begin)));
      break;
    }
    parts.push_back(std::string(text.substr(begin, end - begin)));
    begin = end + 1;
  }
  return parts;
}

bool ParseBool(std::string_view value, bool* out) {
  if (out == nullptr) {
    return false;
  }
  if (value == "1" || value == "true") {
    *out = true;
    return true;
  }
  if (value == "0" || value == "false") {
    *out = false;
    return true;
  }
  return false;
}

bool ParseInt(std::string_view value, int* out) {
  if (out == nullptr) {
    return false;
  }
  try {
    *out = std::stoi(std::string(value));
    return true;
  } catch (...) {
    return false;
  }
}

bool ParseColor(std::string_view value, Rgb8* out) {
  if (out == nullptr) {
    return false;
  }
  const auto channels = Split(value, ',');
  if (channels.size() != 3) {
    return false;
  }

  int r = 0;
  int g = 0;
  int b = 0;
  if (!ParseInt(Trim(channels[0]), &r) || !ParseInt(Trim(channels[1]), &g) ||
      !ParseInt(Trim(channels[2]), &b)) {
    return false;
  }
  if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
    return false;
  }

  out->r = static_cast<uint8_t>(r);
  out->g = static_cast<uint8_t>(g);
  out->b = static_cast<uint8_t>(b);
  return true;
}

bool ParseColorOrPaletteId(std::string_view value,
                           const S57PortrayalCatalog& catalog,
                           Rgb8* out_color,
                           std::string* out_palette_id) {
  if (out_color == nullptr || out_palette_id == nullptr) {
    return false;
  }

  const std::string trimmed = Trim(std::string(value));
  if (trimmed.empty()) {
    return false;
  }

  if (ParseColor(trimmed, out_color)) {
    out_palette_id->clear();
    return true;
  }

  const auto* palette_color = catalog.FindColor(trimmed, ColorScheme::kDay);
  if (palette_color == nullptr) {
    return false;
  }

  *out_color = *palette_color;
  *out_palette_id = trimmed;
  return true;
}

bool ParseGeometryKind(std::string_view value, GeometryKind* out) {
  if (out == nullptr) {
    return false;
  }
  if (value == "point") {
    *out = GeometryKind::kPoint;
    return true;
  }
  if (value == "line") {
    *out = GeometryKind::kLine;
    return true;
  }
  if (value == "area") {
    *out = GeometryKind::kArea;
    return true;
  }
  return false;
}

bool ParseDisplayCategory(std::string_view value, DisplayCategory* out) {
  if (out == nullptr) {
    return false;
  }
  if (value == "base") {
    *out = DisplayCategory::kBase;
    return true;
  }
  if (value == "standard") {
    *out = DisplayCategory::kStandard;
    return true;
  }
  if (value == "all") {
    *out = DisplayCategory::kAll;
    return true;
  }
  return false;
}

bool ParseFontRole(std::string_view value, FontRole* out) {
  if (out == nullptr) {
    return false;
  }
  if (value == "body") {
    *out = FontRole::kBody;
    return true;
  }
  if (value == "important") {
    *out = FontRole::kImportant;
    return true;
  }
  if (value == "sounding") {
    *out = FontRole::kSounding;
    return true;
  }
  return false;
}

bool ParsePointSymbolKind(std::string_view value, PointSymbolKind* out) {
  if (out == nullptr) {
    return false;
  }
  if (value == "circle") {
    *out = PointSymbolKind::kCircle;
    return true;
  }
  if (value == "triangle") {
    *out = PointSymbolKind::kTriangle;
    return true;
  }
  if (value == "square") {
    *out = PointSymbolKind::kSquare;
    return true;
  }
  if (value == "diamond") {
    *out = PointSymbolKind::kDiamond;
    return true;
  }
  return false;
}

bool ParseStrokePatternKind(std::string_view value, StrokePatternKind* out) {
  if (out == nullptr) {
    return false;
  }
  if (value == "solid") {
    *out = StrokePatternKind::kSolid;
    return true;
  }
  if (value == "dash") {
    *out = StrokePatternKind::kDash;
    return true;
  }
  if (value == "dot") {
    *out = StrokePatternKind::kDot;
    return true;
  }
  return false;
}

std::vector<AttributeCondition> ParseAttributeConditions(std::string_view value) {
  std::vector<AttributeCondition> conditions;
  if (value.empty()) {
    return conditions;
  }

  for (const auto& item : Split(value, ';')) {
    const std::string trimmed = Trim(item);
    if (trimmed.empty()) {
      continue;
    }
    const size_t equal = trimmed.find('=');
    if (equal == std::string::npos) {
      continue;
    }
    conditions.push_back(AttributeCondition{
        .key = Trim(trimmed.substr(0, equal)),
        .value = Trim(trimmed.substr(equal + 1)),
    });
  }
  return conditions;
}

bool AttributeConditionsMatch(const std::vector<AttributeCondition>& conditions,
                              const AttributeList* attributes) {
  if (conditions.empty()) {
    return true;
  }
  if (attributes == nullptr) {
    return false;
  }
  for (const auto& condition : conditions) {
    bool matched = false;
    for (const auto& [key, value] : *attributes) {
      if (key == condition.key && value == condition.value) {
        matched = true;
        break;
      }
    }
    if (!matched) {
      return false;
    }
  }
  return true;
}

template <typename Entry>
const typename Entry::style_type* FindStyleById(const std::vector<Entry>& entries,
                                                std::string_view id) = delete;

bool Allows(DisplayCategory requested, DisplayCategory min_category) {
  return static_cast<int>(requested) >= static_cast<int>(min_category);
}

int LookupSpecificityScore(const LookupRule& rule) {
  int score = 0;
  if (!rule.object_class_acronym.empty()) {
    score += 1000;
  }
  score += static_cast<int>(rule.attribute_conditions.size()) * 10;
  score += static_cast<int>(rule.min_display_category);
  return score;
}

std::string ErrorMessage(size_t line_number, std::string_view message) {
  std::ostringstream stream;
  stream << "Failed to parse S-57 portrayal catalog at line " << line_number << ": " << message;
  return stream.str();
}

Status ParseCatalogFromStream(std::istream& stream, S57PortrayalCatalog* out) {
  if (out == nullptr) {
    return Status{StatusCode::kInvalidArgument, "Portrayal catalog output must not be null."};
  }

  S57PortrayalCatalog catalog;
  bool header_seen = false;
  size_t line_number = 0;
  std::string line;
  while (std::getline(stream, line)) {
    ++line_number;
    const std::string trimmed = Trim(line);
    if (trimmed.empty() || trimmed.starts_with('#')) {
      continue;
    }

    const auto fields = Split(trimmed, '|');
    if (fields.empty()) {
      continue;
    }

    const std::string kind = Trim(fields[0]);
    if (kind == "catalog") {
      if (fields.size() != 3) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "catalog header must have 3 fields.")};
      }
      if (Trim(fields[1]) != "s57") {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "only profile 's57' is supported.")};
      }
      if (Trim(fields[2]) != "1") {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "catalog version must be 1.")};
      }
      catalog.set_profile_id(PortrayalProfileId::kS57);
      header_seen = true;
      continue;
    }

    if (!header_seen) {
      return Status{StatusCode::kInvalidArgument,
                    ErrorMessage(line_number, "catalog header must appear before records.")};
    }

    if (kind == "text_role") {
      if (fields.size() != 6) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "text_role must have 6 fields.")};
      }
      FontRole role = FontRole::kBody;
      Rgb8 color{};
      std::string palette_color_id;
      int size_px = 0;
      bool halo = false;
      if (!ParseFontRole(Trim(fields[2]), &role) ||
          !ParseColorOrPaletteId(Trim(fields[3]), catalog, &color, &palette_color_id) ||
          !ParseInt(Trim(fields[4]), &size_px) || !ParseBool(Trim(fields[5]), &halo)) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "invalid text_role record.")};
      }
      catalog.mutable_text_roles().push_back(S57PortrayalCatalog::NamedTextRoleStyle{
          .id = Trim(fields[1]),
          .style =
              TextRoleStyle{
                  .text =
                      TextStyle{
                          .role = role,
                          .color = color,
                          .palette_color_id = palette_color_id,
                          .size_px = size_px,
                          .halo = halo,
                      },
              },
      });
      continue;
    }

    if (kind == "color") {
      if (fields.size() != 3 && fields.size() != 5) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "color must have 3 or 5 fields.")};
      }
      Rgb8 day{};
      Rgb8 dusk{};
      Rgb8 night{};
      if (!ParseColor(Trim(fields[2]), &day) ||
          (fields.size() == 5 && !ParseColor(Trim(fields[3]), &dusk)) ||
          (fields.size() == 5 && !ParseColor(Trim(fields[4]), &night))) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "invalid color record.")};
      }
      if (fields.size() == 3) {
        dusk = day;
        night = day;
      }
      catalog.mutable_colors().push_back(S57PortrayalCatalog::NamedColor{
          .id = Trim(fields[1]),
          .day = day,
          .dusk = dusk,
          .night = night,
      });
      continue;
    }

    if (kind == "area_style") {
      if (fields.size() != 7 && fields.size() != 8) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "area_style must have 7 or 8 fields.")};
      }
      Rgb8 fill{};
      Rgb8 stroke{};
      std::string fill_palette_color_id;
      std::string stroke_palette_color_id;
      bool fill_enabled = false;
      bool stroke_enabled = false;
      int stroke_width = 0;
      StrokePatternKind stroke_pattern = StrokePatternKind::kSolid;
      if (!ParseColorOrPaletteId(Trim(fields[2]), catalog, &fill, &fill_palette_color_id) ||
          !ParseBool(Trim(fields[3]), &fill_enabled) ||
          !ParseColorOrPaletteId(Trim(fields[4]), catalog, &stroke, &stroke_palette_color_id) ||
          !ParseInt(Trim(fields[5]), &stroke_width) ||
          (fields.size() == 8 &&
           !ParseStrokePatternKind(Trim(fields[6]), &stroke_pattern)) ||
          !ParseBool(Trim(fields[fields.size() - 1]), &stroke_enabled)) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "invalid area_style record.")};
      }
      catalog.mutable_area_styles().push_back(S57PortrayalCatalog::NamedAreaStyle{
          .id = Trim(fields[1]),
          .style =
              AreaStyle{
                  .fill = FillStyle{
                      .color = fill,
                      .palette_color_id = fill_palette_color_id,
                      .enabled = fill_enabled,
                  },
                  .stroke =
                      StrokeStyle{
                          .color = stroke,
                          .palette_color_id = stroke_palette_color_id,
                          .width_px = stroke_width,
                          .pattern = stroke_pattern,
                          .enabled = stroke_enabled,
                      },
              },
      });
      continue;
    }

    if (kind == "line_style") {
      if (fields.size() != 5 && fields.size() != 6) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "line_style must have 5 or 6 fields.")};
      }
      Rgb8 stroke{};
      std::string stroke_palette_color_id;
      int stroke_width = 0;
      StrokePatternKind stroke_pattern = StrokePatternKind::kSolid;
      bool enabled = false;
      if (!ParseColorOrPaletteId(Trim(fields[2]), catalog, &stroke, &stroke_palette_color_id) ||
          !ParseInt(Trim(fields[3]), &stroke_width) ||
          (fields.size() == 6 &&
           !ParseStrokePatternKind(Trim(fields[4]), &stroke_pattern)) ||
          !ParseBool(Trim(fields[fields.size() - 1]), &enabled)) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "invalid line_style record.")};
      }
      catalog.mutable_line_styles().push_back(S57PortrayalCatalog::NamedLineStyle{
          .id = Trim(fields[1]),
          .style =
              LineStyle{
                  .stroke =
                      StrokeStyle{
                          .color = stroke,
                          .palette_color_id = stroke_palette_color_id,
                          .width_px = stroke_width,
                          .pattern = stroke_pattern,
                          .enabled = enabled,
                      },
              },
      });
      continue;
    }

    if (kind == "point_style") {
      if (fields.size() != 7) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "point_style must have 7 fields.")};
      }
      PointSymbolKind symbol_kind = PointSymbolKind::kCircle;
      Rgb8 fill{};
      Rgb8 stroke{};
      std::string fill_palette_color_id;
      std::string stroke_palette_color_id;
      int size_px = 0;
      bool enabled = false;
      if (!ParsePointSymbolKind(Trim(fields[2]), &symbol_kind) ||
          !ParseColorOrPaletteId(Trim(fields[3]), catalog, &fill, &fill_palette_color_id) ||
          !ParseColorOrPaletteId(Trim(fields[4]), catalog, &stroke, &stroke_palette_color_id) ||
          !ParseInt(Trim(fields[5]), &size_px) || !ParseBool(Trim(fields[6]), &enabled)) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "invalid point_style record.")};
      }
      catalog.mutable_point_styles().push_back(S57PortrayalCatalog::NamedPointStyle{
          .id = Trim(fields[1]),
          .style =
              PointStyle{
                  .symbol =
                      PointSymbolStyle{
                          .kind = symbol_kind,
                          .fill = fill,
                          .fill_palette_color_id = fill_palette_color_id,
                          .stroke = stroke,
                          .stroke_palette_color_id = stroke_palette_color_id,
                          .size_px = size_px,
                          .enabled = enabled,
                      },
              },
      });
      continue;
    }

    if (kind == "lookup_rule") {
      if (fields.size() != 10) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "lookup_rule must have 10 fields.")};
      }
      GeometryKind geometry_kind = GeometryKind::kArea;
      DisplayCategory min_category = DisplayCategory::kBase;
      int priority = 0;
      bool generate_label = false;
      bool important_label = false;
      if (!ParseGeometryKind(Trim(fields[1]), &geometry_kind) ||
          !ParseDisplayCategory(Trim(fields[3]), &min_category) ||
          !ParseInt(Trim(fields[6]), &priority) ||
          !ParseBool(Trim(fields[7]), &generate_label) ||
          !ParseBool(Trim(fields[8]), &important_label)) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "invalid lookup_rule record.")};
      }
      catalog.mutable_lookup_rules().push_back(LookupRule{
          .geometry_kind = geometry_kind,
          .object_class_acronym = Trim(fields[2]),
          .min_display_category = min_category,
          .style_id = Trim(fields[4]),
          .text_role_id = Trim(fields[5]),
          .priority = priority,
          .generate_label = generate_label,
          .important_label = important_label,
          .attribute_conditions = ParseAttributeConditions(Trim(fields[9])),
      });
      continue;
    }

    if (kind == "buoy_mapping") {
      if (fields.size() != 9) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "buoy_mapping must have 9 fields.")};
      }
      DisplayCategory min_category = DisplayCategory::kBase;
      int priority = 0;
      bool generate_label = false;
      bool important_label = false;
      if (!ParseDisplayCategory(Trim(fields[3]), &min_category) ||
          !ParseInt(Trim(fields[6]), &priority) ||
          !ParseBool(Trim(fields[7]), &generate_label) ||
          !ParseBool(Trim(fields[8]), &important_label)) {
        return Status{StatusCode::kInvalidArgument,
                      ErrorMessage(line_number, "invalid buoy_mapping record.")};
      }
      for (const auto& object_class : Split(Trim(fields[2]), ',')) {
        catalog.mutable_lookup_rules().push_back(LookupRule{
            .geometry_kind = GeometryKind::kPoint,
            .object_class_acronym = Trim(object_class),
            .min_display_category = min_category,
            .style_id = Trim(fields[4]),
            .text_role_id = Trim(fields[5]),
            .priority = priority,
            .generate_label = generate_label,
            .important_label = important_label,
            .attribute_conditions = {},
        });
      }
      continue;
    }

    return Status{StatusCode::kInvalidArgument,
                  ErrorMessage(line_number, "unknown record type.")};
  }

  if (!header_seen) {
    return Status{StatusCode::kInvalidArgument, "Portrayal catalog header is missing."};
  }

  std::string validation_details;
  const Status validation = catalog.Validate(&validation_details);
  if (!validation.ok()) {
    return Status{validation.code, "Invalid S-57 portrayal catalog. " + validation_details};
  }

  *out = std::move(catalog);
  return {};
}

Status LoadS57PortrayalCatalogFromString(std::string_view text, S57PortrayalCatalog* out) {
  std::istringstream stream{std::string(text)};
  return ParseCatalogFromStream(stream, out);
}

}  // namespace

LookupResult S57PortrayalCatalog::ResolveLookup(const FeaturePortrayalContext& context) const {
  const LookupKey& key = context.lookup_key;
  const LookupRule* best_rule = nullptr;
  int best_score = -1;
  for (const auto& rule : lookup_rules_) {
    if (rule.geometry_kind != key.geometry_kind) {
      continue;
    }
    if (!Allows(key.display_category, rule.min_display_category)) {
      continue;
    }
    if (!AttributeConditionsMatch(rule.attribute_conditions, context.attributes)) {
      continue;
    }
    if (!rule.object_class_acronym.empty() &&
        rule.object_class_acronym != key.object_class_acronym) {
      continue;
    }

    const int score = LookupSpecificityScore(rule);
    if (score > best_score) {
      best_score = score;
      best_rule = &rule;
    }
  }

  return LookupResult{
      .rule = best_rule,
      .style_id =
          best_rule == nullptr ? std::string_view{} : std::string_view(best_rule->style_id),
      .text_role = best_rule == nullptr ? nullptr : FindTextRole(best_rule->text_role_id),
  };
}

const LookupRule* S57PortrayalCatalog::ResolveRule(GeometryKind geometry_kind,
                                                   std::string_view object_class_acronym,
                                                   const DisplaySettings& settings) const {
  return ResolveLookup(FeaturePortrayalContext{
                           .lookup_key =
                               LookupKey{
                                   .geometry_kind = geometry_kind,
                                   .object_class_acronym = object_class_acronym,
                                   .display_category = settings.display_category,
                               },
                           .attributes = nullptr,
                           .settings = &settings,
                       })
      .rule;
}

const AreaStyle* S57PortrayalCatalog::FindAreaStyle(std::string_view style_id) const {
  for (const auto& entry : area_styles_) {
    if (entry.id == style_id) {
      return &entry.style;
    }
  }
  return nullptr;
}

const LineStyle* S57PortrayalCatalog::FindLineStyle(std::string_view style_id) const {
  for (const auto& entry : line_styles_) {
    if (entry.id == style_id) {
      return &entry.style;
    }
  }
  return nullptr;
}

const PointStyle* S57PortrayalCatalog::FindPointStyle(std::string_view style_id) const {
  for (const auto& entry : point_styles_) {
    if (entry.id == style_id) {
      return &entry.style;
    }
  }
  return nullptr;
}

const TextRoleStyle* S57PortrayalCatalog::FindTextRole(std::string_view style_id) const {
  if (style_id.empty()) {
    return nullptr;
  }
  for (const auto& entry : text_roles_) {
    if (entry.id == style_id) {
      return &entry.style;
    }
  }
  return nullptr;
}

const Rgb8* S57PortrayalCatalog::FindColor(std::string_view color_id,
                                           ColorScheme scheme) const {
  if (color_id.empty()) {
    return nullptr;
  }
  for (const auto& entry : colors_) {
    if (entry.id == color_id) {
      switch (scheme) {
        case ColorScheme::kDay:
          return &entry.day;
        case ColorScheme::kDusk:
          return &entry.dusk;
        case ColorScheme::kNight:
          return &entry.night;
      }
    }
  }
  return nullptr;
}

Status S57PortrayalCatalog::Validate(std::string* details) const {
  if (profile_id_ != PortrayalProfileId::kS57) {
    if (details != nullptr) {
      *details = "Profile id must be S-57.";
    }
    return Status{StatusCode::kInvalidArgument, "Unsupported portrayal profile."};
  }
  if (colors_.empty() || area_styles_.empty() || line_styles_.empty() || point_styles_.empty() ||
      text_roles_.empty() || lookup_rules_.empty()) {
    if (details != nullptr) {
      *details = "Catalog must include color, area, line, point, text, and lookup sections.";
    }
    return Status{StatusCode::kInvalidArgument, "Portrayal catalog is incomplete."};
  }

  auto validate_ids = [&](const auto& entries, std::string_view label) -> std::optional<std::string> {
    std::unordered_set<std::string> seen;
    for (const auto& entry : entries) {
      if (entry.id.empty()) {
        return std::string(label) + " entry id must not be empty.";
      }
      if (!seen.insert(entry.id).second) {
        return std::string(label) + " entry id is duplicated: " + entry.id;
      }
    }
    return std::nullopt;
  };

  if (const auto error = validate_ids(colors_, "Color"); error.has_value()) {
    if (details != nullptr) {
      *details = *error;
    }
    return Status{StatusCode::kInvalidArgument, *error};
  }
  if (const auto error = validate_ids(area_styles_, "Area style"); error.has_value()) {
    if (details != nullptr) {
      *details = *error;
    }
    return Status{StatusCode::kInvalidArgument, *error};
  }
  if (const auto error = validate_ids(line_styles_, "Line style"); error.has_value()) {
    if (details != nullptr) {
      *details = *error;
    }
    return Status{StatusCode::kInvalidArgument, *error};
  }
  if (const auto error = validate_ids(point_styles_, "Point style"); error.has_value()) {
    if (details != nullptr) {
      *details = *error;
    }
    return Status{StatusCode::kInvalidArgument, *error};
  }
  if (const auto error = validate_ids(text_roles_, "Text role"); error.has_value()) {
    if (details != nullptr) {
      *details = *error;
    }
    return Status{StatusCode::kInvalidArgument, *error};
  }

  auto validate_palette_ref = [&](std::string_view palette_id,
                                  std::string_view label) -> std::optional<std::string> {
    if (palette_id.empty()) {
      return std::nullopt;
    }
    if (FindColor(palette_id, ColorScheme::kDay) != nullptr) {
      return std::nullopt;
    }
    return std::string(label) + " references unknown palette color id: " +
           std::string(palette_id);
  };

  for (const auto& entry : area_styles_) {
    if (const auto error =
            validate_palette_ref(entry.style.fill.palette_color_id, "Area fill style");
        error.has_value()) {
      if (details != nullptr) {
        *details = *error;
      }
      return Status{StatusCode::kInvalidArgument, *error};
    }
    if (const auto error =
            validate_palette_ref(entry.style.stroke.palette_color_id, "Area stroke style");
        error.has_value()) {
      if (details != nullptr) {
        *details = *error;
      }
      return Status{StatusCode::kInvalidArgument, *error};
    }
  }

  for (const auto& entry : line_styles_) {
    if (const auto error =
            validate_palette_ref(entry.style.stroke.palette_color_id, "Line stroke style");
        error.has_value()) {
      if (details != nullptr) {
        *details = *error;
      }
      return Status{StatusCode::kInvalidArgument, *error};
    }
  }

  for (const auto& entry : point_styles_) {
    if (const auto error =
            validate_palette_ref(entry.style.symbol.fill_palette_color_id, "Point fill style");
        error.has_value()) {
      if (details != nullptr) {
        *details = *error;
      }
      return Status{StatusCode::kInvalidArgument, *error};
    }
    if (const auto error =
            validate_palette_ref(entry.style.symbol.stroke_palette_color_id, "Point stroke style");
        error.has_value()) {
      if (details != nullptr) {
        *details = *error;
      }
      return Status{StatusCode::kInvalidArgument, *error};
    }
  }

  for (const auto& entry : text_roles_) {
    if (const auto error =
            validate_palette_ref(entry.style.text.palette_color_id, "Text role style");
        error.has_value()) {
      if (details != nullptr) {
        *details = *error;
      }
      return Status{StatusCode::kInvalidArgument, *error};
    }
  }

  for (const auto& rule : lookup_rules_) {
    if (rule.style_id.empty()) {
      if (details != nullptr) {
        *details = "Lookup rule style id must not be empty.";
      }
      return Status{StatusCode::kInvalidArgument, "Lookup rule style id must not be empty."};
    }

    bool style_found = false;
    switch (rule.geometry_kind) {
      case GeometryKind::kArea:
        style_found = FindAreaStyle(rule.style_id) != nullptr;
        break;
      case GeometryKind::kLine:
        style_found = FindLineStyle(rule.style_id) != nullptr;
        break;
      case GeometryKind::kPoint:
        style_found = FindPointStyle(rule.style_id) != nullptr;
        break;
    }
    if (!style_found) {
      if (details != nullptr) {
        *details = "Lookup rule references unknown style id: " + rule.style_id;
      }
      return Status{StatusCode::kInvalidArgument, "Lookup rule references unknown style id."};
    }

    if (!rule.text_role_id.empty() && FindTextRole(rule.text_role_id) == nullptr) {
      if (details != nullptr) {
        *details = "Lookup rule references unknown text role id: " + rule.text_role_id;
      }
      return Status{StatusCode::kInvalidArgument, "Lookup rule references unknown text role id."};
    }
  }

  return {};
}

Status LoadS57PortrayalCatalogFromFile(const std::string& path, S57PortrayalCatalog* out) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream.is_open()) {
    return Status{StatusCode::kNotFound, "Could not open portrayal catalog: " + path};
  }
  return ParseCatalogFromStream(stream, out);
}

const S57PortrayalCatalog& DefaultS57PortrayalCatalog() {
  static const S57PortrayalCatalog catalog = [] {
    S57PortrayalCatalog loaded;
#if defined(NAVSCENE_DEFAULT_S57_CATALOG_PATH)
    const Status file_status =
        LoadS57PortrayalCatalogFromFile(NAVSCENE_DEFAULT_S57_CATALOG_PATH, &loaded);
    if (file_status.ok()) {
      return loaded;
    }
#endif
    const Status embedded_status =
        LoadS57PortrayalCatalogFromString(kEmbeddedCatalogText, &loaded);
    if (!embedded_status.ok()) {
      return S57PortrayalCatalog{};
    }
    return loaded;
  }();
  return catalog;
}

}  // namespace navscene::portrayal
