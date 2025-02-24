#!/bin/env python3
# Displays waveform in the hdf5 file

# 

import sys
import argparse
import copy

import h5py as h5
import numpy as np
import matplotlib
import matplotlib.pyplot as plt

# Some global settings for plotting
fontsize = 12
textsize = 10
linewidth = 1.5
labelsize = 10

# default figure size, golden ratio!
figsize = (5.2, 5.2/1.618)
figdpi = 300
gridwidth = 0.3

#matplotlib.rcParams['font.family'] = ['Times New Roman']
matplotlib.rcParams['mathtext.fontset'] = 'stix'
matplotlib.rcParams['mathtext.default'] = 'rm'

original_cmap = plt.cm.jet
cmap = copy.copy(original_cmap)
cmap.set_bad(color='white')

colors = plt.rcParams['axes.prop_cycle'].by_key()['color']


##################################################
# function to displays file attributes iteratively
##################################################

def display_attributes( filename, print_config = False ):
    
    file = h5.File( filename, 'r' )
    
    print()
    
    for key in file.attrs.keys():
        if key=='config':
            
            if print_config:
                print( '/'+key, ' : ', file.attrs[key])
            else:
                continue
        
        print( '/'+key, ' : ', file.attrs[key])

    print()
    
    for group in file.keys():
        
        for key in file[group].attrs.keys():
            print( '\t/'+group+'/'+key, ':', file[group].attrs[key])
    
        if 'nb_events' in file[group].attrs.keys() and file[group].attrs['nb_events'] > 0 :
            
            print()
            for key_event in file[group+'/event_0'].attrs.keys():
                print( '\t\t/'+group+'/event_0/'+key_event, ':', file[group+'/event_0'].attrs[key_event] )
        
        print()

    file.close()

    
#########################################
# function to retrieve waveform from file
#########################################

def get_waveform( filename, index = 0, channel = 0 ) :

    data = None
    file = None
    
    if isinstance( filename, str):
        file = h5.File( filename, 'r' )
    else:
        file = filename
            
    max_evt = file['/adc_0/'].attrs['nb_events']        
        
    if index > file['/adc_0/'].attrs['nb_events'] :
        file.close()
        raise Exception('Trying to access event index {} beyond number of events {}'.format( index, max_evt) )
            
    event = 'event_{}'.format( index )
    data = np.array( file['adc_0'][event] )
        
    if channel > len( data[0,:] ) :
        raise Exception( 'Trying to access channel index {} beyond number of enabled channels {}'.format( channel, len(data[0,:]) ) )

    if isinstance( filename, str) :
        file.close()
    
    return np.array( data[channel,:] )
    
#############################################################
# function to estimate baseline by summing pre trigger window
#############################################################

def baseline( data, pre_trig_window ):
    arr = data[ 0:pre_trig_window-5 ]
    return np.average( arr ), np.std( arr )


##########################
# main program
##########################

