cmake_minimum_required(VERSION 3.30)
project(JxPlayer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用Qt自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 设置路径
set(PROJECT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resource")
set(CACHE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cache")
add_definitions(-DRESOURCE_DIR="${RESOURCE_DIR}")
add_definitions(-DCACHE_DIR="${CACHE_DIR}")

# 设置vcpkg工具链文件
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

include(${CMAKE_TOOLCHAIN_FILE})

add_definitions(-DMY_MACRO)           # 定义无值的宏（等同于#define MY_MACRO）

# 添加头文件路径
set(INCLUDE_DIRS
        ${PROJECT_ROOT_DIR}/player/include
        ${PROJECT_ROOT_DIR}/player/src
        ${Qt6_INCLUDES}
        ${Qt6OpenGL_INCLUDE_DIRS}
        ${Qt6OpenGLWidgets_INCLUDE_DIRS}
        ${Qt6Multimedia_INCLUDE_DIRS}
)

find_package(Qt6 COMPONENTS
  Core
  Gui
  Widgets
  OpenGL
  OpenGLWidgets
  Multimedia
  REQUIRED)

add_executable(JxPlayer qt/main.cpp
        qt/mainwindow.cpp
        qt/mainwindow.h
        qt/mainwindow.ui
        qt/ui/playerwidget.cpp
        qt/ui/controllerwidget.cpp
        qt/ui/player_slider.cpp
        player/src/reader/demuxer.cpp
        player/src/reader/audio_decoder.cpp
        player/src/reader/video_decoder.cpp
        player/src/reader/file_reader.cpp
        player/src/core/sync_notifier.cpp
        player/src/engine/player.cpp
        player/src/engine/gl_context.cpp
        player/src/engine/av_synchronizer.cpp
        player/src/core/task_pool.cpp
        player/src/engine/audio_pipeline.cpp
        player/src/engine/video_pipeline.cpp
        player/src/engine/video_display_view.cpp
        player/src/utils/gl_utils.cpp
        player/qt/openGL_view.cpp
        player/src/engine/audio_speaker.cpp
        player/src/filter/video_filter.cpp
        player/src/filter/flip_vertical_filter.cpp
        player/src/filter/gray_filter.cpp
        player/src/filter/invert_filter.cpp
        player/src/writer/audio_encoder.cpp
        player/src/writer/video_encoder.cpp
        player/src/writer/muxer.cpp
        player/src/writer/file_writer.cpp
        player/src/filter/sticker_filter.cpp
        qt/ui/player_slider.cpp
        qt/ui/player_slider.h
        player/src/filter/pause_filter.cpp
        player/src/filter/pause_filter.h
)
#target_link_libraries(JxPlayer
#  Qt::Core
#  Qt::Gui
#  Qt::Widgets
#)

# 替换原有的FFmpeg配置部分
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED
        libavcodec
        libavformat
        libavutil
        libswscale
        libswresample
)

# 添加FFmpeg库路径和头文件路径
#target_include_directories(JxPlayer PRIVATE ${FFMPEG_INCLUDE_DIRS})
#target_link_directories(JxPlayer PRIVATE ${FFMPEG_LIBRARY_DIRS})

# 显式链接所有FFmpeg组件
target_link_libraries(JxPlayer PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        Qt6::Multimedia
        ${FFMPEG_LIBRARIES}
        avcodec
        avformat
        avutil
        swscale
        swresample
        ${PROJECT_ROOT_DIR}/third_party/InspireFace/lib/libInspireFace.dylib
        ${PROJECT_ROOT_DIR}/third_party/SoundTouch/lib/libSoundTouch.dylib
)
target_include_directories(JxPlayer PRIVATE /opt/homebrew/opt/ffmpeg/include)
target_link_directories(JxPlayer PRIVATE /opt/homebrew/opt/ffmpeg/lib)

target_include_directories(JxPlayer PRIVATE /opt/homebrew/opt/glm/include)
target_link_directories(JxPlayer PRIVATE /opt/homebrew/opt/glm/lib)
#

#find_package(FFMPEG REQUIRED)
#target_include_directories(JxPlayer PRIVATE ${FFMPEG_INCLUDE_DIRS})
#target_link_directories(JxPlayer PRIVATE ${FFMPEG_LIBRARY_DIRS})
#target_link_libraries(JxPlayer PRIVATE ${FFMPEG_LIBRARIES})
#find_package(glm CONFIG REQUIRED)
#target_link_libraries(JxPlayer PRIVATE glm::glm-header-only)
