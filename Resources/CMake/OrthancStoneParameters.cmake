# Stone of Orthanc
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017 Osimis, Belgium
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

# TODO => Import
SET(ORTHANC_ROOT /home/jodogne/Subversion/orthanc)

include(${ORTHANC_ROOT}/Resources/CMake/OrthancFrameworkParameters.cmake)


#####################################################################
## CMake parameters tunable by the user
#####################################################################

# Advanced parameters to fine-tune linking against system libraries
SET(USE_SYSTEM_CAIRO ON CACHE BOOL "Use the system version of Cairo")
SET(USE_SYSTEM_PIXMAN ON CACHE BOOL "Use the system version of Pixman")
SET(USE_SYSTEM_SDL ON CACHE BOOL "Use the system version of SDL2")


#####################################################################
## Internal CMake parameters to enable the optional subcomponents of
## the Stone of Orthanc
#####################################################################

SET(ENABLE_SDL ON CACHE INTERNAL "Include support for SDL")
