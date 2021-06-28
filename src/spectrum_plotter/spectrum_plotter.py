# Import the necessary packages and modules
from os import minor
import sys
import matplotlib.pyplot as plt
from matplotlib.collections import PatchCollection
from matplotlib.patches import Rectangle
from matplotlib.patches import Circle
import numpy as np
import enum

class RadioMode(enum.Enum):        
    RADIO_MODE_OFF=0        
    RADIO_MODE_SLEEP=1        
    RADIO_MODE_RECEIVER=2        
    RADIO_MODE_TRANSMITTER=3    
    RADIO_MODE_TRANSCEIVER=4        
    RADIO_MODE_SWITCHING=5

class ReceptionState(enum.Enum):        
    RECEPTION_STATE_UNDEFINED=0
    RECEPTION_STATE_IDLE=1
    RECEPTION_STATE_BUSY=2
    RECEPTION_STATE_RECEIVING=3
    
class TransmissionState(enum.Enum):        
    TRANSMISSION_STATE_UNDEFINED=0
    TRANSMISSION_STATE_IDLE=1
    TRANSMISSION_STATE_TRANSMITTING=2  


class Radio:
    def __init__(self, spectrum, position, patch_width=30, patch_margin = 2):
        self.spectrum = spectrum
        self.radiomode_y = [RadioMode.RADIO_MODE_OFF]
        self.radiomode_x = [0]
        self.txmode_y = [TransmissionState.TRANSMISSION_STATE_UNDEFINED]
        self.txmode_x = [0]
        self.rxmode_y = [ReceptionState.RECEPTION_STATE_UNDEFINED]
        self.rxmode_x = [0]
        self.last_bw_hz = 0
        self.last_cf_hz = 0
        self.position = position
        self.patch_width = patch_width
        self.patch_margin = patch_margin
        self.rx_idle_patches = []
        self.rx_busy_patches = []
        self.rx_receiving_patches = []
        self.tx_idle_patches = []        
        self.tx_transmitting_patches = []         
        self.lasttransition = 0
        
  
    def ModeChange(self, t, value, bandwidth_hz=None, center_frequency_hz = None):
        self.AddPatch(t)
        if value != RadioMode.RADIO_MODE_RECEIVER and value != RadioMode.RADIO_MODE_TRANSCEIVER: 
            if self.radiomode_y[-1] == RadioMode.RADIO_MODE_RECEIVER or self.radiomode_y[-1] == RadioMode.RADIO_MODE_TRANSCEIVER:
                #rx patch
                if t > self.radiomode_x[-1]:
                    self.spectrum.AddRxPatch(self.radiomode_x[-1],t,self.last_cf_hz,self.last_bw_hz)
        
        self.radiomode_x.append(t)
        self.radiomode_y.append(value)
        
        if value == RadioMode.RADIO_MODE_RECEIVER:
            self.last_bw_hz = bandwidth_hz
            self.last_cf_hz = center_frequency_hz        

    def RxModeChange(self, t, value):        
        self.AddPatch(t)
        self.rxmode_x.append(t)
        self.rxmode_y.append(value)

    def TxModeChange(self, t, value):
        self.AddPatch(t)
        self.txmode_x.append(t)
        self.txmode_y.append(value)

    def AddPatch(self,t):

        if self.radiomode_y[-1] == RadioMode.RADIO_MODE_RECEIVER or self.radiomode_y[-1] == RadioMode.RADIO_MODE_TRANSCEIVER:
            start_t = max(self.rxmode_x[-1], self.radiomode_x[-1])                      
            if t-start_t > 0:            
                p = Rectangle((start_t,self.position*(self.patch_width+self.patch_margin)),t-start_t,self.patch_width)            
            
                if self.rxmode_y[-1] == ReceptionState.RECEPTION_STATE_IDLE:            
                    self.rx_idle_patches.append(p)

                if self.rxmode_y[-1] == ReceptionState.RECEPTION_STATE_BUSY:            
                    self.rx_busy_patches.append(p)
                
                if self.rxmode_y[-1] == ReceptionState.RECEPTION_STATE_RECEIVING:            
                    self.rx_receiving_patches.append(p)

        if self.radiomode_y[-1] == RadioMode.RADIO_MODE_TRANSMITTER:
            start_t = max(self.txmode_x[-1], self.radiomode_x[-1])                      
            if t-start_t > 0:
                p = Rectangle((start_t,self.position*(self.patch_width+self.patch_margin)),t-start_t,self.patch_width)
                
                if self.txmode_y[-1] == TransmissionState.TRANSMISSION_STATE_IDLE:                                                      
                    self.tx_idle_patches.append(p)

                if self.txmode_y[-1] == TransmissionState.TRANSMISSION_STATE_TRANSMITTING:                                            
                    self.tx_transmitting_patches.append(p)       
        
        self.lasttransition = t

