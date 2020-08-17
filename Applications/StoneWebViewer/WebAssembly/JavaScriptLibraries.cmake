set(BASE_URL "http://orthanc.osimis.io/ThirdPartyDownloads")

DownloadPackage(
  "f9746611d1d77072f5ce5a1d9e915c42"
  "${BASE_URL}/fontawesome-free-5.14.0-web.zip"
  "${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web")

DownloadPackage(
  "b40f2ec8769ebee23a395fa2b907e4b4"
  "${BASE_URL}/bootstrap-3.4.1-dist.zip"
  "${CMAKE_CURRENT_BINARY_DIR}/bootstrap-3.4.1-dist")

DownloadPackage(
  "8242afdc5bd44105d9dc9e6535315484"
  "${BASE_URL}/dicom-web/vuejs-2.6.10.tar.gz"
  "${CMAKE_CURRENT_BINARY_DIR}/vue-2.6.10")

DownloadPackage(
  "3e2b4e1522661f7fcf8ad49cb933296c"
  "${BASE_URL}/dicom-web/axios-0.19.0.tar.gz"
  "${CMAKE_CURRENT_BINARY_DIR}/axios-0.19.0")

DownloadFile(
  "220afd743d9e9643852e31a135a9f3ae"
  "${BASE_URL}/jquery-3.4.1.min.js")


install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/css/all.css
  ${CMAKE_CURRENT_BINARY_DIR}/bootstrap-3.4.1-dist/css/bootstrap.css
  DESTINATION ${ORTHANC_STONE_INSTALL_PREFIX}/css
  )

install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/bootstrap-3.4.1-dist/js/bootstrap.min.js
  ${CMAKE_SOURCE_DIR}/ThirdPartyDownloads/jquery-3.4.1.min.js
  ${CMAKE_CURRENT_BINARY_DIR}/vue-2.6.10/dist/vue.min.js
  ${CMAKE_CURRENT_BINARY_DIR}/axios-0.19.0/dist/axios.min.js
  ${CMAKE_CURRENT_BINARY_DIR}/axios-0.19.0/dist/axios.min.map
  DESTINATION ${ORTHANC_STONE_INSTALL_PREFIX}/js
  )

install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-brands-400.eot
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-brands-400.svg
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-brands-400.ttf
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-brands-400.woff
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-brands-400.woff2
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-regular-400.eot
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-regular-400.svg
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-regular-400.ttf
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-regular-400.woff
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-regular-400.woff2
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-solid-900.eot
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-solid-900.svg
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-solid-900.ttf
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-solid-900.woff
  ${CMAKE_CURRENT_BINARY_DIR}/fontawesome-free-5.14.0-web/webfonts/fa-solid-900.woff2
  DESTINATION ${ORTHANC_STONE_INSTALL_PREFIX}/webfonts
  )
