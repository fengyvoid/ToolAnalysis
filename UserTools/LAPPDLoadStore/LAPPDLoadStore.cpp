#include "LAPPDLoadStore.h"

LAPPDLoadStore::LAPPDLoadStore() : Tool() {}

bool LAPPDLoadStore::Initialise(std::string configfile, DataModel &data)
{
    /////////////////// Useful header ///////////////////////
    if (configfile != "")
        m_variables.Initialise(configfile); // loading config file
    // m_variables.Print();

    m_data = &data; // assigning transient data pointer
    /////////////////////////////////////////////////////////////////

    // Control variables
    // Control variables used in this tool
    retval = 0;
    eventNo = 0;
    CLOCK_to_NSEC = 3.125; // 3.125 ns per clock cycle
    errorEventsNumber = 0;
    // Control variables that you get from the config file, for this tool
    m_variables.Get("ReadStore", ReadStore);
    m_variables.Get("Nboards", Nboards);
    m_variables.Get("StorePedinputfile", PedFileName);
    m_variables.Get("PedinputfileTXT", PedFileNameTXT);
    m_variables.Get("DoPedSubtraction", DoPedSubtract);
    m_variables.Get("LAPPDStoreReadInVerbosity", LAPPDStoreReadInVerbosity);
    m_variables.Get("num_vector_data", num_vector_data);
    m_variables.Get("num_vector_pps", num_vector_pps);
    m_variables.Get("SelectSingleLAPPD", SelectSingleLAPPD);
    m_variables.Get("SelectedLAPPD", SelectedLAPPD);
    mergingModeReadIn = false;
    m_variables.Get("mergingModeReadIn", mergingModeReadIn);
    ReadStorePdeFile = false;
    m_variables.Get("ReadStorePdeFile", ReadStorePdeFile);
    MultiLAPPDMap = false;
    m_variables.Get("MultiLAPPDMap", MultiLAPPDMap);
    loadPSEC = true;
    m_variables.Get("loadPSEC", loadPSEC);
    loadPPS = false;
    m_variables.Get("loadPPS", loadPPS);
    loadOffsets = false;
    m_variables.Get("loadOffsets", loadOffsets);
    // Control variables in this tool, initialized in this tool
    NonEmptyEvents = 0;
    PPSnumber = 0;
    isFiltered = false;
    isBLsub = false;
    isCFD = false;
    // Global Control variables that you get from the config file
    m_variables.Get("stopEntries", stopEntries);
    m_variables.Get("PsecReceiveMode", PsecReceiveMode);
    m_variables.Get("RawDataOutputWavLabel", OutputWavLabel);
    m_variables.Get("RawDataInputWavLabel", InputWavLabel);
    m_variables.Get("NChannels", NChannels);
    m_variables.Get("Nsamples", Nsamples);
    m_variables.Get("TrigChannel", TrigChannel);
    m_variables.Get("SampleSize", SampleSize);
    m_variables.Get("LAPPDchannelOffset", LAPPDchannelOffset);
    // Data variables
    // Data variables you get from other tools (it not initialized in execute)
    // Data variables you use in this tool
    m_variables.Get("PSECinputfile", NewFileName);
    // Verbosity
    // Details on channels, samples and max vector sizes and trigger channel
    m_variables.Get("Nsamples", Nsamples);
    m_variables.Get("TrigChannel", TrigChannel);

    runNumber = 0;
    subRunNumber = 0;
    partFileNumber = 0;
    eventNumberInPF = 0;

    // get data file
    if (ReadStore == 1)
    {
        // get data from a StoreFile in ANNIEEvent format
        m_data->Stores["ANNIEEvent"] = new BoostStore(false, 2);
        m_data->Stores["ANNIEEvent"]->Initialise(NewFileName);
        cout << "LAPPDStoreReadIn Reading new ANNIEevent from " << NewFileName << endl;
    }
    else if (ReadStore == 0)
    { // get data from previous chain, or m_data
        cout << "LAPPDStoreReadIn Using ANNIEevent or CStore" << endl;
    }
    // Grab all pedestal files and prepare the map channel|pedestal-vector for substraction
    if (DoPedSubtract == 1)
    {
        PedestalValues = new std::map<unsigned long, vector<int>>;
        if (ReadStorePdeFile)
        {
            m_data->Stores["PedestalFile"] = new BoostStore(false, 2);
            bool ret = false;
            if (FILE *file = fopen(PedFileName.c_str(), "r"))
            {
                fclose(file);
                ret = true;
                cout << "Using Store Pedestal File" << endl;
            }
            if (ret)
            {
                m_data->Stores["PedestalFile"]->Initialise(PedFileName);
                long Pedentries;
                m_data->Stores["PedestalFile"]->Header->Get("TotalEntries", Pedentries);
                if (LAPPDStoreReadInVerbosity > 0)
                    cout << PedFileName << " got " << Pedentries << endl;
                m_data->Stores["PedestalFile"]->Get("PedestalMap", PedestalValues);
            }
        }
        else
        {
            for (int i = 0; i < Nboards; i++)
            {
                if (LAPPDStoreReadInVerbosity > 0)
                    cout << "Reading Pedestal File " << PedFileNameTXT << " " << i << endl;
                ReadPedestals(i);
            }
        }
        if (LAPPDStoreReadInVerbosity > 0)
            cout << "PEDSIZES: " << PedestalValues->size() << " " << PedestalValues->at(0).size() << " " << PedestalValues->at(4).at(5) << endl;
    }

    // set some control variables for later tools
    m_data->CStore.Set("SelectSingleLAPPD", SelectSingleLAPPD);
    m_data->Stores["ANNIEEvent"]->Set("Nsamples", Nsamples);
    m_data->Stores["ANNIEEvent"]->Set("NChannels", NChannels);
    m_data->Stores["ANNIEEvent"]->Set("TrigChannel", TrigChannel);
    m_data->Stores["ANNIEEvent"]->Set("LAPPDchannelOffset", LAPPDchannelOffset);
    m_data->Stores["ANNIEEvent"]->Set("SampleSize", SampleSize);
    m_data->Stores["ANNIEEvent"]->Set("isFiltered", isFiltered);
    m_data->Stores["ANNIEEvent"]->Set("isBLsubtracted", isBLsub);
    m_data->Stores["ANNIEEvent"]->Set("isCFD", isCFD);

    if (loadOffsets)
        LoadOffsetsAndCorrections();
    debugStoreReadIn.open("debugStoreReadIn.txt");

    return true;
}

