cp ~/omnetpp-5.6.2/samples/labscim/simulations/wireless/nic/spectrumlog.txt ~
sed -i '$ d' ~/spectrumlog.txt
python3 ~/omnetpp-5.6.2/samples/labscim/src/spectrum_plotter/spectrum_plotter.py ~/spectrumlog.txt 
