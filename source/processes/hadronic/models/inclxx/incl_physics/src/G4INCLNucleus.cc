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
// INCL++ intra-nuclear cascade model
// Pekka Kaitaniemi, CEA and Helsinki Institute of Physics
// Davide Mancusi, CEA
// Alain Boudard, CEA
// Sylvie Leray, CEA
// Joseph Cugnon, University of Liege
//
#define INCLXX_IN_GEANT4_MODE 1

#include "globals.hh"

/*
 * G4INCLNucleus.cc
 *
 *  \date Jun 5, 2009
 * \author Pekka Kaitaniemi
 */

#ifndef G4INCLNucleus_hh
#define G4INCLNucleus_hh 1

#include "G4INCLGlobals.hh"
#include "G4INCLLogger.hh"
#include "G4INCLParticle.hh"
#include "G4INCLIAvatar.hh"
#include "G4INCLNucleus.hh"
#include "G4INCLKinematicsUtils.hh"
#include "G4INCLDecayAvatar.hh"
#include "G4INCLCluster.hh"
#include "G4INCLClusterDecay.hh"
#include "G4INCLDeJongSpin.hh"
#include <iterator>
#include <cstdlib>
#include <sstream>
// #include <cassert>

namespace G4INCL {

  Nucleus::Nucleus(G4int mass, G4int charge, Config const * const conf, const G4double universeRadius)
    : Cluster(charge,mass,true),
     theInitialZ(charge), theInitialA(mass),
     theNpInitial(0), theNnInitial(0),
     initialInternalEnergy(0.),
     incomingAngularMomentum(0.,0.,0.), incomingMomentum(0.,0.,0.),
     initialCenterOfMass(0.,0.,0.),
     remnant(true),
     blockedDelta(NULL),
     initialEnergy(0.),
     tryCN(false),
     projectileZ(0),
     projectileA(0),
     theUniverseRadius(universeRadius),
     isNucleusNucleus(false),
     theProjectileRemnant(NULL),
     theDensity(NULL),
     thePotential(NULL)
  {
    PotentialType potentialType;
    G4bool pionPotential;
    if(conf) {
      potentialType = conf->getPotentialType();
      pionPotential = conf->getPionPotential();
    } else { // By default we don't use energy dependent
      // potential. This is convenient for some tests.
      potentialType = IsospinPotential;
      pionPotential = true;
    }

    thePotential = NuclearPotential::createPotential(potentialType, theA, theZ, pionPotential);

    ParticleTable::setProtonSeparationEnergy(thePotential->getSeparationEnergy(Proton));
    ParticleTable::setNeutronSeparationEnergy(thePotential->getSeparationEnergy(Neutron));

    theDensity = NuclearDensityFactory::createDensity(theA, theZ);

    theParticleSampler->setPotential(thePotential);
    theParticleSampler->setDensity(theDensity);

    if(theUniverseRadius<0)
      theUniverseRadius = theDensity->getMaximumRadius();
    theStore = new Store(conf);
    toBeUpdated.clear();
  }

  Nucleus::~Nucleus() {
    delete theStore;
    deleteProjectileRemnant();
    /* We don't delete the potential and the density here any more -- Factories
     * are caching them
    delete thePotential;
    delete theDensity;*/
  }

  void Nucleus::initializeParticles() {
    // Reset the variables connected with the projectile remnant
    delete theProjectileRemnant;
    theProjectileRemnant = NULL;

    Cluster::initializeParticles();
    for(ParticleIter i=particles.begin(), e=particles.end(); i!=e; ++i) {
      updatePotentialEnergy(*i);
      theStore->add(*i);
    }
    particles.clear();
    initialInternalEnergy = computeTotalEnergy();
    initialCenterOfMass = thePosition;
  }

