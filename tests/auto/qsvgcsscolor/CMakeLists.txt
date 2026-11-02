# Copyright (C) 2026 Daniel Campos Ramos <Capitain_Jack@yahoo.com>
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsvgcsscolor Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qsvgcsscolor LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qsvgcsscolor
    SOURCES
        tst_qsvgcsscolor.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::SvgPrivate
)
