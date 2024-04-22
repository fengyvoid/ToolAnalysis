#include "LAPPDPlots.h"

LAPPDPlots::LAPPDPlots() : Tool() {}

bool LAPPDPlots::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if (configfile != "")
    m_variables.Initialise(configfile); // loading config file
  // m_variables.Print();

  m_data = &data; // assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", _geom);

  m_variables.Get("LAPPDPlotInputWaveLabel", LAPPDPlotInputWaveLabel);
  LAPPDPlotsVerbosity = 0;
  m_variables.Get("LAPPDPlotsVerbosity", LAPPDPlotsVerbosity);
  CanvasXSubPlotNumber = 2;
  m_variables.Get("CanvasXSubPlotNumber", CanvasXSubPlotNumber);
  CanvasYSubPlotNumber = 3;
  m_variables.Get("CanvasYSubPlotNumber", CanvasYSubPlotNumber);
  CanvasTotalSubPlotNumber = CanvasXSubPlotNumber * CanvasYSubPlotNumber;
  Side0EventWaveformDrawPosition = 1;
  m_variables.Get("Side0EventWaveformDrawPosition", Side0EventWaveformDrawPosition);
  Side1EventWaveformDrawPosition = 2;
  m_variables.Get("Side1EventWaveformDrawPosition", Side1EventWaveformDrawPosition);
  drawTriggerChannel = true;
  m_variables.Get("drawTriggerChannel", drawTriggerChannel);
  drawHighThreshold = 50;
  m_variables.Get("drawHighThreshold", drawHighThreshold);
  drawLowThreshold = -20;
  m_variables.Get("drawLowThreshold", drawLowThreshold);
  titleSize = 0.05;
  m_variables.Get("titleSize", titleSize);
  canvasTitleOffset = 1.05;
  m_variables.Get("canvasTitleOffset", canvasTitleOffset);
  canvasMargin = 0.15;
  m_variables.Get("canvasMargin", canvasMargin);
  CanvasWidth = 800;
  m_variables.Get("CanvasWidth", CanvasWidth);
  CanvasHeight = 1200;
  m_variables.Get("CanvasHeight", CanvasHeight);
  maxDrawEventNumber = 20;
  m_variables.Get("maxDrawEventNumber", maxDrawEventNumber);
  colorPalette = 112;
  m_variables.Get("colorPalette", colorPalette);
  DrawEventWaveform = true;
  m_variables.Get("DrawEventWaveform", DrawEventWaveform);
  OnlyDrawInBeamWindow = false;
  m_variables.Get("OnlyDrawInBeamWindow", OnlyDrawInBeamWindow);
  BeamWindowStart = 8000;
  m_variables.Get("BeamWindowStart", BeamWindowStart);
  BeamWindowEnd = 10000;
  m_variables.Get("BeamWindowEnd", BeamWindowEnd);
  Side0BinDrawPosition = 3;
  m_variables.Get("Side0BinDrawPosition", Side0BinDrawPosition);
  Side1BinDrawPosition = 4;
  m_variables.Get("Side1BinDrawPosition", Side1BinDrawPosition);
  DrawBinHist = true;
  m_variables.Get("DrawBinHist", DrawBinHist);
  BinHistMin = -20;
  m_variables.Get("BinHistMin", BinHistMin);
  BinHistMax = 50;
  m_variables.Get("BinHistMax", BinHistMax);
  BinHistNumber = 100;
  m_variables.Get("BinHistNumber", BinHistNumber);

  eventNumber = 0;

  f = new TFile("LAPPDPlots.root", "RECREATE");
  c = new TCanvas("c", "c", CanvasWidth, CanvasHeight);

  return true;
}

