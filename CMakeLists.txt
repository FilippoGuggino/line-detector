cmake_minimum_required(VERSION 3.16)

project(line-detector VERSION 0.0.0)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

Include(FetchContent)

# ---- catch2 ----
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0 # or a later release
)
FetchContent_MakeAvailable(Catch2)
# -------------------

# ---- spdlog ----
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.15.3
)
FetchContent_MakeAvailable(spdlog)
# -------------------

# ---- argparse ----
FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)
# -------------------

# ---- matplotlib-cpp ----
# find python libraries
find_package(Python3 COMPONENTS Interpreter Development NumPy REQUIRED)
find_package(Python 3.0 REQUIRED)
include_directories(${PYTHON3_INCLUDE_DIRS} ${NumPy_INCLUDE_DIRS})

add_compile_definitions(WITH_OPENCV)
FetchContent_Declare(
  matplotlib
  GIT_REPOSITORY https://github.com/FilippoGuggino/matplotlib-cpp.git
  GIT_TAG origin/master
)
FetchContent_GetProperties(matplotlib)

if(NOT matplotlib_POPULATED)
    FetchContent_Populate(matplotlib)
endif()

# -------------------

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/display_widget.cpp
  src/advanced_line_item.cpp
)

if(WIN32)
    set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE true)

    get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir ${_qmake_executable} DIRECTORY)

    find_program(WINDEPLOY_EXEC windeployqt HINTS ${_qt_bin_dir})

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env PATH="${_qt_bin_dir}" ${WINDEPLOY_EXEC} $<TARGET_FILE:${PROJECT_NAME}>
        --no-compiler-runtime --no-opengl-sw --no-translations --verbose=0)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:opencv_world>
        $<TARGET_FILE:Alkeria::alkusb3>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE spdlog argparse Qt${QT_VERSION_MAJOR}::Widgets ${OpenCV_LIBS} ${PYTHON_LIBRARIES} Python3::NumPy)
target_include_directories(${PROJECT_NAME} PRIVATE ${matplotlib_SOURCE_DIR})