void LAPPDLoadStore::CleanDataObjects()
{
    LAPPD_ID = -9999;
    Raw_buffer.clear();
    Parse_buffer.clear();
    ReadBoards.clear();
    data.clear();
    meta.clear();
    pps.clear();
    LAPPDWaveforms.clear();
    EventType = -9999;
    LAPPDana = false;
    ParaBoards.clear();
    meta.clear();
    LAPPDWaveforms.clear();
    data.clear();
    Parse_buffer.clear();
    LAPPDDataMap.clear();
    runInfoLoaded = false;
}

bool LAPPDLoadStore::Execute()
{
    // 1. clean data variables
    // 2. decide loading data or not, load the data from PsecData dat to tool
    // 3. parse and pass data to later tools

    CleanDataObjects();
    m_data->CStore.Set("LAPPD_new_event", false);

    // decide loading data or not, set to LAPPDana for later tools
    LAPPDana = LoadData();
    m_data->CStore.Set("LAPPDana", LAPPDana);
    if (!LAPPDana)
    {
        // not loading data, return
        return true;
    }

    if (!MultiLAPPDMap)
    {
        // parse and pass data to later tools
        int frametype = static_cast<int>(Raw_buffer.size() / ReadBoards.size());
        if (frametype != num_vector_data && frametype != num_vector_pps)
        {
            cout << "Problem identifying the frametype, size of raw vector was " << Raw_buffer.size() << endl;
            cout << "It was expected to be either " << num_vector_data * ReadBoards.size() << " or " << num_vector_pps * ReadBoards.size() << endl;
            cout << "Please check manually!" << endl;
            LAPPDana = false;
            m_data->CStore.Set("LAPPDana", LAPPDana);
            m_data->CStore.Set("LAPPDPPShere", LAPPDana);
            return true;
        }

        if (frametype == num_vector_pps && loadPPS)
        {
            // if it's PPS, don't to anything relate to merging
            m_data->CStore.Set("LAPPDanaData", false);
            // set LAPPDana to false
            LAPPDana = false;
            m_data->CStore.Set("LAPPDana", LAPPDana);
            ParsePPSData();
            m_data->CStore.Set("LAPPD_ID", LAPPD_ID);
            m_data->Stores["ANNIEEvent"]->Set("LAPPD_ID", LAPPD_ID);
            m_data->CStore.Set("LoadingPPS", true);
            if (LAPPDStoreReadInVerbosity > 0)
                cout << "LAPPDStoreReadIn: PPS data loaded, LAPPDanaData is false, set LAPPDana to false" << endl;
            return true;
        }

        if (frametype == num_vector_data && loadPSEC)
        {
            m_data->CStore.Set("LAPPDanaData", true);
            bool parsData = ParsePSECData();
            LoadRunInfo();
            runInfoLoaded = true;
            LAPPDana = parsData;
            m_data->CStore.Set("LAPPDana", LAPPDana);
            m_data->CStore.Set("LoadingPPS", false);

            if (!parsData)
            {
                cout << "LAPPDStoreReadIn: PSEC data parsing failed, set LAPPDana to false and return" << endl;

                return true;
            }
        }

        // parsing finished, do pedestal subtraction
        DoPedestalSubtract();
        // save some timestamps relate to this event, for later using
        SaveTimeStamps();

        if (LAPPDStoreReadInVerbosity > 0)
            cout << "*************************END LAPPDStoreReadIn************************************" << endl;
        m_data->CStore.Set("LAPPD_ID", LAPPD_ID);
        m_data->Stores["ANNIEEvent"]->Set("LAPPD_ID", LAPPD_ID);
        m_data->Stores["ANNIEEvent"]->Set("RawLAPPDData", LAPPDWaveforms); // leave this only for the merger tool
        m_data->Stores["ANNIEEvent"]->Set("MergeLAPPDPsec", LAPPDWaveforms);
        m_data->Stores["ANNIEEvent"]->Set("ACDCmetadata", meta);
        m_data->Stores["ANNIEEvent"]->Set("ACDCboards", ReadBoards);
        m_data->Stores["ANNIEEvent"]->Set("SortedBoards", ParaBoards);
        m_data->Stores["ANNIEEvent"]->Set("TriggerChannelBase", TrigChannel);

        m_data->CStore.Set("NewLAPPDDataAvailable",true);
        debugStoreReadIn<< " Set NewLAPPDDataAvailable to true"<<endl;

        NonEmptyEvents += 1;
        eventNo++;
        if (LAPPDStoreReadInVerbosity > 2)
        {
            cout << "Finish LAPPDStoreReadIn, Printing the ANNIEEvent" << endl;
            m_data->Stores["ANNIEEvent"]->Print(false);
        }
    }
    else
    {
        // if we are reading multiple LAPPD data from one ANNIEEvent
        // assume we only have PSEC data in ANNIEEvent, no PPS event.
        // loop the map, for each PSEC data, do the same loading and parsing.
        // load the waveform by using LAPPD_ID * board_number * channel_number as the key
        if (LAPPDStoreReadInVerbosity > 0)
            cout << "LAPPDStoreReadIn: LAPPDDataMap has " << LAPPDDataMap.size() << " LAPPD PSEC data " << endl;
        bool ValidDataLoaded = false;
        std::map<unsigned long, PsecData>::iterator it;
        for (it = LAPPDDataMap.begin(); it != LAPPDDataMap.end(); it++)
        {
            unsigned long time = it->first;
            PsecData dat = it->second;

            ReadBoards = dat.BoardIndex;
            Raw_buffer = dat.RawWaveform;
            LAPPD_ID = dat.LAPPD_ID;
            if (LAPPD_ID != SelectedLAPPD && SelectSingleLAPPD)
                continue;

            int frametype = static_cast<int>(Raw_buffer.size() / ReadBoards.size());
            if (frametype != num_vector_data)
            {
                cout << "LAPPDStoreReadIn: For LAPPD_ID " << LAPPD_ID << " frametype is not num_vector_data, skip this LAPPD" << endl;
                continue;
            }
            m_data->CStore.Set("LAPPDanaData", true);
            bool parsData = ParsePSECData(); // TODO: now assuming all boards just has 30 channels. Need to be changed for gen 2
            if (parsData)
                ValidDataLoaded = true;
        }
        LAPPDana = ValidDataLoaded;
        m_data->CStore.Set("LAPPDana", LAPPDana);
        DoPedestalSubtract();

        m_data->Stores["ANNIEEvent"]->Set("RawLAPPDData", LAPPDWaveforms); // leave this only for the merger tool
        // TODO: save other timestamps, variables and metadata for later use
    }

    return true;
}

