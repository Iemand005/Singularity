cmake_minimum_required(VERSION 3.16)

project(AIOne VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# First, find Qt6 BEFORE anything else
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Clear any previous Qt settings
unset(QT_QMAKE_EXECUTABLE CACHE)
unset(Qt6_DIR CACHE)

# For MSYS2 UCRT64 - use the correct path
if(WIN32 AND NOT MSVC)
    # For MSYS2 UCRT64 environment
    set(CMAKE_PREFIX_PATH "/ucrt64;${CMAKE_PREFIX_PATH}")
    message(STATUS "Setting CMAKE_PREFIX_PATH for MSYS2 UCRT64")
endif()

# Find Qt6 - CRITICAL: This must come before any Qt-related operations
find_package(Qt6 6.0 REQUIRED COMPONENTS Core Gui Widgets Qml Quick Network)

# Now we can use Qt CMake functions
qt_standard_project_setup()

# Options
option(AIONE_OPENCL "Enable OpenCL support via CLBlast" OFF)
option(AIONE_VULKAN "Enable Vulkan support" OFF)

if(MSVC)
    add_compile_options(/FS /bigobj /MP /W3 /wd4244 /wd4267)
    add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# Third-party directories
set(AIONE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/AIOne")
get_filename_component(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/AIOne/external" ABSOLUTE)
set(LLAMA_CPP_DIR "${THIRD_PARTY_DIR}/llama.cpp")
set(STABLE_DIFFUSION_CPP_DIR "${THIRD_PARTY_DIR}/stable-diffusion.cpp")

# Handle Vulkan
if(AIONE_VULKAN)
    find_package(Vulkan REQUIRED)
    if(Vulkan_FOUND)
        add_definitions(-DGGML_USE_VULKAN -DSD_USE_VULKAN)
        set(LLAMA_VULKAN ON CACHE BOOL "Enable Vulkan for llama.cpp" FORCE)
        set(GGML_VULKAN ON CACHE BOOL "Enable Vulkan for ggml" FORCE)
        set(SD_VULKAN ON CACHE BOOL "Enable Vulkan for stable-diffusion.cpp" FORCE)
        message(STATUS "Vulkan support: ENABLED")
    else()
        message(FATAL_ERROR "Vulkan not found! Install Vulkan SDK")
    endif()
endif()

if(AIONE_OPENCL)
    set(LLAMA_CLBLAST ON CACHE BOOL "Enable CLBlast for OpenCL" FORCE)
    set(SD_OPENCL ON CACHE BOOL "Enable Stable Diffusion OpenCL" FORCE)
    set(GGML_OPENCL ON CACHE BOOL "Enable GGML OpenCL backend" FORCE)
    message(STATUS "OpenCL support: ENABLED")
endif()

set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(LLAMA_METAL OFF CACHE BOOL "" FORCE)
set(LLAMA_CUDA OFF CACHE BOOL "" FORCE)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(QT_QML_DEBUG)
    add_compile_definitions(QT_DECLARATIVE_DEBUG)
endif()

add_subdirectory(${LLAMA_CPP_DIR} ${CMAKE_BINARY_DIR}/llama.cpp_build)
add_subdirectory(${STABLE_DIFFUSION_CPP_DIR} ${CMAKE_BINARY_DIR}/stable-diffusion.cpp_build)

# Create executable with Qt integration
qt_add_executable(AIOne
    MANUAL_FINALIZATION  # We'll finalize after adding sources
    main.cpp
    UIHandler.cpp
    AIOne/src/TextContext.cpp
    # Add all other source files
)

# Set target properties
set_target_properties(AIOne PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE OFF
)

# Add QML module if needed (simpler approach)
qt_add_qml_module(AIOne
    URI AIOne
    VERSION 1.0
    QML_FILES
        Main.qml
        AIOButton.ui.qml
)

# Include directories
target_include_directories(AIOne PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LLAMA_CPP_DIR}
    ${LLAMA_CPP_DIR}/src
    ${STABLE_DIFFUSION_CPP_DIR}
)

# Link libraries - MUST include ALL Qt modules used
target_link_libraries(AIOne PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
    llama
    stable-diffusion
)

if(AIONE_VULKAN)
    target_link_libraries(AIOne PRIVATE Vulkan::Vulkan)
endif()

# Finalize the Qt executable
qt_finalize_executable(AIOne)

# Set output directory
set_target_properties(AIOne PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    DEBUG_POSTFIX "d"
)

# For MSYS2/MINGW: Copy Qt DLLs
if(MINGW)
    # Function to copy Qt DLLs
    function(copy_qt_dlls target)
        # Get all linked Qt targets
        get_target_property(linked_libs ${target} LINK_LIBRARIES)
        
        foreach(lib ${linked_libs})
            if(TARGET ${lib})
                get_target_property(type ${lib} TYPE)
                if(type STREQUAL "SHARED_LIBRARY")
                    add_custom_command(TARGET ${target} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "$<TARGET_FILE:${lib}>"
                            "$<TARGET_FILE_DIR:${target}>"
                    )
                endif()
            endif()
        endforeach()
    endfunction()
    
    copy_qt_dlls(AIOne)
endif()