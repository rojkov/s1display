#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct libusb_device_handle libusb_device_handle;

int lcd_set_orientation(libusb_device_handle * handle, uint8_t endpoint_address, bool portrait);
int lcd_set_time(libusb_device_handle * handle, uint8_t endpoint_address);
int lcd_send_heartbeat(libusb_device_handle * handle, uint8_t endpoint_address);
int lcd_redraw(libusb_device_handle * handle, uint8_t endpoint_address);
int lcd_update(libusb_device_handle * handle, uint8_t endpoint_address);