bool LAPPDLoadStore::Finalise()
{
    cout << "Got pps event in total: " << PPSnumber << endl;
    cout << "Got error events in total: " << errorEventsNumber << endl;
    cout << "Got non empty events in total: " << NonEmptyEvents << endl;
    cout << "Got event in total: " << eventNo << endl;
    return true;
}

bool LAPPDLoadStore::ReadPedestals(int boardNo)
{

    if (LAPPDStoreReadInVerbosity > 0)
        cout << "Getting Pedestals " << boardNo << endl;

    std::string LoadName = PedFileNameTXT;
    string nextLine; // temp line to parse
    double finalsampleNo;
    std::string ext = std::to_string(boardNo);
    ext += ".txt";
    LoadName += ext;
    PedFile.open(LoadName); // final name: PedFileNameTXT + boardNo + .txt
    if (!PedFile.is_open())
    {
        cout << "Failed to open " << LoadName << "!" << endl;
        return false;
    }
    if (LAPPDStoreReadInVerbosity > 0)
        cout << "Opened file: " << LoadName << endl;

    int sampleNo = 0; // sample number
    while (getline(PedFile, nextLine))
    {
        istringstream iss(nextLine);            // copies the current line in the file
        int location = -1;                      // counts the current perameter in the line
        string stempValue;                      // current string in the line
        int tempValue;                          // current int in the line
        unsigned long channelNo = boardNo * 30; // channel number
        // cout<<"NEW BOARD "<<channelNo<<" "<<sampleNo<<endl;
        // starts the loop at the beginning of the line
        while (iss >> stempValue)
        {
            location++;
            int tempValue = stoi(stempValue, 0, 10);
            if (sampleNo == 0)
            {
                vector<int> tempPed;
                tempPed.push_back(tempValue);
                // cout<<"First time: "<<channelNo<<" "<<tempValue<<endl;
                PedestalValues->insert(pair<unsigned long, vector<int>>(channelNo, tempPed));
                if (LAPPDStoreReadInVerbosity > 0)
                    cout << "Inserting pedestal at channelNo " << channelNo << endl;
                // newboard=false;
            }
            else
            {
                // cout<<"Following time: "<<channelNo<<" "<<tempValue<<"    F    "<<PedestalValues->count(channelNo)<<endl;
                (((PedestalValues->find(channelNo))->second)).push_back(tempValue);
            }

            channelNo++;
        }
        sampleNo++;
    }
    if (LAPPDStoreReadInVerbosity > 0)
        cout << "FINAL SAMPLE NUMBER: " << PedestalValues->size() << " " << (((PedestalValues->find(0))->second)).size() << endl;
    PedFile.close();
    return true;
}

bool LAPPDLoadStore::MakePedestals()
{

    // Empty for now...
    // should be moved to ASCII readin?

    return true;
}

int LAPPDLoadStore::getParsedMeta(std::vector<unsigned short> buffer, int BoardId)
{
    // Catch empty buffers
    if (buffer.size() == 0)
    {
        std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl;
        return -1;
    }

    // Prepare the Metadata vector
    // meta.clear();

    // Helpers
    int chip_count = 0;

    // Indicator words for the start/end of the metadata
    const unsigned short startword = 0xBA11;
    unsigned short endword = 0xFACE;
    unsigned short endoffile = 0x4321;

    // Empty metadata map for each Psec chip <PSEC #, vector with information>
    map<int, vector<unsigned short>> PsecInfo;

    // Empty trigger metadata map for each Psec chip <PSEC #, vector with trigger>
    map<int, vector<unsigned short>> PsecTriggerInfo;
    unsigned short CombinedTriggerRateCount;

    // Empty vector with positions of aboves startword
    vector<int> start_indices =
        {
            1539, 3091, 4643, 6195, 7747};

    // Fill the psec info map
    vector<unsigned short>::iterator bit;
    for (int i : start_indices)
    {
        // Write the first word after the startword
        bit = buffer.begin() + (i + 1);

        // As long as the endword isn't reached copy metadata words into a vector and add to map
        vector<unsigned short> InfoWord;
        while (*bit != endword && *bit != endoffile && InfoWord.size() < 14)
        {
            InfoWord.push_back(*bit);
            ++bit;
        }
        PsecInfo.insert(pair<int, vector<unsigned short>>(chip_count, InfoWord));
        chip_count++;
    }

    // Fill the psec trigger info map
    for (int chip = 0; chip < NUM_PSEC; chip++)
    {
        for (int ch = 0; ch < NUM_CH / NUM_PSEC; ch++)
        {
            if (LAPPDStoreReadInVerbosity > 10)
                cout << "parsing meta step1-1" << endl;
            // Find the trigger data at begin + last_metadata_start + 13_info_words + 1_end_word + 1
            bit = buffer.begin() + start_indices[4] + 13 + 1 + 1 + ch + (chip * (NUM_CH / NUM_PSEC));
            if (LAPPDStoreReadInVerbosity > 10)
                cout << "parsing meta step1-2" << endl;
            PsecTriggerInfo[chip].push_back(*bit);
        }
    }

    if (LAPPDStoreReadInVerbosity > 10)
        cout << "parsing meta step1.5" << endl;
    // Fill the combined trigger
    CombinedTriggerRateCount = buffer[7792];

    //----------------------------------------------------------
    // Start the metadata parsing

    meta.push_back(BoardId);
    for (int CHIP = 0; CHIP < NUM_PSEC; CHIP++)
    {
        meta.push_back((0xDCB0 | CHIP));
        // cout<<"size of info word is "<<PsecInfo[CHIP].size()<<endl;
        // cout<<"size of trigger word is "<<PsecTriggerInfo[CHIP].size()<<endl;
        for (int INFOWORD = 0; INFOWORD < 13; INFOWORD++)
        {
            if (LAPPDStoreReadInVerbosity > 10)
                cout << "parsing meta step2-1 infoword " << INFOWORD << endl;
            if (PsecInfo[CHIP].size() < 13)
            {
                NonEmptyEvents = NonEmptyEvents - 1;
                cout << "meta data parsing wrong! PsecInfo[CHIP].size() < 13" << endl;
                m_data->CStore.Set("LAPPDana", false);
                return 1;
            }

            try
            {
                meta.push_back(PsecInfo[CHIP][INFOWORD]);
            }
            catch (...)
            {
                NonEmptyEvents = NonEmptyEvents - 1;
                cout << "meta data parsing wrong! meta.push_back(PsecInfo[CHIP][INFOWORD]);" << endl;
                m_data->CStore.Set("LAPPDana", false);
                return 1;
            }
        }
        for (int TRIGGERWORD = 0; TRIGGERWORD < 6; TRIGGERWORD++)
        {
            if (LAPPDStoreReadInVerbosity > 10)
                cout << "parsing meta step2-2 trigger word" << endl;

            if (PsecTriggerInfo[CHIP].size() < 6)
            {
                NonEmptyEvents = NonEmptyEvents - 1;
                cout << "meta data parsing wrong! PsecTriggerInfo[CHIP].size() < 6" << endl;
                m_data->CStore.Set("LAPPDana", false);
                return 1;
            }

            try
            {
                meta.push_back(PsecTriggerInfo[CHIP][TRIGGERWORD]);
            }
            catch (...)
            {
                NonEmptyEvents = NonEmptyEvents - 1;
                cout << "meta data parsing wrong! meta.push_back(PsecTriggerInfo[CHIP][TRIGGERWORD]);" << endl;
                m_data->CStore.Set("LAPPDana", false);
                return 1;
            }
        }
    }

    meta.push_back(CombinedTriggerRateCount);
    meta.push_back(0xeeee);
    return 0;
}

