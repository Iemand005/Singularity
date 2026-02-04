cmake_minimum_required(VERSION 3.19)
project(AIOneQTWidgets LANGUAGES CXX)

if(MSVC)
    add_compile_options(/FS /bigobj /MP /W3 /wd4244 /wd4267 /wd4305)
    set(CMAKE_JOB_POOLS compile=1 link=1)
    add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug" CACHE STRING "" FORCE)
    
    set(CMAKE_C_FLAGS_INIT "/FS")
    set(CMAKE_CXX_FLAGS_INIT "/FS")
    string(APPEND CMAKE_C_FLAGS " /FS")
    string(APPEND CMAKE_CXX_FLAGS " /FS")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /FS" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /FS" CACHE STRING "" FORCE)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

    # add_compile_options(/fsanitize=address /Zi)
    # add_link_options(/fsanitize=address)
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

set(AIONE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../AIOne")

# option(AIONE_VULKAN "Enable Vulkan support" ON)

qt_standard_project_setup()

qt_add_executable(AIOneQTWidgets
    # WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    README.md
    ../AIOneQTCore/QModelFactory.hpp ../AIOneQTCore/QLLModel.hpp ../AIOneQTCore/QSDModel.hpp ../AIOneQTCore/QChatManager.cpp ../AIOneQTCore/QChatManager.hpp
    ../AIOneQTCore/QTextGenOptions.h
    ${AIONE_DIR}/src/Chat.hpp ${AIONE_DIR}/src/ChatManager.hpp ${AIONE_DIR}/src/ContextInvalidError.hpp ${AIONE_DIR}/src/LlamaUtil.hpp ${AIONE_DIR}/src/LLModel.cpp ${AIONE_DIR}/src/LLModel.hpp ${AIONE_DIR}/src/LLModelOptions.hpp ${AIONE_DIR}/src/Message.hpp ${AIONE_DIR}/src/Model.cpp ${AIONE_DIR}/src/Model.hpp ${AIONE_DIR}/src/ModelFactory.cpp ${AIONE_DIR}/src/ModelFactory.hpp ${AIONE_DIR}/src/PreferredDevice.h ${AIONE_DIR}/src/Role.h ${AIONE_DIR}/src/SDImageOptions.h ${AIONE_DIR}/src/SDModel.cpp ${AIONE_DIR}/src/SDModel.hpp ${AIONE_DIR}/src/SDModelOptions.hpp ${AIONE_DIR}/src/TextContext.cpp ${AIONE_DIR}/src/TextContext.hpp ${AIONE_DIR}/src/TextContextOptions.h ${AIONE_DIR}/src/TextGenOptions.hpp ${AIONE_DIR}/src/TextGenResult.hpp ${AIONE_DIR}/src/Timeable.hpp
    ${AIONE_DIR}/src/ModelOptions.h
    imagepreview.h imagepreview.cpp
    progressbar.h progressbar.cpp
    checkbox.h checkbox.cpp
    longspinbox.h longspinbox.cpp
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
