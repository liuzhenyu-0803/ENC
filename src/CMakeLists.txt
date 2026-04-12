set(NAVSCENE_CORE_SOURCES
  core/chart_selection.cpp
  core/default_view.cpp
  core/engine.cpp
  core/logging.cpp
  data/discovery.cpp
  data/s57/reader.cpp
  data/s57/null_reader.cpp
  geo/mercator_projection.cpp
  portrayal/catalog.cpp
  portrayal/engine_area.cpp
  portrayal/engine_common.cpp
  portrayal/engine.cpp
  portrayal/engine_line.cpp
  portrayal/engine_point.cpp
  portrayal/engine_text.cpp
  portrayal/profile.cpp
  portrayal/rule_program.cpp
  portrayal/rule_program_line.cpp
  portrayal/rule_program_point.cpp
  render/backend.cpp
  render/chart_scene_builder.cpp
  render/label_layout.cpp
  render/polygon_triangulation.cpp
  render/portrayal.cpp
  render/projected_chart_scene.cpp
  render/software_raster.cpp
  render/scene_signature.cpp
  render/scene_svg_export.cpp
  render/vulkan_backend.cpp
)

if (WIN32)
  list(APPEND NAVSCENE_CORE_SOURCES
    render/win32_gdi_presenter.cpp
  )
endif ()

if (NAVSCENE_HAS_GDAL)
  list(APPEND NAVSCENE_CORE_SOURCES
    data/s57/gdal_reader.cpp
    data/s57/object_class_catalog.cpp
  )
endif ()

add_library(navscene STATIC ${NAVSCENE_CORE_SOURCES})
add_library(navscene::navscene ALIAS navscene)

target_include_directories(navscene
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/third_party/mapbox-earcut/include
)

target_compile_features(navscene PUBLIC cxx_std_20)

target_compile_definitions(navscene
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    $<$<BOOL:${NAVSCENE_HAS_VULKAN}>:NAVSCENE_HAS_VULKAN=1>
    $<$<BOOL:${NAVSCENE_HAS_GDAL}>:NAVSCENE_HAS_GDAL=1>
)

set(NAVSCENE_DEFAULT_S57_CATALOG_PATH
  "${PROJECT_SOURCE_DIR}/data/portrayal/s57_catalog.txt")
string(REPLACE "\\" "/" NAVSCENE_DEFAULT_S57_CATALOG_PATH_ESCAPED
  "${NAVSCENE_DEFAULT_S57_CATALOG_PATH}")
target_compile_definitions(navscene
  PRIVATE
    NAVSCENE_DEFAULT_S57_CATALOG_PATH="${NAVSCENE_DEFAULT_S57_CATALOG_PATH_ESCAPED}"
)

if (NAVSCENE_HAS_GDAL)
  if (TARGET GDAL::GDAL)
    set_property(TARGET GDAL::GDAL APPEND PROPERTY
      MAP_IMPORTED_CONFIG_DEBUG RelWithDebInfo)
    set_property(TARGET GDAL::GDAL APPEND PROPERTY
      MAP_IMPORTED_CONFIG_RELEASE RelWithDebInfo)
    set_property(TARGET GDAL::GDAL APPEND PROPERTY
      MAP_IMPORTED_CONFIG_MINSIZEREL RelWithDebInfo)
    target_link_libraries(navscene PRIVATE GDAL::GDAL)
  else ()
    target_include_directories(navscene PRIVATE ${GDAL_INCLUDE_DIRS})
    target_link_libraries(navscene PRIVATE ${GDAL_LIBRARIES})
  endif ()

  set(NAVSCENE_DEFAULT_GDAL_DATA_PATH "")
  if (DEFINED ENV{GDAL_DATA} AND EXISTS "$ENV{GDAL_DATA}")
    set(NAVSCENE_DEFAULT_GDAL_DATA_PATH "$ENV{GDAL_DATA}")
  elseif (DEFINED ENV{OSGEO4W_ROOT} AND EXISTS "$ENV{OSGEO4W_ROOT}/apps/gdal/share/gdal")
    set(NAVSCENE_DEFAULT_GDAL_DATA_PATH "$ENV{OSGEO4W_ROOT}/apps/gdal/share/gdal")
  elseif (DEFINED GDAL_DIR)
    get_filename_component(NAVSCENE_GDAL_CMAKE_DIR "${GDAL_DIR}" ABSOLUTE)
    get_filename_component(NAVSCENE_GDAL_PREFIX "${NAVSCENE_GDAL_CMAKE_DIR}" DIRECTORY)
    get_filename_component(NAVSCENE_GDAL_PREFIX "${NAVSCENE_GDAL_PREFIX}" DIRECTORY)
    get_filename_component(NAVSCENE_GDAL_PREFIX "${NAVSCENE_GDAL_PREFIX}" DIRECTORY)
    if (EXISTS "${NAVSCENE_GDAL_PREFIX}/apps/gdal/share/gdal")
      set(NAVSCENE_DEFAULT_GDAL_DATA_PATH "${NAVSCENE_GDAL_PREFIX}/apps/gdal/share/gdal")
    elseif (EXISTS "${NAVSCENE_GDAL_PREFIX}/share/gdal")
      set(NAVSCENE_DEFAULT_GDAL_DATA_PATH "${NAVSCENE_GDAL_PREFIX}/share/gdal")
    endif ()
  endif ()

  if (NAVSCENE_DEFAULT_GDAL_DATA_PATH)
    string(REPLACE "\\" "/" NAVSCENE_DEFAULT_GDAL_DATA_PATH_ESCAPED
      "${NAVSCENE_DEFAULT_GDAL_DATA_PATH}")
    target_compile_definitions(navscene
      PRIVATE
        NAVSCENE_DEFAULT_GDAL_DATA_PATH="${NAVSCENE_DEFAULT_GDAL_DATA_PATH_ESCAPED}"
    )
  endif ()
