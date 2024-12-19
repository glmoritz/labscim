#!/bin/bash
rm labscim-contiki-radio-protocol.h
ln -s $HOME/LabSCim/contiki-ng/arch/platform/labscim/labscim-contiki-radio-protocol.h
rm labscim-lora-radio-protocol.h 
ln -s  $HOME/LabSCim/LoRaMac-node/src/radio/labscim-lora-radio-protocol.h
rm labscim_contiking_setup.h
ln -s  $HOME/LabSCim/contiki-ng/arch/platform/labscim/labscim_contiking_setup.h
rm labscim_linked_list.cc
ln -s  $HOME/LabSCim/contiki-ng/arch/platform/labscim/labscim_linked_list.c labscim_linked_list.cc
rm labscim_linked_list.h 
ln -s  $HOME/LabSCim/contiki-ng/arch/platform/labscim/labscim_linked_list.h
rm labscim_protocol.h 
ln -s  $HOME/LabSCim/contiki-ng/arch/platform/labscim/labscim_protocol.h
rm labscim_socket.cc
ln -s  $HOME/LabSCim/contiki-ng/arch/platform/labscim/labscim_socket.c labscim_socket.cc
rm labscim_socket.h 
ln -s  $HOME/LabSCim/contiki-ng/arch/platform/labscim/labscim_socket.h
rm labscim_sx126x.h 
ln -s  $HOME/LabSCim/LoRaMac-node/src/radio/labscim_sx126x_driver/src/labscim_sx126x.h
rm labscim_sx126x_lr_fhss.h 
ln -s  $HOME/LabSCim/LoRaMac-node/src/radio/labscim_sx126x_driver/src/labscim_sx126x_lr_fhss.h
rm lora_gateway_setup.h 
ln -s  $HOME/LabSCim/lora_gateway/libloragw/inc/lora_gateway_setup.h
rm lr_fhss_mac.h 
ln -s  $HOME/LabSCim/LoRaMac-node/src/radio/labscim_sx126x_driver/src/lr_fhss_mac.h
rm lr_fhss_v1_base_types.h 
ln -s  $HOME/LabSCim/LoRaMac-node/src/radio/labscim_sx126x_driver/src/lr_fhss_v1_base_types.h
rm shared_mutex.cc 
ln -s  $HOME/LabSCim/contiki-ng/arch/platform/labscim/shared_mutex.c shared_mutex.cc
rm shared_mutex.h 
ln -s  $HOME/LabSCim/contiki-ng/arch/platform/labscim/shared_mutex.h
