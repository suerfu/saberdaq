
for i in {1..32}
do

    polaris --cfg ./baseline_gui.cfg --prefix 20250319_NaI_4GoKan/Baseline_Run${i} --time 30 --comment "Baseline monitoring in KEK 4-Go-kan, with CAEN VME8004B slot 03. Channel 0: NaI PMT anode, with x20 amplification, preamp biased at 5V. HV = -800V. Channel 1: open, for baseline noise monitoring. setup with reduced noise." --disable-input
    
    polaris --cfg ./NaI_gui.cfg --prefix 20250319_NaI_4GoKan/NaI_Background_Run${i}_HV800 --time 1800 --comment "NaI background data in KEK 4-Go-kan, with CAEN VME8004B slot 03. Channel 0: NaI PMT anode, with x20 amplification, preamp biased at 5V. HV = -800V. Channel 1: open, for baseline noise monitoring. Compared to last run: threshold further decreased by 100 ADC; new HDF5 format; setup with less noise." --disable-input

done
