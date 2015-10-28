# HORST - Highly Optimized Radio Scanning Tool
or Horsts OLSR Radio Scanning Tool

Copyright (C) 2005-2015 Bruno Randolf (br1@einfach.org) and licensed under 
the GNU Public License (GPL) V2

## Overview

“horst” is a small, lightweight IEEE802.11 wireless LAN analyzer with a text 
interface. Its basic function is similar to tcpdump, Wireshark or Kismet, but 
it’s much smaller and shows different, aggregated information which is not 
easily available from other tools. It is mainly targeted at debugging wireless 
LANs with a focus on ad-hoc (IBSS) mode in larger mesh networks. It can be 
useful to get a quick overview of what’s going on on all wireless LAN channels 
and to identify problems.

See the man pages for more detailed and up-to-date information:

	$ man -l horst.1
	$ man -l horst.conf.5
	(or)
	$ nroff -mandoc horst.1
	$ nroff -mandoc horst.conf.5

Also see: http://br1.einfach.org/tech/horst/


## Building

Official git repository:	git://br1.einfach.org/horst
GitHub clone:			https://github.com/br101/horst

"horst" is just a simple tool, and "libncurses" is the only hard requirement (be 
sure to install it's header files too). Recently we have added support for nl80211
via libnl, so you normally need libnl3 + header files as well.

Building is as simple as typing:

	$ make

If you have an old or proprietary WLAN driver which only knows the deprecated
"wireless-extensions" you can build horst with support for them:

	$ make WEXT=1

To experimentally build for Mac OSX or other Unix using libpcap use:

	$ make PCAP=1 WEXT=0 LIBNL=0

Please note that PCAP and OSX support is not well tested and some features, like
getting or setting the channel are not implemented on OSX.


## Background

"horst" was created to fill a need in the Wireless Mesh networking / Freifunk
community of Berlin but has since grown to be a useful tool for all kinds of 
Wireless networks.

With the usual wireless tools like iw, iwconfig and iwspy and even kismet or 
WireShark it is hard to measure the received signal strength (RSSI) of
all available access points, stations and ad-hoc networks in a given location. 
It's especially difficult to differentiate the different nodes which form an 
ad-hoc network. This information however is very important for setting up, 
debugging and optimizing wireless mesh networks and antenna positions.

"horst" aims to fill this gap and lists each single node of an ad-hoc network
separately, showing the signal strength (RSSI) of the last received packet. This
way you can see which nodes are part of a specific ad-hoc cell (BSSID), 
discover problems with ad-hoc cell merging ("cell splitting", a problem of 
many WLAN drivers) and get a general overview of what's going on in the "air".

To do this, "horst" uses the monitor mode including radiotap headers (or for 
older drivers prism2 headers) for the signal strength information of the wlan 
cards and listens to all packets which come in the wireless interface. The 
packets are summarized by the MAC address of the sending node, analyzed and
aggregated and displayed in a simple text (ncurses) interface.


## Contributors

Thanks to the following persons for contributions:

* Horst Krause
* Sven-Ola Tuecke
* Robert Schuster
* Jonathan Guerin
* David Rowe
* Antoine Beaupré
* Rami Refaeli
* Joerg Albert
* Tuomas Räsänen
