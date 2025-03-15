# saberdaq

*saberdaq* is a Linux library to be used with *polaris* program. These modules are loaded by the *polaris* executable at runtime and are designed to provide DAQ functionalities related to configuring and reading data from CAEN analog-to-digital converter (ADC) cards and FPGA programmable logic boards and storing them in either raw or HDF5 binary format. *saberdaq* was originally developed as the DAQ program of the *SABRE* NaI(Tl) dark matter experiment. Currently, *saberdaq* only on Linux operating system and have hardware implementation of *V1720* and *V1495*.

NOTE: if this program is used in your experiment, please also cite the following work: [Suerfu, B., 2018. Polaris: a general-purpose, modular data acquisition framework. Journal of Instrumentation, 13(12), p.T12004](https://iopscience.iop.org/article/10.1088/1748-0221/13/12/T12004).

# Installation

saberdaq package depends on:
- [polaris](https://github.com/suerfu/polaris)
- [CAENVMElib](https://www.caen.it/download/?filter=CAENVMELib%20Library)
- [H5CPP](https://www.hdfgroup.org/download-hdf5/source-code/)
  - it is recommended to build HDF5 from source
  - in the building process, make sure to add option `-DHDF5_BUILD_CPP_LIB:BOOL=ON` to enable C++ support

# Format of output HDF5 file

In the output HDF5 file, each ADC board will correspond to one group. For CAEN V1720 ADC board, each board (group) will have a number of enabled channels

## Global Level

* version : [string] current version is - 2.0.0
* comment : [string] comment appended for this run
* config : [string] polaris configuration file used for this data taking
* timestamp : [uint32] timestamp at the start of DAQ
* timestamp_end : [uint32] timestamp at the end of DAQ
* nb_adc_board : [uint] number of ADC boards enabled

The name of ADC board group is "adc_x", where x is board index (0,1,...).

## ADC Board Level

* index : [int] ID/index of this board.
* firmware : [string] firmware of the ADC board
* model : [string] fixed to be "CAEN_V1270"
* sampling_rate : [float] in Hz, fixed to be 250 MHz

* vme_address : [uint32] VME address used on this board.
* run_mode : [string] how the DAQ is started (first trigger controlled or register-controlled).
 
* channel_mask : [uint32] channel enable mask
* nb_enabled_channels	: [uint32] number of enabled channels
* nb_samples : [uint32] number of samples	integer (number of data points in a waveform per channel)
* pre_trigger_sample : [uint32] number of ADC samples before trigger
* post_trigger_sample : [uint32] number of ADC samples after trigger

* trigger_overlap : [bool using int] if true, another trigger will be issued even within an acquisition window
  * **currently this feature is not yet supported (should always be false when running DAQ program)**
* ext_trigger_enable : [bool using int] external trigger enabled or not, 0 for false, 1 for true.
* sw_trigger_enable : [bool using int] software trigger enabled or not.
* loc_trigger_enable : [uint32] local trigger enabled or not
  * The first 8 bits correspond to each channel. This information is later duplicated at channel level.
* coin_level : [uint32] number of minimum channels needed for majority coincidence
* coin_window : [uint32] majority coincidence window in clock cycles
* trigger_polarity : [bool using int] true / 1 if trigger is issued over threshold
 
* fp_ext_trigger_out : [bool using int] frontpanel output of external trigger
* fp_sw_trigger_out : [bool using int] frontpanel output of software trigger
* fp_loc_trigger_out : [uint32] frontpanel output of local trigger
  * Each of the first 8 bit correspond to each channel.

* logic_level_TTL : [bool using int] true - TTL, false - NIM
* LVDS_IO : [bool using int] if true, trigger status will be reflected at the low-voltage differential signal interface

* channel information as arrays:
  - channel_index	: [array of uint32] list of channel indices denoting which ADC channel ([0-7]) data corresponds to
    - e.g., channel 0 and channel 2 are enabled and channel 1 is disabled, it will read [ 0, 2 ]
  - channel_label : [array of string] name, etc. assigned to the channel
  - channel_DAC : [array of uint32] array of DAC for enabled channels
  - channel_loc_trigger_enable : [array of bool] whether local trigger is enabled for this channel or not.
  - channel_threshold : [array of uint32] threshold of local trigger
  - channel_tx_threshold : [array of uint32] minimum time across threshold

### Event Level
Actual events are under each ADC board group (/adc_0, /adc_1, ...), and accessed by `event_id`.

* index : [uint64] event index, which is continuous and starts from 0
* eventID : [uint64] event ID obtained from the ADC board.
* trigger_time_tag : [uint32] time tag when trigger condition is met. It is counted with nb of trigger clock cycles (125 MHz for CAEN V1720).
* timestamp : [uint32] Linux timestamp in second obtained from the PC when the data is read out
* event_xxx : [matrix of uint16] actual waveforms, as a matrix of nb_channel_enabled x nb_sample

## Trigger Board Level

Group logic_trigger (FPGA) contains the following:

* enable : [bool] true if FPGA logical trigger is enabled and used
  * **Note: if this field is false, the following information will NOT be recorded**
* version : [string] 1.0.0
* vme_address : [uint32]
* mode : [string] crystal, veto, liquid_scintillator or calibration

* crystal_gate_length : [uint32]
* veto_gate_length : [uint32]

* dead_time : [uint32]
* veto_time : [uint32]
 
* crystal_retrigger : [uint32]
* veto_retrigger : [uint32]
* veto_mask : [uint32]
* veto_majority_level : [uint32]

* trigger_delay : [uint32]
* trigger_duration : [uint32]

