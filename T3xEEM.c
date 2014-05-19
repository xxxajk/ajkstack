#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "tcpvars.h"
#include "compat.h"

//#define USE_EEM

#if USE_EEM
#include <Arduino.h>
#include "ip.h"
#include "net.h"

#define                  ARPHRD_ETHER 1    /* Ethernet 10/100Mbps.  */
#define                      ETH_ALEN 6    /* Octets in one ethernet addr   */
#define                      ETH_HLEN 14   /* Total octets in header.       */
#define                      ETH_ZLEN 60   /* Min. octets in frame sans FCS */
#define                  ETH_DATA_LEN 1500 /* Max. octets in payload        */
#define                 ETH_FRAME_LEN 1514 /* Max. octets in frame sans FCS */
#define                   ETH_FCS_LEN 4    /* Octets in the FCS             */

struct ethhdr {
        unsigned char h_dest[ETH_ALEN]; /* destination eth addr */
        unsigned char h_source[ETH_ALEN]; /* source ether addr    */
        unsigned short h_proto; /* packet type ID field */
} __attribute__((packed));

struct etharp {
        unsigned char ar_hrdH; /* format of hardware address	*/
        unsigned char ar_hrdL; /* 	''		''	*/
        unsigned char ar_proH; /* format of protocol address	*/
        unsigned char ar_proL; /*	''		''	*/
        unsigned char ar_hln; /* length of hardware address	*/
        unsigned char ar_pln; /* length of protocol address	*/
        unsigned char ar_opH; /* ARP opcode (command)		*/
        unsigned char ar_opL; /*	''		''	*/
        unsigned char ar_sha[ETH_ALEN]; /* sender hardware address	*/
        unsigned char ar_sip[4]; /* sender IP address		*/
        unsigned char ar_tha[ETH_ALEN]; /* target hardware address	*/
        unsigned char ar_tip[4]; /* target IP address		*/
} __attribute__((packed));

struct EEM_data_packet_header {
        uint8_t bmType: 1;
        uint8_t bmCRC: 1;
        uint16_t bmLength: 14;
} __attribute__((packed));

struct EEM_command_packet_header {
        uint8_t bmType: 1;
        uint8_t bmReserved: 1;
        uint8_t bmEEMCmd: 3;
        uint16_t bmEEMCmdParam: 11;
} __attribute__((packed));


#ifdef CORE_TEENSY
#ifdef __arm__
// ALL Teensy 3.x have an unique embedded mac address! \o/ Thanks Paul!
// http://forum.pjrc.com/threads/91-teensy-3-MAC-address

void T3readmac(uint8_t *mac) {
        cli();
        FTFL_FCCOB0 = 0x41; // Selects the READONCE command
        FTFL_FCCOB1 = 0x0e; // read the given word of read once area

        // launch command and wait until complete
        FTFL_FSTAT = FTFL_FSTAT_CCIF;
        while(!(FTFL_FSTAT & FTFL_FSTAT_CCIF));

        *(mac) = FTFL_FCCOB5; // collect only the top three bytes,
        *(mac + 1) = FTFL_FCCOB6; // in the right orientation (big endian).
        *(mac + 2) = FTFL_FCCOB7; // Skip FTFL_FCCOB4 as it's always 0.
        FTFL_FCCOB0 = 0x41; // Selects the READONCE command
        FTFL_FCCOB1 = 0x0f; // read the given word of read once area

        // launch command and wait until complete
        FTFL_FSTAT = FTFL_FSTAT_CCIF;
        while(!(FTFL_FSTAT & FTFL_FSTAT_CCIF));

        *(mac + 3) = FTFL_FCCOB5; // collect only the top three bytes,
        *(mac + 4) = FTFL_FCCOB6; // in the right orientation (big endian).
        *(mac + 5) = FTFL_FCCOB7; // Skip FTFL_FCCOB4 as it's always 0.
        sei();

}
#endif
#endif
#else

/* just make a pretend symbol */
void t3xeem(VOIDFIX) {
        return;
}

#endif


