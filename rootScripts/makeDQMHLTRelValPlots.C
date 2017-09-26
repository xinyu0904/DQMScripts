
#include "TPad.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TPaveStats.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TGraph.h"
#include "TROOT.h"
#include "TKey.h"
#include "TPaletteAxis.h"
#include "TCanvas.h"
#include "TSystem.h"
#include <iostream>

namespace util {
  TH1* getHist(const std::string& histName,const std::string& filename)
  {
    TFile* file = TFile::Open(filename.c_str(),"READ");
    TH1* hist = (TH1*) file->Get(histName.c_str());
    if(hist) hist->SetDirectory(0);
    delete file;
    return hist;
  }
  void printCanvas(const std::string& outFilename,TCanvas* canvas)
  {
    std::string outputNameGif = outFilename + ".gif"; 
    canvas->Print(outputNameGif.c_str());
  }
  template<typename T>
  void setHistAttributes(T* theHist,int lineColour,int lineWidth,int markerStyle,int markerColour)
  {
    if(lineColour!=-1) theHist->SetLineColor(lineColour);
    if(lineWidth!=-1) theHist->SetLineWidth(lineWidth);
    if(markerStyle!=-1) theHist->SetMarkerStyle(markerStyle);
    if(markerColour!=-1) theHist->SetMarkerColor(markerColour);
  }
}

struct ValData {
  std::string filename;
  std::string refFilename;
  std::string legEntry;
  std::string refLegEntry;
  
  ValData(const std::string& iFilename,const std::string& iRefFilename,
	  const std::string& iLegEntry,const std::string& iRefLegEntry):
    filename(iFilename),refFilename(iRefFilename),
    legEntry(iLegEntry),refLegEntry(iRefLegEntry)
  {}
};

TH2* ratioInSigma(TH2* numer,TH2* denom)
{
  TH2* ratio = static_cast<TH2*>(numer->Clone("ratio"));
  for(int xBinNr=0;xBinNr<=numer->GetNbinsX();xBinNr++){ 
    for(int yBinNr=0;yBinNr<=numer->GetNbinsY();yBinNr++){ 
      if(denom){
	float numerVal = numer->GetBinContent(xBinNr,yBinNr);
	float numerErr = numer->GetBinError(xBinNr,yBinNr);
	float denomVal = denom->GetBinContent(xBinNr,yBinNr);
	float denomErr = denom->GetBinError(xBinNr,yBinNr);
	
	float diff = numerVal-denomVal;
	float err = std::sqrt(numerErr*numerErr+denomErr*denomErr);
	//std::cout <<"x "<<xBinNr<<" y " <<yBinNr<<" nummerVal "<<numerVal<<" denom "<<denomVal<<" diff "<<diff<<" +/- "<<err<<std::endl;
	if(err!=0) diff/=err;
	ratio->SetBinContent(xBinNr,yBinNr,diff);
	ratio->SetBinError(xBinNr,yBinNr,0);
	
      }else{
	ratio->SetBinContent(xBinNr,yBinNr,0);
	ratio->SetBinError(xBinNr,yBinNr,0);	
      }	
    }
  } 
  return ratio;
}