  void Nucleus::applyFinalState(FinalState *finalstate) {
    justCreated.clear();
    toBeUpdated.clear(); // Clear the list of particles to be updated by the propagation model.
    blockedDelta = NULL;

    if(!finalstate) // do nothing if no final state was returned
      return;

    G4double totalEnergy = 0.0;

    FinalStateValidity const validity = finalstate->getValidity();
    if(validity == ValidFS) {

      ParticleList const &created = finalstate->getCreatedParticles();
      for(ParticleIter iter=created.begin(), e=created.end(); iter!=e; ++iter) {
        theStore->add((*iter));
        if(!(*iter)->isOutOfWell()) {
          totalEnergy += (*iter)->getEnergy() - (*iter)->getPotentialEnergy();
          justCreated.push_back((*iter));   // New particle, so we must create avatars for it
        }
      }

      ParticleList const &deleted = finalstate->getDestroyedParticles();
      for(ParticleIter iter=deleted.begin(), e=deleted.end(); iter!=e; ++iter) {
        theStore->particleHasBeenDestroyed(*iter);
      }

      ParticleList const &modified = finalstate->getModifiedParticles();
      for(ParticleIter iter=modified.begin(), e=modified.end(); iter!=e; ++iter) {
        theStore->particleHasBeenUpdated(*iter);
        totalEnergy += (*iter)->getEnergy() - (*iter)->getPotentialEnergy();
        toBeUpdated.push_back((*iter)); // Particle is modified so we have to create new avatars for it.
      }

      ParticleList const &out = finalstate->getOutgoingParticles();
      for(ParticleIter iter=out.begin(), e=out.end(); iter!=e; ++iter) {
        if((*iter)->isCluster()) {
	  Cluster *clusterOut = dynamic_cast<Cluster*>((*iter));
// assert(clusterOut);
#ifdef INCLXX_IN_GEANT4_MODE
          if(!clusterOut)
            continue;
#endif
          ParticleList const &components = clusterOut->getParticles();
          for(ParticleIter in=components.begin(), end=components.end(); in!=end; ++in)
            theStore->particleHasBeenEjected(*in);
        } else {
          theStore->particleHasBeenEjected(*iter);
        }
        totalEnergy += (*iter)->getEnergy();      // No potential here because the particle is gone
        theA -= (*iter)->getA();
        theZ -= (*iter)->getZ();
        theStore->addToOutgoing(*iter);
        (*iter)->setEmissionTime(theStore->getBook().getCurrentTime());
      }

      ParticleList const &entering = finalstate->getEnteringParticles();
      for(ParticleIter iter=entering.begin(), e=entering.end(); iter!=e; ++iter) {
        insertParticle(*iter);
        totalEnergy += (*iter)->getEnergy() - (*iter)->getPotentialEnergy();
        toBeUpdated.push_back((*iter)); // Particle is modified so we have to create new avatars for it.
      }
    } else if(validity == PauliBlockedFS) {
      blockedDelta = finalstate->getBlockedDelta();
    } else if(validity == ParticleBelowFermiFS || validity == ParticleBelowZeroFS) {
      INCL_DEBUG("A Particle is entering below the Fermi sea:" << std::endl << finalstate->print() << std::endl);
      tryCN = true;
      ParticleList const &entering = finalstate->getEnteringParticles();
      for(ParticleIter iter=entering.begin(), e=entering.end(); iter!=e; ++iter) {
        insertParticle(*iter);
      }
    }

    if(validity==ValidFS &&
        std::abs(totalEnergy - finalstate->getTotalEnergyBeforeInteraction()) > 0.1) {
      INCL_ERROR("Energy nonconservation! Energy at the beginning of the event = "
          << finalstate->getTotalEnergyBeforeInteraction()
          <<" and after interaction = "
          << totalEnergy << std::endl
          << finalstate->print());
    }
  }

  void Nucleus::propagateParticles(G4double /*step*/) {
    INCL_WARN("Useless Nucleus::propagateParticles -method called." << std::endl);
  }

  G4double Nucleus::computeTotalEnergy() const {
    G4double totalEnergy = 0.0;
    ParticleList const &inside = theStore->getParticles();
    for(ParticleIter p=inside.begin(), e=inside.end(); p!=e; ++p) {
      if((*p)->isNucleon()) // Ugly: we should calculate everything using total energies!
        totalEnergy += (*p)->getKineticEnergy() - (*p)->getPotentialEnergy();
      else if((*p)->isResonance())
        totalEnergy += (*p)->getEnergy() - (*p)->getPotentialEnergy() - ParticleTable::effectiveNucleonMass;
      else
        totalEnergy += (*p)->getEnergy() - (*p)->getPotentialEnergy();
    }
    return totalEnergy;
  }

  void Nucleus::computeRecoilKinematics() {
    // If the remnant consists of only one nucleon, we need to apply a special
    // procedure to put it on mass shell.
    if(theA==1) {
      emitInsidePions();
      computeOneNucleonRecoilKinematics();
      remnant=false;
      return;
    }

    // Compute the recoil momentum and angular momentum
    theMomentum = incomingMomentum;
    theSpin = incomingAngularMomentum;

    ParticleList const &outgoing = theStore->getOutgoingParticles();
    for(ParticleIter p=outgoing.begin(), e=outgoing.end(); p!=e; ++p) {
      theMomentum -= (*p)->getMomentum();
      theSpin -= (*p)->getAngularMomentum();
    }
    if(theProjectileRemnant) {
      theMomentum -= theProjectileRemnant->getMomentum();
      theSpin -= theProjectileRemnant->getAngularMomentum();
    }

    // Subtract orbital angular momentum
    thePosition = computeCenterOfMass();
    theSpin -= (thePosition-initialCenterOfMass).vector(theMomentum);

    setMass(ParticleTable::getTableMass(theA,theZ) + theExcitationEnergy);
    adjustEnergyFromMomentum();
    remnant=true;
  }

