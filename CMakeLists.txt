cmake_minimum_required(VERSION 3.16)

project(idk LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

# raylib качается автоматически из официального репозитория:
#     https://github.com/raysan5/raylib
#
# * Есть git — клонируем репозиторий (`git clone`).
# * Нет git — берём тот же закреплённый релиз исходным архивом прямо с GitHub
#   (это всё тот же официальный репозиторий, просто без лишних инструментов).
include(FetchContent)
find_package(Git QUIET)

set(RAYLIB_VERSION 5.5)

# Нам нужна только библиотека, а не примеры raylib.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

if(GIT_FOUND)
    message(STATUS "Fetching raylib ${RAYLIB_VERSION} from official repo via git")
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY  https://github.com/raysan5/raylib.git
        GIT_TAG         ${RAYLIB_VERSION}
        GIT_SHALLOW     TRUE
        GIT_PROGRESS    TRUE
    )
else()
    message(STATUS "Git not found: downloading raylib ${RAYLIB_VERSION} source archive from GitHub")
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    )
endif()

FetchContent_MakeAvailable(raylib)

# игруля
add_executable(idk
    src/main.cpp
    src/Game.cpp
    src/Player.cpp
    src/Map.cpp
    src/Enemy.cpp
    src/Renderer.cpp
    src/Textures.cpp
    src/Audio.cpp
)

target_include_directories(idk PRIVATE src)
target_link_libraries(idk PRIVATE raylib)
target_link_options(idk PRIVATE
    -static-libgcc -static-libstdc++)

if(MSVC)
    target_compile_options(idk PRIVATE /W4)
else()
    target_compile_options(idk PRIVATE -Wall -Wextra)
endif()