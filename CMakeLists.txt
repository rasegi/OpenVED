cmake_minimum_required(VERSION 3.24)

project(ved_qt
    VERSION 0.1.0
    DESCRIPTION "Modern Qt port skeleton for VED"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC AND DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES "-static$")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(PkgConfig QUIET)

if(MSVC AND "$ENV{INCLUDE}" STREQUAL "")
    get_filename_component(VED_MSVC_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(VED_MSVC_HOST_DIR "${VED_MSVC_BIN_DIR}" DIRECTORY)
    get_filename_component(VED_MSVC_BIN_ROOT "${VED_MSVC_HOST_DIR}" DIRECTORY)
    get_filename_component(VED_MSVC_TOOLSET_DIR "${VED_MSVC_BIN_ROOT}" DIRECTORY)

    file(GLOB VED_WINDOWS_KITS_VERSIONS
        LIST_DIRECTORIES TRUE
        "C:/Program Files (x86)/Windows Kits/10/Include/*"
    )
    if(VED_WINDOWS_KITS_VERSIONS)
        list(SORT VED_WINDOWS_KITS_VERSIONS COMPARE NATURAL ORDER DESCENDING)
        list(GET VED_WINDOWS_KITS_VERSIONS 0 VED_WINDOWS_KITS_VERSION_DIR)
        get_filename_component(VED_WINDOWS_KITS_VERSION "${VED_WINDOWS_KITS_VERSION_DIR}" NAME)

        include_directories(SYSTEM
            "${VED_MSVC_TOOLSET_DIR}/include"
            "C:/Program Files (x86)/Windows Kits/10/Include/${VED_WINDOWS_KITS_VERSION}/shared"
            "C:/Program Files (x86)/Windows Kits/10/Include/${VED_WINDOWS_KITS_VERSION}/ucrt"
            "C:/Program Files (x86)/Windows Kits/10/Include/${VED_WINDOWS_KITS_VERSION}/um"
            "C:/Program Files (x86)/Windows Kits/10/Include/${VED_WINDOWS_KITS_VERSION}/winrt"
            "C:/Program Files (x86)/Windows Kits/10/Include/${VED_WINDOWS_KITS_VERSION}/cppwinrt"
        )
    endif()
endif()

if(MSVC AND "$ENV{LIB}" STREQUAL "")
    file(GLOB VED_WINDOWS_KITS_LIB_VERSIONS
        LIST_DIRECTORIES TRUE
        "C:/Program Files (x86)/Windows Kits/10/Lib/*"
    )
    if(VED_WINDOWS_KITS_LIB_VERSIONS)
        list(SORT VED_WINDOWS_KITS_LIB_VERSIONS COMPARE NATURAL ORDER DESCENDING)
        list(GET VED_WINDOWS_KITS_LIB_VERSIONS 0 VED_WINDOWS_KITS_LIB_VERSION_DIR)
        get_filename_component(VED_WINDOWS_KITS_LIB_VERSION "${VED_WINDOWS_KITS_LIB_VERSION_DIR}" NAME)

        link_directories(
            "${VED_MSVC_TOOLSET_DIR}/lib/x64"
            "C:/Program Files (x86)/Windows Kits/10/Lib/${VED_WINDOWS_KITS_LIB_VERSION}/um/x64"
            "C:/Program Files (x86)/Windows Kits/10/Lib/${VED_WINDOWS_KITS_LIB_VERSION}/ucrt/x64"
        )
    endif()
endif()

set(VED_FREETYPE_STATIC_DEPS)
set(VED_FREETYPE_INCLUDE_DIRS)
set(VED_FREETYPE_LIBRARY_DIRS)
set(VED_HARFBUZZ_STATIC_DEPS)
set(VED_HARFBUZZ_INCLUDE_DIRS)
set(VED_HARFBUZZ_LIBRARY_DIRS)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/freetype/CMakeLists.txt")
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(third_party/freetype EXCLUDE_FROM_ALL)
    set(VED_FREETYPE_LINK_TARGET freetype)
elseif(NOT VED_FREETYPE_LINK_TARGET)
    find_package(freetype CONFIG QUIET)
    if(TARGET Freetype::Freetype)
        set(VED_FREETYPE_LINK_TARGET Freetype::Freetype)
    else()
        if(PkgConfig_FOUND)
            pkg_check_modules(FREETYPE2 freetype2)
        endif()
        find_path(FREETYPE_INCLUDE_DIR
            NAMES ft2build.h
            PATHS ${FREETYPE2_INCLUDE_DIRS}
            PATH_SUFFIXES freetype2
        )
        find_library(FREETYPE_STATIC_LIBRARY
            NAMES libfreetype.a freetype
            PATHS ${FREETYPE2_LIBRARY_DIRS}
        )
        if(NOT FREETYPE_INCLUDE_DIR OR NOT FREETYPE_STATIC_LIBRARY)
            message(FATAL_ERROR "Static FreeType development files not found. Install a static freetype2 package or vendor FreeType under third_party/freetype.")
        endif()
        if(PkgConfig_FOUND)
            pkg_check_modules(FREETYPE2_STATIC freetype2)
            set(VED_FREETYPE_STATIC_DEPS ${FREETYPE2_STATIC_STATIC_LIBRARIES})
            list(REMOVE_ITEM VED_FREETYPE_STATIC_DEPS freetype)
            set(VED_FREETYPE_LIBRARY_DIRS ${FREETYPE2_STATIC_STATIC_LIBRARY_DIRS})
        endif()
        set(VED_FREETYPE_LINK_TARGET ${FREETYPE_STATIC_LIBRARY})
        set(VED_FREETYPE_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIR})
    endif()
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/harfbuzz/CMakeLists.txt")
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(third_party/harfbuzz EXCLUDE_FROM_ALL)
    set(VED_HARFBUZZ_LINK_TARGET harfbuzz)
