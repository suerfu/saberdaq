#!/bin/env python3

"""
==============================================================================
Script Name:    saberdaq-h5proc.py
Description:    Processes HDF5 output from saberdaq HDF5Writer module.
                Runs pulse identifications to extract reduced quantities.

Author:         Suerfu
Created on:     2025-05-17
Last Modified:  2025-06-29

Usage:
    saberdaq-h5proc.py [txt file containing a list of files]

Note:
    Each txt file is considered one configuration and assigned a configuration number for each event
    Each entry will also be assigned a run number based on how the file is named or the order in the txt file
==============================================================================
"""

import sys
import numpy as np
import argparse
import os
import re

from pyradet.h5util import h5reader as h5reader
from pyradet.signal import pulse as ps

'''
# Turn all warnings into errors
# only for debugging
import warnings
warnings.simplefilter("error", RuntimeWarning)

# Trace printing:
# only for debugging
import builtins
import inspect

_original_print = print

def traced_print(*args, **kwargs):
    frame = inspect.currentframe().f_back
    info = inspect.getframeinfo(frame)
    _original_print(f"[{info.filename}:{info.lineno}]", *args, **kwargs)

builtins.print = traced_print
'''

counter = 0


def extract_config_number(filename):
    """
    Returns geometry configuration ID and run ID based on the filename.
    """
    filename = filename.split('/')[-1]  # Just the file name
    filename_lower = filename.lower()

    calibration_offset = 100  # Calibration ID offset

    # Geometry configuration mapping
    config_map = {'int': 0, 'bottom': 1, 'east': 2, 'west': 3, 'south': 4, 'north': 5, 'top': 6}

    configID = None
    runID = None

    # Determine configID (even if background or calibration)
    for key, val in config_map.items():
        if key in filename_lower:
            configID = val
            break

    # Background case
    if "background" in filename_lower:
        run_match = re.search(r'run(\d{1,4})', filename_lower)
        if run_match:
            runID = run_match.group(1)
        return configID, runID

    # Calibration case
    elif "calibration" in filename_lower:
        # Match isotope like "Co60", "Cs137"
        isotope_match = re.search(r'[A-Z][a-z]?\d{2,3}', filename)
        if isotope_match:
            mass_number_match = re.search(r'\d{2,3}', isotope_match.group(0))
            if mass_number_match:
                runID = mass_number_match.group(0)
        if configID is not None:
            return int(configID) + calibration_offset, runID
        else:
            print("Could not determine configID from filename:", filename)
            return None, runID

    else:
        print("Unable to identify Config or Run ID from filename:", filename)
        return configID, runID


