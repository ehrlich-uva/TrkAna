//
// Namespace for collecting tools used in TrkDiag tree filling
// Original author: A. Edmonds (November 2018)
//
#include "TrkAna/inc/InfoStructHelper.hh"
#include "Offline/RecoDataProducts/inc/TrkStrawHitSeed.hh"
#include "KinKal/Trajectory/CentralHelix.hh"
#include "Offline/TrackerGeom/inc/Tracker.hh"
#include "Offline/Mu2eKinKal/inc/WireHitState.hh"
#include <cmath>

namespace mu2e {
  void InfoStructHelper::fillHitCount(StrawHitFlagCollection const& shfC, HitCount& hitcount) {
    hitcount.nsd = shfC.size();
    for(const auto& shf : shfC) {
      if(shf.hasAllProperties(StrawHitFlag::energysel))++hitcount.nesel;
      if(shf.hasAllProperties(StrawHitFlag::radsel))++hitcount.nrsel;
      if(shf.hasAllProperties(StrawHitFlag::timesel))++hitcount.ntsel;
      if(shf.hasAllProperties(StrawHitFlag::bkg))++hitcount.nbkg;
      if(shf.hasAllProperties(StrawHitFlag::trksel))++hitcount.ntpk;
    }
  }

  void InfoStructHelper::fillHitCount(RecoCount const& nrec, HitCount& hitcount) {
    hitcount.nsd = nrec._nstrawdigi;
    hitcount.ncd = nrec._ncalodigi;
    hitcount.ncrvd = nrec._ncrvdigi;
    hitcount.nesel = nrec._nshfesel;
    hitcount.nrsel = nrec._nshfrsel;
    hitcount.ntsel = nrec._nshftsel;
    hitcount.nbkg = nrec._nshfbkg;
    hitcount.ntpk = nrec._nshftpk;
  }

  void InfoStructHelper::fillTrkInfo(const KalSeed& kseed,TrkInfo& trkinfo) {
    if(kseed.status().hasAllProperties(TrkFitFlag::kalmanConverged))
      trkinfo.status = 1;
    else if(kseed.status().hasAllProperties(TrkFitFlag::kalmanOK))
      trkinfo.status = 2;
    else
      trkinfo.status = -1;

    if(kseed.status().hasAllProperties(TrkFitFlag::FitOK)){
      trkinfo.goodfit = 1;
    } else
      trkinfo.goodfit = 0;

    if(kseed.status().hasAllProperties(TrkFitFlag::CPRHelix))
      trkinfo.seedalg = 1;
    else if(kseed.status().hasAllProperties(TrkFitFlag::TPRHelix))
      trkinfo.seedalg = 0;

    if(kseed.status().hasAllProperties(TrkFitFlag::KKLoopHelix)){
      trkinfo.fitalg =1;
    } else if(kseed.status().hasAllProperties(TrkFitFlag::KKCentralHelix))
      trkinfo.fitalg = 2;
    else if(kseed.status().hasAllProperties(TrkFitFlag::KKLine))
      trkinfo.fitalg = 3;
    else
      trkinfo.fitalg = 0;

    trkinfo.pdg = kseed.particle();
    trkinfo.t0 = kseed.t0().t0();
    trkinfo.t0err = kseed.t0().t0Err();

    fillTrkInfoHits(kseed, trkinfo);

    trkinfo.chisq = kseed.chisquared();
    trkinfo.fitcon = kseed.fitConsistency();
    trkinfo.nseg = kseed.nTrajSegments();
    trkinfo.maxgap = kseed._maxgap;
    trkinfo.avggap = kseed._avggap;

    trkinfo.firsthit = kseed.hits().back()._ptoca;
    trkinfo.lasthit = kseed.hits().front()._ptoca;
    for(auto const& hit : kseed.hits()) {
      if(hit.flag().hasAllProperties(StrawHitFlag::active)){
        if( trkinfo.firsthit > hit._ptoca)trkinfo.firsthit = hit._ptoca;
        if( trkinfo.lasthit < hit._ptoca)trkinfo.lasthit = hit._ptoca;
      }
    }

    fillTrkInfoStraws(kseed, trkinfo);
  }

