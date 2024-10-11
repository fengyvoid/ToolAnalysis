# LAPPDLoadStore

LAPPDLoadStore will load LAPPD PsecData objects from boost store.

## Data

**PedFileNameTXT**
Load the Pedestal file in TXT format, based on LAPPD ID and board index.
The board indexes are reorganized based on LAPPD ID, i.e. LAPPD ID 0 will use board 0, 1, ID 2 will use board 4, 5.
Therefore, the input name should be like ~/Pedestal_0.txt, ~/Pedestal_1.txt, and PedFileNameTXT should be set as ~/Pedestal_



## Configuration

**MultiLAPPDMap** True if load the LAPPD map for multi LAPPD data
**loadPSEC** true if load PsedData
**loadPPS** true if load PPS events
**loadOffsets** true if load the offset fitting result from the fitting scripts

**PsecReceiveMode** true if receive data from raw data store

**DoPedSubtract** true if do pedestal subtraction

