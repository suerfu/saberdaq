# saberdaq

*saberdaq* is a Linux library to be used with *polaris* program. The *saberdaq* module is loaded by the *polaris* executable at runtime and is designed to provide DAQ functionalities related to configuring and reading data from CAEN analog-to-digital converter (ADC) cards and FPGA programmable logic boards and storing them in either raw or HDF5 binary format. 

*saberdaq* was originally developed as the DAQ program of the *SABRE* NaI(Tl) dark matter experiment. Currently, *saberdaq* runs only on Linux operating system and has hardware implementations of *V1720* and *V1495* only.

NOTE: if this program is used in your experiment, please also cite the following work: [Suerfu, B., 2018. Polaris: a general-purpose, modular data acquisition framework. Journal of Instrumentation, 13(12), p.T12004](https://iopscience.iop.org/article/10.1088/1748-0221/13/12/T12004).

# Installation

saberdaq package depends on:
- [polaris](https://github.com/suerfu/polaris)
- [CAENVMElib](https://www.caen.it/download/?filter=CAENVMELib%20Library)
- [H5CPP](https://www.hdfgroup.org/download-hdf5/source-code/)
  - it is recommended to build HDF5 from source, with `-DHDF5_BUILD_CPP_LIB:BOOL=ON` option to enable C++ support

# Format of output HDF5 file

In the output HDF5 file, each ADC board will correspond to one group. For CAEN V1720 ADC board, each board (group) will have a number of enabled channels

## Global Level

* version : [string] current version is 2.0.0
* comment : [string] a user comment string appended for this run, describing run setup, purposes, etc.
* config : [string] polaris configuration file used for this data taking
* timestamp : [uint32] timestamp at the start of DAQ
* timestamp_end : [uint32] timestamp at the end of DAQ
* nb_adc_board : [uint] number of ADC boards enabled

The configuration of each ADC board is stored under groups named "adc_x", where x is board index (0,1,...).

## ADC Board Level

**Board configuration:**
* board_address : [uint32] 32-bit VME address of the ADC board
* board_fw_version : [uint32] ROC firmware and release date of the ADC board encoded in a single 32-bit integer
  * **to be impelemented soon**
* board_index : [uint32] ID/index of this board.
* board_model : [string] fixed to be "CAEN_V1270"
* board_run_mode : [string] how the DAQ is started (REGISTER or FIRST_TRIGGER)
* board_sampling_rate : [float] in Hz, fixed to be 250 MHz
* buffer_code : [uint32] buffer code used to configure memory in the run

**Channel configuration:**
* channel_enable_mask : [uint32] channel enable mask
* channel_nb_enabled : [uint32] number of channels enabled
- channel_index	: [array of uint32] list of ADC channel indices ([0-7]) enabled and recorded
- channel_label : [array of string] name, etc. assigned to the channel, empty if not specified
- channel_DAC : [array of uint32] array of DAC for enabled channels (about 30k for 0V offset, higher value for lower offset)
- channel_trigger_loc_enable : [array of bool] whether local trigger is enabled for this channel
- channel_trigger_fp_loc_out : [array of bool] whether local trigger will cause front-panel trigger out to go active
- channel_threshold : [array of uint32] threshold of local trigger
- channel_tx_threshold : [array of uint32] minimum time across threshold

**Event configuration:**
* nb_events : [uint32] number of events recorded in the run
* nb_samples : [uint32] number of ADC samples in a waveform
* nb_pre_trigger_sample : [uint32] number of ADC samples before trigger
* nb_post_trigger_sample : [uint32] number of ADC samples after trigger

**Trigger configuration:**
* trigger_polarity : [bool using int] true / 1 if trigger is issued over threshold
* trigger_ext_enable : [bool using int] external trigger enabled or not, 0 for false, 1 for true
* trigger_sw_enable : [bool using int] software trigger enabled or not
* trigger_loc_enable : [uint32] local trigger enabled or not
* trigger_fp_ext_out : [bool using int] frontpanel output of external trigger
* trigger_fp_sw_out : [bool using int] frontpanel output of software trigger
* trigger_fp_loc_out : [uint32] frontpanel output of local trigger, with the first 8 bit corresponding to each channel
* trigger_overlap : [bool using int] if true, another trigger will be issued even within an acquisition window
  * **currently this feature is not yet supported (should always be false when running DAQ program)**
  * The first 8 bits correspond to each channel. This information is later duplicated at channel level.
* trigger_coin_level : [uint32] number of minimum channels needed for majority coincidence
* trigger_coin_window : [uint32] majority coincidence window in clock cycles
 
**Front-panel configuration:**
* logic_TTL : [bool using int] true - TTL, false - NIM
* logic_LVDS : [bool using int] if true, trigger status will be reflected at the low-voltage differential signal interface

## Event Level

Actual events are under each ADC board group (/adc_0, /adc_1, ...), and accessed by `event_id`.

* event_xxx : [matrix of uint16] actual waveforms, as a matrix of nb_channel_enabled x nb_sample
  * each event is accessed as **file['/adc_0/event_xxx']** or **file['adc_0']['event_xxx']**
  * each event contains the following attributes
* index : [uint64] event index, which is continuous and starts from 0
* counter : [uint64] event ID/counter obtained from the ADC board, includes missing trigger
* trigger_time_tag : [uint32] time tag when trigger condition is met. It is counted with nb of trigger clock cycles (125 MHz for CAEN V1720).
* timestamp : [uint32] Linux timestamp in second obtained from the PC when the data is read out

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