bool LAPPDPlots::Execute()
{
  m_data->CStore.Get("LAPPDana", LAPPDana);
  if (!LAPPDana)
    return true;
  if (eventNumber > maxDrawEventNumber)
    return true;

  inBeamWindow = CheckInBeamgateWindow();
  if (OnlyDrawInBeamWindow && (inBeamWindow == 0 || inBeamWindow == -1))
    return true;

  c->Clear();
  c->Divide(CanvasXSubPlotNumber, CanvasYSubPlotNumber);

  if (LAPPDPlotsVerbosity > 0)
    cout << "Divide canvas into " << CanvasXSubPlotNumber << "x" << CanvasYSubPlotNumber << " subplots" << endl;
  c->SetTitle("Event " + TString::Itoa(eventNumber, 10));
  c->SetName("Event" + TString::Itoa(eventNumber, 10));
  // c->cd(1);
  gStyle->SetPalette(colorPalette);

  bool gotdata = m_data->Stores["ANNIEEvent"]->Get(LAPPDPlotInputWaveLabel, lappddata);
  m_data->Stores["ANNIEEvent"]->Get("ACDCboards", ReadBoards);
  m_data->CStore.Get("LAPPD_ID", LAPPD_ID);

  if (LAPPDPlotsVerbosity > 0)
    cout << "LAPPDPlots execute with data " << LAPPDPlotInputWaveLabel << ", got data " << gotdata << ", data size " << lappddata.size() << ", Boards number" << ReadBoards.size() << ", ID " << LAPPD_ID << endl;

  if (DrawEventWaveform)
  {
    vector<int> DrawPosition = {Side0EventWaveformDrawPosition, Side1EventWaveformDrawPosition};
    for (int i = 0; i < ReadBoards.size(); i++)
    {
      const int drawPosition = DrawPosition[i];
      TPad *p = (TPad *)c->cd(drawPosition);
      // c->cd(drawPosition);
      p->cd();
      p->SetRightMargin(canvasMargin);
      p->SetLeftMargin(canvasMargin);
      p->SetTopMargin(canvasMargin);
      p->SetBottomMargin(canvasMargin);
      if (LAPPDPlotsVerbosity > 0)
        cout << "Drawing board " << ReadBoards[i] << " at position " << DrawPosition[i] << " start" << endl;
      std::map<unsigned long, vector<Waveform<double>>> boarddata = GetDataForBoard(ReadBoards[i]);
      TString HistoName = "Event" + TString::Itoa(eventNumber, 10) + "_B" + ReadBoards[i] + "_ID" + LAPPD_ID;
      // convert BGTiming to string and add to HistoName
      // if (OnlyDrawInBeamWindow)
      HistoName += "_BG" + TString::Itoa(BGTiming, 10);

      // DrawEventWaveform(c, DrawPosition[i], HistoName, boarddata);
      // TH2D h = DrawEventWaveform(HistoName, boarddata);

      int nstrips = 30;
      if (!drawTriggerChannel)
        nstrips = 28;
      TH2D *h = new TH2D(HistoName, HistoName, 256, 0, 256, nstrips, 0 - 0.5, nstrips - 0.5);
      if (LAPPDPlotsVerbosity > 3)
        cout << "Start Drawing event waveform with data size = " << boarddata.size() << endl;
      std::map<unsigned long, vector<Waveform<double>>>::iterator it;
      for (it = boarddata.begin(); it != boarddata.end(); it++)
      {
        unsigned long channelNo = it->first;
        Waveform<double> w = it->second[0];
        Channel *ch = _geom->GetChannel(channelNo);
        int stripNo = ch->GetStripNum();
        if (LAPPDPlotsVerbosity > 3)
          cout << "Drawing channel " << channelNo << " strip " << stripNo << ", with sample size " << w.GetSamples()->size() << endl;
        if (!drawTriggerChannel)
          if ((channelNo % 1000) % 30 == 5)
            continue;

        for (int i = 0; i < w.GetSamples()->size(); i++)
        {
          h->SetBinContent(i, stripNo + 1, -w.GetSamples()->at(i));
        }
      }
      if (LAPPDPlotsVerbosity > 3)
        cout << "Finish Drawing event waveform" << endl;

      h->SetMaximum(drawHighThreshold);
      h->SetMinimum(drawLowThreshold);
      h->SetStats(0);
      h->GetXaxis()->SetTitle("Time (ns)");
      h->GetXaxis()->SetTitleSize(titleSize);
      h->GetXaxis()->SetTitleOffset(canvasTitleOffset);
      h->GetYaxis()->SetTitle("Strip Number");
      h->GetYaxis()->SetTitleSize(titleSize);
      h->GetYaxis()->SetTitleOffset(canvasTitleOffset);
      h->GetZaxis()->SetTitle("Amplitude (mV)");

      h->Draw("colz");
      f->cd();
      h->Write();
      if (LAPPDPlotsVerbosity > 0)
        cout << "Drawing board " << ReadBoards[i] << " finished for Draw Event waveform" << endl;
    }
  }

  if (DrawBinHist)
  {

    vector<int> DrawPositionBinHist = {Side0BinDrawPosition, Side1BinDrawPosition};
    for (int i = 0; i < ReadBoards.size(); i++)
    {
      const int drawPosition = DrawPositionBinHist[i];
      TPad *p = (TPad *)c->cd(drawPosition);
      // c->cd(drawPosition);
      p->cd();
      p->SetRightMargin(canvasMargin);
      p->SetLeftMargin(canvasMargin);
      p->SetTopMargin(canvasMargin);
      p->SetBottomMargin(canvasMargin);
      if (LAPPDPlotsVerbosity > 0)
        cout << "Drawing board " << ReadBoards[i] << " at position " << DrawPositionBinHist[i] << " start" << endl;
      std::map<unsigned long, vector<Waveform<double>>> boarddata = GetDataForBoard(ReadBoards[i]);
      TString HistoName = "Event" + TString::Itoa(eventNumber, 10) + "_Bin_B" + ReadBoards[i] + "_ID" + LAPPD_ID;
      // convert BGTiming to string and add to HistoName
      // if (OnlyDrawInBeamWindow)
      HistoName += "_BG" + TString::Itoa(BGTiming, 10);

      int nstrips = 30;
      if (!drawTriggerChannel)
        nstrips = 28;
      const double BinHistMinConst = BinHistMin;
      const double BinHistMaxConst = BinHistMax;
      const int BinHistNumberConst = BinHistNumber;
      TH2D *h = new TH2D(HistoName, HistoName, BinHistNumberConst, BinHistMinConst, BinHistMaxConst, nstrips, 0 - 0.5, nstrips - 0.5);
      if (LAPPDPlotsVerbosity > 3)
        cout << "Start Drawing event waveform with data size = " << boarddata.size() << endl;
      std::map<unsigned long, vector<Waveform<double>>>::iterator it;
      for (it = boarddata.begin(); it != boarddata.end(); it++)
      {
        unsigned long channelNo = it->first;
        Waveform<double> w = it->second[0];
        Channel *ch = _geom->GetChannel(channelNo);
        int stripNo = ch->GetStripNum();
        if (LAPPDPlotsVerbosity > 3)
          cout << "Drawing channel " << channelNo << " strip " << stripNo << ", with sample size " << w.GetSamples()->size() << endl;
        if (!drawTriggerChannel)
          if ((channelNo % 1000) % 30 == 5)
            continue;

        for (int i = 0; i < w.GetSamples()->size(); i++)
        {
          h->Fill(-w.GetSamples()->at(i), stripNo + 1);
        }
      }
      if (LAPPDPlotsVerbosity > 3)
        cout << "Finish Drawing event waveform" << endl;

      h->SetStats(0);
      h->GetXaxis()->SetTitle("Amplitude (mV)");
      h->GetXaxis()->SetTitleSize(titleSize);
      h->GetXaxis()->SetTitleOffset(canvasTitleOffset);
      h->GetYaxis()->SetTitle("Strip Number");
      h->GetYaxis()->SetTitleSize(titleSize);
      h->GetYaxis()->SetTitleOffset(canvasTitleOffset);
      h->GetZaxis()->SetTitle("Entries");
      h->Draw("colz");

      f->cd();
      h->Write();
      if (LAPPDPlotsVerbosity > 0)
        cout << "Drawing board " << ReadBoards[i] << ", i=" << i << " in " << ReadBoards.size() << " finished for Draw Bin Hist" << endl;
    }
  }

  c->Modified();
  c->Update();

  if (eventNumber == 0)
    c->Print("LAPPDPlots.pdf(");
  else if (eventNumber == maxDrawEventNumber)
    c->Print("LAPPDPlots.pdf)");
  else if (eventNumber < maxDrawEventNumber)
    c->Print("LAPPDPlots.pdf");

  eventNumber++;
  return true;
}

