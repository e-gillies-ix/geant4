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
// $Id: G4ElementaryParticleCollider.cc 71940 2013-06-28 19:04:44Z mkelsey $
//
// 20100114  M. Kelsey -- Remove G4CascadeMomentum, use G4LorentzVector directly
// 20100316  D. Wright (restored by M. Kelsey) -- Replace original (incorrect)
//		pp, pn, nn 2-body to 2-body scattering angular distributions
//		with a new parametrization of elastic scattering data using
//		the sum of two exponentials.
// 20100319  M. Kelsey -- Use new generateWithRandomAngles for theta,phi stuff;
//		eliminate some unnecessary std::pow()
// 20100407  M. Kelsey -- Replace std::vector<>::resize(0) with ::clear()
//		Eliminate return-by-value std::vector<> by creating buffers
//		buffers for particles, momenta, and particle types.
//		The following functions now return void and are non-const:
//		  ::generateSCMfinalState()
//		  ::generateMomModules()
//		  ::generateStrangeChannelPartTypes()
//		  ::generateSCMpionAbsorption()
// 20100408  M. Kelsey -- Follow changes to G4*Sampler to pass particle_kinds
//		as input buffer.
// 20100413  M. Kelsey -- Pass G4CollisionOutput by ref to ::collide()
// 20100428  M. Kelsey -- Use G4InuclParticleNames enum
// 20100429  M. Kelsey -- Change "photon()" to "isPhoton()"
// 20100507  M. Kelsey -- Rationalize multiplicity returns to be actual value
// 20100511  M. Kelsey -- Replace G4PionSampler and G4NucleonSampler with new
//		pi-N and N-N classes, reorganize if-cascades 
// 20100511  M. Kelsey -- Eliminate three residual random-angles blocks.
// 20100511  M. Kelsey -- Bug fix: pi-N two-body final states not correctly
//		tested for charge-exchange case.
// 20100512  M. Kelsey -- Rationalize multiplicity returns to be actual value
// 20100512  M. Kelsey -- Add some additional debugging messages for 2-to-2
// 20100512  M. Kelsey -- Replace "if (is==)" cascades with switch blocks.
//		Use G4CascadeInterpolator for angular distributions.
// 20100517  M. Kelsey -- Inherit from common base class, make arrays static
// 20100519  M. Kelsey -- Use G4InteractionCase to compute "is" values.
// 20100625  M. Kelsey -- Two bugs in n-body momentum, last particle recoil
// 20100713  M. Kelsey -- Bump collide start message up to verbose > 1
// 20100714  M. Kelsey -- Move conservation checking to base class
// 20100714  M. Kelsey -- Add sanity check for two-body final state, to ensure
//		that final state total mass is below etot_scm; also compute
//		kinematics without "rescaling" (which led to non-conservation)
// 20100726  M. Kelsey -- Move remaining std::vector<> buffers to .hh file
// 20100804  M. Kelsey -- Add printing of final-state tables, protected by
//		G4CASCADE_DEBUG_SAMPLER preprocessor flag
// 20101019  M. Kelsey -- CoVerity report: check dynamic_cast<> for null
// 20110214  M. Kelsey -- Follow G4InuclParticle::Model enumerator migration
// 20110718  V. Uzhinskiy -- Drop IL,IM reset for multiplicity-three collisions
// 20110718  M. Kelsey -- Use enum names in switch blocks (c.f. G4NucleiModel)
// 20110720  M. Kelsey -- Follow interface change for cross-section tables,
//		eliminating switch blocks.
// 20110806  M. Kelsey -- Pre-allocate buffers to avoid memory churn
// 20110922  M. Kelsey -- Follow G4InuclParticle::print(ostream&) migration
// 20110926  M. Kelsey -- Protect sampleCMcosFor2to2 from unphysical arguments
// 20111003  M. Kelsey -- Prepare for gamma-N interactions by checking for
//		final-state tables instead of particle "isPhoton()"
// 20111007  M. Kelsey -- Add gamma-N final-state tables to printFinalState
// 20111107  M. Kelsey -- In sampleCMmomentumFor2to2(), hide message about
//		unrecognized gamma-N initial state behind verbosity.
// 20111216  M. Kelsey -- Add diagnostics to generateMomModulesFor2toMany(),
//		defer allocation of buffer in generateSCMfinalState() so that
//		multiplicity failures return zero output, and can be trapped.
// 20120308  M. Kelsey -- Allow photons to interact with dibaryons (see
//		changes in G4NucleiModel).
// 20120608  M. Kelsey -- Fix variable-name "shadowing" compiler warnings.
// 20121206  M. Kelsey -- Add Omega to printFinalStateTables(), remove line
//		about "preliminary" gamma-N.
// 20130129  M. Kelsey -- Add static arrays and interpolators for two-body
//		angular distributions (addresses MT thread-local issue)
// 20130221  M. Kelsey -- Move two-body angular dist classes to factory
// 20130306  M. Kelsey -- Use particle-name enums in if-blocks; add comments
//		to sections of momentum-coefficient matrix; move final state
//		table printing to G4CascadeChannelTables.
// 20130307  M. Kelsey -- Reverse order of dimensions for rmn array
// 20130307  M. Kelsey -- Use new momentum generator factory instead of rmn
// 20130308  M. Kelsey -- Move 3-body angle calc to G4InuclSpecialFunctions.
// 20130422  M. Kelsey -- Move kinematics to G4CascadeFinalStateAlgorithm;
//		reduce nesting and replicated code in collide().
// 20130508  D. Wright -- Implement muon capture
// 20130511  M. Kelsey -- Check for neutrinos and skip them in ::collide()