class Spectrum:
    def __init__(self):
        self.rx_patches = []
        self.tx_patches = []
        self.tx_correct_patches = []        
        self.listen_patches = []   
        self.max = -1
        self.min = -1     

    def update_range(self, center_frequency_hz, bandwidth_hz):
        if self.min == -1 or (center_frequency_hz-bandwidth_hz/2) < self.min:
            self.min = center_frequency_hz-bandwidth_hz/2
        
        if self.max == -1 or (center_frequency_hz+bandwidth_hz/2) > self.max:
            self.max = (center_frequency_hz+bandwidth_hz/2)       

  
    def AddTxPatch(self, start_t, end_t, center_frequency_hz, bandwidth_hz):
        self.update_range(center_frequency_hz,bandwidth_hz)    
        p = Rectangle((start_t,center_frequency_hz - (bandwidth_hz/2)),end_t - start_t,bandwidth_hz)
        self.tx_patches.append(p)

    def AddRxPatch(self, start_t, end_t, center_frequency_hz, bandwidth_hz):
        self.update_range(center_frequency_hz,bandwidth_hz)    
        p = Rectangle((start_t,center_frequency_hz - (bandwidth_hz/2)),end_t - start_t,bandwidth_hz)
        self.rx_patches.append(p)        

    def AddListenPatch(self, start_t, end_t, center_frequency_hz, bandwidth_hz):
        self.update_range(center_frequency_hz,bandwidth_hz)    
        p = Rectangle((start_t,center_frequency_hz - (bandwidth_hz/2)),end_t - start_t,bandwidth_hz)
        self.rx_patches.append(p)
    
    def AddRxCorrectCircle(self, start_t, end_t, center_frequency_hz, bandwidth_hz):
        self.update_range(center_frequency_hz,bandwidth_hz)    
        p = Rectangle((start_t,center_frequency_hz - (bandwidth_hz/2)),end_t - start_t,bandwidth_hz)
        self.tx_correct_patches.append(p)
    





file1 = open(sys.argv[1], 'r')
Lines = file1.readlines()

nodes  = {}
count = 0
specgram = Spectrum()
max_t = 0

# Strips the newline character
for line in Lines:
    count += 1
    data = line.split(",")
    max_t = float(data[1])

    if data[2].strip() == "IRadio::radioModeChangedSignal":
        if data[3].strip() in nodes:
            n = nodes[data[3].strip()]            
        else:
            n = Radio(specgram,len(nodes))
            nodes[data[3].strip()] = n
        
        if RadioMode(int(data[4])) == RadioMode.RADIO_MODE_RECEIVER:
            n.ModeChange(float(data[1]),RadioMode(int(data[4])),float(data[6]),float(data[5])) 
        else:
            n.ModeChange(float(data[1]),RadioMode(int(data[4])))

    if data[2].strip() == "IRadio::receptionStateChangedSignal":
        if data[3].strip() in nodes:
            n = nodes[data[3].strip()]
        else:
            n = Radio(specgram,len(nodes))            
            nodes[data[3].strip()] = n 
        
        n.RxModeChange(float(data[1]),ReceptionState(int(data[4])))

    if data[2].strip() == "IRadio::transmissionStateChangedSignal":
        if data[3].strip() in nodes:
            n = nodes[data[3].strip()]
        else:
            n = Radio(specgram,len(nodes))            
            nodes[data[3].strip()] = n 
        
        n.TxModeChange(float(data[1]),TransmissionState(int(data[4])))

    if data[2].strip() == "IRadio::transmissionEndedSignal":
        specgram.AddTxPatch(float(data[4]),float(data[5]),float(data[6]),float(data[7]))

    if data[2].strip() == "IRadio::receptionEndedSignal":
        specgram.AddRxPatch(float(data[4]),float(data[5]),float(data[6]),float(data[7]))       

    if data[2].strip() == "packetSentToUpperSignal":
        specgram.AddRxCorrectCircle(float(data[4]),float(data[1]),float(data[5]),float(data[6]))       



    print("Line{}: {}".format(count, line.strip()))

