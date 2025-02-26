//
// Namespace for collecting tools used in MC truth evaluation
// Original author: Dave Brown (LBNL) 8/10/2016
//
#ifndef TrkAna_InfoMCStructHelper_hh
#define TrkAna_InfoMCStructHelper_hh
#include "fhiclcpp/types/Atom.h"
#include "Offline/RecoDataProducts/inc/StrawHitIndex.hh"
#include "Offline/MCDataProducts/inc/StrawDigiMC.hh"
#include "Offline/MCDataProducts/inc/StepPointMC.hh"
#include "Offline/MCDataProducts/inc/CrvCoincidenceClusterMC.hh"

#include "Offline/RecoDataProducts/inc/KalSeed.hh"
#include "TrkAna/inc/TrkInfo.hh"
#include "TrkAna/inc/SimInfo.hh"
#include "TrkAna/inc/TrkStrawHitInfoMC.hh"
#include "TrkAna/inc/CaloClusterInfoMC.hh"
#include "TrkAna/inc/MCStepInfo.hh"
#include "Offline/RecoDataProducts/inc/KalSeed.hh"
#include "Offline/MCDataProducts/inc/KalSeedMC.hh"
#include "BTrk/BbrGeom/HepPoint.h"
#include "Offline/MCDataProducts/inc/PrimaryParticle.hh"
#include "Offline/CRVAnalysis/inc/CrvHitInfoMC.hh"

#include <vector>
#include <functional>
namespace mu2e {

  class InfoMCStructHelper {

    private:
      art::InputTag _spctag;
      art::Handle<SimParticleCollection> _spcH;
      double _mingood;
      bool _onSpill;
      bool _mbtime;
      art::InputTag _ewMarkerTag;

      void fillSimInfo(const art::Ptr<SimParticle>& sp, SimInfo& siminfo);
      void fillSimInfo(const SimParticle& sp, SimInfo& siminfo);

    public:

      struct Config {
        using Name=fhicl::Name;
        using Comment=fhicl::Comment;

        fhicl::Atom<art::InputTag> spctag{Name("SimParticleCollectionTag"), Comment("InputTag for the SimParticleCollection"), art::InputTag()};
        fhicl::Atom<double> mingood{Name("MinGoodMomFraction"), Comment("Minimum fraction of the true particle's momentum for a digi to be described as \"good\"")};
        fhicl::Atom<art::InputTag> ewMarkerTag{ Name("EventWindowMarker"), Comment("EventWindowMarker producer"),"EWMProducer" };
     };

      InfoMCStructHelper(const Config& conf) :
        _spctag(conf.spctag()), _mingood(conf.mingood()), _ewMarkerTag(conf.ewMarkerTag()) {};

      void updateEvent(const art::Event& event);

      void fillTrkInfoMC(const KalSeed& kseed, const KalSeedMC& kseedmc, TrkInfoMC& trkinfomc);
      void fillTrkInfoMCDigis(const KalSeed& kseed, const KalSeedMC& kseedmc, TrkInfoMC& trkinfomc);
      void fillHitInfoMC(const KalSeedMC& kseedmc, TrkStrawHitInfoMC& tshinfomc, const TrkStrawHitMC& tshmc);
      void fillAllSimInfos(const KalSeedMC& kseedmc, std::vector<SimInfo>& siminfos, int n_generations);
      void fillPriInfo(const KalSeedMC& kseedmc, const PrimaryParticle& primary, SimInfo& priinfo);
      void fillTrkInfoMCStep(const KalSeedMC& kseedmc, TrkInfoMCStep& trkinfomcstep, std::vector<VirtualDetectorId> const& vids, double target_time);

      void fillHitInfoMCs(const KalSeedMC& kseedmc, std::vector<TrkStrawHitInfoMC>& tshinfomcs);
      void fillCaloClusterInfoMC(CaloClusterMC const& ccmc, CaloClusterInfoMC& ccimc);
      void fillCrvHitInfoMC(art::Ptr<CrvCoincidenceClusterMC> const& crvCoincMC, CrvHitInfoMC& crvHitInfoMC);
      void fillExtraMCStepInfos(KalSeedMC const& kseedmc, StepPointMCCollection const& mcsteps,
      MCStepInfos& mcsic, MCStepSummaryInfo& mcssi);
  };
}

#endif
