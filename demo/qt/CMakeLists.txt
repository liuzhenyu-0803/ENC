find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(navscene-demo-qt
  main.cpp
  main_window.cpp
  main_window.h
  render_widget.cpp
  render_widget.h
)

target_link_libraries(navscene-demo-qt
  PRIVATE
    navscene::navscene
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(navscene-demo-qt
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_compile_features(navscene-demo-qt PRIVATE cxx_std_20)

if (WIN32)
  add_custom_command(TARGET navscene-demo-qt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_RUNTIME_DLLS:navscene-demo-qt>
      $<TARGET_FILE_DIR:navscene-demo-qt>
    COMMAND_EXPAND_LISTS
  )

  if (NAVSCENE_HAS_GDAL)
    set(NAVSCENE_GDAL_RUNTIME_ROOT "")
    if (DEFINED ENV{OSGEO4W_ROOT} AND EXISTS "$ENV{OSGEO4W_ROOT}/bin")
      set(NAVSCENE_GDAL_RUNTIME_ROOT "$ENV{OSGEO4W_ROOT}")
    elseif (DEFINED GDAL_DIR)
      get_filename_component(NAVSCENE_GDAL_CMAKE_DIR "${GDAL_DIR}" ABSOLUTE)
      get_filename_component(NAVSCENE_GDAL_RUNTIME_ROOT "${NAVSCENE_GDAL_CMAKE_DIR}" DIRECTORY)
      get_filename_component(NAVSCENE_GDAL_RUNTIME_ROOT "${NAVSCENE_GDAL_RUNTIME_ROOT}" DIRECTORY)
      get_filename_component(NAVSCENE_GDAL_RUNTIME_ROOT "${NAVSCENE_GDAL_RUNTIME_ROOT}" DIRECTORY)
      unset(NAVSCENE_GDAL_CMAKE_DIR)
    endif ()

    if (EXISTS "${NAVSCENE_GDAL_RUNTIME_ROOT}/bin")
      set(NAVSCENE_GDAL_RUNTIME_DLL_NAMES
        gdal312.dll
        proj_9.dll
        geos.dll
        geos_c.dll
        sqlite3.dll
        libcurl.dll
        libcrypto-3-x64.dll
        libssl-3-x64.dll
        libxml2.dll
        libexpat.dll
        iconv-2.dll
        zlib.dll
        liblzma.dll
        jpeg62.dll
        openjp2.dll
        tiff.dll
        Lerc.dll
        netcdf.dll
        freexl.dll
        spatialite.dll
        libpng16.dll
        brotlicommon.dll
        brotlidec.dll
        brotlienc.dll
        blosc.dll
        lz4.dll
        zstd.dll
        xerces-c_3_2.dll
        charset-1.dll
        msvcp140.dll
        msvcp140_1.dll
        msvcp140_2.dll
        msvcp140_atomic_wait.dll
        vcruntime140.dll
        vcruntime140_1.dll
      )

      foreach (NAVSCENE_GDAL_RUNTIME_DLL IN LISTS NAVSCENE_GDAL_RUNTIME_DLL_NAMES)
        if (EXISTS "${NAVSCENE_GDAL_RUNTIME_ROOT}/bin/${NAVSCENE_GDAL_RUNTIME_DLL}")
          add_custom_command(TARGET navscene-demo-qt POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${NAVSCENE_GDAL_RUNTIME_ROOT}/bin/${NAVSCENE_GDAL_RUNTIME_DLL}"
              "$<TARGET_FILE_DIR:navscene-demo-qt>"
          )
        endif ()
      endforeach ()

      if (EXISTS "${NAVSCENE_GDAL_RUNTIME_ROOT}/bin/curl-ca-bundle.crt")
        add_custom_command(TARGET navscene-demo-qt POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${NAVSCENE_GDAL_RUNTIME_ROOT}/bin/curl-ca-bundle.crt"
            "$<TARGET_FILE_DIR:navscene-demo-qt>"
        )
      endif ()
    endif ()

    unset(NAVSCENE_GDAL_RUNTIME_ROOT)
    unset(NAVSCENE_GDAL_RUNTIME_DLL_NAMES)
  endif ()
endif ()