  ThreeVector Nucleus::computeCenterOfMass() const {
    ThreeVector cm(0.,0.,0.);
    G4double totalMass = 0.0;
    ParticleList const &inside = theStore->getParticles();
    for(ParticleIter p=inside.begin(), e=inside.end(); p!=e; ++p) {
      const G4double mass = (*p)->getMass();
      cm += (*p)->getPosition() * mass;
      totalMass += mass;
    }
    cm /= totalMass;
    return cm;
  }

  G4double Nucleus::computeExcitationEnergy() const {
    const G4double totalEnergy = computeTotalEnergy();
    const G4double separationEnergies = computeSeparationEnergyBalance();

    return totalEnergy - initialInternalEnergy - separationEnergies;
  }

  std::string Nucleus::print()
  {
    std::stringstream ss;
    ss << "Particles in the nucleus:" << std::endl
      << "Inside:" << std::endl;
    G4int counter = 1;
    ParticleList const &inside = theStore->getParticles();
    for(ParticleIter p=inside.begin(), e=inside.end(); p!=e; ++p) {
      ss << "index = " << counter << std::endl
        << (*p)->print();
      counter++;
    }
    ss <<"Outgoing:" << std::endl;
    ParticleList const &outgoing = theStore->getOutgoingParticles();
    for(ParticleIter p=outgoing.begin(), e=outgoing.end(); p!=e; ++p)
      ss << (*p)->print();

    return ss.str();
  }

  G4bool Nucleus::decayOutgoingDeltas() {
    ParticleList const &out = theStore->getOutgoingParticles();
    ParticleList deltas;
    for(ParticleIter i=out.begin(), e=out.end(); i!=e; ++i) {
      if((*i)->isDelta()) deltas.push_back((*i));
    }
    if(deltas.empty()) return false;

    for(ParticleIter i=deltas.begin(), e=deltas.end(); i!=e; ++i) {
      INCL_DEBUG("Decay outgoing delta particle:" << std::endl
          << (*i)->print() << std::endl);
      const ThreeVector beta = -(*i)->boostVector();
      const G4double deltaMass = (*i)->getMass();

      // Set the delta momentum to zero and sample the decay in the CM frame.
      // This makes life simpler if we are using real particle masses.
      (*i)->setMomentum(ThreeVector());
      (*i)->setEnergy((*i)->getMass());

      // Use a DecayAvatar
      IAvatar *decay = new DecayAvatar((*i), 0.0, NULL);
      FinalState *fs = decay->getFinalState();
      Particle * const pion = fs->getCreatedParticles().front();
      Particle * const nucleon = fs->getModifiedParticles().front();

      // Adjust the decay momentum if we are using the real masses
      const G4double decayMomentum = KinematicsUtils::momentumInCM(deltaMass,
          nucleon->getTableMass(),
          pion->getTableMass());
      ThreeVector newMomentum = pion->getMomentum();
      newMomentum *= decayMomentum / newMomentum.mag();

      pion->setTableMass();
      pion->setMomentum(newMomentum);
      pion->adjustEnergyFromMomentum();
      pion->setEmissionTime(theStore->getBook().getCurrentTime());
      pion->boost(beta);

      nucleon->setTableMass();
      nucleon->setMomentum(-newMomentum);
      nucleon->adjustEnergyFromMomentum();
      nucleon->setEmissionTime(theStore->getBook().getCurrentTime());
      nucleon->boost(beta);

      theStore->addToOutgoing(pion);

      delete fs;
      delete decay;
    }

    return true;
  }

