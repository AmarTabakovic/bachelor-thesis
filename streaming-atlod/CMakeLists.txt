cmake_minimum_required(VERSION 3.5)

project(streaming-atlod LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_TARGET streaming-atlod)

add_executable(${APP_TARGET}
    src/main.cpp
    src/application.cpp
    src/shader.cpp
    src/camera.cpp
    src/skybox.cpp
    src/util.cpp
    src/terrain/terrainmanager.cpp
    src/terrain/terraintile.cpp
    src/terrain/terraincache.cpp
    src/terrain/gridmesh.cpp
    src/terrain/skirtmesh.cpp
)


add_definitions(-DGLEW_STATIC)

add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)
add_subdirectory(lib/glew EXCLUDE_FROM_ALL)
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
add_subdirectory(lib/libwebp EXCLUDE_FROM_ALL)
# add_subdirectory(lib/imgui EXCLUDE_FROM_ALL)

# Set where the ImGui files are stored
set(IMGUI_PATH  "lib/imgui")

# Compile as static library
#file(GLOB_RECURSE IMGUI_SOURCES ${IMGUI_PATH}/*.cpp)
#add_library("imgui" STATIC ${IMGUI_SOURCES})
#add_library("imgui" STATIC "lib/imgui/imgui.h")
#target_include_directories("imgui" PUBLIC ${IMGUI_PATH})

add_library(imgui STATIC
    lib/imgui/imgui.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
    lib/imgui/misc/cpp/imgui_stdlib.cpp
)

target_link_libraries(imgui PUBLIC glfw)

target_include_directories(imgui PUBLIC
    lib/imgui
    lib/imgui/backends
)

target_include_directories(${APP_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libwebp/include)

target_link_libraries(${APP_TARGET}
  PRIVATE glfw
  PRIVATE libglew_static
  PRIVATE glm
  PRIVATE webp
  PUBLIC imgui
)

#target_include_directories(atlod
#  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
#  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src
#)



#install(TARGETS streaming-atlod
#    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
