cmake_minimum_required(VERSION 3.16)

project(AIOneQT VERSION 0.1 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# option(AIONE_VULKAN "Enable Vulkan support" ON)

# Better Qt detection - don't hardcode paths
if(MSVC)
    set(Qt6_ROOT_DIR "/Qt/6.10.1/msvc2022_64")
else()
    set(Qt6_ROOT_DIR "/Qt/6.10.2/mingw_64")
endif()

set(Qt6_DIR "${Qt6_ROOT_DIR}/lib/cmake/Qt6")

# Add CMAKE_PREFIX_PATH for Qt (this is what find_package actually uses)
if(MSVC)
    if(EXISTS "C:/Qt/6.10.1/msvc2022_64/lib/cmake/Qt6")
        list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.10.1/msvc2022_64")
    endif()
else()
    # Mingw - MSYS2 paths
    if(EXISTS "/Qt/6.10.2/mingw_64")
        list(APPEND CMAKE_PREFIX_PATH "/Qt/6.10.2/mingw_64")
    endif()
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# if(MSVC)
#     add_compile_options(/FS /bigobj /MP /W3 /wd4244 /wd4267)
#     string(APPEND CMAKE_C_FLAGS " /FS")
#     string(APPEND CMAKE_CXX_FLAGS " /FS")
#     set(CMAKE_JOB_POOLS compile=1 link=1)
#     add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
#     set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug" CACHE STRING "" FORCE)

#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS" CACHE STRING "" FORCE)
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS" CACHE STRING "" FORCE)
#     set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /FS" CACHE STRING "" FORCE)
#     set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /FS" CACHE STRING "" FORCE)
# endif()

# /==

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(QT_QML_DEBUG)
    add_compile_definitions(QT_DECLARATIVE_DEBUG)
endif()





# Qt setup - Find ALL required Qt6 components
find_package(Qt6 REQUIRED COMPONENTS 
    Core
    Quick
    Qml
    Gui
    Widgets
    Network
)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(AIOneQT 
    main.cpp
    AIOneQTCore/QModelFactory.hpp AIOneQTCore/QLLModel.hpp AIOneQTCore/QSDModel.hpp
)



qt_add_qml_module(AIOneQT 
    URI AIOne
    VERSION 1.0
    QML_FILES
        Main.qml
        AIOButton.ui.qml
    SOURCES 
        UIHandler.h 
        UIHandler.cpp
)

# if(AIONE_VULKAN)
#     target_link_libraries(AIOneQT PRIVATE Vulkan::Vulkan)
# endif()
set(AIONE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/AIOne")

add_subdirectory("${AIONE_DIR}" ${CMAKE_BINARY_DIR}/AIOneCore_build)

target_include_directories(AIOneQT PRIVATE
    ${AIONE_DIR}/src
    ${AIONE_DIR}/external/llama.cpp/src
    ${AIONE_DIR}/external/llama.cpp/include
    ${AIONE_DIR}/external/llama.cpp/ggml/include
    ${AIONE_DIR}/external/stable-diffusion.cpp
    ${AIONE_DIR}/external/stable-diffusion.cpp/thirdparty
)

target_link_libraries(AIOneQT 
    PRIVATE 
        Qt6::Core
        Qt6::Quick
        Qt6::Qml
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
    PRIVATE AIOne::Core
)

# target_include_directories(AIOneQT PRIVATE
#     ${LLAMA_CPP_DIR}
#     ${LLAMA_CPP_DIR}/src
#     ${STABLE_DIFFUSION_CPP_DIR}
# )

set_target_properties(AIOneQT PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    DEBUG_POSTFIX "d"
)

if(MSVC)
    set_target_properties(llama PROPERTIES
        COMPILE_PDB_NAME "llama"
        COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb"
    )

    set_target_properties(stable-diffusion PROPERTIES
        COMPILE_PDB_NAME "stable-diffusion"
        COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb"
    )

    # Create PDB directory
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/pdb")
endif()