TCanvas* makeTPPlot(const std::string& prefix,const std::string& path,const std::string& filter,const ValData& valData)
{ 
  std::string baseDir = "DQMData/Run 1/HLT/Run summary/EGTagAndProbeEffs";
  std::vector<std::string> suffexes ={"_EEvsEt_eff","_EBvsEt_eff","_EEvsPhi_eff","_EBvsPhi_eff","_vsSCEtaPhi_eff","_vsSCEta_eff"};
  if(prefix=="eleWPTightTagPhoHighEtaProbe"){
    suffexes ={"_vsEt_eff","_vsPhi_eff","_vsSCEtaPhi_eff","_vsSCEta_eff"};
  }

  TCanvas* c1 = static_cast<TCanvas*>(gROOT->FindObject("effCanvas"));
  if(!c1) c1 = new TCanvas("effCanvas","",900*1.5,600);
  c1->cd();

  int minEntries=std::numeric_limits<int>::max();
  for(size_t histNr=0;histNr<6;histNr++){
    std::string suffex;
    if(histNr<suffexes.size()) suffex = suffexes[histNr];
    std::string histName = baseDir+"/"+path+"/"+prefix+"_"+filter+suffex;
    
    TH1* hist = util::getHist(histName,valData.filename); 
    TH1* histRef = util::getHist(histName,valData.refFilename); 
    float yOffset = ((histNr)%6)%2 * 0.5;
    float xOffset = ((histNr)%6)/2 * 0.33;

    TPad* histPad = new TPad("histPad","",xOffset,yOffset,0.33+xOffset,0.5+yOffset);
    c1->cd();
    histPad->Draw();
    histPad->cd();
    if(hist){
      if(hist->GetEntries()<minEntries) minEntries=hist->GetEntries(); 
      if(hist->ClassName()==std::string("TH2F")){
	//	hist->GetZaxis()->SetRangeUser(0,1);
	hist->Draw("COLZ");
	histPad->SetMargin(0.13,0.114,0.1005,0.09);
	histPad->Update();

	TPaletteAxis *palette = 
	  static_cast<TPaletteAxis*>(hist->GetListOfFunctions()->FindObject("palette"));
	if(palette){
	  palette->SetX1NDC(0.890927);
	  palette->SetX2NDC(0.935887);
	  palette->SetY1NDC(0.108);
	  palette->SetY2NDC(0.912892);
	}

	hist = ratioInSigma(static_cast<TH2*>(hist),
			    static_cast<TH2*>(histRef));


	hist->Draw("COLZ");

      }else{
	hist->GetYaxis()->SetRangeUser(0,1.05);
	hist->GetYaxis()->SetNdivisions(510);
	
	hist->SetMarkerStyle(8); 
	hist->SetMarkerColor(4); 
	//	hist->SetMarkerSize(0.7); 
	hist->SetLineColor(4);

	hist->Draw();
	histPad->SetGridx();
	histPad->SetGridy();

	TLegend* leg = new TLegend(0.305728,0.142857,0.847496,0.344948);
	leg->SetFillStyle(0);
	leg->SetBorderSize(0);
	leg->AddEntry(hist,valData.legEntry.c_str(),"LP");
	
	if(histRef){
	  histRef->SetMarkerStyle(4); 
	  histRef->SetMarkerColor(2); 
	  //	hist->SetMarkerSize(0.7); 
	  histRef->SetLineColor(2);
	  histRef->Draw("SAME");
	  leg->AddEntry(histRef,valData.refLegEntry.c_str(),"LP");
	  
	}else{
	  leg->AddEntry(histRef,valData.refLegEntry.c_str(),"");
	}
	leg->Draw();
      }
      
    }else{
      std::cout <<"histName"<<histName<<"not found"<<std::endl;
    }
  }
  c1->Update(); 
  if(minEntries>20 && minEntries!=std::numeric_limits<int>::max()){
    return c1;
  }else return nullptr;
}