elseif(NOT VED_HARFBUZZ_LINK_TARGET)
    find_package(harfbuzz CONFIG QUIET)
    if(TARGET harfbuzz::harfbuzz)
        set(VED_HARFBUZZ_LINK_TARGET harfbuzz::harfbuzz)
    else()
        if(PkgConfig_FOUND)
            pkg_check_modules(HARFBUZZ harfbuzz)
        endif()
        find_path(HARFBUZZ_INCLUDE_DIR
            NAMES hb.h
            PATHS ${HARFBUZZ_INCLUDE_DIRS}
            PATH_SUFFIXES harfbuzz
        )
        find_library(HARFBUZZ_STATIC_LIBRARY
            NAMES libharfbuzz.a harfbuzz
            PATHS ${HARFBUZZ_LIBRARY_DIRS}
        )
        if(NOT HARFBUZZ_INCLUDE_DIR OR NOT HARFBUZZ_STATIC_LIBRARY)
            message(FATAL_ERROR "Static HarfBuzz development files not found. Install a static harfbuzz package or vendor HarfBuzz under third_party/harfbuzz.")
        endif()
        if(PkgConfig_FOUND)
            pkg_check_modules(HARFBUZZ_STATIC harfbuzz)
            set(VED_HARFBUZZ_STATIC_DEPS ${HARFBUZZ_STATIC_STATIC_LIBRARIES})
            list(REMOVE_ITEM VED_HARFBUZZ_STATIC_DEPS harfbuzz freetype)
            set(VED_HARFBUZZ_LIBRARY_DIRS ${HARFBUZZ_STATIC_STATIC_LIBRARY_DIRS})
        endif()
        set(VED_HARFBUZZ_LINK_TARGET ${HARFBUZZ_STATIC_LIBRARY})
        set(VED_HARFBUZZ_INCLUDE_DIRS ${HARFBUZZ_INCLUDE_DIR})
    endif()
endif()

qt_standard_project_setup()