  void InfoStructHelper::fillTrkFitInfo(const KalSeed& kseed,TrkFitInfo& trkfitinfo, const XYZVectorF& pos) {
    const auto& ksegIter = kseed.nearestSegment(pos);
    if (ksegIter == kseed.segments().end()) {
      cet::exception("InfoStructHelper") << "Couldn't find KalSegment that includes pos = " << pos;
    }
    trkfitinfo.mom = ksegIter->momentum3();
    trkfitinfo.pos = ksegIter->position3();
    trkfitinfo.momerr = ksegIter->momerr();
    // this should be conditional on the trajectory type: maybe removed?
    if(!kseed.status().hasAllProperties(TrkFitFlag::KKLine)){
      auto ltraj = ksegIter->loopHelix();
      trkfitinfo.Rad = ltraj.rad();
      trkfitinfo.Lambda = ltraj.lam();
      trkfitinfo.Cx = ltraj.cx();
      trkfitinfo.Cy = ltraj.cy();
      trkfitinfo.phi0 = ltraj.phi0();
      trkfitinfo.t0 = ltraj.t0();
      // legacy; these follow the BTrk (CentralHelix) convention
      double rc = sqrt(ltraj.cx()*ltraj.cx()+ltraj.cy()*ltraj.cy());
      trkfitinfo.d0 = -ltraj.sign()*( rc - fabs(ltraj.rad()));
      trkfitinfo.maxr = rc + fabs(ltraj.rad());
      trkfitinfo.td = trkfitinfo.mom.Z()/trkfitinfo.mom.Rho();
    }
  }

  void InfoStructHelper::fillTrkInfoHits(const KalSeed& kseed, TrkInfo& trkinfo) {
    trkinfo.nhits = trkinfo.nactive = trkinfo.ndouble = trkinfo.ndactive = trkinfo.nplanes = trkinfo.planespan = trkinfo.nnullambig = 0;
    static StrawHitFlag active(StrawHitFlag::active);
    std::set<unsigned> planes;
    uint16_t minplane(0), maxplane(0);
    for (auto ihit = kseed.hits().begin(); ihit != kseed.hits().end(); ++ihit) {
      ++trkinfo.nhits;
      if (ihit->strawHitState() > WireHitState::inactive){
        ++trkinfo.nactive;
        planes.insert(ihit->strawId().plane());
        minplane = std::min(minplane, ihit->strawId().plane());
        maxplane = std::max(maxplane, ihit->strawId().plane());
        if (ihit->strawHitState()==WireHitState::null) {
          ++trkinfo.nnullambig;
        }
        auto jhit = ihit; jhit++;
        if(jhit != kseed.hits().end() && ihit->strawId().uniquePanel() == jhit->strawId().uniquePanel()){
          ++trkinfo.ndouble;
          if(jhit->strawHitState()>WireHitState::inactive) { ++trkinfo.ndactive; }
        }
      }
      trkinfo.nplanes = planes.size();
      trkinfo.planespan = abs(maxplane-minplane);
    }

    trkinfo.ndof = trkinfo.nactive -5; // this won't work with KinKal fits FIXME
    if (kseed.hasCaloCluster()) {
      ++trkinfo.ndof;
    }
  }

  void InfoStructHelper::fillTrkInfoStraws(const KalSeed& kseed, TrkInfo& trkinfo) {
    trkinfo.nmat = 0; trkinfo.nmatactive = 0; trkinfo.radlen = 0.0;
    for (std::vector<TrkStraw>::const_iterator i_straw = kseed.straws().begin(); i_straw != kseed.straws().end(); ++i_straw) {
      ++trkinfo.nmat;
      if (i_straw->active()) {
        ++trkinfo.nmatactive;
        trkinfo.radlen += i_straw->radLen();
      }
    }
  }

