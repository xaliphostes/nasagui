cmake_minimum_required(VERSION 3.21)
project(nasagui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

# Qt 6.8 links the AGL framework, which recent macOS SDKs no longer ship.
if(APPLE AND TARGET WrapOpenGL::WrapOpenGL)
    get_target_property(_wrapopengl_libs WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
    if(_wrapopengl_libs)
        list(FILTER _wrapopengl_libs EXCLUDE REGEX "AGL")
        set_target_properties(WrapOpenGL::WrapOpenGL PROPERTIES
            INTERFACE_LINK_LIBRARIES "${_wrapopengl_libs}")
    endif()
endif()

add_library(nasagui STATIC
    include/nasagui/Theme.h
    include/nasagui/Style.h
    include/nasagui/HudPanel.h
    include/nasagui/CollapsibleDock.h
    include/nasagui/Controls.h
    include/nasagui/ModelView.h
    include/nasagui/CircularGauge.h
    include/nasagui/BarGauge.h
    include/nasagui/TelemetryPlot.h
    include/nasagui/StatusIndicator.h
    include/nasagui/HudButton.h
    include/nasagui/RadarScope.h
    src/Theme.cpp
    src/Style.cpp
    src/HudPanel.cpp
    src/CollapsibleDock.cpp
    src/Controls.cpp
    src/ModelView.cpp
    src/CircularGauge.cpp
    src/BarGauge.cpp
    src/TelemetryPlot.cpp
    src/StatusIndicator.cpp
    src/HudButton.cpp
    src/RadarScope.cpp
)
target_include_directories(nasagui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nasagui PUBLIC Qt6::Widgets Qt6::OpenGLWidgets)

add_executable(nasagui_demo demo/main.cpp)
target_link_libraries(nasagui_demo PRIVATE nasagui)

# Optional VTK demo (3D model explorer). Needs a VTK built against the SAME
# Qt as this project, e.g.:
#   cmake -B build-vtk -DCMAKE_PREFIX_PATH="$HOME/Qt/6.5.3/macos;$HOME/vtk/build_qt6"
find_package(VTK QUIET COMPONENTS
    CommonColor CommonComputationalGeometry CommonCore CommonDataModel
    FiltersCore FiltersSources GUISupportQt InteractionStyle
    IOGeometry IOLegacy IOPLY IOXML
    RenderingCore RenderingOpenGL2)
if(VTK_FOUND)
    message(STATUS "VTK ${VTK_VERSION} found - building nasagui_vtk_demo")
    add_executable(nasagui_vtk_demo
        demo-vtk/main.cpp
        demo-vtk/TSurfReader.h
        demo-vtk/TSurfReader.cpp)
    target_link_libraries(nasagui_vtk_demo PRIVATE nasagui ${VTK_LIBRARIES})
    vtk_module_autoinit(TARGETS nasagui_vtk_demo MODULES ${VTK_LIBRARIES})
else()
    message(STATUS "VTK not found - skipping nasagui_vtk_demo (set VTK_DIR or CMAKE_PREFIX_PATH)")
endif()
