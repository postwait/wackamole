Wackamole on windows
====================

Requirements:

- Will run on WinXP and Win2003.
  *should* also run on Win2k, but has not been tested there.

- Install Winpcap

- Rename your NICs so that they don't include spaces in their names
  eg: "Local Area Connection" can be renamed to "lan0",
  "Local Area Connection 2" can be renamed to "lan1" etc.
  The names you choose are the names that you will reference from
  wackamole.conf

- Make sure spread listens on TCP

- Make sure wackamole knows to talk to spread over TCP:
  Spead = 4803@127.0.0.1

- Configure wackamole to use TCP for the control connection:
  Control = 127.0.0.1:2413  # or a port of your own choice

- When configuring virtual interfaces, the netmask for IPs that you bring up
  must match the existing netmask  for the IPs already configured on that
  interface.  Windows will refuse to add the IP if it does not match.

- Wackamole.exe must run with administrative privileges to be able to init pcap
  and add/remove IP addresses.

Build Instructions
==================

Tested with Visual Studio.Net.

- Download the spread binary release.
- Download the winpcap developer kit.
- Adjust the nmakefile so it can find the spread and pcap libs
- nmake -f NMakefile


TODO:

syslog:
	replace with NT event log

fork:
	run as a service

vim:se ff=dos et ts=2 sw=2:

