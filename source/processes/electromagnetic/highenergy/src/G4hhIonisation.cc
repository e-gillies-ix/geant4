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
// $Id: G4hhIonisation.cc 66241 2012-12-13 18:34:42Z gunter $
//
// -------------------------------------------------------------------
//
// GEANT4 Class file
//
//
// File name:     G4hhIonisation
//
// Author:        Vladimir Ivanchenko
//
// Creation date: 30.09.2005
//
// Modifications:
// 10-01-06 SetStepLimits -> SetStepFunction (V.Ivantchenko)
// 27-10-06 Add maxKinEnergy (V.Ivantchenko)
//
//
// -------------------------------------------------------------------
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

#include "G4hhIonisation.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4BraggNoDeltaModel.hh"
#include "G4BetheBlochNoDeltaModel.hh"
#include "G4ICRU73NoDeltaModel.hh"
#include "G4UniversalFluctuation.hh"
#include "G4BohrFluctuations.hh"
#include "G4IonFluctuations.hh"
#include "G4UnitsTable.hh"
#include "G4Electron.hh"
#include "G4LossTableManager.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

G4hhIonisation::G4hhIonisation(const G4String& name)
  : G4VEnergyLossProcess(name),
    theParticle(0),
    theBaseParticle(0),
    isInitialised(false)
{
  SetStepFunction(0.1, 0.1*mm);
  SetVerboseLevel(1);
  SetProcessSubType(fIonisation);
  mass = 0.0;
  ratio = 0.0;
  flucModel = 0;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

G4hhIonisation::~G4hhIonisation()
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

G4bool G4hhIonisation::IsApplicable(const G4ParticleDefinition& p)
{
  return (p.GetPDGCharge() != 0.0 && p.GetPDGMass() > 100.0*MeV &&
	 !p.IsShortLived());
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

G4double G4hhIonisation::MinPrimaryEnergy(const G4ParticleDefinition*,
					  const G4Material*,
					  G4double cut)
{
  G4double x = 0.5*cut/electron_mass_c2;
  G4double y = electron_mass_c2/mass;
  G4double gam = x*y + std::sqrt((1. + x)*(1. + x*y*y));
  return mass*(gam - 1.0);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void G4hhIonisation::InitialiseEnergyLossProcess(const G4ParticleDefinition* part,
                                                 const G4ParticleDefinition* bpart)
{
  if(isInitialised) { return; }

  theParticle = part;
  if(bpart) { 
    G4cout << "G4hhIonisation::InitialiseEnergyLossProcess WARNING: no "
	   << "base particle should be defined for the process "
	   << GetProcessName() << G4endl; 
  }
  SetBaseParticle(0);
  SetSecondaryParticle(G4Electron::Electron());
  mass  = theParticle->GetPDGMass();
  ratio = electron_mass_c2/mass;
  G4double eth = 2*MeV*mass/proton_mass_c2;
  flucModel = new G4IonFluctuations();

  G4double emin = std::min(MinKinEnergy(), 0.1*eth);
  G4double emax = std::max(MaxKinEnergy(), 100*eth);

  SetMinKinEnergy(emin);
  SetMaxKinEnergy(emax);
  G4int bin = G4lrint(G4LossTableManager::Instance()->GetNumberOfBinsPerDecade()
		      *std::log10(emax/emin));
  SetDEDXBinning(bin);

  G4VEmModel* em = 0; 
  if(part->GetPDGCharge() > 0.0) { em = new G4BraggNoDeltaModel(); }
  else { em = new G4ICRU73NoDeltaModel(); }
  em->SetLowEnergyLimit(emin);
  em->SetHighEnergyLimit(eth);
  AddEmModel(1, em, flucModel);

  em = new G4BetheBlochNoDeltaModel();
  em->SetLowEnergyLimit(eth);
  em->SetHighEnergyLimit(emax);
  AddEmModel(1, em, flucModel);

  if(verboseLevel>1) {
    G4cout << "G4hhIonisation is initialised" << G4endl;
  }
  isInitialised = true;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void G4hhIonisation::PrintInfo()
{
  G4cout << "      Delta-ray will not be produced; "
	 << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....
