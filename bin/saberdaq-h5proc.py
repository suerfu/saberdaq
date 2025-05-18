#!/bin/env python3

"""
==============================================================================
Script Name:    saberdaq-h5proc.py
Description:    Processes HDF5 output from saberdaq HDF5Writer module.
                Runs pulse identifications to extract reduced quantities.

Author:         Suerfu
Created on:     2025-05-17
Last Modified:  2025-05-17

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

from pyradet.h5util import h5reader as h5reader
from pyradet.signal import pulse as ps

def get_file_list( txt_file ):
    """
        Reads a txt file containing a list of files to be processed.
        Returns a list of HDF5 files to be processed.
    """
    file_list = []
    with open(txt_file, 'r') as f:
        for line in f:
            path = line.strip()
            if not path or path.startswith('#'):
                print( f"Failed to find file {abs_path}" )
                continue
            file_list.append(path)
    return file_list


def main():

    # check to see if ROOT can be imported properly
    #
    try:
        import ROOT
    except ImportError:
        print( "Failed to import ROOT module, which is required by this module.")
        exit(-2)

    from array import array

    # parse input arguments
    #
    parser = argparse.ArgumentParser( description="Process saberdaq HDF5 files specified in txt files as config:txt-file" )
    parser.add_argument( '-f', '--files', metavar='FILE', nargs='+', required=True, help='config:/path/to/input.txt' )
    parser.add_argument( '--calibration', metavar='CALIBRATION', nargs='+', help='config:/path/to/calibration.txt' )
    parser.add_argument( '-c', '--channels', metavar='CHANNELS', nargs='+', type=int, default=[0], help='channels to be processed' )
    parser.add_argument( '-o', '--output', metavar='OUTPUT', type=str, required=True, help='Path to output file' )
    args = parser.parse_args()

    # define parameters needed in the execution of the program.
    #
    window = 750
        # integration window length in number of samples

    recede = 10
    
    min_length = 50
    
    ch = args.channels[0]

    # Perform ROOT-related operations
    #

    # Create output file and tree
    #
    output_file = ROOT.TFile( outputname, "RECREATE")
    tree = ROOT.TTree( "events", "Tree storing processed quantities from HDF5 raw waveforms")

    n_channels = len(args.channels)

    # Define data structures to be written

    # global - common across multiple channels
    #
    configID  = array( 'i', [0])
    runID     = array( 'i', [0])
    eventID   = array( 'i', [0])
    timestamp = array( 'l', [0])  # unsigned long
    ttt       = array( 'i', [0])
    ettt      = array( 'i', [0])

    # Channel-wise Baseline information
    #
    # Baseline information
    #
    blFlag  = array( 'i', n_channels * [0.0])
    blLen1  = array( 'f', n_channels * [0.0])
    blMean1 = array( 'f', n_channels * [0.0])
    blStd1  = array( 'f', n_channels * [0.0])

    blLen2  = array( 'f', n_channels * [0.0])
    blMean2 = array( 'f', n_channels * [0.0])
    blStd2  = array( 'f', n_channels * [0.0])


    preTrigMean   = array( 'f', n_channels * [0.0])
    preTrigStd    = array( 'f', n_channels * [0.0])

    postTrigMean  = array( 'f', n_channels * [0.0])
    postTrigStd   = array( 'f', n_channels * [0.0])
    
    # Raw waveform information
    #
    rawFAMR   = array( 'f', n_channels * [0.0])
    rawMax    = array( 'i', n_channels * [0.0])
    rawMaxLoc = array( 'i', n_channels * [0.0])
    rawMin    = array( 'i', n_channels * [0.0])
    rawMinLoc = array( 'i', n_channels * [0.0])

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

    # Pulse-level Information
    #
    pulseFlag      = array( 'i', n_channels * [0.0])
    pulseID        = array( 'i', n_channels * [0.0])
    pulseHeight    = array( 'f', n_channels * [0.0])
    pulseIntegral  = array( 'f', n_channels * [0.0])
    pulseLen       = array( 'i', n_channels * [0.0])
    pulseBeginTime = array( 'i', n_channels * [0.0])
    pulseRiseTime  = array( 'f', n_channels * [0.0])
    pulsePeakTime  = array( 'i', n_channels * [0.0])
    pulseFWHM      = array( 'f', n_channels * [0.0])
    pulseAWMT      = array( 'f', n_channels * [0.0])
    
    # Branches
    #
    tree.Branch( "configID",  configID,  "configID/I")
    tree.Branch( "runID",     runID,     "runID/I")
    tree.Branch( "eventID",   eventID,   "eventID/I")
    tree.Branch( "timestamp", timestamp, "timestamp/L")
    tree.Branch( "ttt",       ttt,       "ttt/I")
    tree.Branch( "ettt",      ettt,      "ettt/I")

    # Add branches
    tree.Branch( "blFlag",  blFlag, f"blFlag[{n_channels}]/I")
    tree.Branch( "blLen1",  blLen1, f"blLen1[{n_channels}]/F")
    tree.Branch( "blMean1", blMean1, f"blMean1[{n_channels}]/F")
    tree.Branch( "blStd1",  blStd1, f"blStd1[{n_channels}]/F")

    tree.Branch( "blLen2",  blLen2, f"blLen2[{n_channels}]/F")
    tree.Branch( "blMean2", blMean2, f"blMean2[{n_channels}]/F")
    tree.Branch( "blStd2",  blStd2, f"blStd2[{n_channels}]/F")

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

    tree.Branch( "pulseFlag",      pulseFlag,      f"pulseFlag[{n_channels}]/I")
    tree.Branch( "pulseID",        pulseID,        f"pulseID[{n_channels}]/I")
    tree.Branch( "pulseLen",       pulseLen,       f"pulseLen[{n_channels}]/I")
    tree.Branch( "pulseHeight",    pulseHeight,    f"pulseHeight[{n_channels}]/F")
    tree.Branch( "pulseIntegral",  pulseIntegral,  f"pulseIntegral[{n_channels}]/F")
    tree.Branch( "pulseBeginTime", pulseBeginTime, f"pulseBeginTime[{n_channels}]/I")
    tree.Branch( "pulseRiseTime",  pulseRiseTime,  f"pulseRiseTime[{n_channels}]/F")
    tree.Branch( "pulsePeakTime",  pulsePeakTime,  f"pulsePeakTime[{n_channels}]/I")
    tree.Branch( "pulseFWHM",      pulseFWHM,      f"pulseFWHM[{n_channels}]/F")
    tree.Branch( "pulseAWMT",      pulseAWMT,      f"pulseAWMT[{n_channels}]/F")

    # iterate over the files contained in the input lists.
    #
    for txtfile in args.files:

        # check if configuration number is specified with :
        #
        if ':' not in txtfile:
            print( f"Input must be specified as config-number:txt-file", file=sys.stderr)
            exit(-1)
            
        config,file_path = txtfile.split(':')
        print( f"Checking files in {file_path} as configuration {config}..." )

        # obtain the file list - if hdf5 file is given, use it alone as the file
        file_list = []
        if file_path.endswith('.hdf5'):
            file_list = [ file_path ]
        else:
            file_list = get_file_list( file_path )

        # Set configuration ID and iterate over the txt file

        for filename in file_list:
            
            print( f"\tProcessing {filename} as run {runID}" )

            # Get configID and runID from commandline and filename
            #
            configID = config
            runID = extract_run_number( filename )
            

            # Create the reader object and get basic settings of the data taking
            #
            reader = h5reader.DataReader( filename )
            reader.Open()

            adc_attr           = reader.GetADCAttributes()
            pre_trigger_sample = int( adc_attr['nb_pre_trigger_sample'] )
            nb_samples         = adc_attr['nb_samples']
            nb_events          = adc_attr['nb_events']

            counter = 0
                # used to accumulate event counter as event ID

            for event_data, event_attr in reader.GetEventIterator():

                eventID   = counter
                timestamp = event_attr['timestamp']
                ttt       = event_attr['trigger_time_tag']
                ettt      = event_attr['option']
                    # later may think of a way to combine these two

                # iterate over different channels
                # forget about multi-channels first
                #

                # get unprocessed raw waveform and compute some basic quantities.
                #
                raw_wfm[ch] = event_data[ch]

                rawFAMR[ch]   = GetFAMR( raw_wfm )
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

                proc_wfm *= -1

                blFlag[ch] = flag

                # if at least one baseline is found, proceed to perform pulse-finding
                #
                if flag != ps.BaselineFlag.NOT_FOUND:

                    # mask can be obtained in either case
                    #
                    mask = np.zeros( len(raw_wfm), dtype=bool )

                    if flag == ps.BaselineFlag.GOOD:

                        mask = mask0+mask1
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
                        wfmIntegral[ch] = np.trapz( proc_wfm[pre_trigger_sample-recede:] )
                        wfmAWMT[ch] = GetAWMT( proc_wfm[pre_trigger_sample-recede:] )

                    else:

                        baseline = 0*xdata + coeff[0]

                        blLen1[ch]  = len( raw_wfm[mask0] )     if mask0 != None else -1
                        blMean1[ch] = np.mean( raw_wfm[mask0] ) if mask0 != None else -1
                        blStd1[ch]  = np.std(  raw_wfm[mask0] ) if mask0 != None else -1

                        blLen2[ch]  = len( raw_wfm[mask1] )     if mask1 != None else -1
                        blMean2[ch] = np.mean( raw_wfm[mask1] ) if mask1 != None else -1
                        blStd2[ch]  = np.std(  raw_wfm[mask1] ) if mask1 != None else -1

                        blIntercept[ch] = coeff[0]
                        blSlope[ch] = 0

                
                    mean = np.mean( proc_wfm[mask])
                    dev  = np.std( proc_wfm[mask])
            
                    # even if both baselines have been found, check for noise
                    #
                    if np.min(proc_wfm) > -10 and rawFAMR > 0.75:
                
                        # find primary pulse
                        pflag, beg, end = ps.PulseFinder( proc_wfm, threshold=dev, std=dev, beg=0, end=pre_trigger_sample + window)

                        end = max(end,beg+window)
                
                        pulseFlag[ch] = pflag

                        if pflag != ps.PulseFlag.NOT_FOUND:
                            end += 1
                            pulseID[ch] = 0
                            pulseLen[ch] = end - beg
                            pulseBeginTime[ch] = beg
                            pulseHeight[ch] = np.max(proc_wfm[beg:end])
                            pulseIntegral[ch] = np.trapz( proc_wfm[beg:end] )
                            pulsePeakTime[ch] = np.argmax( proc_wfm[beg:end] )
                            pulseRiseTime[ch] = GetRiseTime( proc_wfm[beg:end] )
                            pulseFWHM[ch] = GetFWHM( proc_wfm[beg:end] )
                            pulseAWMT[ch] = GetAWMT( proc_wfm[beg:end] )
                            tree.Fill()

                        # run pulse finding in the auxiliary region
                        #
                        sflag, sbeg, send = ps.PulseFinder( proc_wfm, threshold=dev, std=dev, beg=end, end=-1 )
                
                        if (sflag != ps.PulseFlag.NOT_FOUND) and len(proc_wfm[sbeg:send])>min_length :
                            send += 1
                            pulseID[ch] = 1
                            pulseLen[ch] = send - sbeg
                            pulseBeginTime[ch] = sbeg
                            pulseHeight[ch] = np.max(proc_wfm[sbeg:send])
                            pulseIntegral[ch] = np.trapz( proc_wfm[sbeg:send] )
                            pulsePeakTime[ch] = np.argmax( proc_wfm[sbeg:send] )
                            pulseRiseTime[ch] = GetRiseTime( proc_wfm[sbeg:send] )
                            pulseFWHM[ch] = GetFWHM( proc_wfm[sbeg:send] )
                            pulseAWMT[ch] = GetAWMT( proc_wfm[sbeg:send] )
                            tree.Fill()
                    else:
                        blflag[ch] = XXX
                        tree.Fill()
            
                counter += 1

            reader.Close()

    output_file.Write()
    output_file.Close()

if __name__=="__main__":
    main()
