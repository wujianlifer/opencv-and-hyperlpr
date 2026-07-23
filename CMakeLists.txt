cmake_minimum_required(VERSION 3.24)
project(opencvAndHyperlpr)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
if(NOT DEFINED CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "E:/Qt/6.10.3/msvc2022_64")
endif()
if(NOT DEFINED OpenCV_DIR)
    set(OpenCV_DIR "C:/Program Files/opencv/build/x64/vc16/lib")
endif()
find_package(OpenCV REQUIRED)

set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

find_package(Qt6 COMPONENTS
        Core
        Gui
        Qml
        Quick
        QuickControls2
        REQUIRED)

# 主窗口（QML）转换批次：后端 + QML 界面。
# 图像操作按模块归类到 sources/manipulators、车牌识别归类到 sources/recognition；
# 遗留的 Widgets 对话框（histogram/noise/licence_recognition/mainWindow）已移入 legacy/，不再编译。
add_executable(opencvAndHyperlpr WIN32
        # ---- 核心：入口与后端 ----
        sources/core/main.cpp
        headers/core/globe_define_words.h
        headers/core/image_processor.hpp
        sources/core/image_processor.cpp
        headers/core/processor_worker.hpp
        sources/core/processor_worker.cpp
        headers/core/image_provider.hpp
        sources/core/image_provider.cpp
        headers/core/based.hpp
        sources/core/based.cpp
        headers/core/util.hpp
        sources/core/util.cpp
        # ---- 图像操作（manipulators） ----
        headers/manipulators/manipulator.hpp
        sources/manipulators/manipulator.cpp
        headers/manipulators/segmentation_manipulator.hpp
        sources/manipulators/segmentation_manipulator.cpp
        headers/manipulators/frequency_manipulator.hpp
        sources/manipulators/frequency_manipulator.cpp
        headers/manipulators/dimension_manipulator.hpp
        sources/manipulators/dimension_manipulator.cpp
        headers/manipulators/morphology_manipulator.hpp
        sources/manipulators/morphology_manipulator.cpp
        headers/manipulators/arts_manipulator.hpp
        sources/manipulators/arts_manipulator.cpp
        # ---- 车牌识别（recognition） ----
        headers/recognition/hyperlpr_wrapper.hpp
        sources/recognition/hyperlpr_wrapper.cpp
        headers/recognition/licence_recognition_util.hpp
        sources/recognition/licence_recognition_util.cpp
        qml.qrc
        ui/icons/resources.qrc
        templates.qrc
)

target_include_directories(opencvAndHyperlpr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/headers/core
    ${CMAKE_CURRENT_SOURCE_DIR}/headers/manipulators
    ${CMAKE_CURRENT_SOURCE_DIR}/headers/recognition
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/HyperLPR-master/cpp/c_api
)

if (MSVC)
    # 避免硬编码 Windows SDK 版本路径：换机器/CI 上对应版本不存在会配置失败
    foreach (_kit_lib
            "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64"
            "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.15063.0/um/x64")
        if (EXISTS "${_kit_lib}")
            target_link_directories(opencvAndHyperlpr PRIVATE "${_kit_lib}")
        endif ()
    endforeach ()
endif ()

target_link_libraries(opencvAndHyperlpr
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickControls2
        ${OpenCV_LIBS}
        ${THIRDPARTY_DIR}/hyperlpr3.lib
        ${THIRDPARTY_DIR}/MNN.lib
        winmm
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${THIRDPARTY_DIR}/MNN.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${THIRDPARTY_DIR}/hyperlpr3.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${OpenCV_DIR}/../bin/opencv_world4120$<$<CONFIG:Debug>:d>.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>")



add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resource/models/r2_mobile")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/HyperLPR-master/resource/models/r2_mobile"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resource/models/r2_mobile")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resource/font")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/HyperLPR-master/resource/font/platech.ttf"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resource/font/")

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
    
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_INSTALL_PATH}/bin")
    if (WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} --no-translations
                        --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml"
                        "$<TARGET_FILE:${PROJECT_NAME}>")
    endif ()
endif ()
