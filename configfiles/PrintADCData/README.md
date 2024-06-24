# PrintADCData toolchain

***********************
# Description
**********************

The `PrintADCData` toolchain is used to analyze properties in an ANNIEEvent booststore's RecoADCHits vector (and the Aux information). The tool specifically looks at the raw and calibrated waveforms for Tank PMTs and Aux channels and outputs information including:
* Example raw waveforms for all PMTs channels, and aux channels (BoosterRWM, BoosterRF)
* The number of waveforms collected for each channel
* The number of pulses collected for each channel
* The number of waveforms with at least one pulse for each channel
* 2D histograms of the hit occupancy in y vs, phi space

************************
# Data
************************

Two files are producing after running this toolchain:
* ROOT file with raw waveforms and the 2D hit occupancy histograms
* .txt file with the waveform/pulse info described above

************************
# Configuration of PrintADCData tool
************************
```

UseLEDWaveforms [int]
Specifies whether to show and save full waveforms from the DAQ, or 
the LED waveform windows produced from running 
PhaseIIADCCalibrator with MakeLEDWaveforms set at 1.  
1=Show/save LED waveforms, 
0= Show/save raw waveforms.

verbosity [int]
controls the amount of print output (0- lowest, 5-highest)

OutputFile [string]
Base filename given to both the ROOT and text file that are output from the tool.


SaveWaveforms [bool]
Determines whether or not to save raw waveforms in the output ROOT file.  
1 = yes, 0 = no

WavesWithPulsesOnly [bool]
Controls whether or not to save only waveforms that have a pulse in them.  Whether
or not the wave has a pulse is determined by logic output by the PhaseIIADCHitFinder
tool.

MaxWaveforms [int]
Controls the number of raw waveform examples saved to the output ROOT file.  
Used to keep the ROOT file from exploding under the sheer number of raw 
waveforms saved.

LEDsUsed [string]
Log string saved in the output text file to record what LEDs were used for the 
run being processed.  Will eventually be automated when the DAQ can control 
the LEDs.

LEDSetpoints [string]
Log string saved in the output text file to record what LED setpoints were used
 for the run being processed.  Will eventually be automated when the DAQ 
can control the LEDs.
```