def main():

    from array import array

    ########################
    # parse input arguments
    ########################
    #
    parser = argparse.ArgumentParser( description="Process saberdaq HDF5 files and write to ROOT output" )
    parser.add_argument( '-f', '--force', action='store_true', help='overwrite existing output file' )
    parser.add_argument( '-c', '--channels', nargs='+', type=int, default=[0], help='INDICES (not hdwr ID) of channels to be processed' )
    parser.add_argument( '-i', '--input',  type=str, nargs='*', required=True, help='/path/to/input.hdf5' )
    parser.add_argument( '-o', '--output', type=str, required=True, help='/path/to/output.root' )

    args = parser.parse_args()

    ################################################
    # check to see if ROOT can be imported properly
    ################################################
    #
    try:
        import ROOT
    except ImportError:
        print( "Failed to import ROOT module, which is required by this module.")
        exit(-2)


    ############################################################
    # define parameters needed in the execution of the program.
    ############################################################
    #
    window = 750
        # integration window length in number of samples

    recede = 10
        # number of samples to subtract from pre-trigger-samples in computing baseline
    
    min_length = 50
        # minimum number of samples for secondary pulse
    
    overshoot_threshold = 10
        # maximum amount of overshoot a valid event can have

    famr_threshold = 0.75
        # FAMR threshold for identifying pulse as noise


    #########################
    # Create output ROOT file
    #########################
    #
    # check overwrite options, etc.

    outputname = args.output

    if args.force == True:
        output_file = ROOT.TFile( outputname, "RECREATE")
    else:
        if os.path.exists( outputname ):
            print("Output file", outputname, "already exists. Exitting...")
            exit(-3)
        output_file = ROOT.TFile( outputname, "NEW")


    #############################################################
    # Create output ROOT TTree and define output data structures
    #############################################################
    #
    macro = ROOT.TMacro("inputs")
    tree = ROOT.TTree( "events", "Tree storing processed quantities from HDF5 raw waveforms")
    
    n_channels = len(args.channels)


    ###########################################
    # global - common across multiple channels
    #
    configID  = array( 'I', [0])    # geometrical configuration
    runID     = array( 'I', [0])    # run ID, in the case of calibration, it is the mass number of source
    eventID   = array( 'I', [0])    # event index
    timestamp = array( 'L', [0])    # unsigned long from Unix OS
    ttt       = array( 'I', [0])    # time tag from hardware
    ettt      = array( 'I', [0])    # extended time tag from hardware


    ####################################
    # Channel-wise Baseline information
    #
    # Baseline information
    #
    blFlag  = array( 'i', n_channels * [0])
        # encodes if baseline finder is successful or not and if noise-like or not

    # Head
    blLen1  = array( 'f', n_channels * [0.0])
    blMean1 = array( 'f', n_channels * [0.0])
    blStd1  = array( 'f', n_channels * [0.0])
        # only qualifying points

    preTrigMean   = array( 'f', n_channels * [0.0])
    preTrigStd    = array( 'f', n_channels * [0.0])
        # all points

    # Tail
    blLen2  = array( 'f', n_channels * [0.0])
    blMean2 = array( 'f', n_channels * [0.0])
    blStd2  = array( 'f', n_channels * [0.0])
        # only qualifying points

    postTrigMean  = array( 'f', n_channels * [0.0])
    postTrigStd   = array( 'f', n_channels * [0.0])
        # all points
   

    # Raw waveform information (before baseline subtraction)
    #
    rawFAMR   = array( 'f', n_channels * [0.0])
        # fraction above mid-range
    rawMax    = array( 'i', n_channels * [0])
    rawMaxLoc = array( 'i', n_channels * [0])
        # value and location of maximum
    rawMin    = array( 'i', n_channels * [0])
    rawMinLoc = array( 'i', n_channels * [0])


    # Waveform features dependent on baseline subtraction
    #
    blIntercept = array( 'f', n_channels * [0.0])
    blSlope     = array( 'f', n_channels * [0.0])
    
    wfmMax    = array( 'f', n_channels * [0.0])
    wfmMaxLoc = array( 'f', n_channels * [0.0])
    wfmMin    = array( 'f', n_channels * [0.0])
    wfmMinLoc = array( 'f', n_channels * [0.0])

    wfmIntegral = array( 'f', n_channels * [0.0])
    wfmAWMT     = array( 'f', n_channels * [0.0])
        # of entire waveform


    # Pulse-level Information
    #
    # Primary Pulse
    #
    ppulseFlag      = array( 'i', n_channels * [0])
        # whether pulse starts from and returns to baseline
    ppulseHeight    = array( 'f', n_channels * [0.0])
    ppulseIntegral  = array( 'f', n_channels * [0.0])
    ppulseLen       = array( 'i', n_channels * [0])
    ppulseBeginTime = array( 'i', n_channels * [0])
        # location of begin of the pulse
    ppulseRiseTime  = array( 'f', n_channels * [0.0])
    ppulsePeakTime  = array( 'i', n_channels * [0])
    ppulseFWHM      = array( 'f', n_channels * [0.0])
    ppulseAWMT      = array( 'f', n_channels * [0.0])

    # Secondary Pulse
    #
    spulseFlag      = array( 'i', n_channels * [0])
    spulseHeight    = array( 'f', n_channels * [0.0])
    spulseIntegral  = array( 'f', n_channels * [0.0])
    spulseLen       = array( 'i', n_channels * [0])
    spulseBeginTime = array( 'i', n_channels * [0])
    spulseRiseTime  = array( 'f', n_channels * [0.0])
    spulsePeakTime  = array( 'i', n_channels * [0])
    spulseFWHM      = array( 'f', n_channels * [0.0])
    spulseAWMT      = array( 'f', n_channels * [0.0])
    
    # Branches
    #
    tree.Branch( "configID",  configID,  "configID/s")
    tree.Branch( "runID",     runID,     "runID/s")
    tree.Branch( "eventID",   eventID,   "eventID/i")
    tree.Branch( "timestamp", timestamp, "timestamp/l")
    tree.Branch( "ttt",       ttt,       "ttt/i")
    tree.Branch( "ettt",      ettt,      "ettt/i")

    # Add branches
    tree.Branch( "blFlag",  blFlag, f"blFlag[{n_channels}]/I")
    tree.Branch( "blLen0",  blLen1, f"blLen0[{n_channels}]/F")
    tree.Branch( "blMean0", blMean1, f"blMean0[{n_channels}]/F")
    tree.Branch( "blStd0",  blStd1, f"blStd0[{n_channels}]/F")

    tree.Branch( "blLen1",  blLen2, f"blLen1[{n_channels}]/F")
    tree.Branch( "blMean1", blMean2, f"blMean1[{n_channels}]/F")
    tree.Branch( "blStd1",  blStd2, f"blStd1[{n_channels}]/F")

    tree.Branch( "preTrigMean", preTrigMean, f"preTrigMean[{n_channels}]/F")
    tree.Branch( "preTrigStd",  preTrigStd,  f"preTrigStd[{n_channels}]/F")

    tree.Branch( "postTrigMean", postTrigMean, f"postTrigMean[{n_channels}]/F")
    tree.Branch( "postTrigStd",  postTrigStd,  f"postTrigStd[{n_channels}]/F")

    tree.Branch( "rawFAMR",   rawFAMR,   f"rawFAMR[{n_channels}]/F")
    tree.Branch( "rawMax",    rawMax,    f"rawMax[{n_channels}]/I")
    tree.Branch( "rawMaxLoc", rawMaxLoc, f"rawMaxLoc[{n_channels}]/I")
    tree.Branch( "rawMin",    rawMin,    f"rawMin[{n_channels}]/I")
    tree.Branch( "rawMinLoc", rawMinLoc, f"rawMinLoc[{n_channels}]/I")

    tree.Branch( "blIntercept", blIntercept, f"blIntercept[{n_channels}]/F")
    tree.Branch( "blSlope",     blSlope,     f"blSlope[{n_channels}]/F")

    tree.Branch( "wfmMax",    wfmMax,    f"wfmMax[{n_channels}]/F")
    tree.Branch( "wfmMaxLoc", wfmMaxLoc, f"wfmMaxLoc[{n_channels}]/F")
    tree.Branch( "wfmIntegral", wfmIntegral, f"wfmIntegral[{n_channels}]/F")
    tree.Branch( "wfmAWMT",     wfmAWMT,     f"wfmAWMT[{n_channels}]/F")

    tree.Branch( "ppFlag",      ppulseFlag,      f"ppFlag[{n_channels}]/I")
    tree.Branch( "ppLen",       ppulseLen,       f"ppLen[{n_channels}]/I")
    tree.Branch( "ppHeight",    ppulseHeight,    f"ppHeight[{n_channels}]/F")
    tree.Branch( "ppIntegral",  ppulseIntegral,  f"ppIntegral[{n_channels}]/F")
    tree.Branch( "ppBeginTime", ppulseBeginTime, f"ppBeginTime[{n_channels}]/I")
    tree.Branch( "ppRiseTime",  ppulseRiseTime,  f"ppRiseTime[{n_channels}]/F")
    tree.Branch( "ppPeakTime",  ppulsePeakTime,  f"ppPeakTime[{n_channels}]/I")
    tree.Branch( "ppFWHM",      ppulseFWHM,      f"ppFWHM[{n_channels}]/F")
    tree.Branch( "ppAWMT",      ppulseAWMT,      f"ppAWMT[{n_channels}]/F")

    tree.Branch( "spFlag",      spulseFlag,      f"spFlag[{n_channels}]/I")
    tree.Branch( "spLen",       spulseLen,       f"spLen[{n_channels}]/I")
    tree.Branch( "spHeight",    spulseHeight,    f"spHeight[{n_channels}]/F")
    tree.Branch( "spIntegral",  spulseIntegral,  f"spIntegral[{n_channels}]/F")
    tree.Branch( "spBeginTime", spulseBeginTime, f"spBeginTime[{n_channels}]/I")
    tree.Branch( "spRiseTime",  spulseRiseTime,  f"spRiseTime[{n_channels}]/F")
    tree.Branch( "spPeakTime",  spulsePeakTime,  f"spPeakTime[{n_channels}]/I")
    tree.Branch( "spFWHM",      spulseFWHM,      f"spFWHM[{n_channels}]/F")
    tree.Branch( "spAWMT",      spulseAWMT,      f"spAWMT[{n_channels}]/F")


    ######################################################
    # organize input files in the correct time order
    #
    file_tuple = []

    for filename in args.input:
        reader = h5reader.DataReader( filename )
        reader.Open()
        timestamp_begin = reader.GetAttributes()['timestamp']
        timestamp_end = reader.GetAttributes()['timestamp_end']
        file_tuple.append( (filename, timestamp_begin, timestamp_end) )
        reader.Close()

    file_list = [f[0] for f in sorted(file_tuple, key=lambda x: x[1])]


    ######################################################
    # iterate over the files contained in the input lists
    #
    for filename in file_list:

        fn = filename

        # Get config and run number based on filename
        a, b = extract_config_number( filename )

        if a is None or b is None:
            print( f"\tConfigID/RunID not identified in {filename}. Skipping..." )
            continue

        configID[0] = int(a)
        runID[0] = int(b)
            # extract_config_number may return None, so check None first
            # the return type is str, cast to int

        print( f"\tProcessing {filename} as config {configID[0]} and run {runID[0]}" )


        # Create the reader object and get basic settings of the data taking
        #
        reader = h5reader.DataReader( filename )
        reader.Open()

        timestamp_begin = reader.GetAttributes()['timestamp']
        timestamp_end = reader.GetAttributes()['timestamp_end']
        macro.AddLine( "{0} {1} {2} {3} {4}".format( filename.split('/')[-1], timestamp_begin, timestamp_end, configID[0], runID[0]) )

        adc_attr           = reader.GetADCAttributes()
        pre_trigger_sample = int( adc_attr['nb_pre_trigger_sample'] )
        nb_samples         = adc_attr['nb_samples']
        nb_events          = adc_attr['nb_events']

        xdata = np.arange(0, nb_samples)
            # x-axis index, used in computing baseline slope

        global counter
        counter = 0
            # used to accumulate event counter as event ID

        ######################
        # Iterate over events
        ######################

        for event_data, event_attr in reader.GetEventIterator():

            eventID[0]   = counter
            timestamp[0] = event_attr['timestamp']
            ttt[0]       = event_attr['trigger_time_tag']
            ettt[0]      = event_attr['option']
                # later may think of a way to combine these two

            
            ##################################
            # iterate over different channels
            ##################################

            for ch in args.channels:

                # get unprocessed raw waveform and compute some basic quantities.
                #
                raw_wfm = event_data[ch]

                rawFAMR[ch]   = ps.GetFAMR( raw_wfm )
                rawMax[ch]    = np.max( raw_wfm )
                rawMin[ch]    = np.min( raw_wfm )
                rawMaxLoc[ch] = np.argmax( raw_wfm )
                rawMinLoc[ch] = np.argmin( raw_wfm )

                preTrigMean[ch]  = np.mean( raw_wfm[:pre_trigger_sample-recede] )
                postTrigMean[ch] = np.mean( raw_wfm[-(pre_trigger_sample-recede):])
                preTrigStd[ch]   = np.std(  raw_wfm[:pre_trigger_sample-recede] )
                postTrigStd[ch]  = np.std(  raw_wfm[-(pre_trigger_sample-recede):])

                
                # Perform baseline finding
                #
                flag, proc_wfm, mask0, mask1, coeff = ps.BaselineFinder( raw_wfm, pre_trigger_sample )


                # Set flag accordingly
                blFlag[ch] = int(flag.value)

                ######################################################################
                # if at least one baseline is found, proceed to perform pulse-finding
                ######################################################################
                #
                if flag != ps.BaselineFlag.NOT_FOUND:
                
                    # invert the processed waveform to positive polarity
                    # only do this when baseline is reliably found
                    #
                    proc_wfm *= -1

                    # mask can be obtained in either case
                    #
                    #mask = np.zeros( len(raw_wfm), dtype=bool )

                    mask = mask0+mask1

                    if flag == ps.BaselineFlag.GOOD:

                        baseline = np.polyval(coeff,xdata)

                        blLen1[ch]  = len( raw_wfm[mask0] )
                        blMean1[ch] = np.mean( raw_wfm[mask0] )
                        blStd1[ch]  = np.std(  raw_wfm[mask0] )

                        blLen2[ch]  = len( raw_wfm[mask1] )
                        blMean2[ch] = np.mean( raw_wfm[mask1] )
                        blStd2[ch]  = np.std(  raw_wfm[mask1] )

                        blIntercept[ch] = coeff[1]
                        blSlope[ch] = coeff[0]

                        wfmMax[ch] = np.max( proc_wfm )
                        wfmMaxLoc[ch] = np.argmax( proc_wfm )
                        wfmIntegral[ch] = np.trapezoid( proc_wfm[pre_trigger_sample-recede:] )
                        wfmAWMT[ch] = ps.GetAWMT( proc_wfm[pre_trigger_sample-recede:] )

                    else:

                        baseline = 0*xdata + coeff[0]

                        blLen1[ch]  = len( raw_wfm[mask0] )     #if mask0 is not None else 0
                        if blLen1[ch] > 0:
                            blMean1[ch] = np.mean( raw_wfm[mask0] ) #if mask0 is not None else 0
                            blStd1[ch]  = np.std(  raw_wfm[mask0] ) #if mask0 is not None else 0

                        blLen2[ch]  = len( raw_wfm[mask1] )     #if mask1 is not None else 0
                        if blLen2[ch] > 0:
                            blMean2[ch] = np.mean( raw_wfm[mask1] ) #if mask1 is not None else 0
                            blStd2[ch]  = np.std(  raw_wfm[mask1] ) #if mask1 is not None else 0

                        blIntercept[ch] = coeff[0]
                        blSlope[ch] = 0
                        
                    mean = np.mean( proc_wfm[mask])
                    dev  = np.std( proc_wfm[mask])
            
                    # even if both baselines have been found, check for noise
                    #
                    if np.min(proc_wfm) > -overshoot_threshold and rawFAMR[ch] > famr_threshold:
                
                        # find primary pulse
                        pflag, beg, end = ps.PulseFinder( proc_wfm, threshold=dev, std=dev, beg=0, end=2*pre_trigger_sample)
                        ppulseFlag[ch] = int(pflag.value)
                        end = max(end,beg+window)

                        if pflag != ps.PulseFlag.NOT_FOUND:
                            end += 1
                            ppulseLen[ch] = end - beg
                            ppulseBeginTime[ch] = beg
                            ppulseHeight[ch] = np.max(proc_wfm[beg:end])
                            ppulseIntegral[ch] = np.trapezoid( proc_wfm[beg:end] )
                            ppulsePeakTime[ch] = np.argmax( proc_wfm[beg:end] )+beg
                            #if pflag != ps.PulseFlag.NO_HEAD:
                            #    ppulseRiseTime[ch], _, _ = ps.GetRiseTime( proc_wfm[beg:end] )
                            #if pflag == ps.PulseFlag.GOOD:
                            #    ppulseFWHM[ch], _, _ = ps.GetFWHM( proc_wfm[beg-recede:end] )
                                    # FWHM search has to be moved back a little, otherwise will end incomplete pulse
                                    # this is because the trigger point may be after 1/2 of pulse amplitude
                                    # do this only when the pulse is good
                            ppulseAWMT[ch] = ps.GetAWMT( proc_wfm[beg:end] )

                        # run pulse finding in the auxiliary region
                        #
                        if pflag != ps.PulseFlag.NO_TAIL and end<len(proc_wfm)-1:
                            
                            sflag, sbeg, send = ps.PulseFinder( proc_wfm, threshold=5*dev, std=dev, beg=end, end=-1 )
                            spulseFlag[ch] = int(sflag.value)
                
                            if (sflag != ps.PulseFlag.NOT_FOUND) and send-sbeg>min_length:
                                send += 1
                                spulseLen[ch] = send - sbeg
                                spulseBeginTime[ch] = sbeg
                                spulseHeight[ch] = np.max(proc_wfm[sbeg:send])
                                spulseIntegral[ch] = np.trapezoid( proc_wfm[sbeg:send] )
                                spulsePeakTime[ch] = np.argmax( proc_wfm[sbeg:send] )
                                #if sflag != ps.PulseFlag.NO_HEAD:
                                #    spulseRiseTime[ch], _, _ = ps.GetRiseTime( proc_wfm[sbeg:send] )
                                #if sflag == ps.PulseFlag.GOOD:
                                #    spulseFWHM[ch], _, _ = ps.GetFWHM( proc_wfm[sbeg:send] )
                                        # compute FWHM only when the pulse is complete
                                spulseAWMT[ch] = ps.GetAWMT( proc_wfm[sbeg:send] )
                        
                    # At this point, it means, the pulse has failed noise test. Set flags accordingly
                    #
                    else:
                        if np.min(proc_wfm) > -overshoot_threshold:
                            blFlag[ch] += (1<<3)
                        if rawFAMR[ch] > famr_threshold:
                            blFlag[ch] += (1<<2)

            tree.Fill()
            
            counter += 1

            # set branch variables to default values for the next round
            # blflag will be set to proper value in each new iteration

            for ch in args.channels:

                blLen1[ch]  = 0
                blMean1[ch] = 0
                blStd1[ch]  = 0

                blLen2[ch]  = 0
                blMean2[ch] = 0
                blStd2[ch]  = 0
        
                blIntercept[ch] = 0
                blSlope[ch]     = 0
                
                wfmMax[ch]    = 0
                wfmMaxLoc[ch] = 0
                wfmMin[ch]    = 0
                wfmMinLoc[ch] = 0

                wfmIntegral[ch] = 0
                wfmAWMT[ch]     = 0

                ppulseFlag[ch]      = 0xff
                ppulseHeight[ch]    = 0
                ppulseIntegral[ch]  = 0
                ppulseLen[ch]       = 0
                ppulseBeginTime[ch] = 0
                ppulseRiseTime[ch]  = 0
                ppulsePeakTime[ch]  = 0
                ppulseFWHM[ch]      = 0
                ppulseAWMT[ch]      = 0

                spulseFlag[ch]      = 0xff
                spulseHeight[ch]    = 0
                spulseIntegral[ch]  = 0
                spulseLen[ch]       = 0
                spulseBeginTime[ch] = 0
                spulseRiseTime[ch]  = 0
                spulsePeakTime[ch]  = 0
                spulseFWHM[ch]      = 0
                spulseAWMT[ch]      = 0

        reader.Close()

    macro.Write()
    output_file.Write()
    output_file.Close()

if __name__=="__main__":
    #try:
    main()
    #except Exception as e:
    #    print( "Exception:", e, "at event", counter)
