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
// $Id: G4VDecayChannel.cc 77085 2013-11-21 10:37:09Z gcosmo $
//
// 
// ------------------------------------------------------------
//      GEANT 4 class header file
//
//      History: first implementation, based on object model of
//      27 July 1996 H.Kurashige
//      30 May 1997  H.Kurashige
//      23 Mar. 2000 H.Weber      : add GetAngularMomentum
// ------------------------------------------------------------

#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleTable.hh"
#include "G4DecayTable.hh"
#include "G4DecayProducts.hh"
#include "G4VDecayChannel.hh"

/////@@// This static member is thread local. For each thread, it holds the array
/////@@// size of G4DecayChannelData instances.
/////@@//
/////@@template <class G4DecayChannelData> G4ThreadLocal
/////@@G4int G4PDefSplitter<G4DecayChannelData>::slavetotalspace = 0;
/////@@
/////@@// This static member is thread local. For each thread, it points to the
/////@@// array of G4DecayChannelData instances.
/////@@//
/////@@template <class G4DecayChannelData> G4ThreadLocal
/////@@G4DecayChannelData* G4PDefSplitter<G4DecayChannelData>::offset = 0;
/////@@
/////@@// This new field helps to use the class G4VDecayChannelSubInstanceManager.
/////@@//
/////@@G4DecayChannelManager G4VDecayChannel::subInstanceManager;

const G4String G4VDecayChannel::noName = " ";

G4VDecayChannel::G4VDecayChannel()
  :kinematics_name(""),
   rbranch(0.0),
   numberOfDaughters(0),
   parent_name(0), daughters_name(0),
   particletable(0),
   verboseLevel(1)		
{
/////@@  instanceID = subInstanceManager.CreateSubInstance();
  G4MT_parent = 0;
  G4MT_daughters = 0;
  G4MT_parent_mass = 0.0;
  G4MT_daughters_mass = 0;

  // set pointer to G4ParticleTable (static and singleton object)
  particletable = G4ParticleTable::GetParticleTable();
}

G4VDecayChannel::G4VDecayChannel(const G4String &aName, G4int Verbose)
  :kinematics_name(aName),
   rbranch(0.0),
   numberOfDaughters(0),
   parent_name(0), daughters_name(0),
   particletable(0),
   verboseLevel(Verbose)		
{
/////@@  instanceID = subInstanceManager.CreateSubInstance();
  G4MT_parent = 0;
  G4MT_daughters = 0;
  G4MT_parent_mass = 0.0;
  G4MT_daughters_mass = 0;

  // set pointer to G4ParticleTable (static and singleton object)
  particletable = G4ParticleTable::GetParticleTable();
}

G4VDecayChannel::G4VDecayChannel(const G4String  &aName, 
			       const G4String& theParentName,
			       G4double        theBR,
			       G4int           theNumberOfDaughters,
			       const G4String& theDaughterName1,
			       const G4String& theDaughterName2,
			       const G4String& theDaughterName3,
			       const G4String& theDaughterName4 )
               :kinematics_name(aName),
		rbranch(theBR),
		numberOfDaughters(theNumberOfDaughters),
		parent_name(0), daughters_name(0),
		particletable(0),
		verboseLevel(1)		
{
/////@@  instanceID = subInstanceManager.CreateSubInstance();
  G4MT_parent = 0;
  G4MT_daughters = 0;
  G4MT_parent_mass = 0.0;
  G4MT_daughters_mass = 0;

  // set pointer to G4ParticleTable (static and singleton object)
  particletable = G4ParticleTable::GetParticleTable();

  // parent name
  parent_name = new G4String(theParentName);

  // cleate array
  daughters_name = new G4String*[numberOfDaughters];
  for (G4int index=0;index<numberOfDaughters;index++) daughters_name[index]=0;

  // daughters' name
  if (numberOfDaughters>0) daughters_name[0] = new G4String(theDaughterName1);
  if (numberOfDaughters>1) daughters_name[1] = new G4String(theDaughterName2);
  if (numberOfDaughters>2) daughters_name[2] = new G4String(theDaughterName3);
  if (numberOfDaughters>3) daughters_name[3] = new G4String(theDaughterName4);
}