  G4bool Nucleus::decayInsideDeltas() {
    /* If there is a pion potential, do nothing (deltas will be counted as
     * excitation energy).
     * If, however, the remnant is unphysical (Z<0 or Z>A), force the deltas to
     * decay and get rid of all the pions. In case you're wondering, you can
     * end up with Z<0 or Z>A if the remnant contains more pi- than protons or
     * more pi+ than neutrons, respectively.
     */
    const G4bool unphysicalRemnant = (theZ<0 || theZ>theA);
    if(thePotential->hasPionPotential() && !unphysicalRemnant)
      return false;

    // Build a list of deltas (avoid modifying the list you are iterating on).
    ParticleList const &inside = theStore->getParticles();
    ParticleList deltas;
    for(ParticleIter i=inside.begin(), e=inside.end(); i!=e; ++i)
      if((*i)->isDelta()) deltas.push_back((*i));

    // Loop over the deltas, make them decay
    for(ParticleIter i=deltas.begin(), e=deltas.end(); i!=e; ++i) {
      INCL_DEBUG("Decay inside delta particle:" << std::endl
          << (*i)->print() << std::endl);
      // Create a forced-decay avatar. Note the last boolean parameter. Note
      // also that if the remnant is unphysical we more or less explicitly give
      // up energy conservation and CDPP by passing a NULL pointer for the
      // nucleus.
      IAvatar *decay;
      if(unphysicalRemnant) {
        INCL_WARN("Forcing delta decay inside an unphysical remnant (A=" << theA
             << ", Z=" << theZ << "). Might lead to energy-violation warnings."
             << std::endl);
        decay = new DecayAvatar((*i), 0.0, NULL, true);
      } else
        decay = new DecayAvatar((*i), 0.0, this, true);
      FinalState *fs = decay->getFinalState();

      // The pion can be ejected only if we managed to satisfy energy
      // conservation and if pion emission does not lead to negative excitation
      // energies.
      if(fs->getValidity()==ValidFS) {
        // Apply the final state to the nucleus
        applyFinalState(fs);
      }
      delete fs;
      delete decay;
    }

    // If the remnant is unphysical, emit all the pions
    if(unphysicalRemnant) {
      INCL_DEBUG("Remnant is unphysical: Z=" << theZ << ", A=" << theA << ", emitting all the pions" << std::endl);
      emitInsidePions();
    }

    return true;
  }

  G4bool Nucleus::decayOutgoingClusters() {
    ParticleList const &out = theStore->getOutgoingParticles();
    ParticleList clusters;
    for(ParticleIter i=out.begin(), e=out.end(); i!=e; ++i) {
      if((*i)->isCluster()) clusters.push_back((*i));
    }
    if(clusters.empty()) return false;

    for(ParticleIter i=clusters.begin(), e=clusters.end(); i!=e; ++i) {
      Cluster *cluster = dynamic_cast<Cluster*>(*i); // Can't avoid using a cast here
// assert(cluster);
#ifdef INCLXX_IN_GEANT4_MODE
      if(!cluster)
        continue;
#endif
      cluster->deleteParticles(); // Don't need them
      ParticleList decayProducts = ClusterDecay::decay(cluster);
      for(ParticleIter j=decayProducts.begin(), end=decayProducts.end(); j!=end; ++j)
        theStore->addToOutgoing(*j);
    }
    return true;
  }

  G4bool Nucleus::decayMe() {
    // Do the phase-space decay only if Z=0 or Z=A
    if(theA<=1 || (theZ!=0 && theA!=theZ))
      return false;

    ParticleList decayProducts = ClusterDecay::decay(this);
    for(ParticleIter j=decayProducts.begin(), e=decayProducts.end(); j!=e; ++j)
      theStore->addToOutgoing(*j);

    return true;
  }

  void Nucleus::emitInsidePions() {
    /* Forcing emissions of all pions in the nucleus. This probably violates
     * energy conservation (although the computation of the recoil kinematics
     * might sweep this under the carpet).
     */
    INCL_WARN("Forcing emissions of all pions in the nucleus." << std::endl);

    // Emit the pions with this kinetic energy
    const G4double tinyPionEnergy = 0.1; // MeV

    // Push out the emitted pions
    ParticleList const &inside = theStore->getParticles();
    ParticleList toEject;
    for(ParticleIter i=inside.begin(), e=inside.end(); i!=e; ++i) {
      if((*i)->isPion()) {
        Particle * const thePion = *i;
        INCL_DEBUG("Forcing emission of the following particle: "
                   << thePion->print() << std::endl);
        thePion->setEmissionTime(theStore->getBook().getCurrentTime());
        // Correction for real masses
        const G4double theQValueCorrection = thePion->getEmissionQValueCorrection(theA,theZ);
        const G4double kineticEnergyOutside = thePion->getKineticEnergy() - thePion->getPotentialEnergy() + theQValueCorrection;
        thePion->setTableMass();
        if(kineticEnergyOutside > 0.0)
          thePion->setEnergy(thePion->getMass()+kineticEnergyOutside);
        else
          thePion->setEnergy(thePion->getMass()+tinyPionEnergy);
        thePion->adjustMomentumFromEnergy();
        thePion->setPotentialEnergy(0.);
        theZ -= thePion->getZ();
        toEject.push_back(thePion);
      }
    }
    for(ParticleIter i=toEject.begin(), e=toEject.end(); i!=e; ++i) {
      theStore->particleHasBeenEjected(*i);
      theStore->addToOutgoing(*i);
    }
  }