void printAllTPPlots(const std::string& dir,const ValData& valData)
{
  gSystem->mkdir((dir+"/EGTagAndProbe").c_str(),true);
  std::vector<std::array<std::string,3> > pathsFilters={
   //Ele
    {"eleWPTightTag","HLT_DoubleEle33_CaloIdL_MW","hltEle33CaloIdLMWPMS2Filter"},
    {"eleWPTightTag","HLT_DoubleEle33_CaloIdL_MW","hltDiEle33CaloIdLMWPMS2UnseededFilter"},
    {"eleWPTightTag","HLT_Photon300_NoHE","hltEG300erFilter"},
    {"eleWPTightTag","HLT_DoublePhoton70","hltEG70HEFilter"},
    {"eleWPTightTag","HLT_DoublePhoton70","hltDiEG70HEUnseededFilter"},
    {"eleWPTightTag","HLT_DoublePhoton85","hltEG85HEFilter"},
    {"eleWPTightTag","HLT_DoublePhoton85","hltDiEG85HEUnseededFilter"},
    {"eleWPTightTag","HLT_DiSC30_18_EIso_AND_HE_Mass70","hltEG30EIso15HE30EcalIsoLastFilter"},
    {"eleWPTightTag","HLT_DiSC30_18_EIso_AND_HE_Mass70","hltEG18EIso15HE30EcalIsoUnseededFilter"},
    {"eleWPTightTag","HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL","hltEle23Ele12CaloIdLTrackIdLIsoVLTrackIsoLeg1Filter"},
    {"eleWPTightTag","HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL","hltEle23Ele12CaloIdLTrackIdLIsoVLTrackIsoLeg2Filter"},
    {"eleWPTightTag","HLT_Ele27_WPTight_Gsf","hltEle27WPTightGsfTrackIsoFilter"},
    {"eleWPTightTag","HLT_Ele32_WPTight_Gsf","hltEle32WPTightGsfTrackIsoFilter"},
    {"eleWPTightTag","HLT_Ele35_WPTight_Gsf","hltEle35noerWPTightGsfTrackIsoFilter"},
    {"eleWPTightTag","HLT_Ele38_WPTight_Gsf","hltEle38noerWPTightGsfTrackIsoFilter"},
    {"eleWPTightTag","HLT_Ele32_WPTight_Gsf_L1DoubleEG","hltEle32L1DoubleEGWPTightGsfTrackIsoFilter"},
    {"eleWPTightTag","HLT_Photon25","hltEG25L1EG18HEFilter"},
    {"eleWPTightTag","HLT_Photon33","hltEG33L1EG26HEFilter"},
    {"eleWPTightTag","HLT_Photon50","hltEG50HEFilter"},
    {"eleWPTightTag","HLT_Photon75","hltEG75HEFilter"},
    {"eleWPTightTag","HLT_Photon90","hltEG90HEFilter"},
    {"eleWPTightTag","HLT_Photon120","hltEG120HEFilter"},
    {"eleWPTightTag","HLT_Photon150","hltEG150HEFilter"},
    {"eleWPTightTag","HLT_Photon175","hltEG175HEFilter"},
    {"eleWPTightTag","HLT_Photon200","hltEG200HEFilter"},
    {"eleWPTightTag","HLT_CaloJet500","hltSingleCaloJet500"},
    {"eleWPTightTag","HLT_CaloJet550","hltSingleCaloJet550"},
    {"eleWPTightTag","HLT_Ele28_HighEta_SC20_Mass55","hltEle28HighEtaSC20TrackIsoFilter"},
    {"eleWPTightTag","HLT_Ele50_CaloIdVT_GsfTrkIdT_PFJet165","hltEle50CaloIdVTGsfTrkIdTGsfDphiFilter"},
    {"eleWPTightTag","HLT_Ele115_CaloIdVT_GsfTrkIdT","hltEle115CaloIdVTGsfTrkIdTGsfDphiFilter"},
    {"eleWPTightTag","HLT_Ele135_CaloIdVT_GsfTrkIdT","hltEle135CaloIdVTGsfTrkIdTGsfDphiFilter"},
    {"eleWPTightTag","HLT_Ele145_CaloIdVT_GsfTrkIdT","hltEle145CaloIdVTGsfTrkIdTGsfDphiFilter"},
    {"eleWPTightTag","HLT_Ele200_CaloIdVT_GsfTrkIdT","hltEle200CaloIdVTGsfTrkIdTGsfDphiFilter"},
    {"eleWPTightTag","HLT_Ele250_CaloIdVT_GsfTrkIdT","hltEle250CaloIdVTGsfTrkIdTGsfDphiFilter"},
    {"eleWPTightTag","HLT_Ele300_CaloIdVT_GsfTrkIdT","hltEle300CaloIdVTGsfTrkIdTGsfDphiFilter"},
    {"eleWPTightTag","HLT_Ele20_WPLoose_Gsf","hltEle20WPLoose1GsfTrackIsoFilter"},
    {"eleWPTightTag","HLT_Ele20_eta2p1_WPLoose_Gsf","hltEle20erWPLoose1GsfTrackIsoFilter"},
    {"eleWPTightTag","HLT_Ele20_WPTight_Gsf","hltEle20WPTightGsfTrackIsoFilter"},
    {"eleWPTightTag","HLT_DiEle27_WPTightCaloOnly_L1DoubleEG","hltEle27L1DoubleEGWPTightEcalIsoFilter"},
    {"eleWPTightTag","HLT_DiEle27_WPTightCaloOnly_L1DoubleEG","hltDiEle27L1DoubleEGWPTightEcalIsoFilter"},
    {"eleWPTightTag","HLT_DoubleEle27_CaloIdL_MW","hltEle27CaloIdLMWPMS2Filter"},
    {"eleWPTightTag","HLT_DoubleEle27_CaloIdL_MW","hltDiEle27CaloIdLMWPMS2UnseededFilter"},
    {"eleWPTightTag","HLT_DoubleEle25_CaloIdL_MW","hltEle25CaloIdLMWPMS2Filter"},
    {"eleWPTightTag","HLT_DoubleEle25_CaloIdL_MW","hltDiEle25CaloIdLMWPMS2UnseededFilter"},
    {"eleWPTightTag","HLT_Ele27_Ele37_CaloIdL_MW","hltEle27CaloIdLMWPMS2Filter"},
    {"eleWPTightTag","HLT_Ele27_Ele37_CaloIdL_MW","hltDiEle27CaloIdLMWPMS2UnseededFilter"},
    {"eleWPTightTag","HLT_Ele27_Ele37_CaloIdL_MW","hltEle37CaloIdLMWPMS2UnseededFilter"},
    {"eleWPTightTag","HLT_Ele35_WPTight_Gsf_L1EGMT","hltSingleEle35WPTightGsfL1EGMTTrackIsoFilter"},
    //Pho
    {"eleWPTightTagPhoProbe","HLT_TriplePhoton_20_20_20_CaloIdLV2","hltEG20CaloIdLV2ClusterShapeL1TripleEGFilter"},
    {"eleWPTightTagPhoProbe","HLT_TriplePhoton_20_20_20_CaloIdLV2","hltTriEG20CaloIdLV2ClusterShapeUnseededFilter"},
    {"eleWPTightTagPhoProbe","HLT_TriplePhoton_20_20_20_CaloIdLV2_R9IdVL","hltEG20CaloIdLV2R9IdVLR9IdL1TripleEGFilter"},
    {"eleWPTightTagPhoProbe","HLT_TriplePhoton_20_20_20_CaloIdLV2_R9IdVL","hltTriEG20CaloIdLV2R9IdVLR9IdUnseededFilter"},
    {"eleWPTightTagPhoProbe","HLT_TriplePhoton_30_30_10_CaloIdLV2","hltEG30CaloIdLV2ClusterShapeL1TripleEGFilter"},
    {"eleWPTightTagPhoProbe","HLT_TriplePhoton_30_30_10_CaloIdLV2","hltEG10CaloIdLV2ClusterShapeUnseededFilter"},
    {"eleWPTightTagPhoProbe","HLT_TriplePhoton_30_30_10_CaloIdLV2","hltDiEG30CaloIdLV2EtUnseededFilter"},
    {"eleWPTightTagPhoProbe","HLT_TriplePhoton_30_30_10_CaloIdLV2_R9IdVL","hltEG30CaloIdLV2R9IdVLR9IdL1TripleEGFilter"},
    {"eleWPTightTagPhoProbe","HLT_TriplePhoton_30_30_10_CaloIdLV2_R9IdVL","hltEG10CaloIdLV2R9IdVLR9IdUnseededFilter"},
    {"eleWPTightTagPhoProbe","HLT_TriplePhoton_30_30_10_CaloIdLV2_R9IdVL","hltDiEG30CaloIdLV2R9IdVLEtUnseededFilter"},
    {"eleWPTightTagPhoProbe","HLT_TriplePhoton_35_35_5_CaloIdLV2_R9IdVL","hltEG35CaloIdLV2R9IdVLR9IdL1TripleEGFilter"},
    {"eleWPTightTagPhoProbe","HLT_TriplePhoton_35_35_5_CaloIdLV2_R9IdVL","hltEG5CaloIdLV2R9IdVLR9IdUnseededFilter"},
    {"eleWPTightTagPhoProbe","HLT_TriplePhoton_35_35_5_CaloIdLV2_R9IdVL","hltDiEG35CaloIdLV2R9IdVLEtUnseededFilter"},
    //Pho-HighEta
    {"eleWPTightTagPhoHighEtaProbe","HLT_Ele28_HighEta_SC20_Mass55","hltEle28HighEtaSC20Mass55Filter"},
    {"eleWPTightTagPhoHighEtaProbe","HLT_Ele28_HighEta_SC20_Mass55","hltEle28HighEtaSC20HcalIsoFilterUnseeded"},
    //Pho-HighEta
    {"eleWPTightTagPhoHighEtaProbe","HLT_Ele28_HighEta_SC20_Mass55","hltEle28HighEtaSC20Mass55Filter"},
    {"eleWPTightTagPhoHighEtaProbe","HLT_Ele28_HighEta_SC20_Mass55","hltEle28HighEtaSC20HcalIsoFilterUnseeded"},
    //Mu-Pho
    {"muonIsoMuTagPhoProbe","HLT_Mu12_DoublePhoton20","hltMu12DiEG20HEUnseededFilter"},
    //Mu-Ele
    {"muonIsoMuTagEleProbe","HLT_Mu12_DoublePhoton20","hltMu12DiEG20HEUnseededFilter"},
    {"muonIsoMuTagEleProbe","HLT_Mu37_Ele27_CaloIdL_MW","hltEle27CaloIdLMWPMS2UnseededFilter"},
    {"muonIsoMuTagEleProbe","HLT_Mu27_Ele37_CaloIdL_MW","hltEle37CaloIdLMWPMS2UnseededFilter"},
    {"muonIsoMuTagEleProbe","HLT_DoubleEle33_CaloIdL_MW","hltEle33CaloIdLMWPMS2Filter"},
    {"muonIsoMuTagEleProbe","HLT_Ele32_WPTight_Gsf","hltEle32WPTightGsfTrackIsoFilter"},
    {"muonIsoMuTagEleProbe","HLT_Ele32_WPTight_Gsf_L1DoubleEG","hltEle32L1DoubleEGWPTightGsfTrackIsoFilter"},

  };
  
  // pathsFilters={
  //   //Ele
  //   {"eleWPTightTag","HLT_DoubleEle33_CaloIdL_MW","hltEle33CaloIdLMWPMS2Filter"}
  // };

  for(const auto& path: pathsFilters){
    auto canvas = makeTPPlot(path[0],path[1],path[2],valData);
    if(canvas){
      util::printCanvas(dir+"/EGTagAndProbe/"+path[0]+"-"+path[1]+"-"+path[2],canvas);
    }else{
      std::cout <<"skipping printing "<<path[0]+"-"+path[1]+"-"+path[2]<<std::endl;
    }
  }
}


