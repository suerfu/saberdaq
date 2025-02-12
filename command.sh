
while ps -u suerfu | grep -q polaris
do
    sleep 1
    echo "polaris still running"
done


for i in {1..5}
    do

    DATE=$(date +"%Y%m%d")
    TIME=$(date +"%H%M")

#    OUTFILE=Run${i}_${DATE}_${TIME}_nai033_bgnd.raw
    OUTFILE=Stb${i}_${DATE}_${TIME}_ru001_cs137.raw

    CONFIG=cfg/nai_stability.cfg

    DURATION=10800

    COMMENT=''

    polaris --cfg ${CONFIG} --file ${OUTFILE} --time ${DURATION} --comment "Test of crystal light yield stability. 10-min runs will be taken consecutively over night with CS-137 source to observe change in light yield as function of time."
    #polaris --cfg ${CONFIG} --file ${OUTFILE} --time ${DURATION} --comment "TestNaI-033 octagon, rewrapped with 4-inch PTFE tape from RMD and coupled to two R11065-20 PMTs. The assembly is held in place with acrylic plates and stainless steel threaded rods inside 5-inch ID aluminum shell. Ch0 BC0033 at bottom at -1400 V; Ch1 BC0041 on the top at -1550 V. Ch2 and ch3 are plastic muon veto at -2000 V. Big plastic scintillator placed on the top and the other one on the side. Background run."

    mv ${OUTFILE} /sdbdata/2019_nai033_run2 &

    done
