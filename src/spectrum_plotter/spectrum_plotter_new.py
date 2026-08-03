# Import the necessary packages and modules
from os import minor
import sys
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.collections import PatchCollection
from matplotlib.patches import Rectangle
from matplotlib.patches import Circle
import numpy as np
import enum
import matplotlib.ticker as ticker

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
        self.tx_events = []
        self.tx_correct_patches = []        
        self.listen_patches = []   
        self.power_patches = []   
        self.power_values = []   
        self.max = -1
        self.min = -1
        # Occupancy range, tracked separately from the range above: it counts only what was
        # actually emitted or received as power, and ignores the band a radio merely sat
        # listening on. The distinction matters -- a LoRa gateway starts up listening around
        # 903 MHz before the packet forwarder hands it the channel plan, and letting that one
        # value into the axis limits stretches the plot over 12 MHz of empty space.
        self.occupied_max = -1
        self.occupied_min = -1

    def update_range(self, center_frequency_hz, bandwidth_hz):
        if self.min == -1 or (center_frequency_hz-bandwidth_hz/2) < self.min:
            self.min = center_frequency_hz-bandwidth_hz/2

        if self.max == -1 or (center_frequency_hz+bandwidth_hz/2) > self.max:
            self.max = (center_frequency_hz+bandwidth_hz/2)

    def update_occupancy(self, center_frequency_hz, bandwidth_hz):
        if self.occupied_min == -1 or (center_frequency_hz-bandwidth_hz/2) < self.occupied_min:
            self.occupied_min = center_frequency_hz-bandwidth_hz/2

        if self.occupied_max == -1 or (center_frequency_hz+bandwidth_hz/2) > self.occupied_max:
            self.occupied_max = center_frequency_hz+bandwidth_hz/2


    def AddTxPatch(self, start_t, end_t, center_frequency_hz, bandwidth_hz):
        self.update_range(center_frequency_hz,bandwidth_hz)
        self.update_occupancy(center_frequency_hz,bandwidth_hz)
        # When and where each transmission happened, for picking the tick grid later.
        self.tx_events.append((start_t, center_frequency_hz))
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
        # packetSentToUpperSignal carries the band in a SignalBandInd tag, and the recorder
        # writes zeros when the packet has no such tag -- which is every TSCH packet. A
        # rectangle at 0 Hz with zero height draws nothing anyway, but it used to drag the
        # axis limits down to DC and squash the whole plot into the top pixel row.
        if center_frequency_hz <= 0 or bandwidth_hz <= 0:
            return
        self.update_range(center_frequency_hz,bandwidth_hz)
        self.update_occupancy(center_frequency_hz,bandwidth_hz)
        p = Rectangle((start_t,center_frequency_hz - (bandwidth_hz/2)),end_t - start_t,bandwidth_hz)
        self.tx_correct_patches.append(p)

    def AddPowerPatch(self, start_t, end_t, center_frequency_hz, bandwidth_hz, power):
        self.update_range(center_frequency_hz,bandwidth_hz)
        self.update_occupancy(center_frequency_hz,bandwidth_hz)
        p = Rectangle((start_t,center_frequency_hz - (bandwidth_hz/2)),end_t - start_t,bandwidth_hz)
        self.power_patches.append(p)
        self.power_values.append(power)
    





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


file2 = open(sys.argv[2], 'r')
Lines = file2.readlines()
count = 0

for line in Lines:
    count += 1
    data = line.split(",")    

    if data[0].strip() == "POW":
        specgram.AddPowerPatch(float(data[1]),float(data[2]),float(data[3]),float(data[4]),float(data[5]))        

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


# Frequency limits, fitted to what the run actually used. They were fixed at 915.1-927.9 MHz,
# which wasted most of the panel on a TSCH-only run, living in 920.6-927.8 MHz, and would have
# cropped anything outside AU915 without saying so. The limits now come from the occupancy
# range, which excludes bands a radio only listened on; pass "lo,hi" in MHz as a third
# argument to force them. The ticks below are left exactly as they were -- only the limits
# moved, and a tick outside them simply does not get drawn.
if len(sys.argv) > 3 and sys.argv[3]:
    fmin_hz, fmax_hz = [float(v)*1e6 for v in sys.argv[3].split(',')]
else:
    fmin_hz, fmax_hz = specgram.occupied_min, specgram.occupied_max
    if fmin_hz == -1:   # nothing was ever transmitted -- fall back to what was tuned
        fmin_hz, fmax_hz = specgram.min, specgram.max
    margin = max((fmax_hz - fmin_hz)*0.02, 100e3)
    fmin_hz = fmin_hz - margin
    fmax_hz = fmax_hz + margin

# Time axis. Both panels used to span the whole run, which hides short frames: a 802.15.4
# frame is around 2 ms, so over a 400 s run it is 5 ppm of the width and cannot be drawn.
# Pass "t0,t1" in seconds as a fourth argument to zoom. The patches are still built from the
# whole log -- this only sets the view -- so a frame that starts before t0 is still drawn,
# clipped, instead of disappearing.
if len(sys.argv) > 4 and sys.argv[4]:
    tmin, tmax = [float(v) for v in sys.argv[4].split(',')]
else:
    tmin, tmax = 0, max_t

ax[0].set(xlim=(tmin, tmax), ylim=(fmin_hz, fmax_hz))

scale_y = 1e6
ticks_y = ticker.FuncFormatter(lambda x, pos: '{0:g}'.format(x/scale_y))
ax[0].yaxis.set_major_formatter(ticks_y)

