#include "lcd_device.h"

#include <stdlib.h>  // for alloca()
#include <string.h>  // for memset(), memcpy()
#include <time.h>    // for time_t, localtime(), time(), struct tm
#include <libusb.h>

#include "log.h"

#define MAX_STR 255
#define RGB565(r, g, b) (((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F))
#define SWAPENDIAN(num) (num>>8) | (num<<8);

#define BUFFER_SIZE 4104
#define DATA_SIZE 4096

#define LCD_SIGNATURE 0x55
#define LCD_CONFIG 0xA1
#define LCD_REFRESH 0xA2
#define LCD_REDRAW 0xA3

#define LCD_ORIENTATION 0xF1
#define LCD_HEARTBEAT 0xF2
#define LCD_SET_TIME 0xF3

#define LCD_LANDSCAPE 0x01
#define LCD_PORTRAIT 0x02

#define LCD_REDRAW_START 0xF0
#define LCD_REDRAW_CONTINUE 0xF1
#define LCD_REDRAW_END 0xF2

#define CHUNK_COUNT 27
#define FINAL_CHUNK_INDEX (CHUNK_COUNT - 1)
#define FINAL_CHUNK_SIZE 2304

int lcd_set_orientation(libusb_device_handle * handle, uint8_t endpoint_address, bool portrait) {
    uint8_t *buffer = (uint8_t *) alloca(BUFFER_SIZE);
    memset(buffer, 0, BUFFER_SIZE);
    uint8_t *header = buffer;

    struct set_orientation_cmd {
        uint8_t header;
        uint8_t command;
        uint8_t subcommand;
        uint8_t is_portrait;
        uint8_t unused[4];
    } __attribute__((packed)) cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.header = LCD_SIGNATURE;
    cmd.command = LCD_CONFIG;
    cmd.subcommand = LCD_ORIENTATION;
    cmd.is_portrait = portrait ? LCD_PORTRAIT : LCD_LANDSCAPE;
    memcpy(header, &cmd, sizeof(cmd));

    int transferred = 0;
    int err = libusb_bulk_transfer(handle, endpoint_address, buffer, BUFFER_SIZE, &transferred, 0);
    if (err < 0) {
        log_error("unable to submit transfer: %s", libusb_error_name(err));
        return err;
    }

    log_trace("transferred: %d", transferred);

    return err;
}

static int set_time(libusb_device_handle * handle, uint8_t endpoint_address, bool is_hartbeat) {
    time_t t = time(NULL);
    if (t == -1) {
        return -1;
    }
    struct tm *now = localtime(&t);

    uint8_t *buffer = (uint8_t *) alloca(BUFFER_SIZE);
    memset(buffer, 0, BUFFER_SIZE);
    uint8_t *header = buffer;

    struct set_time_cmd {
        uint8_t header;
        uint8_t command;
        uint8_t subcommand;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
        uint8_t unused[2];
    } __attribute__((packed)) cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.header = LCD_SIGNATURE;
    cmd.command = LCD_CONFIG;
    cmd.subcommand = is_hartbeat ? LCD_HEARTBEAT : LCD_SET_TIME;
    cmd.hour = now->tm_hour;
    cmd.minute = now->tm_min;
    cmd.second = now->tm_sec;
    memcpy(header, &cmd, sizeof(cmd));

    int transferred = 0;
    int err = libusb_bulk_transfer(handle, endpoint_address, buffer, BUFFER_SIZE, &transferred, 0);
    if (err < 0) {
        log_error("unable to submit transfer: %s", libusb_error_name(err));
        return err;
    }

    log_trace("transferred: %d", transferred);

    return err;
}

int lcd_set_time(libusb_device_handle * handle, uint8_t endpoint_address) {
    return set_time(handle, endpoint_address, false);
}
int lcd_send_heartbeat(libusb_device_handle * handle, uint8_t endpoint_address) {
    return set_time(handle, endpoint_address, true);
}

int lcd_redraw(libusb_device_handle * handle, uint8_t endpoint_address) {
    uint8_t *buffer = (uint8_t *) alloca(BUFFER_SIZE);
    uint8_t *imgbuf = (uint8_t *) alloca(DATA_SIZE);

    memset(buffer, 0, BUFFER_SIZE);
    uint8_t *header = buffer;

    for (size_t i = 0; i < DATA_SIZE; i += 2) {
        uint16_t *pixel = (uint16_t *) (imgbuf + i);
        uint16_t color = 0;
        uint8_t intensity = 0;
        color = RGB565(intensity, 0, 0);
        *pixel = SWAPENDIAN(color);
    }

    struct redraw_cmd {
        uint8_t header;
        uint8_t command;
        uint8_t subcommand;
        uint8_t sequence;
        uint16_t offset;
        uint16_t length;
    } __attribute__((packed)) cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.header = LCD_SIGNATURE;
    cmd.command = LCD_REDRAW;

    for (size_t i = 0; i < CHUNK_COUNT; i++) {
        switch (i) {
        case 0:
            cmd.subcommand = LCD_REDRAW_START;
            break;
        case FINAL_CHUNK_INDEX:
            cmd.subcommand = LCD_REDRAW_END;
            break;
        default:
            cmd.subcommand = LCD_REDRAW_CONTINUE;
            break;
        }
        cmd.sequence = i + 1;
        cmd.offset = i * DATA_SIZE;
        const uint16_t length = (i < FINAL_CHUNK_INDEX) ? DATA_SIZE : FINAL_CHUNK_SIZE;
        cmd.length = length;
        uint8_t *databuf = mempcpy(header, &cmd, sizeof(cmd));
        memcpy(databuf, imgbuf, DATA_SIZE);

        int transferred = 0;
        int err = libusb_bulk_transfer(handle, endpoint_address, buffer, BUFFER_SIZE, &transferred, 0);
        if (err < 0) {
            log_error("unable to submit transfer: %s", libusb_error_name(err));
            return err;
        }
    }

    return 0;
}

int lcd_update(libusb_device_handle * handle, uint8_t endpoint_address) {
    uint8_t *buffer = (uint8_t *) alloca(BUFFER_SIZE);
    memset(buffer, 0, BUFFER_SIZE);
    uint8_t *header = buffer;

    struct lcd_update {
        uint8_t header;
        uint8_t command;
        uint16_t x;
        uint16_t y;
        uint8_t w;
        uint8_t h;
    } __attribute__((packed)) cmd;
    cmd.header = LCD_SIGNATURE;
    cmd.command = LCD_REFRESH;
    cmd.x = 90;
    cmd.y = 90;
    cmd.w = 24;
    cmd.h = 12;
    uint8_t *img = mempcpy(header, &cmd, sizeof(cmd));

    for (int x = 0; x < 24; x++) {
        for (int y = 0; y < 12; y++) {
            uint16_t *pixel = (uint16_t *) img;
            uint16_t color = 0;
            uint8_t intensity = 255;
            color = RGB565(intensity, 0, 0);
            *pixel = SWAPENDIAN(color);
            img += 2;
        }
    }

    int transferred = 0;
    int err = libusb_bulk_transfer(handle, endpoint_address, buffer, BUFFER_SIZE, &transferred, 0);
    if (err < 0) {
        log_error("unable to submit transfer: %s", libusb_error_name(err));
        return err;
    }

    log_trace("transferred: %d", transferred);

    return err;
}
