# SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@gmail.com>
#
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.14)

project(myers LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)

add_executable(myers
  main.cpp
)
target_link_libraries(myers Qt${QT_VERSION_MAJOR}::Core)
