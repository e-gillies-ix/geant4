# $Id: GNUmakefile 75578 2013-11-04 12:03:33Z gcosmo $
# --------------------------------------------------------------
# GNUmakefile for geometry.  Gabriele Cosmo, 16/11/96.
# --------------------------------------------------------------

name := G4geometry

ifndef G4INSTALL
  G4INSTALL = ../..
endif

GLOBLIBS  = libG4graphics_reps.lib libG4intercoms.lib libG4global.lib

include $(G4INSTALL)/config/architecture.gmk

SUBDIRS  = management volumes magneticfield navigation biasing divisions
SUBDIRS += solids/CSG solids/specific solids/usolids solids/Boolean
SUBLIBS  = G4geometrymng G4volumes G4magneticfield
SUBLIBS += G4navigation G4geombias G4geomdivision
SUBLIBS += G4csg G4specsolids G4geomBoolean

include $(G4INSTALL)/config/globlib.gmk
