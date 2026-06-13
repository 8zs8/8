cmake_minimum_required(VERSION 3.15)
project(QuickWebLauncher)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 仅在 Windows 下可用
if(NOT WIN32)
    message(FATAL_ERROR "本项目仅支持 Windows 平台。")
endif()

# 资源文件：使用 rc → res (MSVC，MinGW 共用
set(APP_RES    app.cpp app.rc manifest.xml)
set(INST_RES   installer.cpp installer.rc manifest.xml)

# 主程序：后台托盘 + 置顶悬浮按钮
add_executable(app WIN32 ${APP_RES})
target_link_libraries(app shell32 shlwapi user32 gdi32 ole32 uuid)

# 安装程序：一键安装
add_executable(installer WIN32 ${INST_RES})
target_link_libraries(installer shell32 shlwapi user32 gdi32 ole32 uuid)
