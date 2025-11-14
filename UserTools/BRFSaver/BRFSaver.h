#ifndef BRFSaver_H
#define BRFSaver_H

#include <string>
#include <iostream>

#include "Tool.h"

/**
 * \class BRFSaver
 *
 * Extract the BRF waveform from processed data, save the first 100 bins as a vector into a root tree
 * Also save the bin number and value of minimums and amplitudes
 *
 */
class BRFSaver : public Tool
{

public:
    BRFSaver();                                               ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.

private:
    int verbosityBRFSaver;
    int v_message = 1;
    int v_warning = 2;
    int v_error = 3;
    int v_debug = 4;

    std::vector<uint16_t> BRFRawWaveform;

    int SaveRange;
    std::string output_filename;


    std::vector<int> runNumbers;
    std::vector<int> partFileNumbers;
    std::vector<std::vector<uint16_t>> BRFWaveformsToSave;
    std::vector<std::vector<double>> BRFMinimaToSave;
    std::vector<std::vector<double>> BRFMinimaAmplitudesToSave;
    std::vector<std::vector<double>> BRFMinimaTimesToSave;
    std::vector<std::vector<double>> BRFMaximaAmplitudesToSave;
    std::vector<std::vector<double>> BRFMaximaToSave;
    std::vector<std::vector<double>> BRFMaximaTimesToSave;
};

#endif
