# - build library

link_directories (${GEANT4_LIBRARY_DIR})

# library
set(_TARGET Qgeom)
add_library(
  ${_TARGET} SHARED
  QDetectorConstruction.cc
  pyQgeom.cc
)

set_target_properties(${_TARGET} PROPERTIES PREFIX "")
set_target_properties(${_TARGET}
                      PROPERTIES INSTALL_RPATH
                      ${GEANT4_LIBRARY_DIR}
                      BUILD_WITH_INSTALL_RPATH TRUE)

target_link_libraries (${_TARGET}
                      ${GEANT4_LIBRARIES_WITH_VIS} boost_python)

# install
install(TARGETS ${_TARGET} LIBRARY DESTINATION ${G4SITEMODULES_INSTALL_DIR})