int LAPPDLoadStore::getParsedData(std::vector<unsigned short> buffer, int ch_start)
{
    // Catch empty buffers
    if (buffer.size() == 0)
    {
        std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl;
        return -1;
    }

    // Helpers
    int DistanceFromZero;
    int channel_count = 0;

    // Indicator words for the start/end of the metadata
    const unsigned short startword = 0xF005;
    unsigned short endword = 0xBA11;
    unsigned short endoffile = 0x4321;

    // Empty vector with positions of aboves startword
    vector<int> start_indices =
        {
            2, 1554, 3106, 4658, 6210};

    // Fill data map
    vector<unsigned short>::iterator bit;
    for (int i : start_indices)
    {
        // Write the first word after the startword
        bit = buffer.begin() + (i + 1);

        // As long as the endword isn't reached copy metadata words into a vector and add to map
        vector<unsigned short> InfoWord;
        while (*bit != endword && *bit != endoffile)
        {
            InfoWord.push_back((unsigned short)*bit);
            if (InfoWord.size() == NUM_SAMP)
            {
                data.insert(pair<int, vector<unsigned short>>(ch_start + channel_count, InfoWord));
                InfoWord.clear();
                channel_count++;
            }
            ++bit;
        }
    }

    return 0;
}

bool LAPPDLoadStore::LoadData()
{
    // TODO: when looping in Stores["ANNIEEvent"], the multiple PSEC data will be saved in std::map<double,PsecData> LAPPDDatas;
    // so we need to loop the map to get all data and waveforms, using the LAPPD_ID, board number, channel number to form a global channel-waveform map
    // then loop all waveforms based on channel number

    m_data->Stores["ANNIEEvent"]->GetEntry(eventNo);
    if (LAPPDStoreReadInVerbosity > 2)
        cout << "Got eventNo " << eventNo << endl;

    // if loaded enough events, stop the loop and return false
    if (NonEmptyEvents == stopEntries)
    {
        if (LAPPDStoreReadInVerbosity > 0)
            cout << "LAPPDStoreReadIn: NonEmptyEvents is " << NonEmptyEvents << ", stopEntries is " << stopEntries << ", stop the loop" << endl;
        m_data->vars.Set("StopLoop", 1);
        return false;
    }

    std::map<std::string, bool> DataStreams;
    m_data->Stores["ANNIEEvent"]->Get("DataStreams", DataStreams);
    if (mergedEvent)
        DataStreams["LAPPD"] = true;

    if (loadPSEC || loadPPS)
    { // if load any kind of data
        // if there is no LAPPD data in event store, and not getting data from CStore, return false, don't load
        if (!DataStreams["LAPPD"] && PsecReceiveMode == 0)
        {
            return false;
        }
        else if (PsecReceiveMode == 1) // no LAPPD data in event store, but load from CStore (for merging LAPPD to ANNIEEvent)
        {                              // only get PSEC data from CStore, no PPS
            // if loading from raw data, and the loading was set to pause, return false
            bool LAPPDRawLoadingPaused = false;
            m_data->CStore.Get("PauseLAPPDDecoding", LAPPDRawLoadingPaused);
            if (LAPPDRawLoadingPaused){
                m_data->CStore.Set("NewLAPPDDataAvailable",false);
                return false;
            }
                

            PsecData dat;
            bool getData = m_data->CStore.Get("LAPPDData", dat);
            if(getData){
                m_data->CStore.Set("StoreLoadedLAPPDData",dat);
            }
            if (LAPPDStoreReadInVerbosity > 0)
                cout << "LAPPDStoreReadIn: getting LAPPDData from CStore" << endl;
            bool mergingLoad;
            // if in merging mode, but no LAPPD data in CStore, return false, don't load
            m_data->CStore.Get("LAPPDanaData", mergingLoad);
            if (!mergingLoad && mergingModeReadIn)
            {
                if (LAPPDStoreReadInVerbosity > 0)
                    cout << "LAPPDStoreReadIn: mergingMode is true but LAPPDanaData is false, set LAPPDana to false" << endl;
                return false;
            }
            if (getData)
            {
                vector<unsigned int> errorcodes = dat.errorcodes;
                if (errorcodes.size() == 1 && errorcodes[0] == 0x00000000)
                {
                    if (LAPPDStoreReadInVerbosity > 1)
                        printf("No errorcodes found all good: 0x%08x\n", errorcodes[0]);
                }
                else
                {
                    printf("When Loading PPS: Errorcodes found: %li\n", errorcodes.size());
                    for (unsigned int k = 0; k < errorcodes.size(); k++)
                    {
                        printf("Errorcode: 0x%08x\n", errorcodes[k]);
                    }
                    errorEventsNumber++;
                    return false;
                }
                ReadBoards = dat.BoardIndex;
                Raw_buffer = dat.RawWaveform;
                LAPPD_ID = dat.LAPPD_ID;
                if (LAPPD_ID != SelectedLAPPD && SelectSingleLAPPD)
                    return false;
                m_data->CStore.Set("PsecTimestamp", dat.Timestamp);
                if (LAPPDStoreReadInVerbosity > 2)
                {
                    cout << " Got Data " << endl;
                    dat.Print();
                }
                int frameType = static_cast<int>(Raw_buffer.size() / ReadBoards.size());
                if (LAPPDStoreReadInVerbosity > 0)
                    cout << "LAPPDStoreReadIn: got Data from CStore, frame type is " << frameType << endl;
                if (loadPSEC)
                {
                    if (frameType == num_vector_data)
                    {
                        m_data->CStore.Set("LoadingPPS", false);
                        return true;
                    }
                }
                if (loadPPS)
                {
                    if (frameType == num_vector_pps)
                    {
                        m_data->CStore.Set("LoadingPPS", true);
                        return true;
                    }
                }
                return false;
            }
            else
            {
                return false;
            }
        }
        else if (DataStreams["LAPPD"] && PsecReceiveMode == 0)
        {
            PsecData dat;
            m_data->Stores["ANNIEEvent"]->Get("LAPPDData", dat);
            ReadBoards = dat.BoardIndex;
            Raw_buffer = dat.RawWaveform;
            LAPPD_ID = dat.LAPPD_ID;
            if (LAPPD_ID != SelectedLAPPD && SelectSingleLAPPD)
                return false;
            m_data->CStore.Set("PsecTimestamp", dat.Timestamp);

            if (Raw_buffer.size() != 0 || ReadBoards.size() != 0)
            {
                if (LAPPDStoreReadInVerbosity > 0)
                {
                    cout << "Getting data length format" << static_cast<int>(Raw_buffer.size() / ReadBoards.size()) << ", psec timestamp is " << dat.Timestamp << endl;
                    cout << "ReadBoards size " << ReadBoards.size() << " Raw_buffer size " << Raw_buffer.size() << " LAPPD_ID " << LAPPD_ID << endl;
                }
            }
            else
            {
                cout << "LAPPDStoreReadIn: loading data with raw buffer size 0 or ReadBoards size 0, skip loading" << endl;
                cout << "ReadBoards size " << ReadBoards.size() << " Raw_buffer size " << Raw_buffer.size() << " LAPPD_ID " << LAPPD_ID << endl;

                return false;
            }
            return true;
        }
        else if (DataStreams["LAPPD"] && MultiLAPPDMap)
        {
            bool getMap = m_data->Stores["ANNIEEvent"]->Get("LAPPDDataMap", LAPPDDataMap);
            if (LAPPDStoreReadInVerbosity > 0)
                cout << "LAPPDStoreReadIn: getting LAPPDDataMap from ANNIEEvent" << endl;
            if (getMap)
                return true;
        }
    }
    return false; // if not any of the above, return false
}

void LAPPDLoadStore::ParsePPSData()
{
    if (LAPPDStoreReadInVerbosity > 0)
        cout << "Loading PPS frame size " << pps.size() << endl;
    std::vector<unsigned short> pps = Raw_buffer;
    std::vector<unsigned long> pps_vector;
    std::vector<unsigned long> pps_count_vector;

    unsigned long pps_timestamp = 0;
    unsigned long ppscount = 0;
    for (int s = 0; s < ReadBoards.size(); s++)
    {
        unsigned short pps_63_48 = pps.at(2 + 16 * s);
        unsigned short pps_47_32 = pps.at(3 + 16 * s);
        unsigned short pps_31_16 = pps.at(4 + 16 * s);
        unsigned short pps_15_0 = pps.at(5 + 16 * s);
        std::bitset<16> bits_pps_63_48(pps_63_48);
        std::bitset<16> bits_pps_47_32(pps_47_32);
        std::bitset<16> bits_pps_31_16(pps_31_16);
        std::bitset<16> bits_pps_15_0(pps_15_0);
        unsigned long pps_63_0 = (static_cast<unsigned long>(pps_63_48) << 48) + (static_cast<unsigned long>(pps_47_32) << 32) + (static_cast<unsigned long>(pps_31_16) << 16) + (static_cast<unsigned long>(pps_15_0));
        if (LAPPDStoreReadInVerbosity > 0)
            std::cout << "pps combined: " << pps_63_0 << std::endl;
        std::bitset<64> bits_pps_63_0(pps_63_0);
        // pps_timestamp = pps_63_0 * (CLOCK_to_NSEC); // NOTE: Don't do convert to ns because of the precision, do this in later tools
        pps_timestamp = pps_63_0;
        // LAPPDPPS->push_back(pps_timestamp);
        if (LAPPDStoreReadInVerbosity > 0)
            std::cout << "Adding timestamp " << pps_timestamp << " to LAPPDPPS" << std::endl;
        pps_vector.push_back(pps_timestamp);

        unsigned short ppscount_31_16 = pps.at(8 + 16 * s);
        unsigned short ppscount_15_0 = pps.at(9 + 16 * s);
        std::bitset<16> bits_ppscount_31_16(ppscount_31_16);
        std::bitset<16> bits_ppscount_15_0(ppscount_15_0);
        unsigned long ppscount_31_0 = (static_cast<unsigned long>(ppscount_31_16) << 16) + (static_cast<unsigned long>(ppscount_15_0));
        if (LAPPDStoreReadInVerbosity > 0)
            std::cout << "pps count combined: " << ppscount_31_0 << std::endl;
        std::bitset<32> bits_ppscount_31_0(ppscount_31_0);
        ppscount = ppscount_31_0;
        pps_count_vector.push_back(ppscount);

        if (LAPPDStoreReadInVerbosity > 8)
        {
            // Print the bitsets
            cout << "******************************" << endl;
            std::cout << "printing ACDC " << s << ": " << endl;
            std::cout << "bits_pps_63_48: " << bits_pps_63_48 << std::endl;
            std::cout << "bits_pps_47_32: " << bits_pps_47_32 << std::endl;
            std::cout << "bits_pps_31_16: " << bits_pps_31_16 << std::endl;
            std::cout << "bits_pps_15_0: " << bits_pps_15_0 << std::endl;
            // Print the unsigned shorts
            std::cout << "pps_63_48: " << pps_63_48 << std::endl;
            std::cout << "pps_47_32: " << pps_47_32 << std::endl;
            std::cout << "pps_31_16: " << pps_31_16 << std::endl;
            std::cout << "pps_15_0: " << pps_15_0 << std::endl;
            std::cout << "pps_63_0: " << pps_63_0 << std::endl;
            std::cout << "pps_63_0 after conversion in double: " << pps_timestamp << endl;

            for (int x = 0; x < 16; x++)
            {
                std::bitset<16> bit_pps_here(pps.at(x + 16 * s));
                cout << "unsigned short at " << x << " : " << pps.at(x + 16 * s) << ", bit at " << x << " is: " << bit_pps_here << endl;
                ;
            }
        }
    }

    // double ppsDiff = static_cast<double>(pps_vector.at(0)) - static_cast<double>(pps_vector.at(1));
    unsigned long ppsDiff = pps_vector.at(0) - pps_vector.at(1);
    m_data->CStore.Set("LAPPDPPScount0", pps_count_vector.at(0));
    m_data->CStore.Set("LAPPDPPScount1", pps_count_vector.at(1));
    m_data->CStore.Set("LAPPDPPScount", pps_count_vector);
    m_data->CStore.Set("LAPPDPPSDiff0to1", ppsDiff);
    m_data->CStore.Set("LAPPDPPSVector", pps_vector);

    m_data->CStore.Set("LAPPDPPStimestamp0", pps_vector.at(0));
    m_data->CStore.Set("LAPPDPPStimestamp1", pps_vector.at(1));
    m_data->CStore.Set("LAPPDPPShere", true);
    m_data->CStore.Set("LAPPD_ID", LAPPD_ID);

    PPSnumber++;
}

