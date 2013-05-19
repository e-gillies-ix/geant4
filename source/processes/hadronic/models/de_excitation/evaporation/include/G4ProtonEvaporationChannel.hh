//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
//
// $Id$
//
// Hadronic Process: Nuclear De-excitations
// by V. Lara (Nov. 1999)
//
// 17-11-2010 V.Ivanchenko moved constructor and destructor to source and cleanup

#ifndef G4ProtonEvaporationChannel_h
#define G4ProtonEvaporationChannel_h 1

#include "G4EvaporationChannel.hh"
#include "G4ProtonCoulombBarrier.hh"
#include "G4ProtonEvaporationProbability.hh"

class G4ProtonEvaporationChannel : public G4EvaporationChannel
{
public:
  // only available constructor
  G4ProtonEvaporationChannel();

  // destructor
  virtual ~G4ProtonEvaporationChannel();

private:

  const G4ProtonEvaporationChannel & operator=(const G4ProtonEvaporationChannel & right);  

  G4ProtonEvaporationChannel(const G4ProtonEvaporationChannel & right);

  G4bool operator==(const G4ProtonEvaporationChannel & right) const;
  G4bool operator!=(const G4ProtonEvaporationChannel & right) const;

  G4ProtonCoulombBarrier  theCoulombBarrier;
 
  G4ProtonEvaporationProbability  theEvaporationProbability;
	

};
#endif