G4VDecayChannel::G4VDecayChannel(const G4VDecayChannel &right)
{
/////@@  instanceID = subInstanceManager.CreateSubInstance();

  kinematics_name = right.kinematics_name;
  verboseLevel = right.verboseLevel;
  rbranch = right.rbranch;

  // copy parent name
  parent_name = new G4String(*right.parent_name);
  G4MT_parent = 0;
  G4MT_parent_mass = 0.0; 

  //create array
  numberOfDaughters = right.numberOfDaughters;

  daughters_name =0;
  if ( numberOfDaughters >0 ) {
    daughters_name = new G4String*[numberOfDaughters];
    //copy daughters name
    for (G4int index=0; index < numberOfDaughters; index++)
      {
	daughters_name[index] = new G4String(*right.daughters_name[index]);
      }
  }

  //
  G4MT_daughters_mass = 0;
  G4MT_daughters = 0;

  // particle table
  particletable = G4ParticleTable::GetParticleTable();
}

G4VDecayChannel & G4VDecayChannel::operator=(const G4VDecayChannel &right)
{
  if (this != &right) { 
    kinematics_name = right.kinematics_name;
    verboseLevel = right.verboseLevel;
    rbranch = right.rbranch;

    // copy parent name
    parent_name = new G4String(*right.parent_name);

    // clear daughters_name array
    ClearDaughtersName();

    // recreate array
    numberOfDaughters = right.numberOfDaughters;
    if ( numberOfDaughters >0 ) {
      if (daughters_name !=0) ClearDaughtersName();
      daughters_name = new G4String*[numberOfDaughters];
      //copy daughters name
      for (G4int index=0; index < numberOfDaughters; index++) {
	  daughters_name[index] = new G4String(*right.daughters_name[index]);
      }
    }
  }

  //
  G4MT_parent = 0;
  G4MT_daughters = 0;
  G4MT_parent_mass = 0.0;
  G4MT_daughters_mass = 0;

  // particle table
  particletable = G4ParticleTable::GetParticleTable();

  return *this;
}

G4VDecayChannel::~G4VDecayChannel()
{
  ClearDaughtersName();
  if (parent_name != 0) delete parent_name;
  parent_name = 0;
  if (G4MT_daughters_mass != 0) delete [] G4MT_daughters_mass;
  G4MT_daughters_mass =0;
} 

/////@@const G4DecayChannelManager& G4VDecayChannel::GetSubInstanceManager()
/////@@{
/////@@  return subInstanceManager;
/////@@}

void G4VDecayChannel::ClearDaughtersName()
{
  if ( daughters_name != 0) {
    if (numberOfDaughters>0) {
#ifdef G4VERBOSE
      if (verboseLevel>1) {
	G4cout << "G4VDecayChannel::ClearDaughtersName "
	       << " for " << *parent_name << G4endl;
      }
#endif
      for (G4int index=0; index < numberOfDaughters; index++) { 
	if (daughters_name[index] != 0) delete daughters_name[index];
      }
    }
    delete [] daughters_name;
    daughters_name = 0;
  }
  // 
  if (G4MT_daughters != 0) delete [] G4MT_daughters;
  if (G4MT_daughters_mass != 0) delete [] G4MT_daughters_mass;
  G4MT_daughters = 0;
  G4MT_daughters_mass = 0;

  numberOfDaughters = 0;
}

void G4VDecayChannel::SetNumberOfDaughters(G4int size)
{
  if (size >0) {
    // remove old contents
    ClearDaughtersName();
    // cleate array
    daughters_name = new G4String*[size];
    for (G4int index=0;index<size;index++) daughters_name[index]=0;
    numberOfDaughters = size;
  }
}

void G4VDecayChannel::SetDaughter(G4int anIndex, 
				 const G4String &particle_name)
{
  // check numberOfDaughters is positive
  if (numberOfDaughters<=0) {
#ifdef G4VERBOSE
    if (verboseLevel>0) {
      G4cout << "G4VDecayChannel::SetDaughter: "
             << "Number of daughters is not defined" << G4endl;
    }
#endif
    return;
  }

  // check existence of daughters_name array
  if (daughters_name == 0) {
    // cleate array
    daughters_name = new G4String*[numberOfDaughters];
    for (G4int index=0;index<numberOfDaughters;index++) {
      daughters_name[index]=0;
    }
  }

  // check an index    
  if ( (anIndex<0) || (anIndex>=numberOfDaughters) ) {
#ifdef G4VERBOSE
    if (verboseLevel>0) {
      G4cout << "G4VDecayChannel::SetDaughter"
             << "index out of range " << anIndex << G4endl;
    }
#endif
  } else {
    // delete the old name if it exists
    if (daughters_name[anIndex]!=0) delete daughters_name[anIndex];
    // fill the name
    daughters_name[anIndex] = new G4String(particle_name);
    // refill the array of daughters[] if it exists
    if (G4MT_daughters != 0) FillDaughters();
#ifdef G4VERBOSE
    if (verboseLevel>1) {
      G4cout << "G4VDecayChannel::SetDaughter[" << anIndex <<"] :";
      G4cout << daughters_name[anIndex] << ":" << *daughters_name[anIndex]<<G4endl;
    }
#endif
  }
}

