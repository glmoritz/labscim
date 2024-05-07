# labscim
LabSC Channel Simulator

This is an adaptation layer to plug firmware of the major IoT platforms in a phy simulator running on omnet.
We plan to use the vanilla version from the github of the main platforms and provide at least timer and radio drivers to interface with the simulator, along with all modifications which are particular to every supported firmaware (forked from the official repositories or maybe pulled to the official one).

For now we provide drivers and logic for:

* IPv6 over 6TISCH (900MHz and 2.4GHz):

- Contiki-ng - https://github.com/contiki-ng

* IPv6 over 802.15.4 CSMA (900MHz and 2.4GHz):

- Contiki-ng - https://github.com/contiki-ng

* LoRa and LR-FHSS:

- LoraMac - https://github.com/Lora-net/LoRaMac-node
- Lora Packet forwader (we hope that the upper layers may execute without modification) - https://github.com/Lora-net/packet_forwarder