endif ()

if (WIN32)
  target_link_libraries(navscene PRIVATE user32 gdi32)
endif ()

if (NAVSCENE_HAS_VULKAN)
  target_link_libraries(navscene PRIVATE Vulkan::Vulkan)

  set(NAVSCENE_VK_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/vulkan")
  file(MAKE_DIRECTORY "${NAVSCENE_VK_SHADER_DIR}")

  set(NAVSCENE_VK_SHADER_SOURCES
    fullscreen_texture.vert.glsl
    fullscreen_texture.frag.glsl
    solid_color.vert.glsl
    solid_color.frag.glsl
    point_color.vert.glsl
    point_color.frag.glsl
  )

  set(NAVSCENE_VK_SHADER_OUTPUTS)
  foreach(NAVSCENE_VK_SHADER ${NAVSCENE_VK_SHADER_SOURCES})
    set(NAVSCENE_VK_SHADER_SRC
      "${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/${NAVSCENE_VK_SHADER}")
    string(REGEX MATCH "\\.(vert|frag)\\.glsl$" _ "${NAVSCENE_VK_SHADER}")
    set(NAVSCENE_VK_SHADER_STAGE "${CMAKE_MATCH_1}")
    string(REPLACE ".glsl" ".spv" NAVSCENE_VK_SHADER_OUTPUT_NAME "${NAVSCENE_VK_SHADER}")
    set(NAVSCENE_VK_SHADER_OUTPUT
      "${NAVSCENE_VK_SHADER_DIR}/${NAVSCENE_VK_SHADER_OUTPUT_NAME}")

    add_custom_command(
      OUTPUT "${NAVSCENE_VK_SHADER_OUTPUT}"
      COMMAND "${NAVSCENE_GLSLANG_VALIDATOR_EXECUTABLE}" -V -S "${NAVSCENE_VK_SHADER_STAGE}" -o "${NAVSCENE_VK_SHADER_OUTPUT}" "${NAVSCENE_VK_SHADER_SRC}"
      DEPENDS "${NAVSCENE_VK_SHADER_SRC}"
      VERBATIM
    )

    list(APPEND NAVSCENE_VK_SHADER_OUTPUTS "${NAVSCENE_VK_SHADER_OUTPUT}")
  endforeach()

  add_custom_target(navscene-vulkan-shaders
    DEPENDS ${NAVSCENE_VK_SHADER_OUTPUTS}
  )
  add_dependencies(navscene navscene-vulkan-shaders)

  string(REPLACE "\\" "/" NAVSCENE_VK_SHADER_DIR_ESCAPED "${NAVSCENE_VK_SHADER_DIR}")
  target_compile_definitions(navscene
    PRIVATE
      NAVSCENE_VK_SHADER_DIR="${NAVSCENE_VK_SHADER_DIR_ESCAPED}"
  )
endif ()
