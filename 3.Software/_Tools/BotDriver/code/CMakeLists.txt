cmake_minimum_required(VERSION 3.16)
project(BotDriver LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(botdriver
  src/main.cpp
  src/botdriver.cpp
)

find_package(PkgConfig QUIET)
# 优先通过 pkg-config 查找 libusb-1.0（适用于 vcpkg/MSYS2 等环境）
if(PKG_CONFIG_FOUND)
  pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
  target_include_directories(botdriver PRIVATE ${LIBUSB_INCLUDE_DIRS})
  if(LIBUSB_LIBRARY_DIRS)
    target_link_directories(botdriver PRIVATE ${LIBUSB_LIBRARY_DIRS})
  endif()
  target_link_libraries(botdriver PRIVATE ${LIBUSB_LIBRARIES})
  target_compile_definitions(botdriver PRIVATE ${LIBUSB_CFLAGS_OTHER})
else()
  if(NOT DEFINED ENV{LIBUSB_ROOT})
    if(EXISTS "D:/tools/vcpkg-master/installed/x64-windows/include/libusb-1.0/libusb.h")
      set(ENV{LIBUSB_ROOT} "D:/tools/vcpkg-master/installed/x64-windows")
    endif()
  endif()
  if(DEFINED ENV{LIBUSB_ROOT})
    set(CMAKE_PREFIX_PATH "$ENV{LIBUSB_ROOT};${CMAKE_PREFIX_PATH}")
  endif()
  # 备用路径查找：直接定位头文件与库文件
  find_path(LIBUSB_INCLUDE_DIR libusb.h
    HINTS
      $ENV{LIBUSB_ROOT}/include
      $ENV{LIBUSB_ROOT}/include/libusb-1.0
      ${CMAKE_PREFIX_PATH}/include
      ${CMAKE_PREFIX_PATH}/include/libusb-1.0
  )
  find_library(LIBUSB_LIBRARY NAMES usb-1.0 libusb-1.0
    HINTS
      $ENV{LIBUSB_ROOT}/lib
      ${CMAKE_PREFIX_PATH}/lib
  )
  if(NOT LIBUSB_INCLUDE_DIR OR NOT LIBUSB_LIBRARY)
    message(FATAL_ERROR "libusb-1.0 not found. Install via vcpkg or MSYS2.")
  endif()
  target_include_directories(botdriver PRIVATE ${LIBUSB_INCLUDE_DIR})
  target_link_libraries(botdriver PRIVATE ${LIBUSB_LIBRARY})
endif()

add_library(botdriverlib SHARED src/botdriver_lib.cpp src/botdriver.cpp)
target_include_directories(botdriverlib PRIVATE src)
if(PKG_CONFIG_FOUND)
  target_include_directories(botdriverlib PRIVATE ${LIBUSB_INCLUDE_DIRS})
  if(LIBUSB_LIBRARY_DIRS)
    target_link_directories(botdriverlib PRIVATE ${LIBUSB_LIBRARY_DIRS})
  endif()
  target_link_libraries(botdriverlib PRIVATE ${LIBUSB_LIBRARIES})
  target_compile_definitions(botdriverlib PRIVATE ${LIBUSB_CFLAGS_OTHER})
else()
  target_include_directories(botdriverlib PRIVATE ${LIBUSB_INCLUDE_DIR})
  target_link_libraries(botdriverlib PRIVATE ${LIBUSB_LIBRARY})
endif()

add_executable(botdriver_ui WIN32 src/ui_win32.cpp)
target_include_directories(botdriver_ui PRIVATE src)
target_link_libraries(botdriver_ui PRIVATE botdriverlib user32 gdi32)
