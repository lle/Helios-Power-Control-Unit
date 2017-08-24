# Helios Makerspace - PCU: Power Control Unit

## Short Description
Device that request a keycode from the user in order to power-up a tool in a Makerspace
![Helios PCU](https://raw.githubusercontent.com/lle/HeliosPowerControlUnit/master/img/heliosPCU.png)

## Demonstration
https://www.youtube.com/watch?v=vD_NKFLX1eg

## Problem Statement
At Helios Makerspace, there is a number of dangerous tools (ei: CNC, table saws, metal lathe). Members should always the staff if they can use the machine. 

## Creating the PCU
The power control unit locks the power from a tool until the correct password has been inputted. Furthermore, the administrator can set timeouts amount between 15 minutes and 2 hours. When the timeout has been reached, the PCU will produce several waves of warning beep at 60, 30 and 10 seconds, before shutting down.

## Limitations
The current board design only support devices lower than 10 amps at constant current due to the PCB trace width.
Assuming the PCB is 1oz/ft^2, at 10A and trace width of 180 mils the PCB temperature will raise by 21 degrees celcius. I wasn't confortable allowing the PCB to raise too much.
If you wanted to have a PCB that supports 15A, you would need to manufacture a PCB at 2oz/ft^2. 
* Beware: if you manufacture a PCB for 15A, change the fuse+fuse clip. They are only rated for 10A 

## Known Issues
### Relay flicker on boot-up
On boot-up the Arduino will turn on pin D13 and consequently turn on the relay for a breif moment. That is un-safe for the purpose of this device. This is caused by the Arduino bootloader which raises the LED pin HIGH during boot. We have to make this project as portable as possible, so instead of touching the bootloader and asking everybody to reflash their arduino, I'm just going to issue a new PCB design which uses pin D12 for the relay.

### Hardware Patch
To fix this problem on the first manufactured PCB, we can bypass the relay into pin D12 instead of D13.
* First cut the D13 pin off the Arduino Nano
* Then, on the bottom of the PCB, solder a bypass wire to connect D13 to D12
![Relay Flicker Patch](https://raw.githubusercontent.com/lle/HeliosPowerControlUnit/master/img/patch.png)

## BOM
| Digikey               |                       |                                                                             |                    |                |           |
|-----------------------|-----------------------|-----------------------------------------------------------------------------|--------------------|----------------|-----------|
| Quantity              | Part Number           | Description                                                                 | Customer Reference | Unit Price CAD | Sub Total |
| 1                     | Z2616-ND              | RELAY GEN PURPOSE SPDT 16A 5V                                               |                    | 3.06           | 3.06      |
| 10                    | 311-3454-1-ND         | CAP CER 1UF 25V X7R 1206                                                    |                    | 0.137          | 1.37      |
| 1                     | NTD3055L104T4GOSCT-ND | MOSFET N-CH 60V 12A DPAK                                                    |                    | 0.95           | 0.95      |
| 10                    | 311-100KCRCT-ND       | RES SMD 100K OHM 1% 1/8W 0805                                               |                    | 0.031          | 0.31      |
| 10                    | 311-10.0CRCT-ND       | RES SMD 10 OHM 1% 1/8W 0805                                                 |                    | 0.031          | 0.31      |
| 10                    | RMCF0805JT150RCT-ND   | RES SMD 150 OHM 5% 1/8W 0805                                                |                    | 0.024          | 0.24      |
| 10                    | RMCF0805JT470RCT-ND   | RES SMD 470 OHM 5% 1/8W 0805                                                |                    | 0.024          | 0.24      |
| 1                     | 732-4988-1-ND         | LED AMBER CLEAR 1206 SMD                                                    |                    | 0.27           | 0.27      |
| 1                     | 732-5032-1-ND         | LED GREEN CLEAR 1206 SMD                                                    |                    | 0.24           | 0.24      |
| 1                     | 668-1458-ND           | AUDIO MAGNETIC INDICATOR 4-7V TH                                            |                    | 2.16           | 2.16      |
| 1                     | A98472-ND             | CONN BARRIER STRIP 2CIRC 0.325"                                             |                    | 1.33           | 1.33      |
| 1                     | A98473-ND             | CONN BARRIER STRIP 3CIRC 0.325"                                             |                    | 1.52           | 1.52      |
| 2                     | 486-2019-ND           | FUSE CLIP CARTRIDGE 10A PCB                                                 | WARNING 10A ONLY   | 0.14           | 0.28      |
| 1                     | RXEF050-2HFCT-ND      | PTC RESET FUSE 72V 500MA RADIAL                                             |                    | 0.62           | 0.62      |
| 2                     | 493-14520-1-ND        | CAP ALUM 1000UF 20% 16V SMD                                                 |                    | 1.3            | 2.6       |
| 1                     | F1728-ND              | FUSE GLASS 15A 250VAC 5X20MM                                                |                    | 0.46           | 0.46      |
| 1                     | SMBJ7.0A-FDICT-ND     | TVS DIODE 7VWM 12VC SMB                                                     |                    | 0.6            | 0.6       |
| 1                     | 102-2589-ND           | POWER SUPPLY SWITCHING 5V 2W                                                |                    | 17.78          | 17.78     |
| 1                     | 495-6464-ND           | VARISTOR DISC 14MM                                                          |                    | 0.74           | 0.74      |
| 1                     | 3314J-2-103ECT-ND     | TRIMMER 10K OHM 0.25W SMD                                                   |                    | 3.12           | 3.12      |
|                       |                       |                                                                             |                    |                |           |
| Other Stores          |                       |                                                                             |                    |                |           |
| Quantity              | Description           | URL                                                                         | Customer Reference | Unit Price CAD | Sub Total |
| 1                     | Project Box           | https://addison-electronique.com/boitier-abs-257-x-190-x-82-mm.html         |                    | 13.99          | 13.99     |
| 1                     | Extension Cord        | https://addison-electronique.com/rallonge-electrique-16-3-sjtw-1-metre.html |                    | 2.99           | 2.99      |
| 1                     | Switch                | https://addison-electronique.com/interrupteur-a-levier-on-off-7807.html     |                    | 1.99           | 1.99      |
| 1                     | Switch Cover          | https://addison-electronique.com/couvert-pour-interrupteur-a-levier-vert-clair.html |            | 0.99           | 0.99      |
|                       |                       |                                                                             |                    |                |           |
| 1                     | LCD                   | http://bit.ly/2vsK9xS                                                       |                    | 2.49           | 2.49      |
| 1                     | Keypad                | http://bit.ly/2wrNNMV                                                       |                    | 1.60           | 1.60      |
|                       |                       |                                                                             |                    |                |           |
| 1                     | Arduino Nano          | https://www.sunfounder.com/board/arduino/nano.html                          |                    | 13.94          | 13.94     |
|                       |                       |                                                                             |                    |                |           |
|                       |                       |                                                                             |                    | Total          | 69.12     |
