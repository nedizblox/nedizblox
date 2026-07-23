cmake_minimum_required(VERSION 3.22)

set(NEDIZBLOX_VERSION_MAJOR 0)
set(NEDIZBLOX_VERSION_MINOR 6)
set(NEDIZBLOX_VERSION_PATCH 0)

project(nedizblox LANGUAGES C CXX)

set(PROJECT_VERSION "${NEDIZBLOX_VERSION_MAJOR}.${NEDIZBLOX_VERSION_MINOR}.${NEDIZBLOX_VERSION_PATCH}")
set(PROJECT_VERSION_MAJOR ${APP_VERSION_MAJOR})
set(PROJECT_VERSION_MINOR ${APP_VERSION_MINOR})
set(PROJECT_VERSION_PATCH ${APP_VERSION_PATCH})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED COMPONENTS glslangValidator)
find_package(Freetype REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Bullet REQUIRED)
find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/glfw)

set(GLM_ENABLE_CXX_20 ON CACHE BOOL "" FORCE)
add_subdirectory(extern/glm)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_MINIZIP ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
add_subdirectory(extern/assimp)

add_subdirectory(extern/boxer)

include(FetchContent)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.6
)
FetchContent_MakeAvailable(Corrosion)
corrosion_import_crate(MANIFEST_PATH extern/rbx-dom/Cargo.toml)

file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/assets)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(SHADER_SOURCES
    shaders/model.vert
    shaders/model.frag
    shaders/text.vert
    shaders/text.frag
    shaders/billboard.vert
    shaders/imgui.vert
    shaders/imgui.frag
    shaders/skybox.vert
    shaders/skybox.frag
)

set(SPIRV_BINARY_FILES)
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPIRV_OUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPIRV_OUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        COMMAND glslangValidator -V "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}" -o "${SPIRV_OUT}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}"
        COMMENT "Compiling shader ${SHADER}"
    )

    list(APPEND SPIRV_BINARY_FILES ${SPIRV_OUT})
endforeach()

set(MAIN_SOURCES
    src/core/logger.cpp
    src/core/args_parser.cpp
    src/core/camera.cpp
    src/core/msgbox.cpp
    src/core/crypto.cpp
    src/window/window.cpp
    src/graphics/vk/device.cpp
    src/graphics/vk/swapchain.cpp
    src/graphics/vk/buffer.cpp
    src/graphics/vk/sampler.cpp
    src/graphics/vk/descriptors.cpp
    src/graphics/vk/pipeline.cpp
    src/graphics/vk/renderer.cpp
    src/graphics/model.cpp
    src/graphics/texture.cpp
    src/graphics/cubemap.cpp
    src/graphics/skybox.cpp
    src/graphics/billboard.cpp
    src/graphics/ui/text.cpp
    src/graphics/ui/imgui.cpp
    src/graphics/managers/bindless_manager.cpp
    src/graphics/managers/model_manager.cpp
    src/graphics/managers/billboard_manager.cpp
    src/graphics/managers/ui_manager.cpp
    src/audio/sound.cpp
    src/audio/music.cpp
    src/audio/audio_manager.cpp
    src/network/client.cpp
    src/game/utils/rbxl.cpp
    src/game/physics/physics.cpp
    src/game/managers/instance_manager.cpp
    src/game/managers/asset_manager.cpp
    src/game/engines/render_engine.cpp
    src/game/engines/script_engine.cpp
    src/game/prefabs/rig.cpp
    src/game/game_client.cpp
    src/main.cpp
)

set(EXTERN_SOURCES
    extern/stb/stb_image.cpp
    extern/stb/stb_vorbis.cpp
    extern/vma/vk_mem_alloc.cpp
    extern/dr/dr_wav.cpp
    extern/imgui/imgui_demo.cpp
    extern/imgui/imgui_draw.cpp
    extern/imgui/imgui_tables.cpp
    extern/imgui/imgui_widgets.cpp
    extern/imgui/imgui.cpp
    extern/wren/wren_compiler.c
    extern/wren/wren_core.c
    extern/wren/wren_debug.c
    extern/wren/wren_opt_meta.c
    extern/wren/wren_opt_random.c
    extern/wren/wren_primitive.c
    extern/wren/wren_utils.c
    extern/wren/wren_value.c
    extern/wren/wren_vm.c
)

add_executable(nedizblox-client ${MAIN_SOURCES} ${EXTERN_SOURCES} ${SPIRV_BINARY_FILES})

if(WIN32)
    set_target_properties(nedizblox-client PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    if(MSVC)
        target_link_options(nedizblox-client PRIVATE "/ENTRY:mainCRTStartup")
    endif()
endif()

target_link_libraries(nedizblox-client PRIVATE
    Vulkan::Vulkan
    glfw
    Freetype::Freetype
    OpenAL::OpenAL
    ${Boost_LIBRARIES}
    ${BULLET_LIBRARIES}
    OpenSSL::Crypto
    glm::glm
    assimp
    Boxer
    rbx_dom
)

target_compile_definitions(nedizblox-client PRIVATE
    CLIENT_VERSION_MAJOR=${NEDIZBLOX_VERSION_MAJOR}
    CLIENT_VERSION_MINOR=${NEDIZBLOX_VERSION_MINOR}
    CLIENT_VERSION_PATCH=${NEDIZBLOX_VERSION_PATCH}
)

target_compile_definitions(nedizblox-client PUBLIC GLM_ENABLE_EXPERIMENTAL BOOST_ASIO_HEADER_ONLY)

target_include_directories(nedizblox-client PRIVATE extern src ${BULLET_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})