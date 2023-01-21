# mst
A Minimum Spannng Tree Utility for IPv4 Networks

# Usage

**mst** *<network_prefix> <network_width>*

eg. `./mst 128.250.1.1 24`

After starting, the utility will await for commands on the command line.

Valid commands are:

```
x                 - Exits the utility
u <address>       - Marks the host address as up (eg. `u 128.250.1.29`)
d <address>       - Marks the host address as down (eg. `d 128.250.1.31`)
p                 - Prints the minimal routing spanning tree to cover all hosts marked as up
```
