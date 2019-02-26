

macro(GetFilenameFromPath TargetVariable Path)
#message(STATUS "GetFilenameFromPath (1): Path = ${Path} TargetVariable = ${${TargetVariable}}")
string(REPLACE "\\" "/" PathWithFwdSlashes "${Path}")
string(REGEX REPLACE "^.*/" "" ${TargetVariable} "${PathWithFwdSlashes}")
#message(STATUS "GetFilenameFromPath (2): Path = ${Path} Path = ${PathWithFwdSlashes} TargetVariable = ${TargetVariable}")
endmacro()

macro(GetFilePathWithoutLastExtension TargetVariable FilePath)
string(REGEX REPLACE "(^.*)\\.([^\\.]+)" "\\1" ${TargetVariable} "${FilePath}")
#message(STATUS "GetFileNameWithoutLastExtension: FilePath = ${FilePath} TargetVariable = ${${TargetVariable}}")
endmacro()

macro(Test_GetFilePathWithoutLastExtension)
set(tmp "/prout/zi/goui.goui.cpp")
GetFilePathWithoutLastExtension(TargetVariable "${tmp}")
if(NOT ("${TargetVariable}" STREQUAL "/prout/zi/goui.goui"))
  message(FATAL_ERROR "Test_GetFilePathWithoutLastExtension failed (1)")
else()
  #message(STATUS "Test_GetFilePathWithoutLastExtension: <<OK>>")
endif()
endmacro()

Test_GetFilePathWithoutLastExtension()

macro(Test_GetFilenameFromPath)
set(tmp "/prout/../../dada/zi/goui.goui.cpp")
GetFilenameFromPath(TargetVariable "${tmp}")
if(NOT ("${TargetVariable}" STREQUAL "goui.goui.cpp"))
  message(FATAL_ERROR "Test_GetFilenameFromPath failed")
else()
  #message(STATUS "Test_GetFilenameFromPath: <<OK>>")
endif()
endmacro()

Test_GetFilenameFromPath()
