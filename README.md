# LabSCim

The Internet of Things (IoT) has introduced diverse communication technologies operating in unlicensed Industrial, Scientific, and Medical (ISM) bands, with Long Range Wide Area Network (LoRaWAN) and IPv6 over the Time-Slotted Channel Hopping mode of IEEE 802.15.4e (6TiSCH) emerging as prominent standards. However, evaluating the performance and coexistence of such technologies is a complex task due to hardware costs and the vast parameter space involved.

To address these challenges, we present LabSCim, a novel OMNeT++-based simulation framework that integrates manufacturer reference code with advanced physical-layer modeling. LabSCim supports LoRaWAN, 6TiSCH, and other technologies with unprecedented detail, enabling:

- Byte-accurate compliance with specifications.

- Cross-technology interference modeling for realistic scenarios.

- Performance evaluation in high-density networks



# Documentation

- The installation instructions can be found [here](documentation/INSTALLATION.md);
- Contiki-NG usage examples can be found [CSMA](documentation/EXAMPLE_CSMA_CONTIKI.md) and [6TiSCH](documentation/EXAMPLE_6TiSCH_CONTIKI.md);
- LoRaWAN usage example can be found here [LoRaWAN](documentation/EXAMPLE_LoRaWAN.md).

# Structure

This is an adaptation layer to plug firmware of the major IoT platforms in a phy simulator running on omnet.
We plan to use the vanilla version from the github of the main platforms and provide at least timer and radio drivers to interface with the simulator, along with all modifications which are particular to every supported firmaware (forked from the official repositories or maybe pulled to the official one).

For now we provide drivers and logic for:

* IPv6 over 6TISCH (900MHz and 2.4GHz):

- Contiki-ng - https://github.com/contiki-ng
- Fork: https://github.com/glmoritz/contiki-ng

* IPv6 over 802.15.4 CSMA (900MHz and 2.4GHz):

- Contiki-ng - https://github.com/contiki-ng
- Fork: https://github.com/glmoritz/contiki-ng

* LoRa and LR-FHSS:

- LoraMac - https://github.com/Lora-net/LoRaMac-node
Fork: https://github.com/glmoritz/LoRaMac-node
  
- Lora Packet forwader (upper layers execute without modification) - https://github.com/Lora-net/packet_forwarder
- Fork: https://github.com/glmoritz/packet_forwarder

Maybe using the docker image is the place for start?
  https://hub.docker.com/r/glmoritz/labscim (generated using https://github.com/glmoritz/labscim-chirpstack-docker)

Using this is tricky because the documentation is not complete: if you are intested in using or extending this, please contact me at moritz at utfpr dot edu dot br

  
