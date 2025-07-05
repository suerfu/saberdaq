#!/bin/env python3
from pyradet.h5util import h5reader as h5reader
from pyradet.signal import pulse as ps

import sys
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import argparse


# Update function for animation
#def update(frame, text):
def update(frame):
    print( "Plotting event", frame, end='\r' )

    # retrieve the waveform
    eventID = frame
    event,_ = reader.GetEvent( eventID )
    raw_wfm = event[0]
    waveform1.set_ydata( raw_wfm )
    ax[1].set_ylim( [3980,4050] )

    # baseline finding
    flag, proc_wfm, mask0, mask1, coeff = ps.BaselineFinder( raw_wfm, pre_trigger_sample )

    # if at least one baseline is found, perform pulse-finding
    if flag != ps.BaselineFlag.NOT_FOUND:

        # mask can be obtained in either case
        mask = np.zeros( len(raw_wfm), dtype=bool )
        
        if flag == ps.BaselineFlag.GOOD:
            mask = mask0+mask1
            baseline = np.polyval(coeff,xdata)
        else:
            if flag == ps.BaselineFlag.NO_TAIL:
                mask[:int(pre_trigger_sample)] = mask0
            else:
                mask[-int(pre_trigger_sample):] = mask1
            baseline = 0*xdata + coeff[0]
            
        # mark points used for baseline computing
        # check on this later! ax.scatter( xdata[mask], waveform[mask], label='baseline', ls=':', marker='o'  )
            
        proc_wfm *= -1
        mean = np.mean( proc_wfm[mask])
        dev  = np.std( proc_wfm[mask])
    
        #ax[1].set_ylim( [ -10,np.max(proc_wfm)+10] )
        ax[0].set_ylim( [ np.min(proc_wfm)-10,np.max(proc_wfm)+10] )
        
        if np.min(proc_wfm) < -10:
            ax[0].set_title( f"Event {eventID} - Noise (overshoot)" )
        else:
            pflag, beg, end = ps.PulseFinder( proc_wfm, threshold=dev, std=dev, beg=0, end=2*pre_trigger_sample)
            end = max(end,beg+window)
            
            if pflag != ps.PulseFlag.NOT_FOUND:
                end += 1
                #print( f"Primary pulse found between {beg} and {end}: {pflag}" )
                min0 = np.min(proc_wfm[beg:end])
                max0 = np.max(proc_wfm[beg:end])
                pbox.set_data( [beg,beg,end,end,beg], [min0, max0, max0, min0, min0] )
                #print( "AWMT = {}".format(ps.GetAWMT(proc_wfm[beg:end]) ) )
            else:
                pbox.set_data( [], [] )
    
            #print("Continuing to secondary pulse finding")
            
            if pflag != ps.PulseFlag.NO_TAIL and end<len(proc_wfm)-1:

                sflag, sbeg, send = ps.PulseFinder( proc_wfm, threshold=dev, std=dev, beg=end, end=-1 )
                min_length = 50
            
                if (sflag != ps.PulseFlag.NOT_FOUND) and len(proc_wfm[sbeg:send])>min_length :
                    send += 1
                    #print( f"Secondary pulse found between {sbeg} and {send}: {sflag}" )
                    min1 = np.min(proc_wfm[sbeg:send])
                    max1 = np.max(proc_wfm[sbeg:send])
                    sbox.set_data([sbeg,sbeg,send,send,sbeg], [min1, max1, max1, min1, min1])
                    #ax_inset.set_xlim([beg-10,send+10])
                    #ax_inset.set_ylim([min1-10,max1+10])
                else:
                    sbox.set_data( [], [] )
                
                ax[0].set_title( f"Event {eventID} - P:{pflag} - S:{sflag}" )
            
            else:
                ax[0].set_title( f"Event {eventID} - P:{pflag}" )
                
    else:
        ax[0].set_title( f"Event {eventID} - {flag}" )
    
    waveform1.set_ydata( raw_wfm )

    if flag != ps.BaselineFlag.NOT_FOUND:
        waveform2.set_ydata( proc_wfm )
        baseline1.set_ydata( baseline )
        sigma1.set_ydata( baseline - dev)
        sigma2.set_ydata( baseline + dev)
        baseline_mask1.set_data( xdata[mask], raw_wfm[mask] )
    
    #waveform3 = waveform1[beg+window:]
    #second_pulse_window = 20
    #hits = np.convolve( waveform3<mean-5*dev, np.ones(window, dtype=int), mode='valid')

    # Check if any sum is equal to window size (i.e., all 10 are True)
    #has_consecutive = np.any(hits == window)

    # Define multi-line text
    #text_dict = {
        #"event ID" : eventID,
        #"baseline mean" : mean,
        #"baseline std dev" : dev,
        #"pulse begin" :beg,
        #"pulse peak" : peak,
        #"height" : height,
        #"area" : area,
        #"risetime" : risetime,
        #"FWHM" : fwhm,
        #"AWMT" : awmt,
        #"FAMT" : famr,
        #"sample<5sig" : len( waveform3[waveform3<mean-5*dev] )
    #}

    #text_lines = [ f"{k:<16} : {v:10d}" if isinstance(v,int) else f"{k:<16} : {v:10.2f}" for k,v in text_dict.items() ]
    #text.set_text( "\n".join(text_lines) )
    
    return waveform1,waveform2,baseline1,sigma1,sigma2,baseline_mask1,pbox,sbox

    
