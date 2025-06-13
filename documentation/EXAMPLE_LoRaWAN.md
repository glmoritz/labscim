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


### 3. Install Python dependencies

In order to run the LoRaWAN simulated application using Python, some dependencies are necessary. To solve this, execute:

```
python3 -m pip install paho-mqtt psycopg2
```

### 4. Test if Chirpstack is running

Try to login at the Chirpstack configuration panel using the address[http://localhost:8080/](http://localhost:8080/). The user and password are both 'admin'.


### 5. Start the Simulated Application

To start the application (from the LoRaWAN stack), we will execute a Python script that listens for every packet received in the uplink and replies with a corresponding downlink packet. To run it, use:

```bash
cd $HOME/LabSCim/PythonScripts && python3 application_reply.py
```

Note: This script also flushes DevNonce values.
During development or repeated testing with OTAA devices, it's common for devices to fail to join the network due to DevNonce reuse errors. If you only want to reset the DevNonce history stored in the ChirpStack database, run:

```
cd $HOME/LabSCim/PythonScripts && python3 flush_nonce.py
```


### 6. Open OMNeT++
If OMNeT++ is not already open, start it with the following commands:
```bash
cd $HOME/LabSCim/omnetpp-6.0.3 && source setenv && omnetpp
```

### 7. Import the Example Project
If you haven't done so already, import the examle project.
To import the example project into OMNeT++:
1. Navigate to **File -> Import**.
2. Select **General -> Existing Projects into Workspace**.
3. Set the **Select root directory** field to:
   ```
   $HOME/LabSCim/labscim/
   ```
4. Click **Finish**.

### 8. Locate Simulation Configuration
The simulation configurations can be found in the file 'labscim/simulations/wireless/nic/labscim-lora-fhss.ini'

### 9. Configure the Simulation
The simulation configurations can be found in the file 'labscim/simulations/wireless/nic/labscim-lora-fhss.ini'.
Note that the following lines are included and necessary (no need to change it):

```bash
scheduler-class = "omnetpp::cRealTimeScheduler"
realtimescheduler-scaling = 1
```

### 10. Build and Debug the Project
To debug the project:
1. Click on the **Debug** icon in OMNeT++.
2. Select **OMNet++ Simulation**.
3. Select the configuration file 'labscim/simulations/wireless/nic/labscim-lora-fhss.ini'.

### 11. Run the Simulation
After debugging, start the simulation by clicking on one of the **Play** icons available at the upper bar.

### 12. Access Simulation Logs
Once the simulation is complete, log files will be generated. These logs provide detailed information about the simulation.


