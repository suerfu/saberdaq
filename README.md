# saberdaq

## Format of output HDF5 file

In the output HDF5 file, each ADC board will correspond to one group. For CAEN V1720 ADC board, each board (group) will have a number of enabled channels

At the global level, it contains:
* version : [string] currently it is first version - 1.0.0
* comment : [string] comment appended for this run
* config : [string] polaris configuration file used for this data taking
* timestamp : [uint32] timestamp at the start of DAQ
* timestamp_end : [uint32] timestamp at the end of DAQ
* nb_adc_board : [uint] number of ADC boards enabled

The name of ADC board group is "adcX", where X is board index (0,1,...). At the ADC group level, it contains:

* index : [int] ID/index of this board.
* version : [string] currently it is 1.0.0
* model : [string] fixed to be "CAEN V1270"
* sample_rate : [float] in Hz, fixed to be 250 MHz

* vme_address : [uint32] VME address used on this board.
* run_mode : [string] how the DAQ is started (first trigger controlled or register-controlled).
 
* channel_mask : [uint32] channel enable mask
* nb_channels	: [int] Number of channels	integer
* nb_samples : [int] Number of samples	integer
* pre_trigger_sample : [int] pre-trigger window in microsecond.
* post_trigger_sample : [int] post-trigger window in microsecond.

* trigger_overlap : [bool using int] if true, another trigger will be issued even within an acquisition window
* ext_trigger_enable : [bool using int] external trigger enabled or not, 0 for false, 1 for true.
* sw_trigger_enable : [bool using int] software trigger enabled or not.
* local_trig_enable : [uint32] local trigger enabled or not. Each of the first 8 bit correspond to each channel. This information is later duplicated at channel level.
* coin_level : [int] number of minimum channels needed for majority coincidence.
* coin_window : [int] majority coincidence window in clock cycles.
* trigger_polarity : [bool using int] true / 1 if trigger is issued over threshold
 
* frontpanel_ext_trigger_out : [bool using int] frontpanel output of external trigger enabled or not.
* frontpanel_sw_trigger_out : [bool using int] frontpanel output of software trigger enabled or not.
* frontpanel_local_trigger_out : [uint32] frontpanel output of local trigger enabled or not. Each of the first 8 bit correspond to each channel.

* logic_level_TTL : [bool using int] if true, then TTL logic level will be used instead of NIM.
* LVDS_IO : [bool using int] if true, trigger status will be reflected at the low-voltage differential signal interface.

* channel information as arrays:
  - channel_indices	: [array of int] List of channel indices
  - channel_label : [array of string] name, etc. assigned to the channel
  - channel_DAC : [array of uint32] array of DAC for enabled channels
  - channel_local_trigger_enable : [array of bool] whether local trigger is enabled for this channel or not.
  - channel_threshold : [array of uint32] threshold of local trigger
  - channel_tcross_threshold : [array of uint32] minimum time across threshold

* Actual event, under each ADC board group (/adc0, /adc1, ...):
  * nb_events : [uint64] number of events.
  * event_xxx : [matrix of uint16] actual waveforms, as a matrix of nb_channel_enabled x nb_sample
  * trigger_time_tag : [uint32] time tag when trigger condition is met. It is counted with nb of trigger clock cycles (125 MHz for CAEN V1720).
  * index : [uint64] event index, which is continuous and starts from 0
  * eventID : [uint64] event ID obtained from the ADC board.



Group logic_trigger (FPGA) contains: 
* version : [string] 1.0.0
* vme_address : [uint32]
* mode : [string] crystal, veto, liquid_scintillator or calibration

* crystal_gate_length : [int]
* veto_gate_length : [int]

* dead_time : [int]
* veto_time : [int]
 
* crystal_retrigger : [bool]
* veto_retrigger : [bool]
* veto_mask : [int]
* veto_majority_level : [int]

* trigger_delay : [int]
* trigger_duration : [int]

