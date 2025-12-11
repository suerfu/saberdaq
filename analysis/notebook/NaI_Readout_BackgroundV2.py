import h5py as h5;
import numpy as np;
import matplotlib
import matplotlib.pyplot as plt;
from scipy.signal import chirp, find_peaks, peak_widths, butter,sosfilt
from lmfit import Model
from lmfit.models import GaussianModel
from matplotlib.colors import LogNorm
import copy
from matplotlib.gridspec import GridSpec


# Some global settings for plotting
fontsize = 12
textsize = 10
linewidth = 1.5
labelsize = 10

# default figure size, golden ratio!
figsize = (5.2, 5.2/1.618)
figdpi = 500
gridwidth = 0.3
Calib=103833.84981892555

Cs137=661.7

matplotlib.rcParams['font.family'] = ['Times New Roman']
matplotlib.rcParams['mathtext.fontset'] = 'stix'
matplotlib.rcParams['mathtext.default'] = 'rm'

original_cmap = plt.cm.jet
cmap = copy.copy(original_cmap)
cmap.set_bad(color='white')

colors = plt.rcParams['axes.prop_cycle'].by_key()['color']

filedir = r'C:\Users\antco\OneDrive\Bureau\KEK_QUP\NaI\TestRawData/'
filename = 'Cal000_20190120_2254_nai033_bgnd.hdf5'

file = h5.File(filedir+filename, 'r')
#
print( file.attrs.keys() )
print( file.keys() )
print( file['adc_0'].keys() )
print( file['adc_0'].attrs.keys() )

pre_trig_window = file['adc_0'].attrs['pre_trigger_sample']

def Baseline( data, pre_trig_window ):
    # arr = data[:150]
    arr = data[pre_trig_window-200:pre_trig_window-50]
    return np.average( arr ), np.std(arr)

def Integral( data, Start ):
    return np.sum( data[Start-20:Start+750] ) #1252 ns

def PSDTrig(data, MinAmp,pre_trig_window,Start ):
    total = np.sum( data[Start:Start+313] )
    frac = np.sum( data[pre_trig_window:Start+313] )
    return frac/total

def PSDTrig20(data, MinAmp,pre_trig_window,Start ):
    # total = np.sum( data[Start-20:Start+313] )
    total = np.sum( data[Start-20:Start+250] )
    # frac = np.sum( data[pre_trig_window:Start+313] )
    frac = np.sum( data[Start-20:pre_trig_window] )
    return frac/total

def AWMT( data, Start ):
    Tdata=data[Start-20:Start+313]
    Xtime=np.linspace(0,len(Tdata)-1,len(Tdata))*4 #ns
    return np.sum(Xtime*Tdata)/np.sum(Tdata)

def ShowEvent(i):

    event = 'event_{}'.format(i)
    data = file['adc_0']['dataset_0'][event][0]
    ArgMin=np.argmin(data)
    pre_trig_window = file['adc_0'].attrs['pre_trigger_sample']
    baseL,BaseSTD=Baseline(data, pre_trig_window)
    data=data-baseL
    Start=np.where(data[:pre_trig_window]>-5*BaseSTD)[0][-1]


    fig, ax = plt.subplots(figsize=(24, 14));
    plt.plot( data )
    ones = np.ones(len(data))
    plt.plot(ones*0,'--')
    print(pre_trig_window)
    # plt.plot([ArgMin,ArgMin],[np.max(data),np.min(data)])
    plt.plot([pre_trig_window,pre_trig_window],[np.max(data),np.min(data)])
    plt.plot([Start,Start],[np.max(data),np.min(data)])
    plt.plot([Start+750,Start+750],[np.max(data),np.min(data)])
    # plt.plot([pre_trig_window-50,pre_trig_window-50],[np.max(data),np.min(data)])
    # plt.plot([pre_trig_window+400,pre_trig_window+400],[np.max(data),np.min(data)])
    plt.show()


def MoovingAverage(data):
    s=10
    newData=data.copy()
    for j in range(len(data)-s):
        newData[j]=np.average(data[j:j+s])
    return newData

def Spectrum(Spect,Calib,cut):
    plt.figure()
    # counts, bin_edges, patches=plt.hist(-Spect[cut],bins=500, range=[0,5e5],align='left');
    counts, bin_edges, patches=plt.hist(-Spect[cut],bins=500, range=[1.5e5,5e5],align='left');
    bin_centers = (bin_edges[:-1] + bin_edges[1:]) / 2
    # imax = bin_edges[np.argmax(counts)]
    plt.close();
    fig, ax = plt.subplots(figsize=(24, 14));
    x_axis=bin_centers/Calib*Cs137;
    plt.plot(x_axis,counts)
    ax.tick_params(axis='both',which='both',labelsize=20,length=10,width=2)
    ax.set_xlabel('Energy (keV)',fontsize=20)
    ax.set_ylabel('Counts',fontsize=20)
    plt.yscale('log')
    plt.show()
    return 0


