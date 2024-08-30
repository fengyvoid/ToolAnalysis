# CNNImage

CNNImage creates ANNIE event display information in matrix format that
can directly be loaded and used by Machine Learning classifier frameworks like CNNs for image/event 
classification purposes. Currently only PMT information is loaded, no information from the LAPPDs is used.

The event display created by this tool is given in a rectangular format (static mapping mode: 16x10, 
geometric mapping mode: user-defined shape), and produces normalized and absolute charge and time 
(time of first hit, and average hit time) maps that are stored in boost stores or may be written 
to csv files.

## Data

CNNImage creates multiple `.csv`-files and one `.root`-file, when `WriteToFile` 
is set to 1 in the config file. Each csv-file contains the event display information in single rows  
per event for a specific event type (PMTs/LAPPDs, charge/time), whereas the root-file provides the 
same information in a 2D histogram format, where each event gets its own histogram.

The PMT data can be written Static mode, with a pre-defined mapping (16x10 Matrix) or Geometric mode 
(every pixel is defined by its geometric location, if a PMT happens to be within the pixel its value 
is added to the pixel value, PMTs can overlap. Top and bottom caps are given by circles and not 
squished into rows). The configuration variables `DimensionX` and `DimensionY` hence only apply when
using the `Geometric` save mode and not for the `Static` save mode. 

LAPPDs are not yet supported in the static mapping. Further, static mapping does not support 
dynamically resizing the output matrix size. 

The `DataMode` configuration specifies whether the average hit time per PMT is calculated as a simple
mean (`Normal`) or weighted by the charge (`Charge`).

## Boost store
The following data are provided within the RecoEvent boost store
```
m_data->Stores.at("RecoEvent")->Set("CNNImageCharge", cnn_image_charge);
m_data->Stores.at("RecoEvent")->Set("CNNImageChargeAbs", cnn_image_charge_abs);
m_data->Stores.at("RecoEvent")->Set("CNNImageTime", cnn_image_time);
m_data->Stores.at("RecoEvent")->Set("CNNImageTimeFirst", cnn_image_time_first);
```
where the images are implemented as
```
std::vector<double> cnn_image_charge(cnn_image_size, 0);
```

## Binning in Static mapping mode
The Static PMT binning is defined in the `CNNImage::StaticPMTBinning` method. The vertical (y) and 
horizontal (x/phi) binning is implemented (in detector coordinates) as
```
const std::vector<double> vertical_bins = {-180.0, -160.0, -125.0, -75.0, -30.0, 10.0, 30, 75.0, 130.0};
std::vector<double> phi_bins;
    for (int i = 0; i < 17; i++) {
    phi_bins.push_back(i * 2 * TMath::Pi() / 16); //16 bins for 16 PMTs next to eachother
}
```
The bottom and top PMTs are split into two rows, where the inner circle of the tank top PMTs correspond
to the first row, the inner circle of the tank bottom PMTs correspond to the last row in the produced
output.

## Prerequisites
The tool needs the following tools to be loaded at some point beforehand:
```
MCParticleProperties
MCRecoEventLoader
ClusterFinder
DigitBuilder
TimeClustering
EventSelector
EnergyExtractor
```

## Configuration

CNNImage uses the following recommended configuration variables:

```
verbosity 0
DataMode Normal             #options: Charge/Time/Normal
SaveMode Static             #options: Static / Geometric
DimensionX 16               #choose something suitable (32/64/...) Relevant only for geometric mode
DimensionY 10               #choose something suitable (32/64/...)
OutputFile your_base_name   #base name of output files, if WriteToFile == 1

DimensionLAPPD 5            #Size of the LAPPD pannal 
IncludeTopBottom 1          #Whether top and bottom should be included
                            #IncludeTopBottom is only relevant for geometric mode
DetectorConf ANNIEp2v7
useLAPPDs 0                 
WriteToFile 0              

```