# ----------------------------------------------------------------------------------------
# Channel grid, chosen from what the run actually did.
#
# The original drew its lines on the channel BOUNDARIES rather than on the centres, so every
# channel gets a lane of its own and a transmission sits inside one. That is kept. What is
# new is that the grid is no longer the whole AU915 plan regardless of the scenario: a
# TSCH-only run got lines for LoRa channels it never touched, and a LoRa run in one sub-band
# got the other seven as well.
#
# Which plan applies is read off the module names in the log -- lorahost, contikinghost, or
# both -- and, for LoRa, which sub-band from the frequencies actually transmitted on.
# ----------------------------------------------------------------------------------------

def edges(centres, spacing):
    """Boundaries between adjacent channels: one line on each side of every centre."""
    return [c - spacing/2 for c in centres] + [centres[-1] + spacing/2]

AU915_UL125 = [915.2e6 + 200e3*n for n in range(64)]  # uplink, 125 kHz, 8 per sub-band
AU915_UL500 = [915.9e6 + 1.6e6*n for n in range(8)]   # uplink, 500 kHz, 1 per sub-band
AU915_DL500 = [923.3e6 + 600e3*n for n in range(8)]   # downlink, 500 kHz, RX1 grid
TSCH_CH     = [920.6e6 + 200e3*n for n in range(37)]  # 802.15.4, 37 channels, 200 kHz

names = " ".join(nodes.keys())
has_lora = "lorahost" in names
has_tsch = "contikinghost" in names

def subbands_used(events, tol=50e3, share=0.10):
    """AU915 sub-bands the run works in. Sub-band k is 125 kHz channels 8k..8k+7 plus the
    500 kHz channel 64+k, so either kind of uplink identifies it.

    Two filters, because the raw set of frequencies is all eight sub-bands on any run long
    enough to include a join: an end device sweeps the whole plan until the server answers,
    and drawing 64 channels of lanes for that buries the one sub-band the network settles
    into. Only transmissions inside the displayed window count, and a sub-band has to carry
    at least a tenth of them."""
    counts = {}
    total = 0
    for t, f in events:
        if not (tmin <= t <= tmax):
            continue
        for n, c in enumerate(AU915_UL125):
            if abs(f - c) < tol:
                counts[n // 8] = counts.get(n // 8, 0) + 1
                total += 1
        for n, c in enumerate(AU915_UL500):
            if abs(f - c) < tol:
                counts[n] = counts.get(n, 0) + 1
                total += 1
    if not total:
        return []
    return sorted(k for k, v in counts.items() if v >= share*total)

minor_ticks, major_ticks = [], []

if has_lora:
    # Uplink: only the sub-bands in use, as 200 kHz lanes. Downlink: the RX1 grid as 600 kHz
    # lanes, which is the split the original had and the one that reads best -- the two are
    # far enough apart that neither crowds the other.
    for k in subbands_used(specgram.tx_events):
        minor_ticks += edges(AU915_UL125[8*k:8*k + 8], 200e3)
    major_ticks += edges(AU915_DL500, 600e3)

if has_tsch:
    # All 37 channels as lanes. Labelling every one is unreadable, so every fifth boundary --
    # one line per MHz -- carries the label.
    tsch_edges = edges(TSCH_CH, 200e3)
    minor_ticks += tsch_edges
    if not has_lora:
        major_ticks += tsch_edges[::5]

# Coexistence: the uplink sub-band sits below 920.5 MHz and never meets TSCH, but the LoRa
# downlink band lies entirely inside the TSCH range. Keeping the downlink grid as the major
# lines puts them 100 kHz off every TSCH boundary, so they never coincide, and the offset is
# the point of the figure: a 500 kHz downlink straddles two and a half TSCH channels.

if not minor_ticks and not major_ticks:   # neither plan recognised -- let matplotlib decide
    ax[0].yaxis.set_minor_locator(ticker.AutoMinorLocator())
else:
    ax[0].set_yticks(sorted(set(minor_ticks)), minor=True)
    ax[0].set_yticks(sorted(set(major_ticks)))

ax[0].grid(which='major', color='#666666', linestyle='-')
ax[0].grid(which='minor', color='#333333', linestyle='--')

# set_yticks widens the axis to hold every tick it was given, so the fitted limits have to be
# applied again after it. The ticks themselves are untouched; the ones outside just are not
# drawn.
ax[0].set_ylim(fmin_hz, fmax_hz)


pc = PatchCollection(specgram.power_patches, cmap=matplotlib.cm.jet, alpha=0.5, edgecolor='None')
colors = 100*np.random.random(65)
norm_power = specgram.power_values/np.linalg.norm(specgram.power_values)
pc.set_array(np.array(specgram.power_values))
#pc.set_array(colors)
# Add collection to axes
ax[0].add_collection(pc)
cb = fig.colorbar(pc, ax=ax[0], fraction=0.02)
cb.ax.set_title('dBmW / MHz')
fig.colorbar(pc, ax=ax[1], fraction=0.02)

pc = PatchCollection(specgram.tx_correct_patches, facecolor='None', alpha=0.5, edgecolor='k')
# Add collection to axes
ax[0].add_collection(pc)





patch_width = next(iter(nodes.items()))[1].patch_width
patch_margin = next(iter(nodes.items()))[1].patch_margin


#ax[0].set(xlim=(0, max_t), ylim=(specgram.min, specgram.max))
ax[1].set(xlim=(tmin, tmax), ylim=(0, len(nodes)*(patch_margin+patch_width)))

a = (patch_margin+patch_width)*np.array([x for x in range(len(nodes))])+((patch_margin+patch_width)/2)
ax[1].set_yticks(a.tolist())     
ax[1].set_yticklabels(nodes.keys())
ax[1].set_xlabel("t (s)")
ax[0].set_xlabel("t (s)")
ax[0].set_ylabel("f (MHz)")



# Add a legend
#plt.legend()

# Show the plot
plt.show()
