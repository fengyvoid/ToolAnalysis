# EventBuilderRaw

***********************
## Description
**********************

The `EventBuilderRaw` toolchain can be used to produce Processed files that contain the raw waveforms (PMTs) and store them to the ANNIEEvent booststore. This toolchain is essentially the same as the `EventBuilder` toolchain, except that raw waveforms are saved, not just the hits.

This toolchain only builts Tank + CTC events (no MRD). Hit finding tools are typically ommitted from the toolchain but can be including to also save the hits information in parallel with the raw waveforms.

Warning to users: When using this toolchain, enormous amounts of disk space will be used as all ~121 active PMT channels (and aux channels + CTC information) will be saved for every trigger/event. Recommended to run a part file at a time. The output Processed files will be at least x10 larger than their hits only counterparts.

************************
## Tools
************************

The toolchain typically consists of the following tools:

```
LoadGeometry
LoadRawData
PMTDataDecoder
TriggerDataDecoder
ANNIEEventBuilder
```