add_library(ved_core STATIC
    src/ved_core/serialization/ved_binary_reader.cpp
    src/ved_core/serialization/ved_binary_reader.h
    src/ved_core/serialization/ved_binary_format.h
    src/ved_core/serialization/ved_document_view_state.h
    src/ved_core/serialization/ved_font_resolution.cpp
    src/ved_core/serialization/ved_font_resolution.h
    src/ved_core/serialization/ved_model_io.cpp
    src/ved_core/serialization/ved_model_io.h
    src/ved_core/serialization/ved_object_io.cpp
    src/ved_core/serialization/ved_object_io.h
    src/ved_core/serialization/ved_binary_writer.cpp
    src/ved_core/serialization/ved_binary_writer.h
    src/ved_core/fontengine/vec_font.cpp
    src/ved_core/fontengine/vec_font.h
    src/ved_core/fontengine/vec_font_manager.cpp
    src/ved_core/fontengine/vec_font_manager.h
    src/ved_core/gengine/vec_graphic_engine.cpp
    src/ved_core/gengine/vec_graphic_engine.h
    src/ved_core/gengine/vec_graphic_engine_screen_state.cpp
    src/ved_core/gengine/vec_graphic_engine_screen_state.h
    src/ved_core/main/vec_maindef.h
    src/ved_core/main/vec_edit.cpp
    src/ved_core/main/vec_edit.h
    src/ved_core/main/vec_edit_cad.cpp
    src/ved_core/main/vec_edit_cad.h
    src/ved_core/main/vec_model.cpp
    src/ved_core/main/vec_model.h
    src/ved_core/main/vec_document_settings.cpp
    src/ved_core/main/vec_document_settings.h
    src/ved_core/main/vec_measure_scale.cpp
    src/ved_core/main/vec_measure_scale.h
    src/ved_core/main/vec_units.cpp
    src/ved_core/main/vec_units.h
    src/ved_core/main/vec_view_interface.cpp
    src/ved_core/main/vec_view_interface.h
    src/ved_core/mathlib/vec_math_base.cpp
    src/ved_core/mathlib/vec_math_base.h
    src/ved_core/mathlib/vec_math_graph2d.cpp
    src/ved_core/mathlib/vec_math_graph2d.h
    src/ved_core/operations/vop_base.cpp
    src/ved_core/operations/vop_base.h
    src/ved_core/operations/vop_manager.cpp
    src/ved_core/operations/vop_manager.h
    src/ved_core/operations/voc_beziercurve_controlpoint.cpp
    src/ved_core/operations/voc_beziercurve_controlpoint.h
    src/ved_core/operations/voc_circle_diameter.cpp
    src/ved_core/operations/voc_circle_diameter.h
    src/ved_core/operations/voc_circle_diagonal.cpp
    src/ved_core/operations/voc_circle_diagonal.h
    src/ved_core/operations/voc_circle_edge.cpp
    src/ved_core/operations/voc_circle_edge.h
    src/ved_core/operations/voc_circle_midpoint.cpp
    src/ved_core/operations/voc_circle_midpoint.h
    src/ved_core/operations/voc_bspline_controlpoint.cpp
    src/ved_core/operations/voc_bspline_controlpoint.h
    src/ved_core/operations/voc_ellipse_midpoint.cpp
    src/ved_core/operations/voc_ellipse_midpoint.h
    src/ved_core/operations/voc_ellipse_orthogonal.cpp
    src/ved_core/operations/voc_ellipse_orthogonal.h
    src/ved_core/operations/voc_line.cpp
    src/ved_core/operations/voc_line.h
    src/ved_core/operations/voc_polygon_smartline.cpp
    src/ved_core/operations/voc_polygon_smartline.h
    src/ved_core/operations/voc_polycurve.cpp
    src/ved_core/operations/voc_polycurve.h
    src/ved_core/operations/voc_rectangle_notrotated.cpp
    src/ved_core/operations/voc_rectangle_notrotated.h
    src/ved_core/operations/voc_rectangle_rotated.cpp
    src/ved_core/operations/voc_rectangle_rotated.h
    src/ved_core/operations/voc_roundrectangle_notrotated.cpp
    src/ved_core/operations/voc_roundrectangle_notrotated.h
    src/ved_core/operations/voc_vecframetext_empty.cpp
    src/ved_core/operations/voc_vecframetext_empty.h
    src/ved_core/operations/voc_vectext.cpp
    src/ved_core/operations/voc_vectext.h
    src/ved_core/operations/vom_change_edge_round_node.cpp
    src/ved_core/operations/vom_change_edge_round_node.h
    src/ved_core/operations/vom_delete_node.cpp
    src/ved_core/operations/vom_delete_node.h
    src/ved_core/operations/vom_delete_object.cpp
    src/ved_core/operations/vom_delete_object.h
    src/ved_core/operations/vom_insert_node.cpp
    src/ved_core/operations/vom_insert_node.h
    src/ved_core/operations/vom_insert_objects.cpp
    src/ved_core/operations/vom_insert_objects.h
    src/ved_core/operations/vom_modify_curve_attribute.cpp
    src/ved_core/operations/vom_modify_curve_attribute.h
    src/ved_core/operations/vom_move_bspline_controlpoint.cpp
    src/ved_core/operations/vom_move_bspline_controlpoint.h
    src/ved_core/operations/vom_move_node.cpp
    src/ved_core/operations/vom_move_node.h
    src/ved_core/operations/vom_move_object.cpp
    src/ved_core/operations/vom_move_object.h
    src/ved_core/operations/vom_rotate_object_activepoint.cpp
    src/ved_core/operations/vom_rotate_object_activepoint.h
    src/ved_core/operations/vom_scale_3point.cpp
    src/ved_core/operations/vom_scale_3point.h
    src/ved_core/operations/vom_select_object.cpp
    src/ved_core/operations/vom_select_object.h
    src/ved_core/operations/vom_selectmove_object_scale_frame.cpp
    src/ved_core/operations/vom_selectmove_object_scale_frame.h
    src/ved_core/operations/vom_select_move_node_object.cpp
    src/ved_core/operations/vom_select_move_node_object.h
    src/ved_core/operations/vom_vectext.cpp
    src/ved_core/operations/vom_vectext.h
    src/ved_core/vecobjects/vec_bezier_curve.cpp
    src/ved_core/vecobjects/vec_bezier_curve.h
    src/ved_core/vecobjects/vec_bspline.cpp
    src/ved_core/vecobjects/vec_bspline.h
    src/ved_core/vecobjects/vec_contur_point.h
    src/ved_core/vecobjects/vec_ellipse.cpp
    src/ved_core/vecobjects/vec_ellipse.h
    src/ved_core/vecobjects/vec_group.cpp
    src/ved_core/vecobjects/vec_group.h
    src/ved_core/vecobjects/vec_line.cpp
    src/ved_core/vecobjects/vec_line.h
    src/ved_core/vecobjects/vec_object.cpp
    src/ved_core/vecobjects/vec_object.h
    src/ved_core/vecobjects/vec_object_geometry.cpp
    src/ved_core/vecobjects/vec_object_geometry.h
    src/ved_core/vecobjects/vec_polygon.cpp
    src/ved_core/vecobjects/vec_polygon.h
    src/ved_core/vecobjects/vec_polygon_area.cpp
    src/ved_core/vecobjects/vec_polygon_area.h
    src/ved_core/vecobjects/vec_polycurve.cpp
    src/ved_core/vecobjects/vec_polycurve.h
    src/ved_core/vecobjects/vec_polycurve_area.cpp
    src/ved_core/vecobjects/vec_polycurve_area.h
    src/ved_core/vecobjects/vec_text.cpp
    src/ved_core/vecobjects/vec_text.h
)

