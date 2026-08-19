cmake_minimum_required(VERSION 3.22)

set(NEDIZBLOX_VERSION_MAJOR 0)
set(NEDIZBLOX_VERSION_MINOR 7)
set(NEDIZBLOX_VERSION_PATCH 1)

project(nedizblox 
    VERSION ${NEDIZBLOX_VERSION_MAJOR}.${NEDIZBLOX_VERSION_MINOR}.${NEDIZBLOX_VERSION_PATCH}
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SERVER_BUILD "Build Nedizblox Server (without any graphics and audio backends)" OFF)
if(SERVER_BUILD)
    message(STATUS "Building Nedizblox Server")
else()
    message(STATUS "Building Nedizblox Client")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-unsafe-buffer-usage -Wno-unsafe-buffer-usage-in-libc-call)
endif()

if(NOT SERVER_BUILD)
    find_package(Vulkan REQUIRED COMPONENTS glslangValidator)
    find_package(Freetype REQUIRED)
    find_package(OpenAL REQUIRED)
endif()

find_package(Bullet REQUIRED)
find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(NOT SERVER_BUILD)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(extern/glfw)

    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_MINIZIP ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
    add_subdirectory(extern/assimp)

    add_subdirectory(extern/boxer)
endif()

set(GLM_ENABLE_CXX_20 ON CACHE BOOL "" FORCE)
add_subdirectory(extern/glm)

include(FetchContent)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.6
)
FetchContent_MakeAvailable(Corrosion)
corrosion_import_crate(MANIFEST_PATH extern/rbx-dom/Cargo.toml)

if(NOT SERVER_BUILD)
    set(SHADER_SOURCES
        shaders/model.vert
        shaders/model.frag
        shaders/model_outline.vert
        shaders/model_outline.frag
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
            COMMAND Vulkan::glslangValidator -V "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}" -o "${SPIRV_OUT}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}"
            COMMENT "Compiling shader ${SHADER}"
        )

        list(APPEND SPIRV_BINARY_FILES ${SPIRV_OUT})
    endforeach()
endif()

set(BASIC_MAIN_SOURCES
    src/core/logger.cpp
    src/core/args_parser.cpp
    src/core/signal_handler.cpp
    src/core/crypto.cpp
    src/scripting/script_engine.cpp
    src/game/prefabs/rig.cpp
    src/game/utils/rbxl.cpp
    src/game/physics/physics.cpp
)

set(CLIENT_MAIN_SOURCES
    ${BASIC_MAIN_SOURCES}
    src/core/camera.cpp
    src/core/msgbox.cpp
    src/window/window_desktop.cpp
    src/graphics/vk/device.cpp
    src/graphics/vk/swapchain.cpp
    src/graphics/vk/buffer.cpp
    src/graphics/vk/sampler.cpp
    src/graphics/vk/descriptors.cpp
    src/graphics/vk/pipeline.cpp
    src/graphics/vk/renderer.cpp
    src/graphics/geometries.cpp
    src/graphics/model.cpp
    src/graphics/model_outline.cpp
    src/graphics/texture.cpp
    src/graphics/cubemap.cpp
    src/graphics/skybox.cpp
    src/graphics/billboards/text.cpp
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
    src/game/utils/controller.cpp
    src/game/utils/devtools.cpp
    src/game/multiplayer/replicator.cpp
    src/game/multiplayer/ownership.cpp
    src/game/managers/instance_manager.cpp
    src/game/managers/asset_manager.cpp
    src/game/managers/render_manager.cpp
    src/game/render_context.cpp
    src/game/game_client.cpp
    src/client_main.cpp
)

set(SERVER_MAIN_SOURCES
    ${BASIC_MAIN_SOURCES}
    src/network/server.cpp
    src/game/game_server.cpp
    src/server_main.cpp
)

set(BASIC_EXTERN_SOURCES
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

set(CLIENT_EXTERN_SOURCES
    ${BASIC_EXTERN_SOURCES}
    extern/stb/stb_image.cpp
    extern/stb/stb_vorbis.cpp
    extern/vma/vk_mem_alloc.cpp
    extern/dr/dr_wav.cpp
    extern/imgui/imgui_demo.cpp
    extern/imgui/imgui_draw.cpp
    extern/imgui/imgui_tables.cpp
    extern/imgui/imgui_widgets.cpp
    extern/imgui/imgui.cpp
)

set(SERVER_EXTERN_SOURCES
    ${BASIC_EXTERN_SOURCES}
)

if(SERVER_BUILD)
    set(EXECUTABLE_NAME nedizblox-server)
    add_executable(${EXECUTABLE_NAME} ${SERVER_EXTERN_SOURCES} ${SERVER_MAIN_SOURCES})

    target_compile_definitions(${EXECUTABLE_NAME} PUBLIC SERVER_BUILD)
else()
    set(EXECUTABLE_NAME nedizblox-client)
    add_executable(${EXECUTABLE_NAME} ${CLIENT_EXTERN_SOURCES} ${SPIRV_BINARY_FILES} ${CLIENT_MAIN_SOURCES})

    target_compile_definitions(${EXECUTABLE_NAME} PUBLIC CLIENT_BUILD)

    add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_CURRENT_BINARY_DIR}/assets
        COMMENT "Copying assets to build directory"
    )
    add_dependencies(${EXECUTABLE_NAME} copy_assets)
endif()

if(WIN32)
    target_compile_definitions(${EXECUTABLE_NAME} PUBLIC PLATFORM_WINDOWS)

    if(NOT SERVER_BUILD)
        set_target_properties(${EXECUTABLE_NAME} PROPERTIES
            WIN32_EXECUTABLE TRUE
        )

        if(MSVC)
            target_link_options(${EXECUTABLE_NAME} PRIVATE "/ENTRY:mainCRTStartup")
        endif()
    endif()
elseif(UNIX)
    target_compile_definitions(${EXECUTABLE_NAME} PUBLIC PLATFORM_UNIX)
endif()

set(BASIC_LIBRARIES
    Boost::headers
    ${BULLET_LIBRARIES}
    OpenSSL::Crypto
    glm::glm
    rbx_dom
)

set(CLIENT_LIBRARIES
    ${BASIC_LIBRARIES}
    Vulkan::Vulkan
    glfw
    Freetype::Freetype
    OpenAL::OpenAL
    assimp
    Boxer
)

set(SERVER_LIBRARIES
    ${BASIC_LIBRARIES}
)

if(NOT SERVER_BUILD)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${CLIENT_LIBRARIES})
else()
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${SERVER_LIBRARIES})
endif()

target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
    PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

target_compile_definitions(${EXECUTABLE_NAME} PUBLIC GLM_ENABLE_EXPERIMENTAL)

target_include_directories(${EXECUTABLE_NAME} PRIVATE extern src ${BULLET_INCLUDE_DIRS})