#include "G4ElementaryParticleCollider.hh"
#include "G4CascadeChannel.hh"
#include "G4CascadeChannelTables.hh"
#include "G4CollisionOutput.hh"
#include "G4GDecay3.hh"
#include "G4InuclParticleNames.hh"
#include "G4InuclSpecialFunctions.hh"
#include "G4LorentzConvertor.hh"
#include "G4MultiBodyMomentumDist.hh"
#include "G4NucleiModel.hh"
#include "G4ParticleLargerEkin.hh"
#include "G4TwoBodyAngularDist.hh"
#include "G4VMultiBodyMomDst.hh"
#include "G4VTwoBodyAngDst.hh"
#include "Randomize.hh"
#include <algorithm>
#include <cfloat>
#include <vector>

using namespace G4InuclParticleNames;
using namespace G4InuclSpecialFunctions;

typedef std::vector<G4InuclElementaryParticle>::iterator particleIterator;


G4ElementaryParticleCollider::G4ElementaryParticleCollider()
  : G4CascadeColliderBase("G4ElementaryParticleCollider") {;}


void
G4ElementaryParticleCollider::collide(G4InuclParticle* bullet,
				      G4InuclParticle* target,
				      G4CollisionOutput& output) 
{
  if (verboseLevel > 1)
    G4cout << " >>> G4ElementaryParticleCollider::collide" << G4endl;

  if (!useEPCollider(bullet,target)) {		// Sanity check
    G4cerr << " ElementaryParticleCollider -> can collide only particle with particle " 
           << G4endl;
    return;
  }

#ifdef G4CASCADE_DEBUG_SAMPLER
  static G4bool doPrintTables = true;	// Once and only once per job
  if (doPrintTables) {
    G4CascadeChannelTables::Print();
    doPrintTables = false;
  }
#endif

  interCase.set(bullet, target);	// To identify kind of collision

  if (verboseLevel > 1) G4cout << *bullet << G4endl << *target << G4endl;

  G4InuclElementaryParticle* particle1 =
    dynamic_cast<G4InuclElementaryParticle*>(bullet);
  G4InuclElementaryParticle* particle2 =	
    dynamic_cast<G4InuclElementaryParticle*>(target);

  if (!particle1 || !particle2) {	// Redundant with useEPCollider()
    G4cerr << " ElementaryParticleCollider -> can only collide hadrons"
           << G4endl;
    return;
  }

  if (particle1->isNeutrino() || particle2->isNeutrino()) return;

  // Check for available interaction, or pion+dibaryon special case
  if (!G4CascadeChannelTables::GetTable(interCase.hadrons()) &&
      !particle1->quasi_deutron() && !particle2->quasi_deutron()) {
    G4cerr << " ElementaryParticleCollider -> cannot collide " 
	   << particle1->getDefinition()->GetParticleName() << " with "
           << particle2->getDefinition()->GetParticleName() << G4endl;
    return;
  }

  G4LorentzConvertor convertToSCM;	// Utility to handle frame manipulation
  if (particle2->nucleon() || particle2->quasi_deutron()) {
    convertToSCM.setBullet(particle1);
    convertToSCM.setTarget(particle2);
  } else {
    convertToSCM.setBullet(particle2);
    convertToSCM.setTarget(particle1);
  }

  convertToSCM.setVerbose(verboseLevel);
  convertToSCM.toTheCenterOfMass();

  // Generate any particle collision with nucleon
  if (particle1->nucleon() || particle2->nucleon()) {
    G4double ekin = convertToSCM.getKinEnergyInTheTRS();
    G4double etot_scm = convertToSCM.getTotalSCMEnergy();

    generateSCMfinalState(ekin, etot_scm, particle1, particle2);
  }

  // Generate pion or photon collision with quasi-deuteron
  if (particle1->quasi_deutron() || particle2->quasi_deutron()) {
    if (!G4NucleiModel::useQuasiDeuteron(particle1->type(),particle2->type()) &&
	!G4NucleiModel::useQuasiDeuteron(particle2->type(),particle1->type())) {
      G4cerr << " ElementaryParticleCollider -> can only collide pi,mu,gamma with"
	     << " dibaryons " << G4endl;
      return;
    }

    G4double etot_scm = convertToSCM.getTotalSCMEnergy();

    if (particle1->isMuon() || particle2->isMuon()) {
      generateSCMmuonAbsorption(etot_scm, particle1, particle2);
    } else {		// Currently, pion absoprtion also handles gammas
      generateSCMpionAbsorption(etot_scm, particle1, particle2);
    }
  }    

  if (particles.empty()) {	// No final state possible, pass bullet through
    if (verboseLevel) {
      G4cerr << " ElementaryParticleCollider -> failed to collide " 
	     << particle1->getMomModule() << " GeV/c " 
	     << particle1->getDefinition()->GetParticleName() << " with "
	     << particle2->getDefinition()->GetParticleName() << G4endl;
    }
    return;
  }

  // Convert final state back to lab frame
  G4LorentzVector mom;		// Buffer to avoid memory churn
  particleIterator ipart;
  for(ipart = particles.begin(); ipart != particles.end(); ipart++) {	
    mom = convertToSCM.backToTheLab(ipart->getMomentum());
    ipart->setMomentum(mom); 
  };
  
  // Check conservation in multibody final state
  if (verboseLevel && !validateOutput(bullet, target, particles)) {
    G4cout << " incoming particles: \n" << *particle1 << G4endl
	   << *particle2 << G4endl
	   << " outgoing particles: " << G4endl;
    for(ipart = particles.begin(); ipart != particles.end(); ipart++)
      G4cout << *ipart << G4endl;
    
    G4cout << " <<< Non-conservation in G4ElementaryParticleCollider"
	   << G4endl;
  }
    
  std::sort(particles.begin(), particles.end(), G4ParticleLargerEkin());
  output.addOutgoingParticles(particles);
}


