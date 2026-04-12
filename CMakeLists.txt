cmake_minimum_required(VERSION 3.30)

project(navscene_sdk
  VERSION 0.1.0
  LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NAVSCENE_BUILD_QT_DEMO "Build the Qt demo host application." ON)
option(NAVSCENE_ENABLE_VULKAN "Enable Vulkan backend implementation." ON)
option(NAVSCENE_ENABLE_GDAL "Enable GDAL-based S-57 loading." OFF)

set(NAVSCENE_HAS_VULKAN OFF)
set(NAVSCENE_HAS_GDAL OFF)

if (NAVSCENE_ENABLE_VULKAN)
  if (WIN32 AND NOT DEFINED ENV{VULKAN_SDK} AND EXISTS "C:/VulkanSDK")
    file(GLOB _navscene_vulkan_roots LIST_DIRECTORIES true "C:/VulkanSDK/*")
    if (_navscene_vulkan_roots)
      list(SORT _navscene_vulkan_roots COMPARE NATURAL ORDER ASCENDING)
      list(REVERSE _navscene_vulkan_roots)
      list(GET _navscene_vulkan_roots 0 _navscene_vulkan_sdk_root)
      set(ENV{VULKAN_SDK} "${_navscene_vulkan_sdk_root}")
    endif ()
    unset(_navscene_vulkan_roots)
    unset(_navscene_vulkan_sdk_root)
  endif ()

  find_package(Vulkan QUIET)
  find_program(NAVSCENE_GLSLANG_VALIDATOR_EXECUTABLE
    NAMES glslangValidator glslangValidator.exe
    HINTS
      "$ENV{VULKAN_SDK}/Bin"
      "$ENV{VULKAN_SDK}/Bin32"
  )

  if (Vulkan_FOUND AND NAVSCENE_GLSLANG_VALIDATOR_EXECUTABLE)
    set(NAVSCENE_HAS_VULKAN ON)
    message(STATUS "navscene: Vulkan SDK found. Vulkan backend can be enabled.")
  elseif (Vulkan_FOUND)
    message(WARNING
      "navscene: Vulkan headers/libs were found, but glslangValidator was not found. "
      "The build will continue without the Vulkan backend.")
  else ()
    message(WARNING
      "navscene: NAVSCENE_ENABLE_VULKAN=ON but Vulkan SDK was not found. "
      "The build will continue with the software backend.")
  endif ()
endif ()

if (NAVSCENE_ENABLE_GDAL)
  if (DEFINED GDAL_DIR AND NOT DEFINED ENV{OSGEO4W_ROOT})
    get_filename_component(_navscene_gdal_cmake_dir "${GDAL_DIR}" ABSOLUTE)
    get_filename_component(_navscene_gdal_prefix "${_navscene_gdal_cmake_dir}" DIRECTORY)
    get_filename_component(_navscene_gdal_prefix "${_navscene_gdal_prefix}" DIRECTORY)
    get_filename_component(_navscene_gdal_prefix "${_navscene_gdal_prefix}" DIRECTORY)
    if (EXISTS "${_navscene_gdal_prefix}/lib/gdal312.lib")
      set(ENV{OSGEO4W_ROOT} "${_navscene_gdal_prefix}")
    endif ()
    unset(_navscene_gdal_cmake_dir)
    unset(_navscene_gdal_prefix)
  endif ()

  find_package(GDAL CONFIG QUIET)
  if (NOT GDAL_FOUND)
    find_package(GDAL QUIET)
  endif ()
  if (GDAL_FOUND)
    set(NAVSCENE_HAS_GDAL ON)
    message(STATUS "navscene: GDAL found. S-57 loading can be enabled.")
  else ()
    message(WARNING
      "navscene: NAVSCENE_ENABLE_GDAL=ON but GDAL was not found. "
      "The build will continue without GDAL-backed S-57 parsing support.")
  endif ()
endif ()

add_subdirectory(src)

if (NAVSCENE_BUILD_QT_DEMO)
  add_subdirectory(demo/qt)
endif ()

if (BUILD_TESTING)
  add_subdirectory(tests)
endif ()
