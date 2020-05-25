# labscim
LabSC Channel Simulator

This is an adaptation layer to plug firmware of the major IoT platforms in a phy simulator running on omnet.
We plan to use the vanilla version from the github of the main platforms and provide at least timer and radio drivers to interface with the simulator, along with all modifications which are particular to every supported firmaware (forked from the official repositories or maybe pulled to the official one).

For now we are planning to provide drivers and logic for:

* 6TISCH, and Hopefully CSL (900MHz and 2.4GHz):

- Contiki-ng - https://github.com/contiki-ng
- 6lbr (we hope that it would compile with the same driver) - https://github.com/cetic/6lbr/wiki
- Contiki (we hope that minor modifications should be provided to backport our drivers for classic contiki, which is the only that supports Duty Cycled CSMA) 

* LoRa:

- LoraMac - https://github.com/Lora-net/LoRaMac-node
- Lora Packet forwader (we hope that the upper layers may execute without modification) - https://github.com/Lora-net/packet_forwarder

* Bluetooth and Thread:

- Zephyr-os - https://github.com/zephyrproject-rtos/zephyr

Maybe in the future we (or the community) can provide any 802.11, Riot and OpenWSN drivers
