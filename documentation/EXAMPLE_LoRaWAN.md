# LabSCim Simulator LoRaWAN Example Usage Guide

This guide provides step-by-step instructions to configure, run, and debug a simulation example for the LoRaWAN using LabSCim simulator. If you haven't done already, please follow the [installation guide](INSTALLATION.md).

In order to run the LoRaWAN, it is necessary to redirect the requests to a Network Server. The easyest to do it it through a docker container. If you have not yet installed, please install docker engine.

### 1. Running LoRaWAN Locally

Clone the github repository for the custom chirpstack docker for LabSCim:

```bash
cd $HOME/LabSCim && git clone https://github.com/glmoritz/labscim-chirpstack-docker
```

### 2. Start the docker container

Use the following command to start the docker container for chirpstack:

```bash
cd $HOME/LabSCim/labscim-chirpstack-docker && docker compose up -d
```

### 3. Update ChirpStack Database (Reset DevNonce)

Sometimes, devices may fail to join the network due to `DevNonce` reuse errors—especially during development or repeated testing with OTAA devices. To resolve this, you can reset the `DevNonce` history stored in the ChirpStack database. Run the following command:

```bash
cd $HOME/LabSCim/models/labscim/src && python3 flush_nonce.py
```

### 4. Test if Chirpstack is running

Try to login at the Chirpstack configuration panel using the address[http://localhost:8080/](http://localhost:8080/). The user and password are both 'admin'.


### 5. Open OMNeT++
If OMNeT++ is not already open, start it with the following commands:
```bash
cd $HOME/LabSCim/omnetpp-6.0.3 && source setenv && omnetpp
```

### 6. Import the Example Project
If you haven't done so already, import the examle project.
To import the example project into OMNeT++:
1. Navigate to **File -> Import**.
2. Select **General -> Existing Projects into Workspace**.
3. Set the **Select root directory** field to:
   ```
   $HOME/LabSCim/labscim/
   ```
4. Click **Finish**.

### 7. Locate Simulation Configuration
The simulation configurations can be found in the file 'labscim/simulations/wireless/nic/labscim.ini'

### 8. Configure the Simulation
The simulation configurations can be found in the file 'labscim/simulations/wireless/nic/labscim.ini'.
Open it, and change:

```bash
scheduler-class = "omnetpp::cRealTimeScheduler"
realtimescheduler-scaling = 1
```

### 9. Build and Debug the Project
To debug the project:
1. Click on the **Debug** icon in OMNeT++.
2. Select **OMNet++ Simulation**.
3. Select the configuration file 'labscim/simulations/wireless/nic/labscim.ini'.

### 10. Run the Simulation
After debugging, start the simulation by clicking on one of the **Play** icons available at the upper bar.

### 7. Access Simulation Logs
Once the simulation is complete, log files will be generated. These logs provide detailed information about the simulation.