#fig, ax = plt.subplots(2)
fig, ax = plt.subplots(nrows=2, sharex=True, sharey=False)




#ax.plot([0, 20],[900000, 900000])
for n in nodes.items():
    n[1].ModeChange(max_t,RadioMode.RADIO_MODE_OFF) #finalize any pending patch

    color = "#7EC8E3"    
    pc = PatchCollection(n[1].rx_idle_patches, facecolor=color, alpha=0.7, edgecolor='None')
    ax[1].add_collection(pc)

    color = "#0000FF"    
    pc = PatchCollection(n[1].rx_busy_patches, facecolor=color, alpha=0.7, edgecolor='None')
    ax[1].add_collection(pc)

    color = "#000C66"    
    pc = PatchCollection(n[1].rx_receiving_patches, facecolor=color, alpha=0.7, edgecolor='None')
    ax[1].add_collection(pc)

    color = "#ECF87F"    
    pc = PatchCollection(n[1].tx_idle_patches, facecolor=color, alpha=0.7, edgecolor='None')
    ax[1].add_collection(pc)

    color = "#81B622"    
    pc = PatchCollection(n[1].tx_transmitting_patches, facecolor=color, alpha=0.7, edgecolor='None')
    ax[1].add_collection(pc)

pc = PatchCollection(specgram.rx_patches, facecolor='None', alpha=1, edgecolor='k')
ax[0].add_collection(pc)
#ax[0].set(xlim=(0, max_t), ylim=(specgram.min, specgram.max))


#ticks for AU915
ax[0].set(xlim=(0, max_t), ylim=(915.1e6, 927.9e6))
#ytm = np.linspace(915.1e6, 927.9e6, 65)
ytm = np.linspace(915.1e6, 922.9e6, 40)
#ytmajor = np.linspace(915.1e6,927.9e6, 9)
ytmajor = np.linspace(915.1e6,921.5e6, 5)
ytm2 = np.linspace(923e6, 927.8e6, 9)
ax[0].set_yticks(ytm, minor=True)
ax[0].set_yticks( np.concatenate([ytmajor,ytm2]))
ax[0].grid(which='major', color='#666666', linestyle='-')
ax[0].grid(which='minor', color='#333333', linestyle='--')


pc = PatchCollection(specgram.tx_patches, facecolor='r', alpha=0.5, edgecolor='None')
# Add collection to axes
ax[0].add_collection(pc)

pc = PatchCollection(specgram.tx_correct_patches, facecolor='None', alpha=0.5, edgecolor='k')
# Add collection to axes
ax[0].add_collection(pc)





patch_width = next(iter(nodes.items()))[1].patch_width
patch_margin = next(iter(nodes.items()))[1].patch_margin


#ax[0].set(xlim=(0, max_t), ylim=(specgram.min, specgram.max))
ax[1].set(xlim=(0, max_t), ylim=(0, len(nodes)*(patch_margin+patch_width)))

a = (patch_margin+patch_width)*np.array([x for x in range(len(nodes))])+((patch_margin+patch_width)/2)
ax[1].set_yticks(a.tolist())     
ax[1].set_yticklabels(nodes.keys())
ax[1].set_xlabel("t (s)")
ax[0].set_xlabel("t (s)")
ax[0].set_ylabel("f (Hz)")



# Add a legend
#plt.legend()

# Show the plot
plt.show()