G4int 
G4ElementaryParticleCollider::generateMultiplicity(G4int is, 
						   G4double ekin) const 
{
  G4int mul = 0;

  const G4CascadeChannel* xsecTable = G4CascadeChannelTables::GetTable(is);

  if (xsecTable) mul = xsecTable->getMultiplicity(ekin);
  else {
    G4cerr << " G4ElementaryParticleCollider: Unknown interaction channel "
	   << is << " - multiplicity not generated " << G4endl;
  }

  if(verboseLevel > 3){
    G4cout << " G4ElementaryParticleCollider::generateMultiplicity: "  
           << " multiplicity = " << mul << G4endl; 
  }

  return mul;
}

 
void 
G4ElementaryParticleCollider::generateOutgoingPartTypes(G4int is, G4int mult,
							G4double ekin)
{
  particle_kinds.clear();	// Initialize buffer for generation

  const G4CascadeChannel* xsecTable = G4CascadeChannelTables::GetTable(is);

  if (xsecTable)
    xsecTable->getOutgoingParticleTypes(particle_kinds, mult, ekin);
  else {
    G4cerr << " G4ElementaryParticleCollider: Unknown interaction channel "
	   << is << " - outgoing kinds not generated " << G4endl;
  }

  return;
}


void
G4ElementaryParticleCollider::generateSCMfinalState(G4double ekin, 
		                     G4double etot_scm, 
		                     G4InuclElementaryParticle* particle1,
		                     G4InuclElementaryParticle* particle2) {
  if (verboseLevel > 2) {
    G4cout << " >>> G4ElementaryParticleCollider::generateSCMfinalState" 
           << G4endl;
  }

  fsGenerator.SetVerboseLevel(verboseLevel);

  const G4int itry_max = 10;

  G4int type1 = particle1->type();
  G4int type2 = particle2->type();

  G4int is = type1 * type2;

  if (verboseLevel > 3) G4cout << " is " << is << G4endl;

  G4int multiplicity = 0;
  G4bool generate = true;

  G4int itry = 0;
  while (generate && itry++ < itry_max) {
    particles.clear();		// Initialize buffers for this event
    particle_kinds.clear();

    // Generate list of final-state particles
    multiplicity = generateMultiplicity(is, ekin);

    generateOutgoingPartTypes(is, multiplicity, ekin);
    if (particle_kinds.empty()) {
      if (verboseLevel > 3) {
	G4cout << " generateOutgoingPartTypes failed mult " << multiplicity
	       << G4endl;
      }
      continue;
    }

    fillOutgoingMasses();	// Fill mass buffer from particle types

    // Attempt to produce final state kinematics
    fsGenerator.Configure(particle1, particle2, particle_kinds);
    generate = !fsGenerator.Generate(etot_scm, masses, scm_momentums);
  }	// while (generate) 

  if (itry >= itry_max) {		// Unable to generate valid final state
    if (verboseLevel > 2)
      G4cout << " generateSCMfinalState failed " << itry << " attempts"
	     << G4endl;
    return;
  }

  // Store generated momenta into outgoing particles

  particles.resize(multiplicity);		// Preallocate buffer
  for (G4int i=0; i<multiplicity; i++) {
    particles[i].fill(scm_momentums[i], particle_kinds[i],
		      G4InuclParticle::EPCollider);
  }

  if (verboseLevel > 3) {
    G4cout << " <<< G4ElementaryParticleCollider::generateSCMfinalState"
	   << G4endl;
  }

  return;	// Particles buffer filled
}


