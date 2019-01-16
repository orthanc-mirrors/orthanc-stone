# Stone of Orthanc
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017-2019 Osimis S.A., Belgium
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



#####################################################################
## Import the parameters of the Orthanc Framework
#####################################################################

message("CMAKE_CURRENT_LIST_DIR = ${CMAKE_CURRENT_LIST_DIR}")

include(${CMAKE_CURRENT_LIST_DIR}/../../Resources/Orthanc/DownloadOrthancFramework.cmake)
include(${ORTHANC_ROOT}/Resources/CMake/OrthancFrameworkParameters.cmake)

set(ENABLE_DCMTK OFF)
set(ENABLE_GOOGLE_TEST ON)
set(ENABLE_SQLITE OFF)
set(ENABLE_JPEG ON)
set(ENABLE_PNG ON)
set(ENABLE_ZLIB ON)
set(HAS_EMBEDDED_RESOURCES ON)


#####################################################################
## CMake parameters tunable by the user
#####################################################################

# Advanced parameters to fine-tune linking against system libraries
set(USE_SYSTEM_CAIRO ON CACHE BOOL "Use the system version of Cairo")
set(USE_SYSTEM_PIXMAN ON CACHE BOOL "Use the system version of Pixman")
set(USE_SYSTEM_SDL ON CACHE BOOL "Use the system version of SDL2")


#####################################################################
## Internal CMake parameters to enable the optional subcomponents of
## the Stone of Orthanc
#####################################################################

