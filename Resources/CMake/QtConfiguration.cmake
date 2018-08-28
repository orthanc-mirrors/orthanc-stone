# Stone of Orthanc
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017-2018 Osimis S.A., Belgium
#
# This program is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License
# as published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License for more details.
# 
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.


# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Find the QtWidgets library
find_package(Qt5Widgets)
find_package(Qt5Core)

list(APPEND QT_SOURCES
    ${ORTHANC_STONE_ROOT}/Applications/Qt/QCairoWidget.cpp
    ${ORTHANC_STONE_ROOT}/Applications/Qt/BasicQtApplicationRunner.cpp
    ${ORTHANC_STONE_ROOT}/Applications/Qt/QStoneMainWindow.cpp
)

include_directories(${ORTHANC_STONE_ROOT}/Applications/Qt/)

link_libraries(
    Qt5::Widgets
    Qt5::Core
)
