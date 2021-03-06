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
// $Id: G4HadronPhysicsShielding.cc 73040 2013-08-15 09:36:57Z gcosmo $
//
//---------------------------------------------------------------------------
//
// ClassName:   
//
// Author: 2010 Tatsumi Koi, Gunter Folger
//   created from G4HadronPhysicsFTFP_BERT
//
// Modified:
//
//----------------------------------------------------------------------------
//
#include <iomanip>   

#include "G4HadronPhysicsShielding.hh"

#include "globals.hh"
#include "G4ios.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"

#include "G4MesonConstructor.hh"
#include "G4BaryonConstructor.hh"
#include "G4ShortLivedConstructor.hh"
#include "G4IonConstructor.hh"

#include "G4BGGNucleonInelasticXS.hh"
#include "G4NeutronHPBGGNucleonInelasticXS.hh"
#include "G4NeutronHPJENDLHEInelasticData.hh"
#include "G4NeutronHPInelasticData.hh"

#include "G4ChipsKaonMinusInelasticXS.hh"
#include "G4ChipsKaonPlusInelasticXS.hh"
#include "G4ChipsKaonZeroInelasticXS.hh"
#include "G4CrossSectionDataSetRegistry.hh"
#include "G4PhysListUtil.hh"

#include "G4HadronCaptureProcess.hh"
#include "G4NeutronRadCapture.hh"
#include "G4NeutronCaptureXS.hh"
#include "G4NeutronHPCaptureData.hh"
#include "G4LFission.hh"

// factory
#include "G4PhysicsConstructorFactory.hh"
//
G4_DECLARE_PHYSCONSTR_FACTORY(G4HadronPhysicsShielding);

G4ThreadLocal G4HadronPhysicsShielding::ThreadPrivate* 
G4HadronPhysicsShielding::tpdata = 0;

G4HadronPhysicsShielding::G4HadronPhysicsShielding( G4int )
    :  G4VPhysicsConstructor("hInelastic Shielding")
/*    , theNeutrons(0)
    , theLENeutron(0)
    , theBertiniNeutron(0)
    , theFTFPNeutron(0)
    , thePiK(0)
    , theBertiniPiK(0)
    , theFTFPPiK(0)
    , thePro(0)
    , theBertiniPro(0)
    , theFTFPPro(0)
    , theHyperon(0)
    , theAntiBaryon(0)
    , theFTFPAntiBaryon(0) */
//    , QuasiElastic(false)
  /*    , theChipsKaonMinus(0)
    , theChipsKaonPlus(0)
    , theChipsKaonZero(0)
    , theBGGxsNeutron(0)
    , theNeutronHPJENDLHEInelastic(0)
    , theBGGxsProton(0)
    , xsNeutronCaptureXS(0)*/
    , useLEND(false)
    , evaluation()
{}

G4HadronPhysicsShielding::G4HadronPhysicsShielding(const G4String& name, G4bool /* quasiElastic */)
    :  G4VPhysicsConstructor(name) 
/*    , theNeutrons(0)
    , theLENeutron(0)
    , theBertiniNeutron(0)
    , theFTFPNeutron(0)
    , thePiK(0)
    , theBertiniPiK(0)
    , theFTFPPiK(0)
    , thePro(0)
    , theBertiniPro(0)
    , theFTFPPro(0)
    , theHyperon(0)
    , theAntiBaryon(0)
    , theFTFPAntiBaryon(0) */
//    , QuasiElastic(quasiElastic)
  /*    , theChipsKaonMinus(0)
    , theChipsKaonPlus(0)
    , theChipsKaonZero(0)
    , theBGGxsNeutron(0)
    , theNeutronHPJENDLHEInelastic(0)
    , theBGGxsProton(0)
    , xsNeutronCaptureXS(0)*/
    , useLEND(false)
    , evaluation()
{}