// Use generated list of final states to fill mass buffers

void G4ElementaryParticleCollider::fillOutgoingMasses() {
  G4int mult = particle_kinds.size();

  masses.resize(mult,0.);
  masses2.resize(mult,0.);		// Allows direct [i] setting

  for (G4int i = 0; i < mult; i++) {
    masses[i] = G4InuclElementaryParticle::getParticleMass(particle_kinds[i]);
    masses2[i] = masses[i] * masses[i];
  }
}


// generate nucleons momenta for pion or photon absorption by dibaryon
// the nucleon distribution assumed to be isotropic in SCM

void
G4ElementaryParticleCollider::generateSCMpionAbsorption(G4double etot_scm,
			             G4InuclElementaryParticle* particle1,
			             G4InuclElementaryParticle* particle2) {
  if (verboseLevel > 3)
    G4cout << " >>> G4ElementaryParticleCollider::generateSCMpionAbsorption" 
	   << G4endl;

  particles.clear();		// Initialize buffers for this event
  particles.resize(2);

  particle_kinds.clear();

  G4int type1 = particle1->type();
  G4int type2 = particle2->type();

  // Ensure that absportion is valid (charge conservable)
  if (!G4NucleiModel::useQuasiDeuteron(type1, type2)) {
    G4cerr << " pion absorption: "
	   << particle1->getDefinition()->GetParticleName() << " + "
	   << particle2->getDefinition()->GetParticleName() << " -> ?"
	   << G4endl;
    return;
  }

  if (!splitQuasiDeuteron(type2)) return;	// Get constituents of [NN]

  fillOutgoingMasses();

  G4double a = 0.5 * (etot_scm * etot_scm - masses2[0] - masses2[1]);

  G4double pmod = std::sqrt((a*a - masses2[0]*masses2[1])
			    / (masses2[0] + masses2[1] + 2.0*a));
  G4LorentzVector mom1 = generateWithRandomAngles(pmod, masses[0]);
  G4LorentzVector mom2;
  mom2.setVectM(-mom1.vect(), masses[1]);

  particles[0].fill(mom1, particle_kinds[0], G4InuclParticle::EPCollider);
  particles[1].fill(mom2, particle_kinds[1], G4InuclParticle::EPCollider);
}