bool LAPPDLoadStore::ParsePSECData()
{
    if (LAPPDStoreReadInVerbosity > 0)
        std::cout << "PSEC Data Frame was read! Starting the parsing!" << std::endl;

    // while loading single PsecData Object, parse the data by LAPPDID and number of boards on each LAPPD and channel on each board
    //  Create a vector of paraphrased board indices
    //  the board indices goes with LAPPD ID. For example, LAPPD ID = 2, we will have board = 4,5
    //  this need to be converted to 0,1
    int nbi = ReadBoards.size();
    if (LAPPDStoreReadInVerbosity > 0 && nbi != 2)
        cout << "Number of board is " << nbi << endl;
    if (nbi == 0)
    {
        cout << "LAPPDStoreReadIn: error here! number of board is 0" << endl;
        errorEventsNumber++;
        return false;
    }
    if (nbi % 2 != 0)
    {
        errorEventsNumber++;
        cout << "LAPPDStoreReadIn: uneven number of boards in this event" << endl;
        if (nbi == 1)
        {
            ParaBoards.push_back(ReadBoards[0]);
        }
        else
        {
            return false;
        }
    }
    else
    {
        for (int cbi = 0; cbi < nbi; cbi++)
        {
            ParaBoards.push_back(cbi);
            if (LAPPDStoreReadInVerbosity > 2)
                cout << "Board " << cbi << " is added to the list of boards to be parsed!" << endl;
        }
    }
    // loop all boards, 0, 1
    for (int bi : ParaBoards)
    {
        Parse_buffer.clear();
        if (LAPPDStoreReadInVerbosity > 2)
            std::cout << "Starting with board " << ReadBoards[bi] << std::endl;
        // Go over all ACDC board data frames by seperating them
        int frametype = static_cast<int>(Raw_buffer.size() / ReadBoards.size());
        for (int c = bi * frametype; c < (bi + 1) * frametype; c++)
        {
            Parse_buffer.push_back(Raw_buffer[c]);
        }
        if (LAPPDStoreReadInVerbosity > 2)
            std::cout << "Data for board " << ReadBoards[bi] << " was grabbed!" << std::endl;

        // Grab the parsed data and give it to a global variable 'data'
        // insert the data start with channel number 30*ReadBoards[bi]
        // for instance, when bi=0 , LAPPD ID = 2, ReadBoards[bi] = 4, insert to channel number start with 120, to 150
        retval = getParsedData(Parse_buffer, ReadBoards[bi] * NUM_CH); //(because there are only 2 boards, so it's 0*30 or 1*30). Inserting the channel number start from this then ++ to 30
        if (retval == 0)
        {
            if (LAPPDStoreReadInVerbosity > 2)
                std::cout << "Data for board " << ReadBoards[bi] << " was parsed!" << std::endl;
            // Grab the parsed metadata and give it to a global variable 'meta'
            retval = getParsedMeta(Parse_buffer, ReadBoards[bi]);
            if (retval != 0)
            {
                std::cout << "Meta parsing went wrong! " << retval << endl;
                return false;
            }
            else
            {
                if (LAPPDStoreReadInVerbosity > 2)
                    std::cout << "Meta for board " << ReadBoards[bi] << " was parsed!" << std::endl;
            }
        }
        else
        {
            std::cout << "Parsing went wrong! " << retval << endl;
            return false;
        }
    }

    return true;
}

bool LAPPDLoadStore::DoPedestalSubtract()
{
    Waveform<double> tmpWave;
    vector<Waveform<double>> VecTmpWave;
    int pedval, val;
    // Loop over data stream
    for (std::map<int, vector<unsigned short>>::iterator it = data.begin(); it != data.end(); ++it) // looping over the data map by channel number, from 0 to 60
    {
        int wrongPedChannel = 0;
        for (int kvec = 0; kvec < it->second.size(); kvec++)
        { // loop all data point in this channel
            if (DoPedSubtract == 1)
            {
                auto iter = PedestalValues->find((it->first));
                if (iter != PedestalValues->end() && iter->second.size() > kvec)
                {
                    pedval = iter->second.at(kvec);
                }
                else
                {
                    pedval = 0;
                    wrongPedChannel = (it->first);
                }
            }
            else
            {
                pedval = 0;
            }
            val = it->second.at(kvec);
            tmpWave.PushSample(0.3 * (double)(val - pedval));
        }
        if (wrongPedChannel != 0)
            cout << "Pedestal value not found for channel " << wrongPedChannel << "with it->first channel" << it->first << ", LAPPD channel shift " << LAPPD_ID * 60 << endl;

        VecTmpWave.push_back(tmpWave);

        unsigned long pushChannelNo = (unsigned long)it->first;
        LAPPDWaveforms.insert(pair<unsigned long, vector<Waveform<double>>>(pushChannelNo, VecTmpWave));

        tmpWave.ClearSamples();
        VecTmpWave.clear();
    }
    return true;
}

