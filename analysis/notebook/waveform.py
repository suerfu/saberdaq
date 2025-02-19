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
    
    try :
        file = h5.File( filename, 'r' )
        
        max_evt = file['/adc_0/'].attrs['nb_events']        
        if index > file['/adc_0/'].attrs['nb_events'] :
            file.close()
            raise Exception('Trying to access event index {} beyond number of events {}'.format( index, max_evt) )
            
        event = 'event_{}'.format( index )
        data = np.array( file['adc_0'][event] )
        
        if channel > len( data[0,:] ) :
            raise Exception( 'Trying to access channel index {} beyond number of enabled channels {}'.format( channel, len(data[0,:]) ) )
    
    except Exception as e:
        print('Exception has ocurred:', repr(e) )
        file.close()
        return None
        
    file.close()
    
    return data[channel,:]
    
    
#     ArgMin=np.argmin(data)
#     pre_trig_window = file['adc_0'].attrs['pre_trigger_sample']
#     baseL,BaseSTD=Baseline(data, pre_trig_window)
#     data=data-baseL
#     Start=np.where(data[:pre_trig_window]>-5*BaseSTD)[0][-1]


#     fig, ax = plt.subplots(figsize=(24, 14));
#     plt.plot( data )
#     ones = np.ones(len(data))
#     plt.plot(ones*0,'--')
#     print(pre_trig_window)
#     # plt.plot([ArgMin,ArgMin],[np.max(data),np.min(data)])
#     plt.plot([pre_trig_window,pre_trig_window],[np.max(data),np.min(data)])
#     plt.plot([Start,Start],[np.max(data),np.min(data)])
#     plt.plot([Start+750,Start+750],[np.max(data),np.min(data)])
#     # plt.plot([pre_trig_window-50,pre_trig_window-50],[np.max(data),np.min(data)])
#     # plt.plot([pre_trig_window+400,pre_trig_window+400],[np.max(data),np.min(data)])
#     plt.show()


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

    parser.add_argument('-i', '--info', 
                        metavar = 'info',
                        nargs = '*',
                        help = 'Displays information about the HDF5 files. No other actions will be taken.' )
    
    parser.add_argument('-v', '--verbose', 
                        action = 'store_true',
                        help = 'Verbosity: displays configuration file as well in info mode.' )
        # positional argument
        
    parser.add_argument('-f', '--file', 
                        metavar = 'filename(s)',
                        nargs = '*',
                        help = 'HDF5 files to read and display event(s)' )
        # positional argument

#     parser.add_argument('-c', '--channel', 
#                         metavar = 'channel ID',
#                         nargs = '*',
#                         type = int,
#                         help = 'Channel ID to be displayed' )
        
    parser.add_argument('-e', '--event', 
                        metavar = 'event ID',
                        nargs = '*',
                        type = int,
                        default = [0],
                        help = 'ID of event(s) to be displayed' )

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
    threshold = 0
    threshold_prev = 0
        
    for file in args.file:
        
        print('Processing', file)
        
        with h5.File( file, 'r') as f:
            pre_trig_window = f['adc_0'].attrs['pre_trigger_sample']
            threshold = f['adc_0'].attrs['channel_threshold']

        minimum, maximum = 4096, 0
        
        for event in args.event:

            data = get_waveform( file, event )

            #################
            # making the plot
            #################

            plt.plot( 0.004 * np.arange( 0, len(data), 1), data, label= file.split('/')[-1]+', event {}'.format( event ) )
            
#             threshold = 4060
#                 # manual fix of a bug regarding storing config parameter when 1st channel is not enabled
            
            if threshold_prev != threshold:
                plt.plot( 0.004 * np.arange( 0, len(data), 1), threshold * np.ones( len(data) ), 
                         '--', label='threshold ({})'.format( file.split('/')[-1])  )
                threshold_prev = threshold
            
            minimum = min( np.min(data), minimum)
            maximum = max( np.max(data), maximum)
            #ones = np.ones(len(data))
            #plt.plot(ones*0,'--')

            #print(pre_trig_window)
            # plt.plot([ArgMin,ArgMin],[np.max(data),np.min(data)])

            #plt.plot([pre_trig_window,pre_trig_window],[np.max(data),np.min(data)])
            #plt.plot([Start,Start],[np.max(data),np.min(data)])
            #plt.plot([Start+750,Start+750],[np.max(data),np.min(data)])
            # plt.plot([pre_trig_window-50,pre_trig_window-50],[np.max(data),np.min(data)])
            # plt.plot([pre_trig_window+400,pre_trig_window+400],[np.max(data),np.min(data)])
    
    delta = maximum - minimum
    plt.plot( 0.004 * pre_trig_window * np.ones(2), [minimum - .1*delta, maximum + 0.1*delta], '--', label='trigger point'  )

    plt.xlabel('Time [us]')
    plt.ylabel('ADC Count [a.u.]')
    plt.legend()
    plt.show()


if __name__=="__main__":
    main()
