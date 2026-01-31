cmake_minimum_required(VERSION 3.19)
project(AIOneQTWidgets LANGUAGES CXX)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

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

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools)



qt_standard_project_setup()

qt_add_executable(AIOneQTWidgets
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    README.md
    AIOneQTCore/QModelFactory.hpp AIOneQTCore/QLLModel.hpp AIOneQTCore/QSDModel.hpp
)



qt_add_translations(
    TARGETS AIOneQTWidgets
    TS_FILES AIOneQTWidgets_en_US.ts
)



target_link_libraries(AIOneQTWidgets
    PRIVATE
        Qt::Core
        Qt::Widgets
    PRIVATE AIOne::Core
)

set(AIONE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../AIOne")

add_subdirectory("${AIONE_DIR}" ${CMAKE_BINARY_DIR}/AIOneCore_build)

target_include_directories(AIOneQTWidgets PRIVATE
    ${AIONE_DIR}/src
    ${AIONE_DIR}/external/llama.cpp/src
    ${AIONE_DIR}/external/llama.cpp/include
    ${AIONE_DIR}/external/llama.cpp/ggml/include
    ${AIONE_DIR}/external/llama.cpp/vendor
    ${AIONE_DIR}/external/stable-diffusion.cpp
    ${AIONE_DIR}/external/stable-diffusion.cpp/thirdparty
)

include(GNUInstallDirs)

install(TARGETS AIOneQTWidgets
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET AIOneQTWidgets
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