def main():

    # parse arguments for configuration
    # key parameters are:
        # files to be opened and read
        # channels to be read and plotted
        # events event indexes to be displayed in the final plot; multiple events plotting are allowed
        
    parser = argparse.ArgumentParser( prog='waveform', 
                                     description='Displays saberdaq waveform recorded in the HDF5 file', 
                                     epilog='')

    parser.add_argument( '-i', '--info', 
                        metavar = 'info',
                        nargs = '*',
                        help = 'Displays information about the HDF5 files. No other actions will be taken.' )
    
    parser.add_argument( '-v', '--verbose', 
                        action = 'store_true',
                        help = 'Verbosity: displays configuration file as well in info mode.' )
        
    parser.add_argument( '-f', '--file', 
                        metavar = 'filename(s)',
                        nargs = '*',
                        help = 'HDF5 files to read and display event(s).' )
    
    parser.add_argument( '-b', '--baseline', 
                        action = 'store_true',
                        help = 'Subtracts baseline. Baseline estimated as average of waveform in the pre-trigger window.' )
    
    parser.add_argument( '-s', '--sum', 
                        action = 'store_true',
                        help = 'Computes average pulse shape by averaging all waveforms in the file or specified via --event.' )
    
    parser.add_argument( '--fft', 
                        action = 'store_true',
                        help = 'Computes FFT of the waveform. In this mode, --event is used to specify number of events to average.' )
    
     parser.add_argument( '-c', '--channel', 
                        metavar = 'Channel ID',
                        nargs = '*',
                        type = int,
                        default = [0],
                        help = 'Index of channel(s) to be displayed. Default is 0 only.' )

    parser.add_argument( '-e', '--event', 
                        metavar = 'event ID',
                        nargs = '*',
                        type = int,
                        default = [0],
                        help = 'ID of event(s) to be displayed. In sum/average mode, this is the number of events to sum.' )

    args = parser.parse_args()
    
    ###########################################################################
    # if info option is given, then display the attributes of the file and exit
    ###########################################################################
    
    if args.info != None:
        
        # for-loop for processing the files
        # iterate through each file
        for file in args.info:
            display_attributes( file, args.verbose )
        
        exit(0)
    
    ######################
    # preparing the figure
    ######################
    
    fig, ax = plt.subplots(figsize=(10, 6));
    
    #####################################
    # process the list of files specified
    #####################################

    pre_trig_window = 0
    threshold_cur = 0
    threshold_plotted_list = []
        
    for file in args.file:
        
        print('Processing', file)
        
        with h5.File( file, 'r') as f:
            pre_trig_window = f['adc_0'].attrs['pre_trigger_sample']
            threshold = f['adc_0'].attrs['channel_threshold']

        minimum, maximum = 4096, 0
        
        #########################################################
        # if not in average mode, plot individual selected pulses
        #########################################################
        
        if args.sum == False:

            for event in args.event:

                for chan in args.channel:

                    threshold_cur = threshold[chan]

                    data = get_waveform( file, event, chan)
                    
                    ###################################################
                    # if argument is specified, subtract baseline first
                    ###################################################
                    
                    if args.baseline == True :
                        avg, _ = baseline(data, pre_trig_window)
                        data = ( avg - data )

                    #################
                    # making the plot
                    #################

                    if args.fft == False :
                        plt.plot( 0.004 * np.arange( 0, len(data), 1), data, label = file.split('/')[-1]+', channel {} event {}'.format( chan, event ) )

                    else :
                        data_fft = np.fft.rfft( data )
                        freq = np.fft.rfftfreq( len(data), 0.004 )
                        data_fft /= (data_fft[1] - data_fft[0]) * len( data )

                    if threshold_cur not in threshold_plotted_list and args.baseline == False:
                        threshold_plotted_list.append( threshold_cur )

                    minimum = min( np.min(data), minimum)
                    maximum = max( np.max(data), maximum)
    
            for tc in threshold_plotted_list:
                plt.plot( 0.004 * np.arange( 0, len(data), 1), tc * np.ones( len(data) ),  ':', label='threshold ({})'.format( file.split('/')[-1])  )
            
            delta = maximum - minimum
            plt.plot( 0.004 * pre_trig_window * np.ones(2), [minimum - .1*delta, maximum + 0.1*delta], ':', label='trigger ({})'.format( file.split('/')[-1]) )
    
        else:
            
            nb_events = 0
            
            Sum = 0 
            
            with h5.File( file, 'r') as f:
                
                nb_events = f['adc_0'].attrs['nb_events']
            
                nSum = args.event[0]
                    # --event argument will be number of events to sum if in --average/--sum mode
                    # if it is not specified, then all events will be processed.
                if nSum == 0:
                    nSum = nb_events

                for event in range( 0, min(nb_events, nSum ) ):

                    if event%1000 == 0:
                        print('Processing event {}'.format(event), end='\r' )

                    data = get_waveform( file, event )

                    # always subtract baseline
                    avg, _ = baseline(data, pre_trig_window)

                    if args.fft == False :
                        Sum += ( avg - data )

                    else : 
                        data = avg - data
                        data_fft = np.fft.rfft( data )
                        freq = np.fft.rfftfreq( len(data), 0.004 )
                        data_fft /= ( freq[1] - freq[0] ) * len(data)
                        Sum += data_fft

                print()

            if args.fft == False:
                Sum /= np.max(Sum)
                    # normalize such that highest sample is 1.
            else:
                Sum /= nSum

            plt.plot( 0.004 * np.arange( 0, len(Sum), 1), Sum, label = file.split('/')[-1] )
    
    plt.xlabel('Time [us]')
    
    plt.ylabel('ADC Count [a.u.]')
    plt.yscale('log')
    
    plt.legend()
    plt.show()


if __name__=="__main__":
    main()