bool LAPPDPlots::Finalise()
{
  if (eventNumber < maxDrawEventNumber)
    c->Print("LAPPDPlots.pdf)");
  c->Clear();
  c->Close();
  delete c;

  f->cd();
  f->Close();
  delete f;
  return true;
}

std::map<unsigned long, vector<Waveform<double>>> LAPPDPlots::GetDataForBoard(int boardID)
{

  std::map<unsigned long, vector<Waveform<double>>> boarddata;
  for (auto &it : lappddata)
  {
    unsigned long channelNo = it.first;

    Channel *ch = _geom->GetChannel(channelNo);
    int stripSide = ch->GetStripSide();
    if (LAPPDPlotsVerbosity > 4)
      cout << "GetData, channelNo: " << channelNo << ", side number" << stripSide << ", boardID " << boardID << endl;
    if (static_cast<int>((channelNo % 1000) / 60) == static_cast<int>(boardID / 2) * 2 && stripSide == boardID % 2)
    {
      boarddata[channelNo] = it.second;
      if (LAPPDPlotsVerbosity > 4)
        cout << "insearting data for channel " << channelNo << endl;
    }
  }

  return boarddata;
}

void LAPPDPlots::CleanObjects()
{
  lappddata.clear();
  ReadBoards.clear();
  LAPPD_ID = -1;
  inBeamWindow = -1;
  BGTiming = -1;
}

int LAPPDPlots::CheckInBeamgateWindow()
{
  unsigned long LAPPDDataBeamgateUL;
  unsigned long LAPPDDataTimeStampUL;
  bool got = m_data->CStore.Get("LAPPDBeamgate_Raw", LAPPDDataBeamgateUL);
  got = m_data->CStore.Get("LAPPDTimestamp_Raw", LAPPDDataTimeStampUL);

  if (got)
  {
    unsigned long Timing = (LAPPDDataTimeStampUL - LAPPDDataBeamgateUL) * 3.125;
    BGTiming = Timing;
    if (Timing > BeamWindowStart && Timing < BeamWindowEnd)
      return 1;
    else
      return 0;
  }
  else
    return -1;
}