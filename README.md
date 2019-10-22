# iAts_pro(Antenna automatic tracking system pro)

## Development is on the way...

Not long ago, I created an automatic tracking system that supports bluetooth connection and parameter configuration based on the open source project [amv-open360tracker](https://github.com/raul-ortega/amv-open360tracker/) (You can find my project at [here](https://github.com/akari-tun/iAts)).

This project was built with Ateml 328p. As it is an 8-bit chip, its performance was low, so I couldn't implement more functions on it.Therefore, I came up with the idea of redeveloping it built on chips with higher performance.

Through the project of [RavenLRS](https://github.com/RavenLRS), I got to know a module espressif-ESP32 designed for IOT, which is integration bluetooth and wifi, and has 240mhz main frequency and 2m+ flash. And the official provides good SDK for development, which meets my needs very much. Therefore, I redeveloped the antenna automatic tracking system on this module.

After I graduating from college, I never use C language to develop any project, using C language development this project is a big challenge to me, I need to relearn C language feature, and the need to better understand how to use of Point. 

[RavenLRS](https://github.com/RavenLRS) project is built on ESP32 module, its also the realization of the function of some of its similar to some of my ideas, I learn by [RavenLRS](https://github.com/RavenLRS) project by learned a lot of of C language development experience, and use a lot of RavenLRS code to my project,Thanks to [RavenLRS](https://github.com/RavenLRS) author [fiam](https://github.com/fiam) open source for such an excellent project.

### PCB LAYOUT

#### PCB_LAYOUT_TOP
![PCB_LAYOUT_TOP](doc/pcb/PCB_layout_top.png?raw=true "PCB_LAYOUT_TOP")

#### PCB_LAYOUT_BOTTOM
![PCB_LAYOUT_BOTTOM](doc/pcb/PCB_layout_bottom.png?raw=true "PCB_LAYOUT_BOTTOM")

### 3D PCB LAYOUT

#### 3D_PCB_LAYOUT_TOP
![3D_PCB_LAYOUT_TOP](doc/pcb/3D_PCB_layout_top.png?raw=true "3D_PCB_LAYOUT_TOP")

#### 3D_PCB_LAYOUT_BOTTOM
![3D_PCB_LAYOUT_BOTTOM](doc/pcb/3D_PCB_layout_bottom.png?raw=true "3D_PCB_LAYOUT_BOTTOM")

### DESIGN IMAGE

#### 3D Printed
<img src="doc/design_sketch/image/IMG_1726.JPG?raw=true " width="480" height="640" alt="TX">

#### Setup 1
<img src="doc/design_sketch/image/IMG_1727.JPG?raw=true " width="480" height="640" alt="TX">

#### Setup 2
<img src="doc/design_sketch/image/IMG_1728.JPG?raw=true " width="640" height="480" alt="TX">

#### Setup 3
<img src="doc/design_sketch/image/IMG_1729.JPG?raw=true " width="640" height="480" alt="TX">

#### Setup 4
<img src="doc/design_sketch/image/IMG_1730.JPG?raw=true " width="480" height="640" alt="TX">

#### Setup 5
<img src="doc/design_sketch/image/IMG_1731.JPG?raw=true " width="480" height="640" alt="TX">

#### Setup 6
<img src="doc/design_sketch/image/IMG_1886.JPG?raw=true " width="480" height="640" alt="TX">


## CONTROL APP
This app can be control aat to course the right direction and view the aircraft's location, some status infomation.Its supported two protocol now.

##### App Image

<img src="doc/app/image/welcome.JPG?raw=true " width="320" height="" alt="TX">

main page

<img src="doc/app/image/main.JPG?raw=true " width="" height="180" alt="TX">
<img src="doc/app/image/setting.JPG?raw=true " width="" height="180" alt="TX">

##### Smartport from R9M
Frsky R9m module can output Smartport, I diy a HC-05 module, It Contains the inverter. So it's easy to plug to the R9m.

My app use classic bluetooth to connect HC-05 and get Smartport.

<img src="doc/app/image/bt_module_top.JPG?raw=true " width="240" height="320" alt="TX">
<img src="doc/app/image/bt_module_bottom.JPG?raw=true " width="320" height="320" alt="TX">

plugged to R9m

<img src="doc/app/image/bt_module_plugged.JPG?raw=true " width="480" height="640" alt="TX">

##### Raven LRS

[RavenLRS](https://github.com/RavenLRS) provides telemetry service for BLE's gatt server, but it don't provides notify.So I add the gatt server notify to RavenLRS,my app can use BLE notify get telemetry data.

You can find this module at here: [AF_TX_Lite](https://github.com/RavenLRS/raven-hardware/tree/AFelite)

<img src="doc/app/image/AF_TX_Lite.JPG?raw=true " width="480" height="640" alt="TX">