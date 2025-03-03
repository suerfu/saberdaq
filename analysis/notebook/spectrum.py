#!/bin/env python3

################################################################
# Displays pulse spectrum and other information in the hdf5 file
################################################################

import sys
import argparse
import copy

import h5py as h5
import numpy as np
import matplotlib
import matplotlib.pyplot as plt

from matplotlib.colors import LogNorm

###################################
# Some global settings for plotting
###################################

fontsize = 12
textsize = 10
linewidth = 1.5
labelsize = 10

# default figure size, golden ratio!
#
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


#########################################
# function to retrieve waveform from file
#########################################

def get_waveform( filename, index = 0, channel = 0 ) :

    data = None
    file = None
    
    # check if the input is a name to hdf5 file, or an hdf5 file object
    # handling object directly is more efficient
    # since repeated opening and closing are not required.
    #
    if isinstance( filename, str):
        file = h5.File( filename, 'r' )
    else:
        file = filename
            
    max_evt = file['/adc_0/'].attrs['nb_events']
        # number of events in the file
        
    # raise error if trying to access beyond the available number of events
    #
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


##########################################################
# function to compute the integral of given pulse waveform
##########################################################

def integral( data, start, length = 3*250 ):
    return np.sum( data[start : start+length] * 4e-3 ) # integrate for 3 us by default


##########################
# main program
##########################

def main():

    # parse arguments for configuration
    # key parameters are:
        # files to be opened and read
        # channels to be read and plotted
        # events event indexes to be displayed in the final plot; multiple events plotting are allowed
        
    parser = argparse.ArgumentParser( prog='spectrum', 
                                     description='Displays saberdaq spectrum by analyzing HDF5 files', 
                                     epilog='')
    parser.add_argument( '-m', '--mode',
                        metavar = 'mode',
                        nargs = '*',
                        choices = [ 'ht', 'e', 'ht_e' ],
                        help = 'Displays information about the HDF5 files. No other actions will be taken.' )
    parser.add_argument( '-v', '--verbose', 
                        action = 'store_true',
                        help = 'Verbosity: displays more detailed information as procesing goes on.' )
    parser.add_argument( '-f', '--file', 
                        metavar = 'filename(s)',
                        nargs = '*',
                        help = 'HDF5 files to read and display event(s).' )
    parser.add_argument( '--pyroot',
                        nargs = 1,
                        help = 'Generates a ROOT file from the input raw files.' )
    parser.add_argument( '--no-plot', 
                        action = 'store_true',
                        help = 'Do not make any plots (useful if one only wants ROOT output).' )
    parser.add_argument( '-s', '--sum', 
                        action = 'store_true',
                        help = 'Processes the files by merging them together as a single run (default is treat them independently).' )
    parser.add_argument( '-e', '--event', 
                        metavar = 'event ID',
                        nargs = '?',
                        type = int,
                        default = -1,
                        help = 'Number of event(s) to be analyzed. Default is all events in the files.' )

    args = parser.parse_args()
    
    height = []
    energy = []
        # variable used to hold result of computation as a tuple
    
    ROOTFile = None
        # ROOT output file
        # default is None, but if pyROOT is given in the argument, and pyROOT is installed
        # this variable will be a ROOT TFile object

    ##############################
    # Check if pyroot is installed
    ##############################

    if len( args.pyroot ) > 0 :
        
        try:
            import ROOT
            from array import array

        except ImportError:
            print("ROOT output requested but pyROOT is NOT installed. Aborting...")
            exit(-1)

        treeEventID = array( 'l', [0])
        treeHeight  = array( 'i', [0])
        treeEnergy  = array( 'f', [0.0]) 

        ROOTFile = ROOT.TFile( args.pyroot[0], "recreate")
        tree = ROOT.TTree("events", "event tree storing reduced quantities")
    
        #treeEventID = ROOT.std.vector('long')()
        #treeHeight = ROOT.std.vector('int')()
        #treeEnergy = ROOT.std.vector('float')()

        tree.Branch("eventID", treeEventID, "eventID/L" )
        tree.Branch("height",  treeHeight, "height/I" )
        tree.Branch("energy", treeEnergy, "energy/F" )
    

    ############################################################################################
    # Determine what quantities to store in the tuple, and then process and fill the tuple
    ############################################################################################
    
    for filename in args.file:
    
        nb_events = 0
        pre_trig_window = 0
        
        with h5.File( filename, 'r' ) as file:
            
            nb_events = file['adc_0'].attrs['nb_events']
            pre_trig_window = file['adc_0'].attrs['pre_trigger_sample']

            # get number of events, which is the smaller of that specified or that available
            #
            if args.event > 0:

                nb_events = min( args.event, nb_events)

            # loop over the events to get statistics
            #
            for i in range( 0, nb_events):

                if( i%1000==0 ):
                    print("Processing event", i, end='\r')

                data = get_waveform( filename, i)

                baseL,baseStd = baseline( data, pre_trig_window)
                    # average and standard deviation of baseline

                # take the start as when the pulse first crosses 5 sigma deviation
                # start of integral is 4 samples (16 ns) prior to this point
                #
                start = np.where( data[:pre_trig_window] > -5 * baseStd)[0][-1]
                startIntegral = start - 4

                data = data - baseL

                height_tmp = -np.min( data )
                energy_tmp = -integral( data, startIntegral )

                if args.no_plot is False:

                    height.append( height_tmp )
                    energy.append( energy_tmp )

                if ROOTFile is not None:

                    # Fill ROOT tree branches
                    #