void G4VDecayChannel::SetDaughter(G4int anIndex, const G4ParticleDefinition * parent_type)
{
  if (parent_type != 0) SetDaughter(anIndex, parent_type->GetParticleName());
}

void G4VDecayChannel::FillDaughters()
{
  G4int index;
  
#ifdef G4VERBOSE
  if (verboseLevel>1) G4cout << "G4VDecayChannel::FillDaughters()" <<G4endl;
#endif
  if (G4MT_daughters != 0) {
    delete [] G4MT_daughters;
    G4MT_daughters = 0;
  }

  // parent mass
  if (G4MT_parent == 0) FillParent();  
  G4double parentmass = G4MT_parent->GetPDGMass();

  //
  G4double sumofdaughtermass = 0.0;
  if ((numberOfDaughters <=0) || (daughters_name == 0) ){
#ifdef G4VERBOSE
    if (verboseLevel>0) {
      G4cout << "G4VDecayChannel::FillDaughters   "
             << "[ " << G4MT_parent->GetParticleName() << " ]"
             << "numberOfDaughters is not defined yet";
    }
#endif
    G4MT_daughters = 0;
    G4Exception("G4VDecayChannel::FillDaughters",
		"PART011", FatalException,
		"Can not fill daughters: numberOfDaughters is not defined yet");    
  } 

  //create and set the array of pointers to daughter particles
  G4MT_daughters = new G4ParticleDefinition*[numberOfDaughters];
  if (G4MT_daughters_mass != 0) delete [] G4MT_daughters_mass;
  G4MT_daughters_mass = new G4double[numberOfDaughters];
  // loop over all daughters
  for (index=0; index < numberOfDaughters;  index++) { 
    if (daughters_name[index] == 0) {
      // daughter name is not defined
#ifdef G4VERBOSE
      if (verboseLevel>0) {
	G4cout << "G4VDecayChannel::FillDaughters  "
	       << "[ " << G4MT_parent->GetParticleName() << " ]"
	       << index << "-th daughter is not defined yet" << G4endl;
      }
#endif
      G4MT_daughters[index] = 0;
      G4Exception("G4VDecayChannel::FillDaughters",
		  "PART011", FatalException,
		  "Can not fill daughters: name of a daughter is not defined yet");    
    } 
    //search daughter particles in the particle table 
    G4MT_daughters[index] = particletable->FindParticle(*daughters_name[index]);
    if (G4MT_daughters[index] == 0) {
      // can not find the daughter particle
#ifdef G4VERBOSE
      if (verboseLevel>0) {
	G4cout << "G4VDecayChannel::FillDaughters  "
	        << "[ " << G4MT_parent->GetParticleName() << " ]"
               << index << ":" << *daughters_name[index]
	       << " is not defined !!" << G4endl;
        G4cout << " The BR of this decay mode is set to zero " << G4endl;
      }
#endif
      SetBR(0.0);
      return;
    }
#ifdef G4VERBOSE
    if (verboseLevel>1) {
      G4cout << index << ":" << *daughters_name[index];
      G4cout << ":" << G4MT_daughters[index] << G4endl;
    }
#endif
    G4MT_daughters_mass[index] = G4MT_daughters[index]->GetPDGMass();
    sumofdaughtermass += G4MT_daughters[index]->GetPDGMass();
  }  // end loop over all daughters

  // check sum of daghter mass
  G4double widthMass = G4MT_parent->GetPDGWidth();
  if ( (G4MT_parent->GetParticleType() != "nucleus") &&
       (sumofdaughtermass > parentmass + 5*widthMass) ){
   // !!! illegal mass  !!!
#ifdef G4VERBOSE
   if (GetVerboseLevel()>0) {
     G4cout << "G4VDecayChannel::FillDaughters "
            << "[ " << G4MT_parent->GetParticleName() << " ]"
            << "    Energy/Momentum conserevation breaks " <<G4endl;
     if (GetVerboseLevel()>1) {
       G4cout << "    parent:" << *parent_name
              << " mass:" << parentmass/GeV << "[GeV/c/c]" <<G4endl;
       for (index=0; index < numberOfDaughters; index++){
	 G4cout << "     daughter " << index << ":" << *daughters_name[index]
	        << " mass:" << G4MT_daughters[index]->GetPDGMass()/GeV
  	        << "[GeV/c/c]" <<G4endl;
       }
     }
   }
#endif
 }
}


