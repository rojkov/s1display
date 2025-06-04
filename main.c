#include <stdio.h>
#include <unistd.h>             /* sleep() */
#include <signal.h>
#include <libusb.h>
#include <errno.h>

#include "log.h"
#include "lcd_device.h"

#define VENDOR_ID  0x04d9
#define PRODUCT_ID 0xfd01

/* Error codes */
enum {
    SUCCESS = 0,
    ERR_ARGS = -1,
    ERR_USB_INIT = -2,
    ERR_NO_DEVICE = -3,
    ERR_USB_CLAIM = -4,
    ERR_USB_CONFIG = -5
};

static volatile sig_atomic_t keep_running = 1;

static void sig_handler(int sig) {
    (void)sig;
    keep_running = 0;
}

static void usage(const char *progname) {
    printf("Usage: %s [-l trace|debug|info|warn|error]\n", progname);
}

int main(int argc, char *argv[]) {
    int ch;
    int ret = SUCCESS;
    libusb_device *dev = NULL;
    libusb_device **devs = NULL;
    struct libusb_device_descriptor desc;
    struct libusb_config_descriptor *config_desc = NULL;
    libusb_device_handle *handle = NULL;

    while ((ch = getopt(argc, argv, "l:")) != -1) {
        switch (ch) {
        case 'l':
            if (log_set_level_by_string(optarg) != 0) {
                usage(argv[0]);
                return ERR_ARGS;
            }
            break;
        case '?':
        default:
            usage(argv[0]);
            return ERR_ARGS;
        }
    }
    argc -= optind;
    argv += optind;

    log_info("Start");

    /* Set up signal handlers */
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1) {
        log_error("Failed to set up signal handlers");
        return ERR_ARGS;
    }

    int err = libusb_init_context(NULL, NULL, 0);
    if (err != 0) {
        log_error("unable to init USB context: %s", libusb_error_name(err));
        return ERR_USB_INIT;
    }

    int dev_count = libusb_get_device_list(NULL, &devs);
    if (dev_count < 0) {
        log_error("unable to get device list: %s", libusb_error_name(dev_count));
        ret = ERR_USB_INIT;
        goto cleanup_exit;
    } else if (dev_count == 0) {
        log_error("no USB devices found. Do you have enough permissions to list them?");
        ret = ERR_NO_DEVICE;
        goto cleanup_exit;
    }

    /* Find our device */
    for (int i = 0; devs[i] != NULL; i++) {
        err = libusb_get_device_descriptor(devs[i], &desc);
        if (err != 0) {
            log_error("unable to get device descriptor for device %d: %s",
                      i, libusb_error_name(err));
            continue;
        }

        if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID) {
            dev = devs[i];
            log_debug("found the required USB device");
            break;
        }
        log_trace("skipping device %04x:%04x", desc.idVendor, desc.idProduct);
    }

    if (dev == NULL) {
        log_error("the Holtek LCD screen is not found");
        ret = ERR_NO_DEVICE;
        goto cleanup_exit;
    }

    err = libusb_get_active_config_descriptor(dev, &config_desc);
    if (err != 0) {
        log_error("unable to get active configuration: %s", libusb_error_name(err));
        ret = ERR_USB_CONFIG;
        goto cleanup_exit;
    }

    /* Find the output endpoint */
    uint8_t endpoint_out = 0;
    int interface_num = -1;
    for (int i = 0; i < config_desc->bNumInterfaces; i++) {
        const struct libusb_interface *iface = &config_desc->interface[i];
        if (iface->num_altsetting != 1)
            continue;

        const struct libusb_interface_descriptor *iface_desc = &iface->altsetting[0];
        if (iface_desc->bNumEndpoints != 1)
            continue;

        const struct libusb_endpoint_descriptor *ep = &iface_desc->endpoint[0];
        if (!(ep->bEndpointAddress & LIBUSB_ENDPOINT_IN)) {
            endpoint_out = ep->bEndpointAddress;
            interface_num = i;
            log_debug("found OUT endpoint 0x%02x on interface %d", endpoint_out, i);
            break;
        }
    }

    if (endpoint_out == 0) {
        log_error("suitable endpoint not found");
        ret = ERR_USB_CONFIG;
        goto cleanup_exit;
    }

    err = libusb_open(dev, &handle);
    if (err) {
        log_error("unable to open device: %s", libusb_error_name(err));
        ret = ERR_USB_CLAIM;
        goto cleanup_exit;
    }

    int status = libusb_kernel_driver_active(handle, interface_num);
    if (status != 0) {
        log_error("unable to claim interface: %s",
                  status == 1 ? "kernel driver is active" : "unknown error");
        ret = ERR_USB_CLAIM;
        goto cleanup_handle;
    }

    err = libusb_claim_interface(handle, interface_num);
    if (err != 0) {
        log_error("unable to claim interface: %s", libusb_error_name(err));
        ret = ERR_USB_CLAIM;
        goto cleanup_handle;
    }

    /* Initialize the display */
    err = lcd_set_orientation(handle, endpoint_out, true);
    if (err < 0) {
        log_error("unable to set orientation: %s", libusb_error_name(err));
        ret = ERR_USB_CONFIG;
        goto cleanup_interface;
    }

    if (lcd_set_time(handle, endpoint_out) < 0) {
        ret = ERR_USB_CONFIG;
        goto cleanup_interface;
    }

    /* Paint it black */
    if (lcd_redraw(handle, endpoint_out) < 0) {
        ret = ERR_USB_CONFIG;
        goto cleanup_interface;
    }

    /* Main loop */
    while (keep_running) {
        log_trace("sending heartbeat");
        if (lcd_send_heartbeat(handle, endpoint_out) < 0) {
            ret = ERR_USB_CONFIG;
            break;
        }
        sleep(1);
    }

cleanup_interface:
    err = libusb_release_interface(handle, interface_num);
    if (err != 0) {
        log_error("unable to release interface: %s", libusb_error_name(err));
        ret = ERR_USB_CLAIM;
    }

cleanup_handle:
    if (handle) {
        libusb_close(handle);
    }

cleanup_exit:
    if (config_desc) {
        libusb_free_config_descriptor(config_desc);
    }
    if (devs) {
        libusb_free_device_list(devs, 1);
    }
    libusb_exit(NULL);
    log_info("Stop");

    return ret;
}
