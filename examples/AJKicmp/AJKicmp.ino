/* (C) 2001-2014 Andrew J. Kroll */
/* see file README for license details */
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

volatile uint8_t booted=0;
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
        // File System is now ready. Initialize Networking
        // Start the ip task
        uint8_t t1 = xmem::SetupTask(IP_task);
        xmem::StartTask(t1);
        booted=1;
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
}

void loop(void) {
        xmem::Yield(); // Don't hog, we are not used anyway.
//        if(booted) {
//                // Your stuff here
//        }
}

