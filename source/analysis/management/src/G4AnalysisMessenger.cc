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
// $Id: G4AnalysisMessenger.cc 66310 2012-12-17 11:56:35Z ihrivnac $

// Author: Ivana Hrivnacova, 24/06/2013  (ivana@ipno.in2p3.fr)

#include "G4AnalysisMessenger.hh"
#include "G4VAnalysisManager.hh"
#include "G4FileMessenger.hh"
#include "G4H1Messenger.hh"
#include "G4H2Messenger.hh"
#include "G4HnMessenger.hh"

#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithAnInteger.hh"

//_____________________________________________________________________________
G4AnalysisMessenger::G4AnalysisMessenger(G4VAnalysisManager* manager)
  : G4UImessenger(),
    fManager(manager),
    fFileMessenger(0),
    fH1Messenger(0),
    fH2Messenger(0),
    fH1HnMessenger(0),
    fH2HnMessenger(0),
    fAnalysisDir(0),  
    fSetActivationCmd(0),
    fVerboseCmd(0)
{  
  fAnalysisDir = new G4UIdirectory("/analysis/");
  fAnalysisDir->SetGuidance("analysis control");

  fSetActivationCmd = new G4UIcmdWithABool("/analysis/setActivation",this);
  G4String guidance = "Set activation. \n";
  guidance += "When this option is enabled, only the histograms marked as activated\n";
  guidance += "are returned, filled or saved on file.\n";
  guidance += "No warning is issued when Get or Fill is called on inactive histogram.";
  fSetActivationCmd->SetGuidance(guidance);
  fSetActivationCmd->SetParameterName("Activation",false);

  fVerboseCmd = new G4UIcmdWithAnInteger("/analysis/verbose",this);
  fVerboseCmd->SetGuidance("Set verbose level");
  fVerboseCmd->SetParameterName("VerboseLevel",false);
  fVerboseCmd->SetRange("VerboseLevel>=0 && VerboseLevel<=4");
  
  fFileMessenger = new G4FileMessenger(manager);
  fH1Messenger = new G4H1Messenger(manager);
  fH2Messenger = new G4H2Messenger(manager);
}

//_____________________________________________________________________________
G4AnalysisMessenger::~G4AnalysisMessenger()
{
  delete fSetActivationCmd;
  delete fVerboseCmd;
  delete fAnalysisDir;
  delete fFileMessenger;
  delete fH1Messenger;
  delete fH2Messenger;
  delete fH1HnMessenger;
  delete fH2HnMessenger;
}

//
// public functions
//

//_____________________________________________________________________________
void G4AnalysisMessenger::SetH1HnManager(G4HnManager* h1HnManager)
{
  fH1HnMessenger = new G4HnMessenger(h1HnManager);
}  

//_____________________________________________________________________________
void G4AnalysisMessenger::SetH2HnManager(G4HnManager* h2HnManager)
{
  fH2HnMessenger = new G4HnMessenger(h2HnManager);
}  

//_____________________________________________________________________________
void G4AnalysisMessenger::SetNewValue(G4UIcommand* command, G4String newValues)
{
  if ( command == fSetActivationCmd ) {
    fManager->SetActivation(fSetActivationCmd->GetNewBoolValue(newValues));
  }  
  else if ( command == fVerboseCmd ) {
    fManager->SetVerboseLevel(fVerboseCmd->GetNewIntValue(newValues));
  }  
}  
