ajkstack
========

IP stack with TCP and UDP protocols for Arduino

Hardware Requirements:<BR>
Arduino Mega, Mega 2560 or Teensy++ 2.0 from www.PJRC.com<BR>
USB host Shield from www.circuitsathome.com<BR>
Minimum of 128K external memory-- Rugged circuits, or Andy Brown, or compatable.<BR>
TTL USB to Serial adapter for terminal I/O to the AVR on Serial 1<BR>
Fat 32 formatted thumb drive or other USB storage device<BR>
<BR>
Software Requirements:<BR>
Arduino 1.0.5<BR>
https://github.com/xxxajk/Arduino_Makefile_master<BR>
https://github.com/xxxajk/xmem2<BR>
https://github.com/felis/USB_Host_Shield_2.0<BR>
https://github.com/xxxajk/generic_storage<BR>
https://github.com/xxxajk/RTClib<BR>
https://github.com/xxxajk/xmemUSB<BR>
https://github.com/xxxajk/xmemUSBFS<BR>

Additional Requirements:<BR>
Linux with SLIP<BR>
<BR>
Setup:<BR>
1: Format the tumb drive with fat32 and do NOT set the label so that it may mount as the root drive.<BR>
2: On the thumb drive, make a directory named etc<BR>
3: Place a copy of the tcp.rc file in the etc directory.<BR>
You will also possibly have to edit the tcp.rc file.<BR>
Settings details are within the tcp.rc file.<BR>
<BR>
Quick how-to -- connect with SLIP interface on Linux:<BR>
1: The commands below need either login as root, su, or use sudo in two different sessions.<BR>
2: Choose 2 IPs on a subnet NOT on your LAN. My LAN uses the 192.168.123.0 subnet, so I use 192.168.3.X for SLIP.<BR>
3: In terminal 1<BR>
slattach -d -p slip -s 115200 /dev/ttyACM0<BR>
4: In terminal 2<BR>
ifconfig sl0 192.168.3.73 pointopoint 192.168.3.74 up mtu 554<BR>
<BR>
If you can't route packets to the internet, it means you need to enable IP-forwarding and/or IP-masquerade. You can find this information by searching for the usual Linux how-to.<BR>