parser = argparse.ArgumentParser(description="Process a file with a specified range.")
parser.add_argument("filename", help="Path to the input file")
parser.add_argument("--range", type=int, nargs=2, metavar=('START', 'END'), default=[0,-1], help="Range values: start and end" )
parser.add_argument("--random", action="store_true", help="Randomly display events in the specified range" )
args = parser.parse_args()

print(f"Filename: {args.filename}")
print(f"Range: {args.range[0]} to {args.range[1]}")

#filename = 'data/20250325_NaI_Kamioka/NaI_Background_Bottom_Run1_HV800_20250325_120615.hdf5' #parser.filename
reader = h5reader.DataReader( args.filename )
reader.Open()

adc_attr = reader.GetADCAttributes()
for key,val in adc_attr.items():
    print( "{} : {}".format(key,val) )
pre_trigger_sample = reader.GetADCAttributes()['nb_pre_trigger_sample']
nb_samples = reader.GetADCAttributes()['nb_samples']
nb_events = reader.GetADCAttributes()['nb_events']

# Initialize plot
# Make a 2x2 plot: top left - waveform in the entire window, top-right: baseline view
#                : bottom left - primary pulse if found,     top-right: secondary pulse if found
fig, ax = plt.subplots( 2,1, figsize=(12,8), sharex='col' )

xdata = np.arange(0, nb_samples)

ax[0].set_xlabel( 'Time Index', fontsize=14 )
for a in ax.flat:
    a.set_ylabel( 'ADC Count', fontsize=14 )

waveform2, = ax[0].plot( xdata, 0*xdata, lw=1, linestyle='-' )
baseline2, = ax[0].plot( xdata, 0*xdata, lw=1, linestyle='-', color='g' )

waveform1, = ax[1].plot( xdata, 0*xdata, lw=1, linestyle='-' )
baseline1, = ax[1].plot( xdata, 0*xdata, lw=1, linestyle='-', color='r' )
sigma1, = ax[1].plot( xdata, 0*xdata, lw=1, linestyle='--', color='r')
sigma2, = ax[1].plot( xdata, 0*xdata, lw=1, linestyle='--', color='r')
baseline_mask1, = ax[1].plot( xdata, 0*xdata, '.' )

pbox, = ax[0].plot( np.zeros(5), np.zeros(5), lw=1, color='g')
sbox, = ax[0].plot( np.zeros(5), np.zeros(5), lw=1, color='g')

window = 750

# text object
#text = ax.text(
#        0.5, 0.1, "",
#        transform=ax.transAxes,
#        fontsize=12,
#        family='monospace',
#        verticalalignment='bottom',
#        horizontalalignment='left',
#        bbox=dict(boxstyle="round,pad=0.3", facecolor="white", alpha=0.7) )

# Create animation
begin = args.range[0]
end = nb_events if args.range[1] < 0 else min(nb_events, args.range[1])

intval = 2500
frame = range(begin,end) if args.random==False else np.random.randint( begin, end, nb_events)

ani = FuncAnimation(fig, update, frames=frame, interval=intval, blit=True)
#ani = FuncAnimation(fig, update, frames=range(begin, end), fargs=(text,), interval=intval, blit=True)


plt.show()

print("Closing HDF5 Reader...")

reader.Close()