void G4VDecayChannel::FillParent()
{
  if (parent_name == 0) {
    // parent name is not defined
#ifdef G4VERBOSE
    if (verboseLevel>0) {
      G4cout << "G4VDecayChannel::FillParent   "
             << ": parent name is not defined !!" << G4endl;
    }
#endif
    G4MT_parent = 0;
    G4Exception("G4VDecayChannel::FillParent()",
		"PART012", FatalException,
		"Can not fill parent: parent name is not defined yet");    
    return;
  }
  // search parent particle in the particle table
  G4MT_parent = particletable->FindParticle(*parent_name);
  if (G4MT_parent == 0) {
    // parent particle does not exist
#ifdef G4VERBOSE
    if (verboseLevel>0) {
      G4cout << "G4VDecayChannel::FillParent   "
             << *parent_name << " does not exist !!" << G4endl;
    }
#endif
    G4Exception("G4VDecayChannel::FillParent()",
		"PART012", FatalException,
		"Can not fill parent: parent does not exist");
    return;
  }
  G4MT_parent_mass = G4MT_parent->GetPDGMass();
}

void G4VDecayChannel::SetParent(const G4ParticleDefinition * parent_type)
{
  if (parent_type != 0) SetParent(parent_type->GetParticleName());
}

G4int G4VDecayChannel::GetAngularMomentum()
{
  // determine angular momentum

  // fill pointers to daughter particles if not yet set  
  if (G4MT_daughters == 0) FillDaughters();

  const G4int PiSpin = G4MT_parent->GetPDGiSpin();
  const G4int PParity = G4MT_parent->GetPDGiParity();
  if (2==numberOfDaughters) {     // up to now we can only handle two particle decays
    const G4int D1iSpin  = G4MT_daughters[0]->GetPDGiSpin();
    const G4int D1Parity = G4MT_daughters[0]->GetPDGiParity();
    const G4int D2iSpin  = G4MT_daughters[1]->GetPDGiSpin();
    const G4int D2Parity = G4MT_daughters[1]->GetPDGiParity();
    const G4int MiniSpin = std::abs (D1iSpin - D2iSpin);
    const G4int MaxiSpin = D1iSpin + D2iSpin;
    const G4int lMax = (PiSpin+D1iSpin+D2iSpin)/2; // l is allways int
    G4int lMin;
#ifdef G4VERBOSE
    if (verboseLevel>1) {
      G4cout << "iSpin: " << PiSpin << " -> " << D1iSpin << " + " << D2iSpin << G4endl;
      G4cout << "2*jmin, 2*jmax, lmax " << MiniSpin << " " << MaxiSpin << " " << lMax << G4endl;
    }
#endif
    for (G4int j=MiniSpin; j<=MaxiSpin; j+=2){    // loop over all possible spin couplings
      lMin = std::abs(PiSpin-j)/2;
#ifdef G4VERBOSE 
      if (verboseLevel>1)
	G4cout << "-> checking 2*j=" << j << G4endl;
#endif
      for (G4int l=lMin; l<=lMax; l++) {
#ifdef G4VERBOSE
	if (verboseLevel>1)
	  G4cout << " checking l=" << l << G4endl;
#endif
        if (l%2==0) {
	  if (PParity == D1Parity*D2Parity) {    // check parity for this l
	    return l;
          } 
	} else {
	  if (PParity == -1*D1Parity*D2Parity) {    // check parity for this l
            return l;
          }
        }
      }
    }
  } else {
    G4Exception("G4VDecayChannel::GetAngularMomentum",
		"PART111", JustWarning,
		"Sorry, can't handle 3 particle decays (up to now)");
    return 0;
  }
  G4Exception ("G4VDecayChannel::GetAngularMomentum",
		"PART111", JustWarning,
		"Can't find angular momentum for this decay");
  return 0;
}

void G4VDecayChannel::DumpInfo()
{
  G4cout << " BR:  " << rbranch << "  [" << kinematics_name << "]";
  G4cout << "   :  " ;
  for (G4int index=0; index < numberOfDaughters; index++)
  {
    if(daughters_name[index] != 0) {
      G4cout << " " << *(daughters_name[index]);
    } else {
      G4cout << " not defined ";
    }
  }
  G4cout << G4endl;
}

const G4String& G4VDecayChannel::GetNoName() const
{
  return noName;
}
