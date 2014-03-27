/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */

// Set this to the name or IP address you want to find.
// Examples:
//  #define DNS_QUERY "mozilla.org"
//  #define DNS_QUERY "8.8.8.8"
#define DNS_QUERY "www.google.com"

#include <Arduino.h>
// These are needed for auto-config and Arduino
#include <Wire.h>
#include <SPI.h>

#ifndef USE_MULTIPLE_APP_API
#define USE_MULTIPLE_APP_API 16
#endif

#include <xmem.h>
#include <RTClib.h>
#include <usbhub.h>
#include <masstorage.h>
#include <stdio.h>
#include <xmemUSB.h>
#include <xmemUSBFS.h>
#include <Storage.h>
#include <ajkstack.h>

USB Usb;
USBHub Hub(&Usb);

void start_system(void) {
        uint8_t fd = _VOLUMES;
        uint8_t slots = 0;
        uint8_t *ptr;
        uint8_t spin = 0;
        uint8_t last = _VOLUMES;
        char fancy[4];

        fancy[0] = '-';
        fancy[1] = '\\';
        fancy[2] = '|';
        fancy[3] = '/';
        printf_P(PSTR("\nWaiting for '/' to mount..."));
        while(fd == _VOLUMES) {
                slots = fs_mountcount();
                if(slots != last) {
                        last = slots;
                        if(slots != 0) {
                                printf_P(PSTR(" \n"), ptr);
                                fd = fs_ready("/");
                                for(uint8_t x = 0; x < _VOLUMES; x++) {
                                        ptr = (uint8_t *)fs_mount_lbl(x);
                                        if(ptr != NULL) {
                                                printf_P(PSTR("'%s' is mounted\n"), ptr);
                                                free(ptr);
                                        }
                                }
                                if(fd == _VOLUMES) printf_P(PSTR("\nWaiting for '/' to mount..."));
                        }
                }
                printf_P(PSTR(" \b%c\b"), fancy[spin]);
                spin = (1 + spin) % 4;
                xmem::Sleep(100);
        }
        printf_P(PSTR(" \b"));
        // File System is now ready. Start networking.
        // Start the ip task using a 3KB stack, 29KB malloc arena
        uint8_t t1 = xmem::SetupTask(IP_task, 1024*3);
        xmem::StartTask(t1);
}

void lookup_program(void) {
        // This holds the returned information.
        // You must free the pointers within when done with it immediately
        struct u_nameip buf;
        int x = -1;
        printf_P(PSTR("\nStart SLIP on your server now!\007"));
        while(!network_booted) xmem::Yield(); // wait for networking to be ready.
        xmem::Sleep(10000); // Give user time to start slip on SLIP server.
        while(x) { // Keep trying until success.
                printf_P(PSTR("\nLooking up %s..."), DNS_QUERY);
                x = resolve(DNS_QUERY, &buf);
                if(!x) {
                        printf_P(PSTR("\n'%s' = %u.%u.%u.%u\n"), buf.hostname, (uint8_t)buf.hostip[0], (uint8_t)buf.hostip[1], (uint8_t)buf.hostip[2], (uint8_t)buf.hostip[3]);
                        // We are done with the structure. Release unused memory.
                        free(buf.hostip); // free this pointer...
                        free(buf.hostname); // ... and free this pointer
                } else {
                        printf_P(PSTR("Failed!\nCheck your network! Will retry in a few seconds..."));
                        // Fail, Sleep a little so we don't hammer the network.
                        xmem::Sleep(5000);
                        // The resolver tries automatically, anyway.
                        // I place it in this loop so that if there is some
                        // troubles with user setup, it will retry.
                }
        }
        // Demo program exits here...
}

void setup(void) {
        USB_Module_Calls Calls[2];
        {
                uint8_t c = 0;
                Calls[c] = GenericFileSystem::USB_Module_Call;
                c++;
                Calls[c] = NULL;
        }
        USB_Setup(Calls);
        uint8_t t1 = xmem::SetupTask(start_system);
        xmem::StartTask(t1);
        uint8_t t2 = xmem::SetupTask(lookup_program);
        xmem::StartTask(t2);
}

void loop(void) {
        xmem::Yield(); // Don't hog
}