#include "G4NeutronLENDBuilder.hh"
void G4HadronPhysicsShielding::CreateModels()
{
  G4bool quasiElasticFTF= false;   // Use built-in quasi-elastic (not add-on)

  tpdata->theNeutrons=new G4NeutronBuilder( true ); // Fission on
  tpdata->theFTFPNeutron=new G4FTFPNeutronBuilder(quasiElasticFTF);
  tpdata->theNeutrons->RegisterMe(tpdata->theFTFPNeutron);
  tpdata->theNeutrons->RegisterMe(tpdata->theBertiniNeutron=new G4BertiniNeutronBuilder);
  tpdata->theBertiniNeutron->SetMinEnergy(19.9*MeV);
  tpdata->theBertiniNeutron->SetMaxEnergy(5*GeV);
  //tpdata->theNeutrons->RegisterMe(tpdata->theHPNeutron=new G4NeutronHPBuilder);

    if ( useLEND != true )
     tpdata->theNeutrons->RegisterMe(tpdata->theLENeutron=new G4NeutronHPBuilder);
  else
  {
     tpdata->theNeutrons->RegisterMe(tpdata->theLENeutron=new G4NeutronLENDBuilder(evaluation));
  }

  tpdata->thePro=new G4ProtonBuilder;
  tpdata->theFTFPPro=new G4FTFPProtonBuilder(quasiElasticFTF);
  tpdata->thePro->RegisterMe(tpdata->theFTFPPro);
  tpdata->thePro->RegisterMe(tpdata->theBertiniPro=new G4BertiniProtonBuilder);
  tpdata->theBertiniPro->SetMaxEnergy(5*GeV);

  tpdata->thePiK=new G4PiKBuilder;
  tpdata->theFTFPPiK=new G4FTFPPiKBuilder(quasiElasticFTF);
  tpdata->thePiK->RegisterMe(tpdata->theFTFPPiK);
  tpdata->thePiK->RegisterMe(tpdata->theBertiniPiK=new G4BertiniPiKBuilder);
  tpdata->theBertiniPiK->SetMaxEnergy(5*GeV);

  tpdata->theHyperon=new G4HyperonFTFPBuilder;
    
  tpdata->theAntiBaryon=new G4AntiBarionBuilder;
  tpdata->theAntiBaryon->RegisterMe(tpdata->theFTFPAntiBaryon=new G4FTFPAntiBarionBuilder(quasiElasticFTF));
}

G4HadronPhysicsShielding::~G4HadronPhysicsShielding()
{
  delete tpdata->theNeutrons;
  delete tpdata->theBertiniNeutron;
  delete tpdata->theFTFPNeutron;
  //delete tpdata->theHPNeutron;
  delete tpdata->theLENeutron;
    
  delete tpdata->thePiK;
  delete tpdata->theBertiniPiK;
  delete tpdata->theFTFPPiK;
    
  delete tpdata->thePro;
  delete tpdata->theBertiniPro;
  delete tpdata->theFTFPPro;    
    
  delete tpdata->theHyperon;
  delete tpdata->theAntiBaryon;
  delete tpdata->theFTFPAntiBaryon;

  delete tpdata->theBGGxsNeutron;
  delete tpdata->theNeutronHPJENDLHEInelastic;
  delete tpdata->theBGGxsProton;

  delete tpdata->xsNeutronCaptureXS;

  delete tpdata; tpdata=0;
}

void G4HadronPhysicsShielding::ConstructParticle()
{
  G4MesonConstructor pMesonConstructor;
  pMesonConstructor.ConstructParticle();

  G4BaryonConstructor pBaryonConstructor;
  pBaryonConstructor.ConstructParticle();

  G4ShortLivedConstructor pShortLivedConstructor;
  pShortLivedConstructor.ConstructParticle();  

  G4IonConstructor pIonConstructor;
  pIonConstructor.ConstructParticle();
}