  G4bool Nucleus::isEventTransparent() const {

    Book const &theBook = theStore->getBook();
    const G4int nEventCollisions = theBook.getAcceptedCollisions();
    const G4int nEventDecays = theBook.getAcceptedDecays();
    const G4int nEventClusters = theBook.getEmittedClusters();
    if(nEventCollisions==0 && nEventDecays==0 && nEventClusters==0)
      return true;

    return false;

  }

  void Nucleus::computeOneNucleonRecoilKinematics() {
    // We should be here only if the nucleus contains only one nucleon
// assert(theStore->getParticles().size()==1);

    INCL_ERROR("Computing one-nucleon recoil kinematics. We should never be here nowadays, cascade should stop earlier than this." << std::endl);

    // No excitation energy!
    theExcitationEnergy = 0.0;

    // Move the nucleon to the outgoing list
    Particle *remN = theStore->getParticles().front();
    theA -= remN->getA();
    theZ -= remN->getZ();
    theStore->particleHasBeenEjected(remN);
    theStore->addToOutgoing(remN);
    remN->setEmissionTime(theStore->getBook().getCurrentTime());

    // Treat the special case of a remaining delta
    if(remN->isDelta()) {
      IAvatar *decay = new DecayAvatar(remN, 0.0, NULL);
      FinalState *fs = decay->getFinalState();
      // Eject the pion
      ParticleList const &created = fs->getCreatedParticles();
      for(ParticleIter j=created.begin(), e=created.end(); j!=e; ++j)
        theStore->addToOutgoing(*j);
      delete fs;
      delete decay;
    }

    // Do different things depending on how many outgoing particles we have
    ParticleList const &outgoing = theStore->getOutgoingParticles();
    if(outgoing.size() == 2) {

      INCL_DEBUG("Two particles in the outgoing channel, applying exact two-body kinematics" << std::endl);

      // Can apply exact 2-body kinematics here. Keep the CM emission angle of
      // the first particle.
      Particle *p1 = outgoing.front(), *p2 = outgoing.back();
      const ThreeVector aBoostVector = incomingMomentum / initialEnergy;
      // Boost to the initial CM
      p1->boost(aBoostVector);
      const G4double sqrts = std::sqrt(initialEnergy*initialEnergy - incomingMomentum.mag2());
      const G4double pcm = KinematicsUtils::momentumInCM(sqrts, p1->getMass(), p2->getMass());
      const G4double scale = pcm/(p1->getMomentum().mag());
      // Reset the momenta
      p1->setMomentum(p1->getMomentum()*scale);
      p2->setMomentum(-p1->getMomentum());
      p1->adjustEnergyFromMomentum();
      p2->adjustEnergyFromMomentum();
      // Unboost
      p1->boost(-aBoostVector);
      p2->boost(-aBoostVector);

    } else {

      INCL_DEBUG("Trying to adjust final-state momenta to achieve energy and momentum conservation" << std::endl);

      const G4int maxIterations=8;
      G4double totalEnergy, energyScale;
      G4double val=1.E+100, oldVal=1.E+100, oldOldVal=1.E+100, oldOldOldVal;
      ThreeVector totalMomentum, deltaP;
      std::vector<ThreeVector> minMomenta;  // use it to store the particle momenta that minimize the merit function

      // Reserve the vector size
      minMomenta.reserve(outgoing.size());

      // Compute the initial total momentum
      totalMomentum.setX(0.0);
      totalMomentum.setY(0.0);
      totalMomentum.setZ(0.0);
      for(ParticleIter i=outgoing.begin(), e=outgoing.end(); i!=e; ++i)
        totalMomentum += (*i)->getMomentum();

      // Compute the initial total energy
      totalEnergy = 0.0;
      for(ParticleIter i=outgoing.begin(), e=outgoing.end(); i!=e; ++i)
        totalEnergy += (*i)->getEnergy();

      // Iterative algorithm starts here:
      for(G4int iterations=0; iterations < maxIterations; ++iterations) {

        // Save the old merit-function values
        oldOldOldVal = oldOldVal;
        oldOldVal = oldVal;
        oldVal = val;

        if(iterations%2 == 0) {
          INCL_DEBUG("Momentum step" << std::endl);
          // Momentum step: modify all the particle momenta
          deltaP = incomingMomentum - totalMomentum;
          G4double pOldTot = 0.0;
          for(ParticleIter i=outgoing.begin(), e=outgoing.end(); i!=e; ++i)
            pOldTot += (*i)->getMomentum().mag();
          for(ParticleIter i=outgoing.begin(), e=outgoing.end(); i!=e; ++i) {
            const ThreeVector mom = (*i)->getMomentum();
            (*i)->setMomentum(mom + deltaP*mom.mag()/pOldTot);
            (*i)->adjustEnergyFromMomentum();
          }
        } else {
          INCL_DEBUG("Energy step" << std::endl);
          // Energy step: modify all the particle momenta
          energyScale = initialEnergy/totalEnergy;
          for(ParticleIter i=outgoing.begin(), e=outgoing.end(); i!=e; ++i) {
            const ThreeVector mom = (*i)->getMomentum();
            G4double pScale = ((*i)->getEnergy()*energyScale - std::pow((*i)->getMass(),2))/mom.mag2();
            if(pScale>0) {
              (*i)->setEnergy((*i)->getEnergy()*energyScale);
              (*i)->adjustMomentumFromEnergy();
            }
          }
        }

        // Compute the current total momentum and energy
        totalMomentum.setX(0.0);
        totalMomentum.setY(0.0);
        totalMomentum.setZ(0.0);
        totalEnergy = 0.0;
        for(ParticleIter i=outgoing.begin(), e=outgoing.end(); i!=e; ++i) {
          totalMomentum += (*i)->getMomentum();
          totalEnergy += (*i)->getEnergy();
        }

        // Merit factor
        val = std::pow(totalEnergy - initialEnergy,2) +
          0.25*(totalMomentum - incomingMomentum).mag2();
        INCL_DEBUG("Merit function: val=" << val << ", oldVal=" << oldVal << ", oldOldVal=" << oldOldVal << ", oldOldOldVal=" << oldOldOldVal << std::endl);

        // Store the minimum
        if(val < oldVal) {
          INCL_DEBUG("New minimum found, storing the particle momenta" << std::endl);
          minMomenta.clear();
          for(ParticleIter i=outgoing.begin(), e=outgoing.end(); i!=e; ++i)
            minMomenta.push_back((*i)->getMomentum());
        }

        // Stop the algorithm if the search diverges
        if(val > oldOldVal && oldVal > oldOldOldVal) {
          INCL_DEBUG("Search is diverging, breaking out of the iteration loop: val=" << val << ", oldVal=" << oldVal << ", oldOldVal=" << oldOldVal << ", oldOldOldVal=" << oldOldOldVal << std::endl);
          break;
        }
      }

      // We should have made at least one successful iteration here
// assert(minMomenta.size()==outgoing.size());

      // Apply the optimal momenta
      INCL_DEBUG("Applying the solution" << std::endl);
      std::vector<ThreeVector>::const_iterator v = minMomenta.begin();
      for(ParticleIter i=outgoing.begin(), e=outgoing.end(); i!=e; ++i, ++v) {
        (*i)->setMomentum(*v);
        (*i)->adjustEnergyFromMomentum();
        INCL_DATABLOCK((*i)->print());
      }

    }

  }