  void InfoStructHelper::fillHitInfo(const KalSeed& kseed, std::vector<TrkStrawHitInfo>& tshinfos ) {
    tshinfos.clear();
    // loop over hits

    static StrawHitFlag active(StrawHitFlag::active);
    const Tracker& tracker = *GeomHandle<Tracker>();
    for(std::vector<TrkStrawHitSeed>::const_iterator ihit=kseed.hits().begin(); ihit != kseed.hits().end(); ++ihit) {
      TrkStrawHitInfo tshinfo;
      auto const& straw = tracker.getStraw(ihit->strawId());

      tshinfo.state = ihit->_ambig;
      tshinfo.usetot = ihit->_kkshflag.hasAnyProperty(KKSHFlag::tot);
      tshinfo.usedriftdt = ihit->_kkshflag.hasAnyProperty(KKSHFlag::driftdt);
      tshinfo.useabsdt = ihit->_kkshflag.hasAnyProperty(KKSHFlag::absdrift);
      tshinfo.usendvar = ihit->_kkshflag.hasAnyProperty(KKSHFlag::nhdrift);
      tshinfo.algo = ihit->_algo;
      tshinfo.frozen = ihit->_frozen;
      tshinfo.bkgqual = ihit->_bkgqual;
      tshinfo.signqual = ihit->_signqual;
      tshinfo.driftqual = ihit->_driftqual;
      tshinfo.chi2qual = ihit->_chi2qual;
      tshinfo.earlyend   = ihit->_eend.end();
      tshinfo.plane = ihit->strawId().plane();
      tshinfo.panel = ihit->strawId().panel();
      tshinfo.layer = ihit->strawId().layer();
      tshinfo.straw = ihit->strawId().straw();

      tshinfo.edep   = ihit->_edep;
      tshinfo.etime   = ihit->_etime;
      tshinfo.wdist   = ihit->_wdist;
      tshinfo.werr   = ihit->_werr;
      tshinfo.tottdrift = ihit->_tottdrift;
      tshinfo.tot = ihit->_tot;
      tshinfo.ptoca   = ihit->_ptoca;
      tshinfo.stoca   = ihit->_stoca;
      tshinfo.rdoca   = ihit->_rdoca;
      tshinfo.rdocavar   = ihit->_rdocavar;
      tshinfo.rdt   = ihit->_rdt;
      tshinfo.rtocavar   = ihit->_rtocavar;
      tshinfo.udoca   = ihit->_udoca;
      tshinfo.udocavar   = ihit->_udocavar;
      tshinfo.udt   = ihit->_udt;
      tshinfo.utocavar   = ihit->_utocavar;
      tshinfo.rupos   = ihit->_rupos;
      tshinfo.uupos   = ihit->_uupos;
      tshinfo.rdrift   = ihit->_rdrift;
      tshinfo.cdrift   = ihit->_cdrift;
      tshinfo.sderr   = ihit->_sderr;
      tshinfo.uderr   = ihit->_uderr;
      tshinfo.dvel   = ihit->_dvel;
      tshinfo.lang   = ihit->_lang;
      tshinfo.utresid   = ihit->_utresid;
      tshinfo.utresidmvar   = ihit->_utresidmvar;
      tshinfo.utresidpvar   = ihit->_utresidpvar;
      tshinfo.udresid   = ihit->_udresid;
      tshinfo.udresidmvar   = ihit->_udresidmvar;
      tshinfo.udresidpvar   = ihit->_udresidpvar;
      tshinfo.rtresid   = ihit->_rtresid;
      tshinfo.rtresidmvar   = ihit->_rtresidmvar;
      tshinfo.rtresidpvar   = ihit->_rtresidpvar;
      tshinfo.rdresid   = ihit->_rdresid;
      tshinfo.rdresidmvar   = ihit->_rdresidmvar;
      tshinfo.rdresidpvar   = ihit->_rdresidpvar;

      // find nearest segment
      auto ikseg = kseed.nearestSegment(ihit->_ptoca);
      if(ikseg != kseed.segments().end()){
        auto tdir(ikseg->momentum3().Unit());
        tshinfo.wdot = tdir.Dot(straw.getDirection());
      }
      auto const& wiredir = straw.getDirection();
      auto const& mid = straw.getMidPoint();
      auto hpos = mid + wiredir*ihit->_wdist;
      tshinfo.poca = XYZVectorF(hpos);

      // count correlations with other TSH
      // OBSOLETE: replace this with a test for KinKal StrawHitClusters
      for(std::vector<TrkStrawHitSeed>::const_iterator jhit=kseed.hits().begin(); jhit != kseed.hits().end(); ++jhit) {
        if(jhit != ihit && ihit->strawId().plane() ==  jhit->strawId().plane() &&
            ihit->strawId().panel() == jhit->strawId().panel() ){
          tshinfo.dhit = true;
          if (jhit->flag().hasAllProperties(active)) {
            tshinfo.dactive = true;
            break;
          }
        }
      }
      tshinfos.push_back(tshinfo);
    }
  }

