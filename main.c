#include <stdio.h>
#include <unistd.h>             /* sleep() */
#include <signal.h>
#include <libusb.h>

#include "log.h"
#include "lcd_device.h"

#define VENDOR_ID  0x04d9
#define PRODUCT_ID 0xfd01

static volatile sig_atomic_t keep_running = 1;

void sig_handler(int sig) {
    (void)sig;
    keep_running = 0;
}

void usage() {
    printf("Usage: s1display [-l trace|debug|info|warn|error]\n");
}

int main(int argc, char *argv[]) {
    int ch;
    int ret = 0;
    libusb_device *dev, **devs;
    struct libusb_device_descriptor desc;
    struct libusb_config_descriptor *config_desc = NULL;

    while ((ch = getopt(argc, argv, "l:")) != -1) {
        switch (ch) {
        case 'l':
            if (log_set_level_by_string(optarg) != 0) {
                usage();
                return 1;
            }
            break;
        case '?':
        default:
            usage();
            return 1;
        }
    }
    argc -= optind;
    argv += optind;

    log_info("Start");

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int err = libusb_init_context( /* ctx= */ NULL, /* options= */ NULL, /* num_options= */ 0);
    if (err != 0) {
        log_error("unable to init USB context: %s", libusb_error_name(err));
        return 1;
    }

    int dev_count = libusb_get_device_list(NULL, &devs);
    if (dev_count < 0) {
        log_error("unable to get device list: %s", libusb_error_name(dev_count));
        libusb_exit(NULL);
        return 1;
    } else if (dev_count == 0) {
        log_error("no USB devices found. Do you have enough permissions to list them?");
        libusb_exit(NULL);
        return 1;
    }

    for (int i; (dev = devs[i]) != NULL; i++) {
        err = libusb_get_device_descriptor(dev, &desc);
        if (err != 0) {
            log_error("unable to get device descriptor for the device number %d: %s", i, libusb_error_name(err));
            libusb_exit(NULL);
            return 1;
        }

        if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID) {
            log_debug("found the required USB device");
            err = libusb_get_active_config_descriptor(dev, &config_desc);
            if (err != 0) {
                log_error("unable to get active configuration: %s", libusb_error_name(err));
                ret = 1;
                goto free_device_list;
            }
            break;
        }

        log_trace("skipping the device %04x:%04x", desc.idVendor, desc.idProduct);
    }


    if (config_desc == NULL) {
        log_error("the Holtek LCD screen is not found");
        ret = 1;
        goto free_device_list;
    }

    log_debug("the device's configuration has got %d interfaces", config_desc->bNumInterfaces);
    uint8_t endpoint_out = 0;
    int interface_num = -1;
    for (int i = 0; i < config_desc->bNumInterfaces; i++) {
        struct libusb_interface iface = config_desc->interface[i];
        log_debug("interface %d has got %d altsettings", i, iface.num_altsetting);
        log_debug("interface %d has got %d endpoints", i, iface.altsetting[0].bNumEndpoints);
        if (iface.num_altsetting == 1 && iface.altsetting[0].bNumEndpoints == 1) {
            log_trace("endpoint address for interface %d is %02x", i, iface.altsetting[0].endpoint[0].bEndpointAddress);
            if (!(iface.altsetting[0].endpoint[0].bEndpointAddress & LIBUSB_ENDPOINT_IN)) {
                log_debug("found OUT endpoint");
                endpoint_out = iface.altsetting[0].endpoint[0].bEndpointAddress;
                interface_num = i;
                break;
            }
        }
    }

    if (endpoint_out == 0) {
        log_info("endpoint not found, exiting...");
        ret = 1;
        goto free_device_list;
    }

    libusb_device_handle *handle;

    err = libusb_open(dev, &handle);
    if (err) {
        log_error("unable to open device: %s", libusb_error_name(err));
        ret = 1;
        goto free_device_list;
    }

    int status = libusb_kernel_driver_active(handle, interface_num);
    if (status != 0) {
        log_error("unable to clame interface as %s", status == 1 ? "kernel driver is active" : "there's some error");
        libusb_close(handle);
        ret = 1;
        goto free_device_list;
    }

    err = libusb_claim_interface(handle, interface_num);
    if (err != 0) {
        log_error("unable to clame interface: %s", libusb_error_name(err));
        libusb_close(handle);
        ret = 1;
        goto free_device_list;
    }

    err = lcd_set_orientation(handle, endpoint_out, true);
    if (err < 0) {
        log_error("unable to submit transfer: %s", libusb_error_name(err));
        ret = 1;
        goto release_interface;
    }

    if (lcd_set_time(handle, endpoint_out) < 0) {
        ret = 1;
        goto release_interface;
    }

    /* Paint it in black. */
    if (lcd_redraw(handle, endpoint_out) < 0) {
        ret = 1;
        goto release_interface;
    }

    while (keep_running) {
        log_trace("in the loop before sleep");
        sleep(1);
        log_trace("in the loop after sleep");
        /* Send heartbeat. */
        if (lcd_send_heartbeat(handle, endpoint_out) < 0) {
            ret = 1;
            goto release_interface;
        }
    }

release_interface:
    err = libusb_release_interface(handle, interface_num);
    if (err != 0) {
        log_error("unable to release interface: %s", libusb_error_name(err));
		ret = 1;
    }

    libusb_close(handle);

free_device_list:
    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);
    log_info("Stop");

    return ret;
}
