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
// $Id: G4DNAElectronSolvatation.hh 74551 2013-10-14 12:59:14Z gcosmo $
//
// Author: Mathieu Karamitros (kara (AT) cenbg . in2p3 . fr) 
//
// WARNING : This class is released as a prototype.
// It might strongly evolve or even disapear in the next releases.
//
// History:
// -----------
// 10 Oct 2011 M.Karamitros created
//
// -------------------------------------------------------------------

#ifndef G4DNAElectronSolvatation_h
#define G4DNAElectronSolvatation_h 1

#include "G4VEmProcess.hh"

// Available models
#include "G4DNAOneStepSolvatationModel.hh"
#include "G4DNATransformElectronModel.hh"

class G4DNAElectronSolvatation : public G4VEmProcess
{
public :
    G4DNAElectronSolvatation(const G4String& processName ="DNAElectronSolvatation",
                             G4ProcessType type = fElectromagnetic);
    virtual ~G4DNAElectronSolvatation();

    virtual G4bool IsApplicable(const G4ParticleDefinition&);
    virtual void PrintInfo();

protected:
    virtual void InitialiseProcess(const G4ParticleDefinition*);

private:
    G4bool       isInitialised;
};

#endif
