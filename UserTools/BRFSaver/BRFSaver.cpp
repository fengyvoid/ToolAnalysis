#include "BRFSaver.h"

BRFSaver::BRFSaver() : Tool() {}

bool BRFSaver::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if (configfile != "")
    m_variables.Initialise(configfile); // loading config file
  // m_variables.Print();

  m_data = &data; // assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  verbosityBRFSaver = 0;
  m_variables.Get("verbosityBRFSaver", verbosityBRFSaver);
  output_filename = "BRFData.root";
  m_variables.Get("output_filename", output_filename);
  SaveRange = 100;
  m_variables.Get("SaveRange", SaveRange);

  return true;
}

bool BRFSaver::Execute()
{

  Log("FitRWMWaveform: Execute()", v_debug, verbosityBRFSaver);
  m_data->Stores["ANNIEEvent"]->Get("BRFRawWaveform", BRFRawWaveform);

  // if not a beam event, return
  int fPrimaryTriggerWord = 0;
  m_data->Stores["ANNIEEvent"]->Get("PrimaryTriggerWord", fPrimaryTriggerWord);
  if (fPrimaryTriggerWord != 14){
    return true;
  }

  // get run number and part file number
  int fRunNumber, fPartFileNumber;
  m_data->Stores["ANNIEEvent"]->Get("RunNumber", fRunNumber);
  m_data->Stores["ANNIEEvent"]->Get("PartNumber", fPartFileNumber);
  runNumbers.push_back(fRunNumber);
  partFileNumbers.push_back(fPartFileNumber);

  // extract the first 100 bins of the BRF waveform, save it to a vector
  std::vector<uint16_t> waveform_to_save;
  for (int i = 0; i<SaveRange ; i++)
  {
    if (i < BRFRawWaveform.size())
    {
      waveform_to_save.push_back(BRFRawWaveform[i]);
    }else{
      waveform_to_save.push_back(0);
    }
  }
  BRFWaveformsToSave.push_back(waveform_to_save);


  // find the minima and maxima of the BRF waveform in the saveRange, save the bin number and value
  // Define the minima as a bin with smaller value compare with two bins before it and two bins after it, 
  // Define the maxima as a bin with larger value compare with two bins before it and two bins after it
  std::vector<double> minima_bins;
  std::vector<double> minima_amplitudes;
  std::vector<double> minima_times;
  std::vector<double> maxima_bins;
  std::vector<double> maxima_amplitudes;
  std::vector<double> maxima_times;

  for (int i = 2; i < SaveRange - 2; i++)
  {
    if (waveform_to_save[i] < waveform_to_save[i - 1] && waveform_to_save[i] < waveform_to_save[i - 2] && waveform_to_save[i] < waveform_to_save[i + 1] && waveform_to_save[i] < waveform_to_save[i + 2])
    {
      minima_bins.push_back(i);
      minima_amplitudes.push_back(waveform_to_save[i]);
      minima_times.push_back(i * 2); // 2ns per bin
    }
    if (waveform_to_save[i] > waveform_to_save[i - 1] && waveform_to_save[i] > waveform_to_save[i - 2] && waveform_to_save[i] > waveform_to_save[i + 1] && waveform_to_save[i] > waveform_to_save[i + 2])
    {
      maxima_bins.push_back(i);
      maxima_amplitudes.push_back(waveform_to_save[i]);
      maxima_times.push_back(i * 2); // 2ns per bin
    }
  }
  BRFMinimaToSave.push_back(minima_bins);
  BRFMinimaAmplitudesToSave.push_back(minima_amplitudes);
  BRFMinimaTimesToSave.push_back(minima_times);
  BRFMaximaToSave.push_back(maxima_bins);
  BRFMaximaAmplitudesToSave.push_back(maxima_amplitudes);
  BRFMaximaTimesToSave.push_back(maxima_times);

  Log("FitRWMWaveform: Execute(): BRF waveform saved", v_debug, verbosityBRFSaver);

  return true;
}

bool BRFSaver::Finalise()
{

  //Now save them to root tree BRF into the output file
  TFile *fOutput_tfile = new TFile(output_filename.c_str(), "recreate");
  TTree *fOutput_tree = new TTree("BRF", "BRF");
  int runNumber;
  int partFileNumber;
  std::vector<uint16_t> waveform;
  std::vector<double> minima_bins;
  std::vector<double> minima_amplitudes;
  std::vector<double> minima_times;
  std::vector<double> maxima_bins;
  std::vector<double> maxima_amplitudes;
  std::vector<double> maxima_times;
  fOutput_tree->Branch("runNumber", &runNumber);
  fOutput_tree->Branch("partFileNumber", &partFileNumber);
  fOutput_tree->Branch("waveform", &waveform);
  fOutput_tree->Branch("minima_bins", &minima_bins);
  fOutput_tree->Branch("minima_amplitudes", &minima_amplitudes);
  fOutput_tree->Branch("minima_times", &minima_times);
  fOutput_tree->Branch("maxima_bins", &maxima_bins);
  fOutput_tree->Branch("maxima_amplitudes", &maxima_amplitudes);
  fOutput_tree->Branch("maxima_times", &maxima_times);  
  for (size_t i = 0; i < runNumbers.size(); ++i) {
    runNumber = runNumbers[i];
    partFileNumber = partFileNumbers[i];
    waveform = BRFWaveformsToSave[i];
    minima_bins = BRFMinimaToSave[i];
    minima_amplitudes = BRFMinimaAmplitudesToSave[i];
    minima_times = BRFMinimaTimesToSave[i];
    maxima_bins = BRFMaximaToSave[i];
    maxima_amplitudes = BRFMaximaAmplitudesToSave[i];
    maxima_times = BRFMaximaTimesToSave[i];
    fOutput_tree->Fill();
  }
  fOutput_tree->Write();
  fOutput_tfile->Close();

  Log("FitRWMWaveform: Finalise(): BRF waveform saved to root file", v_debug, verbosityBRFSaver);

  return true;
}