void LAPPDLoadStore::SaveTimeStamps()
{
    unsigned short beamgate_63_48 = meta.at(7);
    unsigned short beamgate_47_32 = meta.at(27);
    unsigned short beamgate_31_16 = meta.at(47);
    unsigned short beamgate_15_0 = meta.at(67);
    std::bitset<16> bits_beamgate_63_48(beamgate_63_48);
    std::bitset<16> bits_beamgate_47_32(beamgate_47_32);
    std::bitset<16> bits_beamgate_31_16(beamgate_31_16);
    std::bitset<16> bits_beamgate_15_0(beamgate_15_0);
    unsigned long beamgate_63_0 = (static_cast<unsigned long>(beamgate_63_48) << 48) + (static_cast<unsigned long>(beamgate_47_32) << 32) + (static_cast<unsigned long>(beamgate_31_16) << 16) + (static_cast<unsigned long>(beamgate_15_0));
    std::bitset<64> bits_beamgate_63_0(beamgate_63_0);
    unsigned long beamgate_timestamp = beamgate_63_0 * (CLOCK_to_NSEC);
    m_data->CStore.Set("LAPPDbeamgate", beamgate_timestamp);
    m_data->CStore.Set("LAPPDBeamgate_Raw", beamgate_63_0);

    unsigned long BGTruncation = beamgate_63_0 % 8;
    unsigned long BGTruncated = beamgate_63_0 - BGTruncation;
    unsigned long BGInt = BGTruncated / 8 * 25;
    unsigned long BGIntTruncation = BGTruncation * 3;
    // save these two to CStore
    double BGFloat = BGTruncation * 0.125;
    unsigned long BGIntCombined = BGInt + BGIntTruncation;
    m_data->CStore.Set("LAPPDBGIntCombined", BGIntCombined);
    m_data->CStore.Set("LAPPDBGFloat", BGFloat);

    unsigned short timestamp_63_48 = meta.at(70);
    unsigned short timestamp_47_32 = meta.at(50);
    unsigned short timestamp_31_16 = meta.at(30);
    unsigned short timestamp_15_0 = meta.at(10);
    std::bitset<16> bits_timestamp_63_48(timestamp_63_48);
    std::bitset<16> bits_timestamp_47_32(timestamp_47_32);
    std::bitset<16> bits_timestamp_31_16(timestamp_31_16);
    std::bitset<16> bits_timestamp_15_0(timestamp_15_0);
    unsigned long timestamp_63_0 = (static_cast<unsigned long>(timestamp_63_48) << 48) + (static_cast<unsigned long>(timestamp_47_32) << 32) + (static_cast<unsigned long>(timestamp_31_16) << 16) + (static_cast<unsigned long>(timestamp_15_0));
    unsigned long lappd_timestamp = timestamp_63_0 * (CLOCK_to_NSEC);
    m_data->CStore.Set("LAPPDtimestamp", lappd_timestamp);
    m_data->CStore.Set("LAPPDTimestamp_Raw", timestamp_63_0);

    unsigned long TSTruncation = timestamp_63_0 % 8;
    unsigned long TSTruncated = timestamp_63_0 - TSTruncation;
    unsigned long TSInt = TSTruncated / 8 * 25;
    unsigned long TSIntTruncation = TSTruncation * 3;
    // save these two to CStore
    double TSFloat = TSTruncation * 0.125;
    unsigned long TSIntCombined = TSInt + TSIntTruncation;
    m_data->CStore.Set("LAPPDTSIntCombined", TSIntCombined);
    m_data->CStore.Set("LAPPDTSFloat", TSFloat);

    m_data->Stores["ANNIEEvent"]->Set("LAPPDbeamgate", beamgate_timestamp); // in ns
    m_data->Stores["ANNIEEvent"]->Set("LAPPDtimestamp", lappd_timestamp);   // in ns
    m_data->Stores["ANNIEEvent"]->Set("LAPPDBeamgate_Raw", beamgate_63_0);
    m_data->Stores["ANNIEEvent"]->Set("LAPPDTimestamp_Raw", timestamp_63_0);

debugStoreReadIn<<eventNo<<" LAPPDStoreReadIn, Saving timestamps, beamgate_timestamp: "<<beamgate_63_0<<", lappd_timestamp: "<<timestamp_63_0<<endl;

    if (loadOffsets && runInfoLoaded)
    {
        // run number + sub run number + partfile number + LAPPD_ID to make a string key
        // search it in the maps, then load
        SaveOffsets();
    }
    m_data->CStore.Set("LAPPD_new_event", true);
}

void LAPPDLoadStore::SaveOffsets()
{

    std::string key = std::to_string(runNumber) + "_" + std::to_string(subRunNumber) + "_" + std::to_string(partFileNumber) + "_" + std::to_string(LAPPD_ID);

    int LAPPDBGCorrection = 0;
    int LAPPDTSCorrection = 0;
    int LAPPDOffset_minus_ps = 0;
    uint64_t LAPPDOffset = 0;

    // Check if the key exists and the index is within range for BGCorrections
    if (BGCorrections.find(key) != BGCorrections.end() && eventNumberInPF < BGCorrections[key].size())
    {
        LAPPDBGCorrection = BGCorrections[key][eventNumberInPF];
    }
    else
    {
        if (BGCorrections.find(key) == BGCorrections.end())
        {
            std::cerr << "Error: Key not found in BGCorrections: " << key << std::endl;
        }
        else
        {
            std::cerr << "Error: eventNumberInPF out of range for BGCorrections with key: " << key << std::endl;
        }
    }

    // Repeat the checks for TSCorrections, Offsets_minus_ps, and Offsets
    if (TSCorrections.find(key) != TSCorrections.end() && eventNumberInPF < TSCorrections[key].size())
    {
        LAPPDTSCorrection = TSCorrections[key][eventNumberInPF];
    }
    else
    {
        if (TSCorrections.find(key) == TSCorrections.end())
        {
            std::cerr << "Error: Key not found in TSCorrections: " << key << std::endl;
        }
        else
        {
            std::cerr << "Error: eventNumberInPF out of range for TSCorrections with key: " << key << std::endl;
        }
    }

    if (Offsets_minus_ps.find(key) != Offsets_minus_ps.end() && eventNumberInPF < Offsets_minus_ps[key].size())
    {
        LAPPDOffset_minus_ps = Offsets_minus_ps[key][eventNumberInPF];
    }
    else
    {
        if (Offsets_minus_ps.find(key) == Offsets_minus_ps.end())
        {
            std::cerr << "Error: Key not found in Offsets_minus_ps: " << key << std::endl;
        }
        else
        {
            std::cerr << "Error: eventNumberInPF out of range for Offsets_minus_ps with key: " << key << std::endl;
        }
    }

    if (Offsets.find(key) != Offsets.end() && eventNumberInPF < Offsets[key].size())
    {
        LAPPDOffset = Offsets[key][eventNumberInPF];
    }
    else
    {
        if (Offsets.find(key) == Offsets.end())
        {
            std::cerr << "Error: Key not found in Offsets: " << key << std::endl;
        }
        else
        {
            std::cerr << "Error: eventNumberInPF out of range for Offsets with key: " << key << std::endl;
        }
    }

    // start to fill data
    m_data->CStore.Set("LAPPDBGCorrection", LAPPDBGCorrection);
    m_data->CStore.Set("LAPPDTSCorrection", LAPPDTSCorrection);
    m_data->CStore.Set("LAPPDOffset", LAPPDOffset);
    m_data->CStore.Set("LAPPDOffset_minus_ps", LAPPDOffset_minus_ps);

    cout<<"LAPPDStoreReadIn, Saving offsets and corrections, key: "<<key<<", LAPPDOffset: "<<LAPPDOffset<<", LAPPDOffset_minus_ps: "<<LAPPDOffset_minus_ps<<", LAPPDBGCorrection: "<<LAPPDBGCorrection<<", LAPPDTSCorrection: "<<LAPPDTSCorrection<<endl;

    debugStoreReadIn<<eventNo<<"+LAPPDStoreReadIn, Saving offsets and corrections, key: "<<key<<", LAPPDOffset: "<<LAPPDOffset<<", LAPPDOffset_minus_ps: "<<LAPPDOffset_minus_ps<<", LAPPDBGCorrection: "<<LAPPDBGCorrection<<", LAPPDTSCorrection: "<<LAPPDTSCorrection<<endl;
    }

