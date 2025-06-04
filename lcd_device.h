#ifndef LCD_DEVICE_H
#define LCD_DEVICE_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Forward declaration of libusb device handle
 */
typedef struct libusb_device_handle libusb_device_handle;

/**
 * Sets the orientation of the LCD display
 * @param handle The USB device handle
 * @param endpoint_address The USB endpoint address for communication
 * @param portrait True for portrait orientation, false for landscape
 * @return 0 on success, negative value on error
 */
int lcd_set_orientation(libusb_device_handle * const handle, const uint8_t endpoint_address, const bool portrait);

/**
 * Sets the current time on the LCD display
 * @param handle The USB device handle
 * @param endpoint_address The USB endpoint address for communication
 * @return 0 on success, negative value on error
 */
int lcd_set_time(libusb_device_handle * const handle, const uint8_t endpoint_address);

/**
 * Sends a heartbeat signal to the LCD display
 * @param handle The USB device handle
 * @param endpoint_address The USB endpoint address for communication
 * @return 0 on success, negative value on error
 */
int lcd_send_heartbeat(libusb_device_handle * const handle, const uint8_t endpoint_address);

/**
 * Redraws the entire LCD display
 * @param handle The USB device handle
 * @param endpoint_address The USB endpoint address for communication
 * @return 0 on success, negative value on error
 */
int lcd_redraw(libusb_device_handle * const handle, const uint8_t endpoint_address);

/**
 * Updates a portion of the LCD display
 * @param handle The USB device handle
 * @param endpoint_address The USB endpoint address for communication
 * @return 0 on success, negative value on error
 */
int lcd_update(libusb_device_handle * const handle, const uint8_t endpoint_address);

#endif                          /* LCD_DEVICE_H */
