# Real Time Embedded Systems final project
---
## Description

In order to run the program, after you build the project, simply execute the script `start.sh`, which will handle reconnection in case of network errors and disconnects.

Before executing, make sure you include your FinnHub API key in the `start.sh` script, and specify the crypto/stock ticker you wish to monitor.

Output files will be generated in an `out/` folder that will be created. For each ticker, three files will be generated: a `PRICES` file, containing trade logs, a `MA` file containing the rolling moving average, and a `C` file, containing a candlestick. Also, each filename includes the timestamp at which the program was started.

---
### Notes
To build for Raspberry Pi:

```
export CC=/path/to/cross/compiler
cmake .
make
```
and then transfer the executable to the RPi, assuming you already have a workning cross compiler set up with the library libwebsockets built for the RPi architecture and available to the cross compilation toolchain.