def SpectrumError(Spect,Calib,cut):
    plt.figure()
    counts, bin_edges, patches=plt.hist(-Spect[cut],bins=200, range=[0,3.9e5],align='left');
    bin_centers = (bin_edges[:-1] + bin_edges[1:]) / 2
 # Utiliser numpy.digitize pour obtenir les indices des bins pour chaque valeur
    bin_indices = np.digitize(-Spect[cut], bin_edges) - 1

    # Calculer la moyenne et l'erreur standard des valeurs dans chaque bin
    bin_means = np.array([np.mean(-Spect[cut][bin_indices == i]) for i in range(len(bin_edges) - 1)])
    bin_stds = np.array([np.std(-Spect[cut][bin_indices == i]) for i in range(len(bin_edges) - 1)])
    bin_counts = np.array([np.sum(bin_indices == i) for i in range(len(bin_edges) - 1)])
    bin_se = bin_stds / np.sqrt(bin_counts)
    bin_ci = 1.96 * bin_stds / np.sqrt(bin_counts)

    # imax = bin_edges[np.argmax(counts)]
    plt.close();
    fig, ax = plt.subplots(figsize=(24, 14));
    x_axis=bin_centers/Calib*Cs137;
    ax.errorbar(x_axis, counts, yerr=bin_se, fmt='o', capsize=5, label='Histogram with Error Bars')
    # plt.plot(x_axis,counts)
    ax.tick_params(axis='both',which='both',labelsize=20,length=10,width=2)
    ax.set_xlabel('Energy (keV)',fontsize=20)
    ax.set_ylabel('Counts',fontsize=20)
    plt.yscale('log')
    plt.show()
    print(patches)
    return 0

Int = []
Height=[]
psdtrig = []
psdtrig20 = []
MeanTime=[]
issue=[]

for i in range(0,file['adc_0'].attrs['nb_events']):
    if(i%50000==0):print(i)
    event = 'event_{}'.format(i)

    data = file['adc_0']['dataset_0'][event][0]


    ArgMin=np.argmin(data)
    baseL,BaseStd=Baseline(data, pre_trig_window)
    Start=np.where(data[:pre_trig_window]>-5*BaseStd)[0][-1]
    data=data-baseL
    if(baseL<4050):
        issue.append(i)
        # print(i)
        continue

    a = Integral( data, Start )
    ftrig = PSDTrig(data, ArgMin,pre_trig_window,Start )
    ftrig20 = PSDTrig20(data, ArgMin,pre_trig_window,Start )
    AverageMeanT= AWMT( data, Start )


    #if f>1.5:
    #    print(i)

    # if a<0 and f<1.25 and f>0.75:
    Int.append( a )
    psdtrig.append( ftrig )
    psdtrig20.append( ftrig20 )
    Height.append(-np.min(data))
    MeanTime.append(float(AverageMeanT))
    # ShowEvent(i)
Area_bgnd = np.array(Int)
PSD_bgndTrig  = np.array(psdtrig)
PSD_bgndTrig20  = np.array(psdtrig20)
Height_bgnd=np.array(Height)
MeanTime_bgnd=np.array(MeanTime)


matplotlib.use('Qt5Agg')
plt.ion()

# Calcul de l'énergie
Energy = -Area_bgnd / Calib * Cs137

##  Plot Energy vs PSD

fig = plt.figure(figsize=(20, 6))
gs = GridSpec(1, 3, figure=fig)
fig.suptitle('No cut', fontsize=24)
# Premier graphique : Energy vs PSD
ax1 = fig.add_subplot(gs[0, 0])
ax1.grid()
hist2 = ax1.hist2d(Energy, PSD_bgndTrig20, range=[[0, 4000], [0.1, 0.4]], bins=[300, 300], norm=LogNorm(), cmap=cmap)
ax1.tick_params(axis='both', which='both', labelsize=20, length=10, width=2)
ax1.set_xlabel('Energy (keV)', fontsize=20)
ax1.set_ylabel('PSD', fontsize=20)
cbar = plt.colorbar(hist2[3], ax=ax1)
cbar.set_label('Counts (Log scale)', fontsize=20)
cbar.ax.tick_params(labelsize=20)