  void InfoStructHelper::fillMatInfo(const KalSeed& kseed, std::vector<TrkStrawMatInfo>& tminfos ) {
    tminfos.clear();
    // loop over sites, pick out the materials

    for(const auto& i_straw : kseed.straws()) {
      TrkStrawMatInfo tminfo;

      tminfo.plane = i_straw.straw().getPlane();
      tminfo.panel = i_straw.straw().getPanel();
      tminfo.layer = i_straw.straw().getLayer();
      tminfo.straw = i_straw.straw().getStraw();

      tminfo.active = i_straw.active();
      tminfo.dp = i_straw.pfrac();
      tminfo.radlen = i_straw.radLen();
      tminfo.doca = i_straw.doca();
      tminfo.tlen = i_straw.trkLen();

      tminfos.push_back(tminfo);
    }
  }

  void InfoStructHelper::fillCaloHitInfo(const KalSeed& kseed, TrkCaloHitInfo& tchinfo) {
    if (kseed.hasCaloCluster()) {
      auto const& tch = kseed.caloHit();
      auto const& cc = tch.caloCluster();
      tchinfo.active = tch._flag.hasAllProperties(StrawHitFlag::active);
      tchinfo.did = cc->diskID();
      tchinfo.poca = tch._cpos;
      tchinfo.mom = tch._tmom;
      tchinfo.cdepth = tch._cdepth;
      tchinfo.doca = tch._udoca;
      tchinfo.dt = tch._udt;
      tchinfo.ptoca = tch._uptoca;
      tchinfo.tocavar = tch._utocavar;
      tchinfo.tresid   = tch._tresid;
      tchinfo.tresidmvar   = tch._tresidmvar;
      tchinfo.tresidpvar   = tch._tresidpvar;
      tchinfo.ctime = cc->time();
      tchinfo.ctimeerr = cc->timeErr();
      tchinfo.csize = cc->size();
      tchinfo.edep = cc->energyDep();
      tchinfo.edeperr = cc->energyDepErr();
    }
  }

  void InfoStructHelper::fillTrkQualInfo(const TrkQual& tqual, TrkQualInfo& trkqualInfo) {
    int n_trkqual_vars = TrkQual::n_vars;
    for (int i_trkqual_var = 0; i_trkqual_var < n_trkqual_vars; ++i_trkqual_var) {
      TrkQual::MVA_varindex i_index = TrkQual::MVA_varindex(i_trkqual_var);
      trkqualInfo.trkqualvars[i_trkqual_var] = tqual[i_index];
    }
    trkqualInfo.mvaout = tqual.MVAOutput();
    trkqualInfo.mvastat = tqual.status();
  }


