add_executable(navscene-smoke
  engine_smoke.cpp
)

target_link_libraries(navscene-smoke
  PRIVATE
    navscene::navscene
)

target_compile_features(navscene-smoke PRIVATE cxx_std_20)
target_compile_definitions(navscene-smoke
  PRIVATE
    $<$<BOOL:${NAVSCENE_HAS_VULKAN}>:NAVSCENE_HAS_VULKAN=1>
    $<$<BOOL:${NAVSCENE_HAS_GDAL}>:NAVSCENE_HAS_GDAL=1>
)

add_test(
  NAME navscene-smoke
  COMMAND navscene-smoke
)

add_executable(navscene-error-paths
  engine_error_paths.cpp
)

target_link_libraries(navscene-error-paths
  PRIVATE
    navscene::navscene
)

target_compile_features(navscene-error-paths PRIVATE cxx_std_20)
target_compile_definitions(navscene-error-paths
  PRIVATE
    $<$<BOOL:${NAVSCENE_HAS_VULKAN}>:NAVSCENE_HAS_VULKAN=1>
)

add_test(
  NAME navscene-error-paths
  COMMAND navscene-error-paths
)

add_executable(navscene-portrayal-mvp
  portrayal_mvp.cpp
)

target_link_libraries(navscene-portrayal-mvp
  PRIVATE
    navscene::navscene
)
target_include_directories(navscene-portrayal-mvp
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_compile_features(navscene-portrayal-mvp PRIVATE cxx_std_20)

add_test(
  NAME navscene-portrayal-mvp
  COMMAND navscene-portrayal-mvp
)

add_executable(navscene-portrayal-catalog
  portrayal_catalog.cpp
)

target_link_libraries(navscene-portrayal-catalog
  PRIVATE
    navscene::navscene
)
target_include_directories(navscene-portrayal-catalog
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_compile_features(navscene-portrayal-catalog PRIVATE cxx_std_20)

add_test(
  NAME navscene-portrayal-catalog
  COMMAND navscene-portrayal-catalog
)

add_executable(navscene-chart-selection
  chart_selection.cpp
)

target_link_libraries(navscene-chart-selection
  PRIVATE
    navscene::navscene
)
target_include_directories(navscene-chart-selection
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_compile_features(navscene-chart-selection PRIVATE cxx_std_20)

add_test(
  NAME navscene-chart-selection
  COMMAND navscene-chart-selection
)

add_executable(navscene-chart-scene-builder-filters
  chart_scene_builder_filters.cpp
)

target_link_libraries(navscene-chart-scene-builder-filters
  PRIVATE
    navscene::navscene
)
target_include_directories(navscene-chart-scene-builder-filters
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_compile_features(navscene-chart-scene-builder-filters PRIVATE cxx_std_20)

add_test(
  NAME navscene-chart-scene-builder-filters
  COMMAND navscene-chart-scene-builder-filters
)

add_executable(navscene-scene-signature
  scene_signature.cpp
)

target_link_libraries(navscene-scene-signature
  PRIVATE
    navscene::navscene
)
target_include_directories(navscene-scene-signature
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_compile_features(navscene-scene-signature PRIVATE cxx_std_20)

add_test(
  NAME navscene-scene-signature
  COMMAND navscene-scene-signature
)

add_executable(navscene-scene-svg
  scene_svg_export.cpp
)

target_link_libraries(navscene-scene-svg
  PRIVATE
    navscene::navscene
)
target_include_directories(navscene-scene-svg
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_compile_features(navscene-scene-svg PRIVATE cxx_std_20)

add_test(
  NAME navscene-scene-svg
  COMMAND navscene-scene-svg
)

add_executable(navscene-projected-chart-scene
  projected_chart_scene.cpp
)

target_link_libraries(navscene-projected-chart-scene
  PRIVATE
    navscene::navscene
)
target_include_directories(navscene-projected-chart-scene
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_compile_features(navscene-projected-chart-scene PRIVATE cxx_std_20)

add_test(
  NAME navscene-projected-chart-scene
  COMMAND navscene-projected-chart-scene
)

add_executable(navscene-label-layout
  label_layout.cpp
)

target_link_libraries(navscene-label-layout
  PRIVATE
    navscene::navscene
)
target_include_directories(navscene-label-layout
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_compile_features(navscene-label-layout PRIVATE cxx_std_20)

add_test(
  NAME navscene-label-layout
  COMMAND navscene-label-layout
)

add_executable(navscene-polygon-triangulation
  polygon_triangulation.cpp
)

target_link_libraries(navscene-polygon-triangulation
  PRIVATE
    navscene::navscene
)
target_include_directories(navscene-polygon-triangulation
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_compile_features(navscene-polygon-triangulation PRIVATE cxx_std_20)

add_test(
  NAME navscene-polygon-triangulation
  COMMAND navscene-polygon-triangulation
)

add_executable(navscene-layer-composition
  layer_composition.cpp
)

target_link_libraries(navscene-layer-composition
  PRIVATE
    navscene::navscene
)
target_compile_features(navscene-layer-composition PRIVATE cxx_std_20)
target_compile_definitions(navscene-layer-composition
  PRIVATE
    $<$<BOOL:${NAVSCENE_HAS_GDAL}>:NAVSCENE_HAS_GDAL=1>
)

add_test(
  NAME navscene-layer-composition
  COMMAND navscene-layer-composition
)

if (WIN32 AND NAVSCENE_HAS_VULKAN)
  add_executable(navscene-vulkan-win32-render-modes
    vulkan_win32_render_modes.cpp
  )

  target_link_libraries(navscene-vulkan-win32-render-modes
    PRIVATE
      navscene::navscene
  )
  target_include_directories(navscene-vulkan-win32-render-modes
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src
  )
  target_compile_features(navscene-vulkan-win32-render-modes PRIVATE cxx_std_20)
  target_compile_definitions(navscene-vulkan-win32-render-modes
    PRIVATE
      NAVSCENE_HAS_VULKAN=1
  )

  add_test(
    NAME navscene-vulkan-win32-render-modes
    COMMAND navscene-vulkan-win32-render-modes
  )
endif ()

if (NAVSCENE_HAS_GDAL)
  add_executable(navscene-s57-feature-dump
    s57_feature_dump.cpp
  )

  target_link_libraries(navscene-s57-feature-dump
    PRIVATE
      navscene::navscene
  )
  target_include_directories(navscene-s57-feature-dump
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src
  )
  target_compile_features(navscene-s57-feature-dump PRIVATE cxx_std_20)
  target_compile_definitions(navscene-s57-feature-dump
    PRIVATE
      NAVSCENE_HAS_GDAL=1
  )

  add_executable(navscene-s57-samples
    s57_sample_validation.cpp
  )

  target_link_libraries(navscene-s57-samples
    PRIVATE
      navscene::navscene
  )
  target_include_directories(navscene-s57-samples
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src
  )

  target_compile_features(navscene-s57-samples PRIVATE cxx_std_20)
  target_compile_definitions(navscene-s57-samples
    PRIVATE
      NAVSCENE_HAS_GDAL=1
  )

  add_test(
    NAME navscene-s57-samples
    COMMAND navscene-s57-samples
  )

  if (WIN32)
    add_executable(navscene-s57-reference-render
      s57_reference_render.cpp
    )

    target_link_libraries(navscene-s57-reference-render
      PRIVATE
        navscene::navscene
    )
    target_include_directories(navscene-s57-reference-render
      PRIVATE
        ${PROJECT_SOURCE_DIR}/src
    )
    target_compile_features(navscene-s57-reference-render PRIVATE cxx_std_20)
    target_compile_definitions(navscene-s57-reference-render
      PRIVATE
        NAVSCENE_HAS_GDAL=1
    )

    add_test(
      NAME navscene-s57-reference-render
      COMMAND navscene-s57-reference-render
    )
  endif ()
endif ()

set(NAVSCENE_ALL_TESTS
  navscene-smoke
  navscene-error-paths
  navscene-portrayal-mvp
  navscene-portrayal-catalog
  navscene-chart-selection
  navscene-chart-scene-builder-filters
  navscene-scene-signature
  navscene-scene-svg
  navscene-projected-chart-scene
  navscene-label-layout
  navscene-polygon-triangulation
  navscene-layer-composition
)

if (WIN32 AND NAVSCENE_HAS_VULKAN)
  list(APPEND NAVSCENE_ALL_TESTS
    navscene-vulkan-win32-render-modes
  )
endif ()

if (NAVSCENE_HAS_GDAL)
  list(APPEND NAVSCENE_ALL_TESTS
    navscene-s57-samples
  )
  if (WIN32)
    list(APPEND NAVSCENE_ALL_TESTS
      navscene-s57-reference-render
    )
  endif ()
endif ()

set(NAVSCENE_TEST_ENVIRONMENT)
set(NAVSCENE_TEST_ENVIRONMENT_MODIFICATION)

if (DEFINED ENV{OSGEO4W_ROOT} AND EXISTS "$ENV{OSGEO4W_ROOT}/bin")
  list(APPEND NAVSCENE_TEST_ENVIRONMENT_MODIFICATION
    "PATH=path_list_prepend:$ENV{OSGEO4W_ROOT}/bin")
endif ()

if (DEFINED ENV{VULKAN_SDK} AND EXISTS "$ENV{VULKAN_SDK}/Bin")
  list(APPEND NAVSCENE_TEST_ENVIRONMENT_MODIFICATION
    "PATH=path_list_prepend:$ENV{VULKAN_SDK}/Bin")
endif ()

if (DEFINED ENV{GDAL_DATA})
  list(APPEND NAVSCENE_TEST_ENVIRONMENT "GDAL_DATA=$ENV{GDAL_DATA}")
elseif (DEFINED ENV{OSGEO4W_ROOT} AND EXISTS "$ENV{OSGEO4W_ROOT}/apps/gdal/share/gdal")
  list(APPEND NAVSCENE_TEST_ENVIRONMENT
    "GDAL_DATA=$ENV{OSGEO4W_ROOT}/apps/gdal/share/gdal")
elseif (DEFINED GDAL_DIR)
  get_filename_component(NAVSCENE_GDAL_CMAKE_DIR "${GDAL_DIR}" ABSOLUTE)
  get_filename_component(NAVSCENE_GDAL_PREFIX "${NAVSCENE_GDAL_CMAKE_DIR}" DIRECTORY)
  get_filename_component(NAVSCENE_GDAL_PREFIX "${NAVSCENE_GDAL_PREFIX}" DIRECTORY)
  get_filename_component(NAVSCENE_GDAL_PREFIX "${NAVSCENE_GDAL_PREFIX}" DIRECTORY)
  if (EXISTS "${NAVSCENE_GDAL_PREFIX}/apps/gdal/share/gdal")
    list(APPEND NAVSCENE_TEST_ENVIRONMENT
      "GDAL_DATA=${NAVSCENE_GDAL_PREFIX}/apps/gdal/share/gdal")
  endif ()
endif ()

if (DEFINED ENV{PROJ_LIB})
  list(APPEND NAVSCENE_TEST_ENVIRONMENT "PROJ_LIB=$ENV{PROJ_LIB}")
elseif (DEFINED ENV{OSGEO4W_ROOT} AND EXISTS "$ENV{OSGEO4W_ROOT}/share/proj")
  list(APPEND NAVSCENE_TEST_ENVIRONMENT
    "PROJ_LIB=$ENV{OSGEO4W_ROOT}/share/proj")
elseif (DEFINED NAVSCENE_GDAL_PREFIX AND EXISTS "${NAVSCENE_GDAL_PREFIX}/share/proj")
  list(APPEND NAVSCENE_TEST_ENVIRONMENT
    "PROJ_LIB=${NAVSCENE_GDAL_PREFIX}/share/proj")
endif ()

if (DEFINED ENV{NAVSCENE_S57_SAMPLE_ROOT})
  list(APPEND NAVSCENE_TEST_ENVIRONMENT
    "NAVSCENE_S57_SAMPLE_ROOT=$ENV{NAVSCENE_S57_SAMPLE_ROOT}")
elseif (EXISTS "${PROJECT_SOURCE_DIR}/../enc")
  list(APPEND NAVSCENE_TEST_ENVIRONMENT
    "NAVSCENE_S57_SAMPLE_ROOT=${PROJECT_SOURCE_DIR}/../enc")
endif ()

if (NAVSCENE_TEST_ENVIRONMENT OR NAVSCENE_TEST_ENVIRONMENT_MODIFICATION)
  string(JOIN ";" NAVSCENE_TEST_ENVIRONMENT_STRING ${NAVSCENE_TEST_ENVIRONMENT})
  string(JOIN ";" NAVSCENE_TEST_ENVIRONMENT_MODIFICATION_STRING
    ${NAVSCENE_TEST_ENVIRONMENT_MODIFICATION})
  foreach(NAVSCENE_TEST_NAME IN LISTS NAVSCENE_ALL_TESTS)
    if (NAVSCENE_TEST_ENVIRONMENT)
      set_tests_properties(${NAVSCENE_TEST_NAME}
        PROPERTIES
          ENVIRONMENT "${NAVSCENE_TEST_ENVIRONMENT_STRING}"
      )
    endif ()
    if (NAVSCENE_TEST_ENVIRONMENT_MODIFICATION)
      set_tests_properties(${NAVSCENE_TEST_NAME}
        PROPERTIES
          ENVIRONMENT_MODIFICATION "${NAVSCENE_TEST_ENVIRONMENT_MODIFICATION_STRING}"
      )
    endif ()
  endforeach()
endif ()