  void Nucleus::fillEventInfo(EventInfo *eventInfo) {
    eventInfo->nParticles = 0;
    G4bool isNucleonAbsorption = false;

    G4bool isPionAbsorption = false;
    // It is possible to have pion absorption event only if the
    // projectile is pion.
    if(eventInfo->projectileType == PiPlus ||
       eventInfo->projectileType == PiMinus ||
       eventInfo->projectileType == PiZero) {
      isPionAbsorption = true;
    }

    // Forced CN
    eventInfo->forcedCompoundNucleus = tryCN;

    // Outgoing particles
    ParticleList const &outgoingParticles = getStore()->getOutgoingParticles();

    // Check if we have a nucleon absorption event: nucleon projectile
    // and no ejected particles.
    if(outgoingParticles.size() == 0 &&
       (eventInfo->projectileType == Proton ||
	eventInfo->projectileType == Neutron)) {
      isNucleonAbsorption = true;
    }

    // Reset the remnant counter
    eventInfo->nRemnants = 0;
    eventInfo->history.clear();

    for(ParticleIter i=outgoingParticles.begin(), e=outgoingParticles.end(); i!=e; ++i ) {
      // We have a pion absorption event only if the projectile is
      // pion and there are no ejected pions.
      if(isPionAbsorption) {
        if((*i)->isPion()) {
          isPionAbsorption = false;
        }
      }

      eventInfo->A[eventInfo->nParticles] = (*i)->getA();
      eventInfo->Z[eventInfo->nParticles] = (*i)->getZ();
      eventInfo->emissionTime[eventInfo->nParticles] = (*i)->getEmissionTime();
      eventInfo->EKin[eventInfo->nParticles] = (*i)->getKineticEnergy();
      ThreeVector mom = (*i)->getMomentum();
      eventInfo->px[eventInfo->nParticles] = mom.getX();
      eventInfo->py[eventInfo->nParticles] = mom.getY();
      eventInfo->pz[eventInfo->nParticles] = mom.getZ();
      eventInfo->theta[eventInfo->nParticles] = Math::toDegrees(mom.theta());
      eventInfo->phi[eventInfo->nParticles] = Math::toDegrees(mom.phi());
      eventInfo->origin[eventInfo->nParticles] = -1;
      eventInfo->history.push_back("");
      eventInfo->nParticles++;
    }
    eventInfo->nucleonAbsorption = isNucleonAbsorption;
    eventInfo->pionAbsorption = isPionAbsorption;
    eventInfo->nCascadeParticles = eventInfo->nParticles;

    // Projectile-like remnant characteristics
    if(theProjectileRemnant && theProjectileRemnant->getA()>0) {
      eventInfo->ARem[eventInfo->nRemnants] = theProjectileRemnant->getA();
      eventInfo->ZRem[eventInfo->nRemnants] = theProjectileRemnant->getZ();
      G4double eStar = theProjectileRemnant->getExcitationEnergy();
      if(std::abs(eStar)<1E-10)
        eStar = 0.0; // blame rounding and set the excitation energy to zero
      eventInfo->EStarRem[eventInfo->nRemnants] = eStar;
      if(eventInfo->EStarRem[eventInfo->nRemnants]<0.) {
	INCL_WARN("Negative excitation energy in projectile-like remnant! EStarRem = " << eventInfo->EStarRem[eventInfo->nRemnants] << std::endl);
      }
      const ThreeVector &spin = theProjectileRemnant->getSpin();
      if(eventInfo->ARem[eventInfo->nRemnants]%2==0) { // even-A nucleus
	eventInfo->JRem[eventInfo->nRemnants] = (G4int) (spin.mag()/PhysicalConstants::hc + 0.5);
      } else { // odd-A nucleus
	eventInfo->JRem[eventInfo->nRemnants] = ((G4int) (spin.mag()/PhysicalConstants::hc)) + 0.5;
      }
      eventInfo->EKinRem[eventInfo->nRemnants] = theProjectileRemnant->getKineticEnergy();
      const ThreeVector &mom = theProjectileRemnant->getMomentum();
      eventInfo->pxRem[eventInfo->nRemnants] = mom.getX();
      eventInfo->pyRem[eventInfo->nRemnants] = mom.getY();
      eventInfo->pzRem[eventInfo->nRemnants] = mom.getZ();
      eventInfo->jxRem[eventInfo->nRemnants] = spin.getX() / PhysicalConstants::hc;
      eventInfo->jyRem[eventInfo->nRemnants] = spin.getY() / PhysicalConstants::hc;
      eventInfo->jzRem[eventInfo->nRemnants] = spin.getZ() / PhysicalConstants::hc;
      eventInfo->thetaRem[eventInfo->nRemnants] = Math::toDegrees(mom.theta());
      eventInfo->phiRem[eventInfo->nRemnants] = Math::toDegrees(mom.phi());
      eventInfo->nRemnants++;
    }

    // Target-like remnant characteristics
    if(hasRemnant()) {
      eventInfo->ARem[eventInfo->nRemnants] = getA();
      eventInfo->ZRem[eventInfo->nRemnants] = getZ();
      eventInfo->EStarRem[eventInfo->nRemnants] = getExcitationEnergy();
      if(eventInfo->EStarRem[eventInfo->nRemnants]<0.) {
	INCL_WARN("Negative excitation energy in target-like remnant! EStarRem = " << eventInfo->EStarRem[eventInfo->nRemnants] << std::endl);
      }
      const ThreeVector &spin = getSpin();
      if(eventInfo->ARem[eventInfo->nRemnants]%2==0) { // even-A nucleus
	eventInfo->JRem[eventInfo->nRemnants] = (G4int) (spin.mag()/PhysicalConstants::hc + 0.5);
      } else { // odd-A nucleus
	eventInfo->JRem[eventInfo->nRemnants] = ((G4int) (spin.mag()/PhysicalConstants::hc)) + 0.5;
      }
      eventInfo->EKinRem[eventInfo->nRemnants] = getKineticEnergy();
      const ThreeVector &mom = getMomentum();
      eventInfo->pxRem[eventInfo->nRemnants] = mom.getX();
      eventInfo->pyRem[eventInfo->nRemnants] = mom.getY();
      eventInfo->pzRem[eventInfo->nRemnants] = mom.getZ();
      eventInfo->jxRem[eventInfo->nRemnants] = spin.getX() / PhysicalConstants::hc;
      eventInfo->jyRem[eventInfo->nRemnants] = spin.getY() / PhysicalConstants::hc;
      eventInfo->jzRem[eventInfo->nRemnants] = spin.getZ() / PhysicalConstants::hc;
      eventInfo->thetaRem[eventInfo->nRemnants] = Math::toDegrees(mom.theta());
      eventInfo->phiRem[eventInfo->nRemnants] = Math::toDegrees(mom.phi());
      eventInfo->nRemnants++;
    }

    // Global counters, flags, etc.
    Book const &theBook = theStore->getBook();
    eventInfo->nCollisions = theBook.getAcceptedCollisions();
    eventInfo->nBlockedCollisions = theBook.getBlockedCollisions();
    eventInfo->nDecays = theBook.getAcceptedDecays();
    eventInfo->nBlockedDecays = theBook.getBlockedDecays();
    eventInfo->firstCollisionTime = theBook.getFirstCollisionTime();
    eventInfo->firstCollisionXSec = theBook.getFirstCollisionXSec();
    eventInfo->firstCollisionSpectatorPosition = theBook.getFirstCollisionSpectatorPosition();
    eventInfo->firstCollisionSpectatorMomentum = theBook.getFirstCollisionSpectatorMomentum();
    eventInfo->firstCollisionIsElastic = theBook.getFirstCollisionIsElastic();
    eventInfo->nReflectionAvatars = theBook.getAvatars(SurfaceAvatarType);
    eventInfo->nCollisionAvatars = theBook.getAvatars(CollisionAvatarType);
    eventInfo->nDecayAvatars = theBook.getAvatars(DecayAvatarType);
    eventInfo->nEnergyViolationInteraction = theBook.getEnergyViolationInteraction();
  }

