cmake_minimum_required(VERSION 3.15)

project(uno_server
    VERSION 2.0.0
    DESCRIPTION "UNO_server"
    HOMEPAGE_URL "https://github.com/oeasy1412/uno_server"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# include_directories(${PROJECT_SOURCE_DIR}/thirdparty)
add_executable(server ${SRC_LIST})

target_link_libraries(server PRIVATE
    d3d11
    dwmapi
    gdi32
    d3dcompiler
    ws2_32
    winmm
    pthread
)
target_compile_options(server PRIVATE
    -static-libgcc
    -static-libstdc++
    # -O2
    # -DNDEBUG
)

set(OUTPUT ${CMAKE_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${OUTPUT})