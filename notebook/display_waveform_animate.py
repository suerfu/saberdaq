from pyradet.h5util import h5reader as h5reader
from pyradet.signal import pulse as ps

import sys
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation



filedir = 'data/20250319_NaI_4GoKan/'
filename = 'NaI_Background_20250319_195105.hdf5'
reader = h5reader.DataReader( filedir + filename )
reader.Open()

adc_attr = reader.GetADCAttributes()
for key,val in adc_attr.items():
    print( "{} : {}".format(key,val) )
pre_trigger_sample = reader.GetADCAttributes()['nb_pre_trigger_sample']
nb_samples = reader.GetADCAttributes()['nb_samples']
nb_events = reader.GetADCAttributes()['nb_events']


# Initialize plot

fig, ax = plt.subplots( figsize=(12,8) )

plt.xlabel( 'Time Index', fontsize=14 )
plt.ylabel( 'ADC Count', fontsize=14 )
plt.grid()

ax.set_ylim( 3950, 4050 )

x = np.arange(0, nb_samples)

waveform, = ax.plot( x, 0*x, lw=1, linestyle=':', marker='.' )
baseline, = ax.plot( x, 0*x, lw=1, linestyle=':', marker='.' )
sigma_p, = ax.plot( [0, nb_samples], [0,0], lw=1)
sigma_n, = ax.plot( [0, nb_samples], [0,0], lw=1)
sigma_5n, = ax.plot( [0, nb_samples], [0,0], lw=1)
peak_line, = ax.plot( [0, nb_samples], [0,0], lw=1)

window=500

# text object
text = ax.text(
        0.5, 0.1, "",
        transform=ax.transAxes,
        fontsize=12,
        family='monospace',
        verticalalignment='bottom',
        horizontalalignment='left',
        bbox=dict(boxstyle="round,pad=0.3", facecolor="white", alpha=0.7) )

# Update function for animation
def update(frame, text):
    
    print( "Plotting event", frame, end='\r' )
    
    eventID = frame #np.random.randint(0,nb_events)
    
    event,_ = reader.GetEvent( eventID )
    waveform1 = event[0]

    mean, dev = ps.GetBaseline( waveform1, pre_trigger_window=pre_trigger_sample, polarity=-1 )

    waveform2 = mean - waveform1
    height, area, beg, peak = ps.GetPulseStats( waveform2, window=window, pre_trigger_window=pre_trigger_sample )

    risetime = ps.GetRiseTime( waveform2 )
    fwhm = ps.GetFWHM( waveform2 )
    awmt = ps.GetAWMT( waveform2 )
    famr = ps.GetFAMR( waveform2 )

    waveform.set_ydata( waveform1 )
    baseline.set_xdata( np.arange(0,int(beg)) )
    baseline.set_ydata( waveform1[:int(beg)])
    sigma_n.set_data  ( [0, len(waveform1)], [mean-dev,mean-dev] )
    sigma_5n.set_data ( [0, len(waveform1)], [mean-5*dev,mean-5*dev] )
    sigma_p.set_data  ( [0, len(waveform1)], [mean+dev,mean+dev] )
    peak_line.set_data( [0, peak], [mean - height,mean - height] )

    waveform3 = waveform1[beg+window:]

    second_pulse_window = 20
    hits = np.convolve( waveform3<mean-5*dev, np.ones(window, dtype=int), mode='valid')

    # Check if any sum is equal to window size (i.e., all 10 are True)
    has_consecutive = np.any(hits == window)

    # Define multi-line text
    text_dict = {
        "event ID" : eventID,
        "baseline mean" : mean,
        "baseline std dev" : dev,
        "pulse begin" :beg,
        "pulse peak" : peak,
        "height" : height,
        "area" : area,
        "risetime" : risetime,
        "FWHM" : fwhm,
        "AWMT" : awmt,
        "FAMT" : famr,
        "sample<5sig" : len( waveform3[waveform3<mean-5*dev] )
    }

    text_lines = [ f"{k:<16} : {v:10d}" if isinstance(v,int) else f"{k:<16} : {v:10.2f}" for k,v in text_dict.items() ]
    text.set_text( "\n".join(text_lines) )
    
    ax.set_title( f"Event {eventID}" if has_consecutive==False else f"Event {eventID} - Double Pulse Detected" )
    
    return waveform, baseline, sigma_n, sigma_5n, sigma_p, peak_line, text

# Create animation
begin = 0
end = nb_events
intval = 2000

if len(sys.argv)>1:
    begin = int(sys.argv[1])
if len(sys.argv)>2:
    end = int(sys.argv[2])
if len(sys.argv)>3:
    intval = int(sys.argv[3])

end = min(nb_events, end)

ani = FuncAnimation(fig, update, frames=range(begin, end), fargs=(text,), interval=intval, blit=True)

plt.show()

print("Closing HDF5 Reader...")

reader.Close()
