#ifndef LAPPDLoadStore_H
#define LAPPDLoadStore_H

#include <string>
#include <iostream>
#include <map>
#include <bitset>
#include <fstream>
#include "Tool.h"
#include "PsecData.h"
#include "TFile.h"
#include "TTree.h"

#define NUM_CH 30
#define NUM_PSEC 5
#define NUM_SAMP 256

using namespace std;
/**
 * \class LAPPDLoadStore
 *
 * Load LAPPD PSEC data and PPS data from BoostStore.
 *
 */
class LAPPDLoadStore : public Tool
{

public:
    LAPPDLoadStore();                                       ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.
    bool ReadPedestals(int boardNo);                          ///< Read in the Pedestal Files
    bool MakePedestals();                                     ///< Make a Pedestal File

    int getParsedData(vector<unsigned short> buffer, int ch_start);
    int getParsedMeta(vector<unsigned short> buffer, int BoardId);

    void CleanDataObjects();
    bool LoadData();
    bool ParsePSECData();
    void ParsePPSData();
    bool DoPedestalSubtract();
    void SaveTimeStamps(); // save some timestamps relate to this event
    void LoadOffsetsAndCorrections();
    void LoadRunInfo();
    void SaveOffsets();

private:
    // This tool, control variables (only used in this tool, every thing that is not an data object)
    // Variables that you get from the config file
    int ReadStore;         // read data from a StoreFile (tool chain start with this tool rather than LoadANNIEEvent)
    int Nboards;           // total number of boards to load pedestal file, LAPPD number * 2
    string PedFileName;    // store format pedestal file name
    string PedFileNameTXT; // txt format pedestal file name
    int DoPedSubtract;     // 1: do pedestal subtraction, 0: don't do pedestal subtraction
    int LAPPDStoreReadInVerbosity;
    int num_vector_data;
    int num_vector_pps;
    bool SelectSingleLAPPD;
    int SelectedLAPPD;
    bool mergingModeReadIn;
    bool ReadStorePdeFile;
    bool MultiLAPPDMap; // loading map of multiple LAPPDs from ANNIEEvent
    bool loadOffsets;
    // Variables that you need in the tool
    int retval; // track the data parsing and meta parsing status
    int eventNo;
    double CLOCK_to_NSEC;
    int errorEventsNumber;
    bool runInfoLoaded;
    

    // LAPPD tool chain, control variables (Will be shared in multiple LAPPD tools to show the state of the tool chain in each loop)
    // Variables that you get from the config file
    int stopEntries;      // stop tool chain after loading this number of PSEC data events
    int NonEmptyEvents;   // count how many non empty data events were loaded
    bool PsecReceiveMode; // Get PSEC data from CStore or Stores["ANNIEEvent"]. 1: CStore, 0: Stores["ANNIEEvent"]
    string OutputWavLabel;
    string InputWavLabel;
    int NChannels;
    int Nsamples;
    int TrigChannel;
    double SampleSize;
    int LAPPDchannelOffset;
    int PPSnumber;
    bool loadPPS;
    bool loadPSEC;
    bool mergedEvent; // in some merged Event, the LAPPD events was merged to ANNIEEvent, but the data stream was not changed to be true. use this = 1 to read it
    bool LAPPDana;    // run other LAPPD tools
    // Variables that you need in the tool
    bool isCFD;
    bool isBLsub;
    bool isFiltered;
    int EventType; // 0: PSEC, 1: PPS
    // LAPPD tool chain, data variables. (Will be used in multiple LAPPD tools)
    // everything you get or set to Store, which means it may be used in other tools or it's from other tools
    int LAPPD_ID;
    vector<int> ParaBoards; // save the board index for this PsecData
    std::map<unsigned long, vector<Waveform<double>>> LAPPDWaveforms;

    // This tool, data variables (only used in this tool, every thing that is an data object)
    ifstream PedFile;   // stream for reading in the Pedestal Files
    string NewFileName; // name of the new Data File
    std::map<unsigned long, vector<int>> *PedestalValues;
    std::vector<unsigned short> Raw_buffer;
    std::vector<unsigned short> Parse_buffer;
    std::vector<int> ReadBoards;
    std::map<int, vector<unsigned short>> data;
    vector<unsigned short> meta;
    vector<unsigned short> pps;
    vector<int> LAPPD_ID_Channel;     // for each LAPPD, how many channels on it's each board
    vector<int> LAPPD_ID_BoardNumber; // for each LAPPD, how many boards with it
    std::map<unsigned long, PsecData> LAPPDDataMap;

    // data variables don't need to be cleared in each loop
    std::map<string, vector<uint64_t>> Offsets;     // Loaded offset, use string = run number + sub run number + partfile number as key.
    std::map<string, vector<int>> Offsets_minus_ps; // offset in ps, use offset - this/1e3 as the real offset
    std::map<string, vector<int>> BGCorrections;    // Loaded BGcorrections, same key as Offsets, but offset saved on event by event basis in that part file, in unit of ticks
    std::map<string, vector<int>> TSCorrections;    // TS corrections, in unit of ticks
    int runNumber;
    int subRunNumber;
    int partFileNumber;
    int eventNumberInPF;
    std::ofstream debugStoreReadIn;
};

#endif