target_include_directories(ved_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ved_core/gengine"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ved_core/main"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ved_core/mathlib"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ved_core/operations"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ved_core/serialization"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ved_core/fontengine"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ved_core/vecobjects"
)

set_target_properties(ved_core PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

add_library(ved_qt_gengine STATIC
    src/ved_qt/gengine/vec_graphic_engine_qt.cpp
    src/ved_qt/gengine/vec_graphic_engine_qt.h
)

target_include_directories(ved_qt_gengine
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ved_qt/gengine"
)

target_link_libraries(ved_qt_gengine
    PUBLIC
        ved_core
        Qt6::Gui
)

set_target_properties(ved_qt_gengine PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

file(GLOB VED_TOOLBAR_ICONS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app/resources/toolbar/*.bmp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app/resources/toolbar/*.png"
)

file(GLOB VED_NODE_IMAGES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app/resources/nodes/*.bmp"
)

file(GLOB VED_CURSOR_IMAGES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app/resources/cursors/*.png"
)

file(GLOB VED_FONT_RESOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app/resources/font/*.vfn"
)

qt_add_executable(ved_qt_app
    src/app/main.cpp
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/app/MainWindowCurveDock.cpp
    src/app/MainWindowRotateDock.cpp
    src/app/MainWindowRoundRectangleDock.cpp
    src/app/MainWindowTextDock.cpp
    src/app/MainWindowToolbars.cpp
    src/app/PageSetupDialog.cpp
    src/app/PageSetupDialog.h
    src/app/QtVecFontProviders.cpp
    src/app/QtVecFontProviders.h
    src/app/QVedWidget.cpp
    src/app/QVedWidget.h
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.qt/rcc")
qt_add_resources(ved_qt_app "ved_resources"
    PREFIX "/ved"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/src/app/resources"
    FILES
        ${VED_TOOLBAR_ICONS}
        ${VED_NODE_IMAGES}
        ${VED_CURSOR_IMAGES}
        ${VED_FONT_RESOURCES}
)

target_link_libraries(ved_qt_app
    PRIVATE
        ved_qt_gengine
        Qt6::Widgets
        ${VED_HARFBUZZ_LINK_TARGET}
        ${VED_FREETYPE_LINK_TARGET}
        ${VED_HARFBUZZ_STATIC_DEPS}
        ${VED_FREETYPE_STATIC_DEPS}
)

target_include_directories(ved_qt_app
    PRIVATE
        ${VED_FREETYPE_INCLUDE_DIRS}
        ${VED_HARFBUZZ_INCLUDE_DIRS}
)

target_link_directories(ved_qt_app
    PRIVATE
        ${VED_FREETYPE_LIBRARY_DIRS}
        ${VED_HARFBUZZ_LIBRARY_DIRS}
)

set_target_properties(ved_qt_app PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

function(ved_copy_qt_runtime target)
    if(WIN32 AND TARGET Qt6::Core AND TARGET Qt6::QWindowsIntegrationPlugin)
        get_target_property(VED_QT_CORE_DLL Qt6::Core IMPORTED_LOCATION_DEBUG)
        if(NOT VED_QT_CORE_DLL)
            get_target_property(VED_QT_CORE_DLL Qt6::Core IMPORTED_LOCATION)
        endif()
        if(VED_QT_CORE_DLL)
            get_filename_component(VED_QT_BIN_DIR "${VED_QT_CORE_DLL}" DIRECTORY)
            file(GLOB VED_QT_RUNTIME_DLLS CONFIGURE_DEPENDS "${VED_QT_BIN_DIR}/*.dll")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:${target}>/platforms"
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${VED_QT_RUNTIME_DLLS} "$<TARGET_FILE_DIR:${target}>"
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>" "$<TARGET_FILE_DIR:${target}>/platforms/"
                VERBATIM
            )
        endif()
    endif()
endfunction()

ved_copy_qt_runtime(ved_qt_app)

enable_testing()

add_executable(ved_core_line_tests
    tests/ved_core_line_tests.cpp
)

add_executable(ved_core_model_tests
    tests/ved_core_model_tests.cpp
)

add_executable(ved_core_select_operation_tests
    tests/ved_core_select_operation_tests.cpp
)

add_executable(ved_core_select_move_operation_tests
    tests/ved_core_select_move_operation_tests.cpp
)

add_executable(ved_core_delete_operation_tests
    tests/ved_core_delete_operation_tests.cpp
)

add_executable(ved_core_move_object_operation_tests
    tests/ved_core_move_object_operation_tests.cpp
)

add_executable(ved_core_select_scale_frame_operation_tests
    tests/ved_core_select_scale_frame_operation_tests.cpp
)

add_executable(ved_core_circle_ellipse_operation_tests
    tests/ved_core_circle_ellipse_operation_tests.cpp
)

add_executable(ved_core_rectangle_operation_tests
    tests/ved_core_rectangle_operation_tests.cpp
)

add_executable(ved_core_polygon_polycurve_operation_tests
    tests/ved_core_polygon_polycurve_operation_tests.cpp
)

add_executable(ved_core_curve_object_tests
    tests/ved_core_curve_object_tests.cpp
)

add_executable(ved_core_change_edge_round_node_operation_tests
    tests/ved_core_change_edge_round_node_operation_tests.cpp
)

add_executable(ved_core_node_operation_tests
    tests/ved_core_node_operation_tests.cpp
)

add_executable(ved_core_scale_3point_operation_tests
    tests/ved_core_scale_3point_operation_tests.cpp
)

add_executable(ved_core_rotate_operation_tests
    tests/ved_core_rotate_operation_tests.cpp
)

add_executable(ved_core_zoom_pan_tests
    tests/ved_core_zoom_pan_tests.cpp
)

add_executable(ved_core_serialization_tests
    tests/ved_core_serialization_tests.cpp
)

add_executable(ved_core_model_io_tests
    tests/ved_core_model_io_tests.cpp
)

add_executable(ved_core_vec_font_tests
    tests/ved_core_vec_font_tests.cpp
)

add_executable(ved_core_text_tests
    tests/ved_core_text_tests.cpp
)

add_executable(ved_core_document_settings_tests
    tests/ved_core_document_settings_tests.cpp
)

add_executable(ved_core_measure_scale_tests
    tests/ved_core_measure_scale_tests.cpp
)

add_executable(ved_core_units_tests
    tests/ved_core_units_tests.cpp
)

add_executable(ved_qt_gengine_outline_tests
    tests/ved_qt_gengine_outline_tests.cpp
)

target_link_libraries(ved_core_line_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_model_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_select_operation_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_select_move_operation_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_delete_operation_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_move_object_operation_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_select_scale_frame_operation_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_circle_ellipse_operation_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_rectangle_operation_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_polygon_polycurve_operation_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_curve_object_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_change_edge_round_node_operation_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_node_operation_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_scale_3point_operation_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_rotate_operation_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_zoom_pan_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_serialization_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_model_io_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_vec_font_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_text_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_document_settings_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_measure_scale_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_core_units_tests
    PRIVATE
        ved_core
)

target_link_libraries(ved_qt_gengine_outline_tests
    PRIVATE
        ved_qt_gengine
        Qt6::Gui
)

add_test(NAME ved_core_line_tests COMMAND ved_core_line_tests)
add_test(NAME ved_core_model_tests COMMAND ved_core_model_tests)
add_test(NAME ved_core_select_operation_tests COMMAND ved_core_select_operation_tests)
add_test(NAME ved_core_select_move_operation_tests COMMAND ved_core_select_move_operation_tests)
add_test(NAME ved_core_delete_operation_tests COMMAND ved_core_delete_operation_tests)
add_test(NAME ved_core_move_object_operation_tests COMMAND ved_core_move_object_operation_tests)
add_test(NAME ved_core_select_scale_frame_operation_tests COMMAND ved_core_select_scale_frame_operation_tests)
add_test(NAME ved_core_circle_ellipse_operation_tests COMMAND ved_core_circle_ellipse_operation_tests)
add_test(NAME ved_core_rectangle_operation_tests COMMAND ved_core_rectangle_operation_tests)
add_test(NAME ved_core_polygon_polycurve_operation_tests COMMAND ved_core_polygon_polycurve_operation_tests)
add_test(NAME ved_core_curve_object_tests COMMAND ved_core_curve_object_tests)
add_test(NAME ved_core_change_edge_round_node_operation_tests COMMAND ved_core_change_edge_round_node_operation_tests)
add_test(NAME ved_core_node_operation_tests COMMAND ved_core_node_operation_tests)
add_test(NAME ved_core_scale_3point_operation_tests COMMAND ved_core_scale_3point_operation_tests)
add_test(NAME ved_core_rotate_operation_tests COMMAND ved_core_rotate_operation_tests)
add_test(NAME ved_core_zoom_pan_tests COMMAND ved_core_zoom_pan_tests)
add_test(NAME ved_core_serialization_tests COMMAND ved_core_serialization_tests)
add_test(NAME ved_core_model_io_tests COMMAND ved_core_model_io_tests)
add_test(NAME ved_core_vec_font_tests COMMAND ved_core_vec_font_tests)
add_test(NAME ved_core_text_tests COMMAND ved_core_text_tests)
add_test(NAME ved_core_document_settings_tests COMMAND ved_core_document_settings_tests)
add_test(NAME ved_core_measure_scale_tests COMMAND ved_core_measure_scale_tests)
add_test(NAME ved_core_units_tests COMMAND ved_core_units_tests)
add_test(NAME ved_qt_gengine_outline_tests COMMAND ved_qt_gengine_outline_tests)
set_tests_properties(ved_qt_gengine_outline_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
