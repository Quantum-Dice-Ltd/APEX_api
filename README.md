# APEX_api
Quantum Dice APEX server API, test and demonstration programs
APEX API and test/demo code

Build everything by running `make`.

There are two programs and a library. The programs are:

* `qdapexmini`

	A minimal example of using a Quantum Dice APEX server. Run it giving the name of the interface to which the APEX
	is attached. It will fetch and print in hex 1024 octets of randomness.

* `qdapextest`

	A bigger and more complex example of using a Quantum Dice APEX
	server. Run it giving the name of the interface to which the APEX
	is attached. It will fetch 4 GB of randomness and write it out as raw binary. It then prints statistics showing
	the speed of the transfer. You should redirect the output to a file (or /dev/null), as it is random bytes and
	should be converted to a readable representation if you are going to look at it.

	While it is running, the test program prints a dot for every second in which some data was received. This is to
	allow you to see if it has got stuck, as it will probably take at least a minute to run. Typically you will see a
	speed of around 500Mb/s over a 1Gb/s Ethernet. This is for a point-to-point cable which directly links an APEX to
	the PC which is using it. It can be slower if sent via a switch. For performance and security reasons, do not put
	an APEX on a subnet shared between multiple devices.


The library is `qdapex_api.c` and its associated header `qdapex_api.h`
Include the header in your program, and build and link with the .c file.

A test script `apex-test` is included which runs `qdapextest` appropriately. You need to specify the interface to use,
which is probably something like `eth1` or `enp4s0f2np2`. On Linux, the `ip addr` command lists interfaces and their
current configuration.
