# iAts_pro(Anttena automatic tracking system pro)

Not long ago, I created an automatic tracking system that supports bluetooth connection and parameter configuration based on the open source project [amv-open360tracker_rx_v1_user](https://github.com/raul-ortega/amv-open360tracker/) (You can find my project at [here](https://github.com/akari-tun/iAts)).

This project was built with Ateml 328p. As it is an 8-bit chip, its performance was low, so I couldn't implement more functions on it.

Therefore, I came up with the idea of redeveloping an antenna automatic tracking system based on chips with higher performance.

Through the project of [RavenLRS](https://github.com/RavenLRS), I got to know a module espressif-ESP32 designed for IOT, which is integration bluetooth and wifi, and has 240mhz main frequency and 2m+ flash. And the official provides a very complete SDK for development, which meets my needs very much. Therefore, I redeveloped the antenna automatic tracking system on this module.



