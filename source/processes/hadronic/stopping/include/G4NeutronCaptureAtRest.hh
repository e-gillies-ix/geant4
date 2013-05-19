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
//   G4NeutronCaptureAtRest physics process
//   Larry Felawka (TRIUMF), April 1998
//---------------------------------------------------------------------

#ifndef G4NeutronCaptureAtRest_h
#define G4NeutronCaptureAtRest_h 1

// Class Description:
//
// Process for capture of neutrons at rest. 
// To be used in your physics list in case you need this physics.

 
#include "globals.hh"
#include "Randomize.hh" 
#include "G4VRestProcess.hh"
#include "G4VParticleChange.hh"
#include "G4ParticleDefinition.hh"
#include "G4GHEKinematicsVector.hh"
#include "G4HadronicProcessType.hh"

class G4NeutronCaptureAtRest : public G4VRestProcess
 
{ 
  private:
  // hide assignment operator as private 
      G4NeutronCaptureAtRest& operator=(const G4NeutronCaptureAtRest &right);
      G4NeutronCaptureAtRest(const G4NeutronCaptureAtRest& );
   
  public:
 
     G4NeutronCaptureAtRest(const G4String& processName ="NeutronCaptureAtRest", 
                       G4ProcessType   aType = fHadronic );
 
    ~G4NeutronCaptureAtRest();

     G4bool IsApplicable(const G4ParticleDefinition&);

     void PreparePhysicsTable(const G4ParticleDefinition&);

  // null physics table
     void BuildPhysicsTable(const G4ParticleDefinition&);

     G4double AtRestGetPhysicalInteractionLength(const G4Track&,
						 G4ForceCondition*);

  // zero mean lifetime
     G4double GetMeanLifeTime(const G4Track& ,
			      G4ForceCondition* ) {return 0.0;}

     G4VParticleChange* AtRestDoIt(const G4Track&, const G4Step&); 

  // return number of secondaries produced
     G4int GetNumberOfSecondaries();

  // pointer to array containg kinematics of secondaries
     G4GHEKinematicsVector* GetSecondaryKinematics();

  private:

     void GenerateSecondaries();
     void Normal( G4float* );
     void NeutronCapture( G4int* );
     G4double AtomAs( G4float, G4float );

  private:

// global time-of-flight of stopped hadron
     G4float  globalTime;

// atomic mass of target nucleus
     G4float  targetAtomicMass;

// charge of target nucleus
     G4float  targetCharge;

     G4GHEKinematicsVector* pv;
     G4GHEKinematicsVector* eve;
     G4GHEKinematicsVector* gkin;

     G4int    ngkine;

     G4int    ntot;
     G4GHEKinematicsVector result;

     G4float  massProton;
     G4float  massNeutron;
     G4float  massElectron;
     G4float  massDeuteron;
     G4float  massAlpha;

     G4ParticleDefinition* pdefGamma;
     G4ParticleDefinition* pdefNeutron;

};

#endif
 