#include "G4ProcessManager.hh"
void G4HadronPhysicsShielding::ConstructProcess()
{
  if ( tpdata == 0 ) tpdata = new ThreadPrivate;
  CreateModels();

  //tpdata->theBGGxsNeutron=new  G4BGGNucleonInelasticXS(G4Neutron::Neutron()); 
  tpdata->thePro->Build();
  tpdata->theNeutrons->Build();
    
  tpdata->theBGGxsNeutron = 0; //set explictly to zero or destructor may fail
//  tpdata->theBGGxsNeutron=new  G4NeutronHPBGGNucleonInelasticXS(G4Neutron::Neutron());
//  FindInelasticProcess(G4Neutron::Neutron())->AddDataSet(tpdata->theBGGxsNeutron);
//

  G4PhysListUtil::FindInelasticProcess(G4Neutron::Neutron())->AddDataSet(new  G4BGGNucleonInelasticXS(G4Neutron::Neutron()));
  tpdata->theNeutronHPJENDLHEInelastic=new G4NeutronHPJENDLHEInelasticData;
  G4PhysListUtil::FindInelasticProcess(G4Neutron::Neutron())->AddDataSet(tpdata->theNeutronHPJENDLHEInelastic);
  G4PhysListUtil::FindInelasticProcess(G4Neutron::Neutron())->AddDataSet(new G4NeutronHPInelasticData);
    
  tpdata->theBGGxsProton=0;
//  tpdata->theBGGxsProton=new G4BGGNucleonInelasticXS(G4Proton::Proton());
//  G4PhysListUtil::FindInelasticProcess(G4Proton::Proton())->AddDataSet(tpdata->theBGGxsProton);

  tpdata->thePiK->Build();
  // use CHIPS cross sections also for Kaons

  tpdata->theChipsKaonMinus = G4CrossSectionDataSetRegistry::Instance()->GetCrossSectionDataSet(G4ChipsKaonMinusInelasticXS::Default_Name());
  tpdata->theChipsKaonPlus = G4CrossSectionDataSetRegistry::Instance()->GetCrossSectionDataSet(G4ChipsKaonPlusInelasticXS::Default_Name());
  tpdata->theChipsKaonZero = G4CrossSectionDataSetRegistry::Instance()->GetCrossSectionDataSet(G4ChipsKaonZeroInelasticXS::Default_Name());
    
  G4PhysListUtil::FindInelasticProcess(G4KaonMinus::KaonMinus())->AddDataSet(tpdata->theChipsKaonMinus);
  G4PhysListUtil::FindInelasticProcess(G4KaonPlus::KaonPlus())->AddDataSet(tpdata->theChipsKaonPlus);
  G4PhysListUtil::FindInelasticProcess(G4KaonZeroShort::KaonZeroShort())->AddDataSet(tpdata->theChipsKaonZero );
  G4PhysListUtil::FindInelasticProcess(G4KaonZeroLong::KaonZeroLong())->AddDataSet(tpdata->theChipsKaonZero );

  tpdata->theHyperon->Build();
  tpdata->theAntiBaryon->Build();

  // --- Neutrons ---
  G4HadronicProcess* capture = 0;
  G4HadronicProcess* fission = 0;
  G4ProcessManager* pmanager = G4Neutron::Neutron()->GetProcessManager();
  G4ProcessVector*  pv = pmanager->GetProcessList();
  for ( size_t i=0; i < static_cast<size_t>(pv->size()); ++i ) {
    if ( fCapture == ((*pv)[i])->GetProcessSubType() ) {
      capture = static_cast<G4HadronicProcess*>((*pv)[i]);
    } else if ( fFission == ((*pv)[i])->GetProcessSubType() ) {
      fission = static_cast<G4HadronicProcess*>((*pv)[i]);
    }
  }
  if ( ! capture ) {
    capture = new G4HadronCaptureProcess("nCapture");
    pmanager->AddDiscreteProcess(capture);
  }
  tpdata->xsNeutronCaptureXS = new G4NeutronCaptureXS();
  capture->AddDataSet(tpdata->xsNeutronCaptureXS);
  capture->AddDataSet( new G4NeutronHPCaptureData );
  G4NeutronRadCapture* theNeutronRadCapture = new G4NeutronRadCapture(); 
  theNeutronRadCapture->SetMinEnergy( 19.9*MeV ); 
  capture->RegisterMe( theNeutronRadCapture );
  if ( ! fission ) {
    fission = new G4HadronFissionProcess("nFission");
    pmanager->AddDiscreteProcess(fission);
  }
  G4LFission* theNeutronLEPFission = new G4LFission();
  theNeutronLEPFission->SetMinEnergy( 19.9*MeV );
  fission->RegisterMe( theNeutronLEPFission );
}
