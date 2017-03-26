# rtl_tcp server for SDRPlay

(C)2015 Tony Hoyle (tony@hoyle.me.uk).  Licensed under the BSD license. 

## HISTORY
- Version 0.0.0: Initial build
- Version 1.0.0: Gain settings should now work in client apps.
- Version 1.0.1: Tidy up project, give better binary name.  Run in background by default.
- Version 1.0.2: When no clients collected, don't initialise the device as it uses CPU when idle.

## BUILDING
```
  mkdir build
  cd build
  cmake ..
  make
```

## NOTES
 The SDRPlay libraries are compiled against a recent libusb - at least 1.0.18.  The one that ships with the Raspberry Pi is not new enough and will crash.  Suitable upgrades are at https://www.hoyle.me.uk/pi/ 

 CMake for building requires libmirsdrapi-rsp. It is available at http://www.sdrplay.com/downloads/ . Have a look below SOFTWARE for API/HARDWARE DRIVER in the appropriate system tab.

  The server pretends to be an R820T - this enables gain setting but the values the R820T use don't really map to the SDRPlay, so there won't be the same results.

## OPTIONS
```
 -f initial frequency
 -p listening port (defaults to 1234)
 -s initial sample rate (default 2048k)
 -4 ipv4 only
 -6 ipv6 only
 -d debug (implies -g)
 -g run in foreground
```

## TODO
  AGC is not currently supported, due to lack of documentation.

## BUGS
  Every now and then the SDRPlay goes loopy in SDR#.  Disconnect and reconnect to fix this.. no idea why this happens.