void LAPPDLoadStore::LoadOffsetsAndCorrections()
{
    // load here from the root tree to:
    /*
    std::map<string, vector<uint64_t>> Offsets; //Loaded offset, use string = run number + sub run number + partfile number as key.
    std::map<string, vector<int>> Offsets_minus_ps; //offset in ps, use offset - this/1e3 as the real offset
    std::map<string, vector<int>> BGCorrections; //Loaded BGcorrections, same key as Offsets, but offset saved on event by event basis in that part file, in unit of ticks
    std::map<string, vector<int>> TSCorrections; //TS corrections, in unit of ticks
    */

    TFile *file = new TFile("offsetFitResult.root", "READ");
    TTree *tree;
    file->GetObject("Events", tree);

    if (!tree)
    {
        std::cerr << "LAPPDStoreReadIn Loading offsets, Tree not found!" << std::endl;
        return;
    }

    int runNumber, subRunNumber, partFileNumber, LAPPD_ID;
    ULong64_t final_offset_ns_0, final_offset_ps_negative_0, EventIndex;
    ULong64_t BGCorrection_tick, TSCorrection_tick;
    tree->SetBranchAddress("runNumber", &runNumber);
    tree->SetBranchAddress("subRunNumber", &subRunNumber);
    tree->SetBranchAddress("partFileNumber", &partFileNumber);
    tree->SetBranchAddress("LAPPD_ID", &LAPPD_ID);
    tree->SetBranchAddress("EventIndex", &EventIndex);
    tree->SetBranchAddress("final_offset_ns_0", &final_offset_ns_0);
    tree->SetBranchAddress("final_offset_ps_negative_0", &final_offset_ps_negative_0);
    tree->SetBranchAddress("BGCorrection_tick", &BGCorrection_tick);
    tree->SetBranchAddress("TSCorrection_tick", &TSCorrection_tick);

    Long64_t nentries = tree->GetEntries();
    cout<<"LAPPDStoreReadIn Loading offsets and corrections, total entries: "<<nentries<<endl;
    for (Long64_t i = 0; i < nentries; ++i)
    {
        tree->GetEntry(i);

        std::string key = std::to_string(runNumber) + "_" + std::to_string(subRunNumber) + "_" + std::to_string(partFileNumber) + "_" + std::to_string(LAPPD_ID);

        // Prepare the vector sizes for each map
        if (Offsets[key].size() <= EventIndex)
        {
            Offsets[key].resize(EventIndex + 1);
            Offsets_minus_ps[key].resize(EventIndex + 1);
            BGCorrections[key].resize(EventIndex + 1);
            TSCorrections[key].resize(EventIndex + 1);
        }

        // Now using EventIndex to place each event correctly
        Offsets[key][EventIndex] = final_offset_ns_0;
        Offsets_minus_ps[key][EventIndex] = static_cast<int>(final_offset_ps_negative_0);
        BGCorrections[key][EventIndex] = static_cast<int>(BGCorrection_tick) - 1000;
        TSCorrections[key][EventIndex] = static_cast<int>(TSCorrection_tick) - 1000;

        if(i%(static_cast<int>(nentries/10))==0)
            {cout<<"LAPPDStoreReadIn Loading offsets and corrections, "<<i<<" entries loaded"<<endl;
            cout<<"Printing key: "<<key<<", EventIndex: "<<EventIndex<<", final_offset_ns_0: "<<final_offset_ns_0<<", final_offset_ps_negative_0: "<<final_offset_ps_negative_0<<", BGCorrection_tick: "<<BGCorrection_tick<<", TSCorrection_tick: "<<TSCorrection_tick<<endl;
            }
    }

    file->Close();
    delete file;

    // The data structures are now correctly filled and can be used as needed.
}

void LAPPDLoadStore::LoadRunInfo()
{
    if(LAPPDStoreReadInVerbosity>0)
        cout<<"LAPPDStoreReadIn, Loading run info"<<endl;
    int PFNumberBeforeGet = partFileNumber;
    m_data->CStore.Get("rawFileNumber", partFileNumber);
    m_data->CStore.Get("runNumber", runNumber);
    m_data->CStore.Get("subrunNumber", subRunNumber);

    if (partFileNumber != PFNumberBeforeGet)
    {
        eventNumberInPF = 0;
    }
    else
    {
        eventNumberInPF++;
    }
    if(LAPPDStoreReadInVerbosity>0)
        cout<<"LAPPDStoreReadIn, Loaded run info, runNumber: "<<runNumber<<", subRunNumber: "<<subRunNumber<<", partFileNumber: "<<partFileNumber<<", eventNumberInPF: "<<eventNumberInPF<<endl;
}