#-----------------------------------------------------------------------
# CMakeLists.txt
# Module : G4memoryprotection
# Package: Geant4.src.G4externals.G4memoryprotection
#
# CMakeLists.txt for G4memoryprotection. We do not use the usual Geant4
# sources.cmake approach because G4memoryprotection requires a custom install
# solution for its headers. These are in a nested structure so this needs
# to be replicated when we install, i.e:
# <INCLUDE>
# +- geant4
#    +- memoryprotection
#       +- UDSignals.h
#       ...
#
# Created on: 18/04/2013
#
# $Date$
# $Revision$
# $Author$
#
#------------------------------------------------------------------------------

#-----------------------------------------------------------------------
# Need pthreads
#
find_package(Threads)

#-----------------------------------------------------------------------
# Force the compiler flags to always use -g -O0
# Handle both build and configuration types
set(CMAKE_C_FLAGS "-g -O0")

string(TOUPPER "${CMAKE_BUILD_TYPE}" MEMORYPROTECTION_BUILD_TYPE)
set(CMAKE_C_FLAGS_${MEMORYPROTECTION_BUILD_TYPE} "")

foreach(_type ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER "${_type}" _upper_type)
  set(CMAKE_C_FLAGS_${MEMORYPROTECTION_BUILD_TYPE} "")
endforeach()

#----------------------------------------------------------------------------
# Includes for this modules
#
include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/source/global/management/include)

#----------------------------------------------------------------------------
# List the headers and sources
#
# - Headers
set(G4memoryprotection_HEADERS
    include/memoryprotection/MemorySegment.h
    include/memoryprotection/ProtectedMemory.h
    include/memoryprotection/UDSignals.h
    include/memoryprotection/filter.h
)

# - Sources
set(G4memoryprotection_SOURCES
    src/ProtectedMemory.cc
    src/filter.cc
)

#----------------------------------------------------------------------------
# Now add the library targets
#
include(Geant4MacroLibraryTargets)
GEANT4_LIBRARY_TARGET(NAME G4memoryprotection SOURCES ${G4memoryprotection_SOURCES} ${G4memoryprotection_HEADERS})

#----------------------------------------------------------------------------
# Ensure the header directory gets added to the list of ones to export
#
set_property(GLOBAL APPEND PROPERTY GEANT4_BUILDTREE_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

#----------------------------------------------------------------------------
# Install the headers, retaining their nested structure
#
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
    COMPONENT Development
    PATTERN ".svn" EXCLUDE
    PATTERN "*.h")

#-----------------------------------------------------------------------
# Build the tracer program
#
add_executable(tracer tracer.cxx)
target_link_libraries(tracer ${CMAKE_THREAD_LIBS_INIT})
