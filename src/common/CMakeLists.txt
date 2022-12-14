################################################################################
# Copyright 2018-2022, Barcelona Supercomputing Center (BSC), Spain            #
# Copyright 2015-2022, Johannes Gutenberg Universitaet Mainz, Germany          #
#                                                                              #
# This software was partially supported by the                                 #
# EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).    #
#                                                                              #
# This software was partially supported by the                                 #
# ADA-FS project under the SPPEXA project funded by the DFG.                   #
#                                                                              #
# This file is part of GekkoFS.                                                #
#                                                                              #
# GekkoFS is free software: you can redistribute it and/or modify              #
# it under the terms of the GNU General Public License as published by         #
# the Free Software Foundation, either version 3 of the License, or            #
# (at your option) any later version.                                          #
#                                                                              #
# GekkoFS is distributed in the hope that it will be useful,                   #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU General Public License for more details.                                 #
#                                                                              #
# You should have received a copy of the GNU General Public License            #
# along with GekkoFS.  If not, see <https://www.gnu.org/licenses/>.            #
#                                                                              #
# SPDX-License-Identifier: GPL-3.0-or-later                                    #
################################################################################

include(FetchContent)

add_subdirectory(arithmetic)

add_library(distributor STATIC)
set_property(TARGET distributor PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(distributor
    PUBLIC
    ${INCLUDE_DIR}/common/rpc/distributor.hpp
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/rpc/distributor.cpp
    )

add_library(statistics STATIC)
set_property(TARGET statistics PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(statistics
    PUBLIC
    ${INCLUDE_DIR}/common/statistics/stats.hpp
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/statistics/stats.cpp
    )


if(GKFS_ENABLE_PROMETHEUS)
    find_package(CURL REQUIRED)
    find_package(prometheus-cpp REQUIRED)
    set(PROMETHEUS_LINK_LIBRARIES
        prometheus-cpp::pull
        prometheus-cpp::push
        prometheus-cpp::core
        curl)
    target_include_directories(statistics PRIVATE ${prometheus-cpp_INCLUDE_DIR})
endif()

  target_link_libraries(statistics
      PRIVATE
      ${PROMETHEUS_LINK_LIBRARIES}
  )


if(GKFS_ENABLE_CODE_COVERAGE)
  target_code_coverage(distributor AUTO)
  target_code_coverage(statistics AUTO)
endif()

# get spdlog
set(FETCHCONTENT_QUIET ON)

if (EXISTS ${CMAKE_SOURCE_DIR}/external/spdlog)
  message(STATUS "[gkfs] Using git submodule spdlog...")
  FetchContent_Declare(spdlog
      SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/spdlog
      )
else()
  message(STATUS "[gkfs] git submodule spdlog not found. Downloading...")
  FetchContent_Declare(spdlog
      GIT_REPOSITORY https://github.com/gabime/spdlog.git
      GIT_TAG eb3220622e73a4889eee355ffa37972b3cac3df5 # v1.9.2
      GIT_SHALLOW ON
      GIT_PROGRESS ON
      )
endif()
FetchContent_MakeAvailable(spdlog)

add_library(log_util STATIC)
set_property(TARGET log_util PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(log_util
    PUBLIC
    ${INCLUDE_DIR}/common/log_util.hpp
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/log_util.cpp
    )
target_link_libraries(log_util
  PRIVATE
    spdlog
    )

if(GKFS_ENABLE_CODE_COVERAGE)
  target_code_coverage(log_util AUTO)
endif()

add_library(env_util STATIC)
set_property(TARGET env_util PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(env_util
    PUBLIC
    ${INCLUDE_DIR}/common/env_util.hpp
    PRIVATE
    ${INCLUDE_DIR}/config.hpp
    ${CMAKE_CURRENT_LIST_DIR}/env_util.cpp
    )

if(GKFS_ENABLE_CODE_COVERAGE)
  target_code_coverage(env_util AUTO)
endif()

add_library(metadata STATIC)
set_property(TARGET metadata PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(metadata
    PUBLIC
    ${INCLUDE_DIR}/common/metadata.hpp
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/metadata.cpp
    )
target_link_libraries(metadata
  PRIVATE
    fmt::fmt
    )

if(GKFS_ENABLE_CODE_COVERAGE)
  target_code_coverage(metadata AUTO)
endif()

add_library(path_util
  STATIC
  )
set_property(TARGET path_util PROPERTY POSITION_INDEPENDENT_CODE ON)

target_sources(path_util
  PRIVATE
    path_util.cpp
  PUBLIC
    ${INCLUDE_DIR}/common/path_util.hpp
  )