  void InfoStructHelper::fillHelixInfo(art::Ptr<HelixSeed> const& hptr, HelixInfo& hinfo) {
    if(hptr.isNonnull()){
      // count hits, active and not
      for(size_t ihit=0;ihit < hptr->hits().size(); ihit++){
        auto const& hh = hptr->hits()[ihit];
        hinfo.nch++;
        hinfo.nsh += hh.nStrawHits();
        if(!hh.flag().hasAnyProperty(StrawHitFlag::outlier)){
          hinfo.ncha++;
          hinfo.nsha += hh.nStrawHits();
        }
        if( hptr->status().hasAllProperties(TrkFitFlag::TPRHelix))
          hinfo.flag = 1;
        else if( hptr->status().hasAllProperties(TrkFitFlag::CPRHelix))
          hinfo.flag = 2;
        hinfo.t0err = hptr->t0().t0Err();
        hinfo.mom = 0.299792*hptr->helix().momentum()*_bz0; //FIXME!
        hinfo.chi2xy = hptr->helix().chi2dXY();
        hinfo.chi2fz = hptr->helix().chi2dZPhi();
        if(hptr->caloCluster().isNonnull())
          hinfo.ecalo  = hptr->caloCluster()->energyDep();
      }
    }
  }

  // this function won't work with KinKal fits and needs to be rewritten from scratch //FIXME
  void InfoStructHelper::fillTrkPIDInfo(const TrkCaloHitPID& tchp, const KalSeed& kseed, TrkPIDInfo& trkpidInfo) {
    mu2e::GeomHandle<mu2e::Calorimeter> calo;
    int n_trktchpid_vars = TrkCaloHitPID::n_vars;
    for (int i_trktchpid_var = 0; i_trktchpid_var < n_trktchpid_vars; ++i_trktchpid_var) {
      TrkCaloHitPID::MVA_varindex i_index = TrkCaloHitPID::MVA_varindex(i_trktchpid_var);
      trkpidInfo._tchpvars[i_trktchpid_var] = (float) tchp[i_index];
    }
    trkpidInfo._mvaout = tchp.MVAOutput();
    trkpidInfo._mvastat = tchp.status();
    // extrapolate the track to the calorimeter disk faces and record the transverse radius
    // Use the last segment
    auto const& trkhel = kseed.segments().back().helix();
    static const CLHEP::Hep3Vector origin;
    for(int idisk=0;idisk < 2; idisk++){
      auto ffpos = calo->geomUtil().mu2eToTracker(calo->geomUtil().diskFFToMu2e(idisk,origin));
      float flen = trkhel.zFlight(ffpos.z());
      float blen = trkhel.zFlight(ffpos.z()+207.5); // private communication B. Echenard FIXME!!!!
      XYZVectorF extpos;
      trkhel.position(flen,extpos);
      trkpidInfo._diskfrad[idisk] = sqrt(extpos.Perp2());
      trkhel.position(blen,extpos);
      trkpidInfo._diskbrad[idisk] = sqrt(extpos.Perp2());
    }
  }

  void InfoStructHelper::fillCrvHitInfo(art::Ptr<CrvCoincidenceCluster> const& crvCoinc, CrvHitInfoReco& crvHitInfo) {
    crvHitInfo._crvSectorType = crvCoinc->GetCrvSectorType();
    crvHitInfo._x = crvCoinc->GetAvgCounterPos().x();
    crvHitInfo._y = crvCoinc->GetAvgCounterPos().y();
    crvHitInfo._z = crvCoinc->GetAvgCounterPos().z();
    crvHitInfo._timeWindowStart = crvCoinc->GetStartTime();
    crvHitInfo._timeWindowEnd = crvCoinc->GetEndTime();
    crvHitInfo._PEs = crvCoinc->GetPEs();
    crvHitInfo._nCoincidenceHits = crvCoinc->GetCrvRecoPulses().size();
  }
}