##  Plot PSD vs Height

# Deuxième graphique : Height vs PSD
ax2 = fig.add_subplot(gs[0, 1])
ax2.grid()
hist2 = ax2.hist2d(Height_bgnd, PSD_bgndTrig20, range=[[0, 5000], [0.1, 0.4]], bins=[300, 300], norm=LogNorm(), cmap=cmap)
ax2.tick_params(axis='both', which='both', labelsize=20, length=10, width=2)
ax2.set_xlabel('Height (ADC count)', fontsize=20)
ax2.set_ylabel('PSD', fontsize=20)
cbar = plt.colorbar(hist2[3], ax=ax2)
cbar.set_label('Counts (Log scale)', fontsize=20)
cbar.ax.tick_params(labelsize=20)

##  Plot Average mean time vs Energy

# Troisième graphique : Average mean time vs Energy
ax3 = fig.add_subplot(gs[0, 2])
ax3.grid()
hist2 = ax3.hist2d(MeanTime_bgnd, Energy, range=[[100, 450], [0, 1500 * 4]], bins=[300, 300], norm=LogNorm(), cmap=cmap)
ax3.tick_params(axis='both', which='both', labelsize=20, length=10, width=2)
ax3.set_ylabel('Energy (keV)', fontsize=20)
ax3.set_xlabel('AWMT (ns)', fontsize=20)
cbar = plt.colorbar(hist2[3], ax=ax3)
cbar.set_label('Counts (Log scale)', fontsize=20)
cbar.ax.tick_params(labelsize=20)

# Ajuster les espaces entre les sous-graphiques
plt.tight_layout()

# Afficher la première figure
plt.show()

##  Plot PSD vs Energy Remove satuate
SpecialRange = np.where(Height_bgnd < 4000)[0]

fig2 = plt.figure(figsize=(20, 6))
gs2 = GridSpec(1, 2, figure=fig2)
fig2.suptitle('Height cut > 4000 ADC count', fontsize=24)

# Premier graphique : Energy vs PSD
ax4 = fig2.add_subplot(gs2[0, 0])
# Quatrième graphique : PSD vs Energy Remove satuate
ax4.grid()
hist2 = ax4.hist2d(Energy[SpecialRange], PSD_bgndTrig20[SpecialRange], range=[[0, 4000], [0.15, 0.4]], bins=[300, 300], norm=LogNorm(), cmap=cmap)
ax4.tick_params(axis='both', which='both', labelsize=20, length=10, width=2)
ax4.set_xlabel('Energy (keV)', fontsize=20)
ax4.set_ylabel('PSD', fontsize=20)
cbar = plt.colorbar(hist2[3], ax=ax4)
cbar.set_label('Counts (Log scale)', fontsize=20)
cbar.ax.tick_params(labelsize=20)

##  Plot Average mean time vs Energy Remove satuate

# Cinquième graphique : Average mean time vs Energy Remove satuate
ax5 = fig2.add_subplot(gs2[0, 1])
ax5.grid()
hist2 = ax5.hist2d(MeanTime_bgnd[SpecialRange], Energy[SpecialRange], range=[[100, 400], [0, 4000]], bins=[300, 300], norm=LogNorm(), cmap=cmap)
ax5.tick_params(axis='both', which='both', labelsize=20, length=10, width=2)
ax5.set_ylabel('Energy (keV)', fontsize=20)
ax5.set_xlabel('AWMT (ns)', fontsize=20)
cbar = plt.colorbar(hist2[3], ax=ax5)
cbar.set_label('Counts (Log scale)', fontsize=20)
cbar.ax.tick_params(labelsize=20)

# Ajuster les espaces entre les sous-graphiques
plt.tight_layout()

# Afficher la deuxième figure
plt.show()

fig, ax1 = plt.subplots(figsize=(24, 14));
# Premier graphique : Energy vs PSD
ax1.grid()
hist2 = ax1.hist2d(Energy, Height,  bins=[300, 300], norm=LogNorm(), cmap=cmap)
ax1.tick_params(axis='both', which='both', labelsize=20, length=10, width=2)
ax1.set_xlabel('Energy (keV)', fontsize=20)
ax1.set_ylabel('Height (ADC channel)', fontsize=20)
cbar = plt.colorbar(hist2[3], ax=ax1)
cbar.set_label('Counts (Log scale)', fontsize=20)
cbar.ax.tick_params(labelsize=20);plt.show()

##Energy spectrum

Spectrum(Area_bgnd,Calib,SpecialRange)

plt.ioff()