std::vector<std::string> getHistNames(TDirectory* dir,const std::string& dirName)
{
  std::vector<std::string> names;
  dir->cd(dirName.c_str());
  TIter keyIt(gDirectory->GetListOfKeys());
  TKey* key;
  while((key = static_cast<TKey*>(keyIt()))){
    if(key->GetClassName()==std::string("TH1F")){
      std::string keyName(key->GetName());
      names.push_back(keyName);
    }
  }
  return names;
}
std::vector<std::string> getPathNames(TDirectory* dir,const std::string& dirName)
{
  std::vector<std::string> names;
  dir->cd(dirName.c_str());
  TIter keyIt(gDirectory->GetListOfKeys());
  TKey* key;
  while((key = static_cast<TKey*>(keyIt()))){
    //std::cout<<" class "<<key->GetClassName()<<std::endl;
    if(key->GetClassName()==std::string("TDirectoryFile")){
      std::string keyName(key->GetName());
      names.push_back(keyName);
    }
  }
  return names;
}

void printNonTPDQM(const std::string& plotDir,const ValData& valData)
{
  std::string dir="DQMData/Run 1/HLT/Run summary/EgOffline/Source_Histos";
  std::vector<std::string> paths={"hltEle23Ele12CaloIdLTrackIdLIsoVLTrackIsoLeg1Filter",
				  "hltEle27WPTightGsfTrackIsoFilter",
				  "hltEle115CaloIdVTGsfTrkIdTGsfDphiFilter",
				  "hltEG200HEFilter"
  };
  
  TFile* file = TFile::Open(valData.filename.c_str(),"READ");
  
  gSystem->mkdir((plotDir+"/EGOffline").c_str(),true);

  for(const auto& path : paths){
    file->cd(dir.c_str());
    std::vector<std::string> histNames = getHistNames(file,dir+"/"+path);
    file->cd();
    for(const auto& histName : histNames){
      auto hist = util::getHist(dir+"/"+path+"/"+histName,valData.filename);
      auto histRef = util::getHist(dir+"/"+path+"/"+histName,valData.refFilename);
      
      if(hist){
	util::setHistAttributes(hist,4,1,8,4);
	hist->Draw();
	if(histRef){
	  util::setHistAttributes(histRef,2,1,4,2);
	  histRef->Draw("SAME");
	}
	TCanvas* c1 = static_cast<TCanvas*>(gROOT->FindObject("c1"));
	c1->Update();
	util::printCanvas(plotDir+"/EGOffline"+"/"+histName,c1);
      }

    }
	
  }
}

