#pragma once

#include <string>
#include <string_view>

namespace navscene::data::s57 {

struct ObjectClassInfo {
  int code = 0;
  std::string object_class_name;
  std::string acronym;
};

const ObjectClassInfo* FindObjectClassInfo(int code);
bool HasObjectClassAcronym(std::string_view acronym, std::string_view expected);

}  // namespace navscene::data::s57