  Nucleus::ConservationBalance Nucleus::getConservationBalance(const EventInfo &theEventInfo, const G4bool afterRecoil) const {
    ConservationBalance theBalance;
    // Initialise balance variables with the incoming values
    theBalance.Z = theEventInfo.Zp + theEventInfo.Zt;
    theBalance.A = theEventInfo.Ap + theEventInfo.At;

    theBalance.energy = getInitialEnergy();
    theBalance.momentum = getIncomingMomentum();

    // Process outgoing particles
    ParticleList const &outgoingParticles = theStore->getOutgoingParticles();
    for(ParticleIter i=outgoingParticles.begin(), e=outgoingParticles.end(); i!=e; ++i ) {
      theBalance.Z -= (*i)->getZ();
      theBalance.A -= (*i)->getA();
      // For outgoing clusters, the total energy automatically includes the
      // excitation energy
      theBalance.energy -= (*i)->getEnergy(); // Note that outgoing particles should have the real mass
      theBalance.momentum -= (*i)->getMomentum();
    }

    // Projectile-like remnant contribution, if present
    if(theProjectileRemnant && theProjectileRemnant->getA()>0) {
      theBalance.Z -= theProjectileRemnant->getZ();
      theBalance.A -= theProjectileRemnant->getA();
      theBalance.energy -= ParticleTable::getTableMass(theProjectileRemnant->getA(),theProjectileRemnant->getZ()) +
        theProjectileRemnant->getExcitationEnergy();
      theBalance.energy -= theProjectileRemnant->getKineticEnergy();
      theBalance.momentum -= theProjectileRemnant->getMomentum();
    }

    // Target-like remnant contribution, if present
    if(hasRemnant()) {
      theBalance.Z -= getZ();
      theBalance.A -= getA();
      theBalance.energy -= ParticleTable::getTableMass(getA(),getZ()) +
        getExcitationEnergy();
      if(afterRecoil)
        theBalance.energy -= getKineticEnergy();
      theBalance.momentum -= getMomentum();
    }

    return theBalance;
  }

  void Nucleus::useFusionKinematics() {
    setEnergy(initialEnergy);
    setMomentum(incomingMomentum);
    setSpin(incomingAngularMomentum);
    theExcitationEnergy = std::sqrt(theEnergy*theEnergy-theMomentum.mag2()) - getTableMass();
    setMass(getTableMass() + theExcitationEnergy);
  }

  void Nucleus::finalizeProjectileRemnant(const G4double anEmissionTime) {
    // Deal with the projectile remnant
    const G4int prA = theProjectileRemnant->getA();
    if(prA>=1) {
      // Set the mass
      const G4double aMass = theProjectileRemnant->getInvariantMass();
      theProjectileRemnant->setMass(aMass);

      // Compute the excitation energy from the invariant mass
      const G4double anExcitationEnergy = aMass
        - ParticleTable::getTableMass(prA, theProjectileRemnant->getZ());

      // Set the excitation energy
      theProjectileRemnant->setExcitationEnergy(anExcitationEnergy);

      // No spin!
      theProjectileRemnant->setSpin(ThreeVector());

      // Set the emission time
      theProjectileRemnant->setEmissionTime(anEmissionTime);
    }
  }

}

#endif
