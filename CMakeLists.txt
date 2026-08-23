cmake_minimum_required(VERSION 3.16)
project(LunarDraw VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

add_executable(lunardraw
    src/main.cpp
    src/shape.cpp
    src/layer.cpp
    src/project.cpp
    src/canvas.cpp
    src/mainwindow.cpp
)

target_include_directories(lunardraw PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GTK3_INCLUDE_DIRS}
)

target_link_libraries(lunardraw PRIVATE
    ${GTK3_LIBRARIES}
    m
)

target_compile_options(lunardraw PRIVATE ${GTK3_CFLAGS_OTHER})

# Install target
install(TARGETS lunardraw DESTINATION bin)

# Install desktop file
install(FILES lunardraw.desktop DESTINATION share/applications)

# Install MIME type
install(FILES lunardraw-mime.xml DESTINATION share/mime/packages)
