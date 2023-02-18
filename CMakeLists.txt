#
# Copyright (C) 2023 Intel Corporation
# SPDX-License-Identifier: Apache 2.0
#

#

set(CMAKE_POLICY_DEFAULT_CMP0115 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0116 OLD)

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project("BIT_COMPACTOR")

set(BITCOMPACTOR_TARGET_NAME "bit_compactor")

add_library(${BITCOMPACTOR_TARGET_NAME}
    SHARED
    "${CMAKE_CURRENT_SOURCE_DIR}/src")

set_target_properties(${BITCOMPACTOR_TARGET_NAME}
    PROPERTIES
        LINKER_LANGUAGE CXX)

target_include_directories(${BITCOMPACTOR_TARGET_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