void printValidationDQM(const std::string& plotDir,const ValData& valData)
{
  gSystem->mkdir((plotDir+"/EGValidation").c_str(),true);
  std::string dir="DQMData/Run 1/HLT/Run summary/HLTEgammaValidation";
  std::vector<std::string> histNames={"final_eff_vs_et","final_eff_vs_eta"};
  
  TFile* file = TFile::Open(valData.filename.c_str(),"READ");
  
  file->cd(dir.c_str());
  std::vector<std::string> pathNames = getPathNames(file,dir);
  file->cd();
  for(const auto& path : pathNames){
    if(path.find("Mu")!=std::string::npos || path.find("PFHT")!=std::string::npos){
      std::cout <<"skipping "<<path<<std::endl;
      continue;
    }
    for(const auto& histName : histNames){
      auto hist = util::getHist(dir+"/"+path+"/"+histName,valData.filename);
      auto histRef = util::getHist(dir+"/"+path+"/"+histName,valData.refFilename);
      
      if(hist){
	hist->SetTitle((path+" "+hist->GetTitle()).c_str());
	util::setHistAttributes(hist,4,1,8,4);
	hist->Draw();

	
	TLegend* leg = new TLegend(0.305728,0.142857,0.847496,0.344948);
	leg->SetBorderSize(0);
	leg->SetFillStyle(0);
	leg->AddEntry(hist,valData.legEntry.c_str(),"LP");

	if(histRef){
	  util::setHistAttributes(histRef,2,1,4,2);
	  histRef->Draw("SAME");  
	  leg->AddEntry(histRef,valData.refLegEntry.c_str(),"LP");
	}

	leg->Draw();
	TCanvas* c1 = static_cast<TCanvas*>(gROOT->FindObject("c1"));
	c1->Update();
	util::printCanvas(plotDir+"/EGValidation/"+path+"_"+histName,c1);
      }

    }
	
  }
}


void printAllPlots(const std::string& plotDir,const ValData& valData)
{
  printAllTPPlots(plotDir,valData);
  //cleaning up of the canvas when we leave the function
  TCanvas* c1 = static_cast<TCanvas*>(gROOT->FindObject("effCanvas"));
  delete c1;
  
  printNonTPDQM(plotDir,valData);
  printValidationDQM(plotDir,valData);
}

void printAllPlots(const std::string& plotDir,const std::string& filename,const std::string& refFilename,const std::string& legEntry,const std::string& refLegEntry)
{
  printAllPlots(plotDir,{filename,refFilename,legEntry,refLegEntry});
}