#                    treeEventID.clear()
#                    treeHeight.clear()
#                    treeEnergy.clear()

                    treeEventID[0] = i
                    treeHeight[0] = int(height_tmp)
                    treeEnergy[0] = energy_tmp

#                    treeEventID.push_back( i )
#                    treeHeight.push_back( int(height_tmp) )
#                    treeEnergy.push_back( energy_tmp )

                    tree.Fill()

            print()

    if ROOTFile is not None:
        ROOTFile.Write()
        ROOTFile.Close()

    if args.no_plot is True:
        exit()

    # make the plot of pulse height spectrum
    #
    title = ['Pulse Height [a.u.]'] #, ['Pulse Integral [a.u.]'] #, ['Pulse Height','Integral'] ]:
    
    fig, ax = plt.subplots(figsize=(10, 6))
        
    counts, bin_edges, patches = plt.hist( height, bins=500, align='left');
    bin_centers = (bin_edges[:-1] + bin_edges[1:]) / 2
        
    #plt.step( bin_centers, counts, label = file.split('/')[-1] )
    plt.step( bin_centers, counts )
    
    plt.xlabel( title[0] )
    plt.yscale( 'log' )
    
    # make the plot of pulse integral spectrum
    #
    title = ['Pulse Integral [a.u.]'] #, ['Pulse Height','Integral'] ]:
    
    fig, ax = plt.subplots(figsize=(10, 6))
        
    counts, bin_edges, patches = plt.hist( energy, bins=500, align='left');
    bin_centers = (bin_edges[:-1] + bin_edges[1:]) / 2
        
#     plt.step( bin_centers, counts, label = file.split('/')[-1] )
    plt.step( bin_centers, counts )
    
    plt.xlabel( title[0] )
    plt.yscale( 'log' )
    
    # make pulse height v.s. pulse integral (energy) 2D spectra
    #
    title = [ 'Pulse Height [a.u]', 'Pulse Integral [a.u.]' ]
    
    fig, ax2 = plt.subplots(figsize=(10, 6))

    hist2 = ax2.hist2d( height, energy, bins=[300, 300], norm=LogNorm(), cmap=cmap)
    bin_centers = (bin_edges[:-1] + bin_edges[1:]) / 2

    ax2.tick_params(axis='both', which='both', labelsize=20, length=10, width=2)
    ax2.set_xlabel('Height (ADC count)', fontsize=20)
    ax2.set_ylabel('PSD', fontsize=20)

    cbar = plt.colorbar(hist2[3], ax=ax2)
    cbar.set_label('Counts (Log scale)', fontsize=20)
    cbar.ax.tick_params(labelsize=20)    
    
    plt.xlabel( title[0] )
    plt.ylabel( title[1] )
    
    plt.legend()
    plt.show()
    



if __name__=="__main__":
    main()