// generate nucleons momenta for muon absorption by dibaryon
// the nucleon distribution assumed to be isotropic in SCM

void
G4ElementaryParticleCollider::generateSCMmuonAbsorption(G4double etot_scm,
                                     G4InuclElementaryParticle* particle1,
                                     G4InuclElementaryParticle* particle2)
{
  if (verboseLevel > 3)
    G4cout << " >>> G4ElementaryParticleCollider::generateSCMmuonAbsorption"
           << G4endl;

  // A phase space generator is required for the 3-body final state

  particles.clear();            // Initialize buffers for this event
  particles.resize(3);

  scm_momentums.clear();
  scm_momentums.resize(3);

  particle_kinds.clear();

  G4int type1 = particle1->type();
  G4int type2 = particle2->type();

  if (type1 != muonMinus) return;	// Sanity check, only mu- absorption

  // Ensure that absportion is valid (charge conservable)
  if (!G4NucleiModel::useQuasiDeuteron(type1, type2)) {
    G4cerr << " mu- absorption: "
	   << particle1->getDefinition()->GetParticleName() << " + "
	   << particle2->getDefinition()->GetParticleName() << " -> ?"
	   << G4endl;
    return;
  }

  if (!splitQuasiDeuteron(type2)) return;	// Get constituents of [NN]
  particle_kinds.push_back(mnu);
  
  fillOutgoingMasses();

  G4GDecay3 breakup(etot_scm, masses[0], masses[1], masses[2]);
  std::vector<G4ThreeVector> theMomenta = breakup.GetThreeBodyMomenta();

  for (size_t i=0; i<3; i++) {
    scm_momentums[i].setVectM(theMomenta[i], masses[i]);
    particles[i].fill(scm_momentums[i], particle_kinds[i], G4InuclParticle::EPCollider);
  }
} 


// generate constituents of dibaryon for "explosion"

G4bool G4ElementaryParticleCollider::splitQuasiDeuteron(G4int qdtype) {
  if (qdtype != diproton && qdtype != unboundPN && qdtype != dineutron) {
    G4cerr << " type " << qdtype << " not dibaryon!" << G4endl;
    return false;
  }

  G4int b2 = qdtype % 10;	// Dibaryon codes are 1ab (a=1,2; b=1,2)
  G4int b1 = (qdtype/10) % 10;

  particle_kinds.push_back(b1);
  particle_kinds.push_back(b2);

  return true;
}
