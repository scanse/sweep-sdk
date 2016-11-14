### Interface
UART  
Bit Rate    :115.2 Kbps  
Parity      :None  
Data Bit    :8  
Stop Bit    :1  
Flow Control    :None  

---
### Measurement Direction and Data Points

Rotation Direction  :Counterclockwise  
Detection Range :360 degrees

---
### Data Encoding and Decoding

All characters used for commands and responses are ASCII code in addition to CR and LF with the exception of the measurement packet.

Responses with float values are sent as 16bit integer values  
example conversion:
angle_f = 1.0f * ((float)(angle_i >> 4) + ((angle_i & 15) / 16.0f));

---
### Communication Format

#### (HOST -> SENSOR)
Command with no parameter

Command Symbol (2 bytes) | Line Feed(LF)
| --- | --- |

or  

Command with parameter

Command Symbol (2 bytes) | Parameter (2 bytes) | Line Feed(LF)
| --- | --- | ---|


#### (SENSOR -> HOST)

Response with no parameter echoed

Command Symbol (2 bytes) | Status (2 bytes) | Sum of Status | Line Feed(LF)  
| --- | --- | ---| --- |  

or  

Command with parameter echoed

Command Symbol (2 bytes) | Parameter (2 bytes) | Line Feed(LF) | Status (2 bytes) | Sum of Status | Line Feed(LF)  
| --- | --- | ---| --- | --- | --- |    

#### Command Symbol
2 byte code at the beginning of every command

#### Parameter
Information that is needed to change sensor settings.

#### Line Feed (LF) or Carriage Return (CR)
Terminating code. Command can have LF or CR or both as termination code but reply will always have LF as its termination code.

#### Status
2 bytes of data in reply that informs normal processing if command is authenticated or errors if undefined, invalid or incomplete command is received by sensor. Status other than 00 and 99 are error codes.

#### Sum of Status
1 byte of data used in authentication. Calculated by adding status bytes, taking lower 6 byte of this sum and adding 30H to this sum.  
Sum = 111111 = 3fH+30H = 6fH = o  
Example: [LF] 0 0 [LF] = P  

#### Responses to Invalid Commands
11 -- Invalid parameter  

---
### Sensor Commands

**DS** - Start data acquisition  
**DX** - Stop data acquisition  
**MS** - Adjust Motor Speed  
**MI** - Motor Information  
**IV** - Version Info  
**ID** - Device Info  
**RR** - Reset Device  

---

#### DS - Start data acquisition
* Initiates scanning
* Responds with header containing status.  
* Next responds with measurement packets indefinitely until commanded to stop.  

#### (HOST -> SENSOR)

D  |  S  |  LF
| --- | --- | ---|

#### (SENSOR -> HOST)
Header response

D  |  S  |  Status  |  SUM  |  LF
| --- | --- | ---| --- | --- |

Data Block (7 bytes)
Data Block


sync/error (1byte)  |  Azimuth - degrees(float) (2bytes)  |  Distance - cm(int) (2bytes)  |  Signal Strength (1byte)  | Checksum (1byte)
| ------ | ------ | ------ | ------ | ------ |

**sync / error** : 0 bit indicates the sync value, a value of 1 indicates the packet is the beginning of a new scan, a value of 0 indicates all other measurement packets. Bits 1-6 are reserved for error codes.

**azimuth** : Angle that ranging was recorded at. 
Azimuth is a float value - needs to be converted from 16bit int to float, use instructions at the top

**distance** : Distance of range measurement.

**signal strength** : Signal strength of current ranging measurement. Larger is better. Range: 0-255

**checksum** : Calculated by adding the 6 bytes of data then dividing by 255 and keeping the remainder.  (Sum of bytes 0-6) % 255

Status  
    00 -- Command received without any Error  
    22 -- Stopped to verify error  
    55 -- Hardware trouble  
    99 -- Resuming operation  

---
#### DX - Stop data acquisition
* Stops outputting measurement data.

#### (HOST -> SENSOR)

D |  X  |  LF
| --- | --- | ---|

#### (SENSOR -> HOST)

D  |  X  |  Status  |  SUM  |  LF
| --- | --- | ---| --- | --- |

---
#### MS - Adjust Motor Speed
Default Speed - 5Hz  

#### (HOST -> SENSOR)

M |  S |  Speed Parameter (2 bytes)  |  LF
| --- | --- | ---| --- |

Speed Parameter:
00 - 10 :  10 different speed levels according to Hz, increments of 1. ie: 01,02,..  
00 = Motor stopped

#### (SENSOR -> HOST)

M |  S |  Speed(Hz) (2 bytes)  |  LF  |  Status  |  Sum  |  LF
| --- | --- | ---| --- | --- | ---| --- |

---
#### MI - Motor Information
Returns current rotation frequency in Hz in ASCII 00 - 10 (increments of 1)

#### (HOST -> SENSOR)

M  |  I  |  LF
| --- | --- | ---|

#### (SENSOR -> HOST)

M  |  I   |  Speed(Hz) (2 bytes)  |  LF  |  Status  |  Sum  |  LF
| --- | --- | ---| --- | --- | --- | --- |

---
#### IV - Version Details
* Model
* Protocol Version 
* Firmware Version
* Hardware Version
* Serial Number

#### (HOST -> SENSOR)

I  |  V  | LF
| --- | --- | ---|


#### (SENSOR -> HOST)

I  |  V  | Model (5 bytes) | Protocol (2 bytes) | Firmware V (2 bytes) | Hardware V | Serial Number (8 bytes) | LF
| --- | --- | ---| --- | ---  | --- | --- | --- |

Example:  
    IVSWEEP01011100000001
    
---

#### ID - Device Info
* Bit Rate
* Laser State
* Mode
* Diagnostic
* Motor Speed
* Sample Rate

#### (HOST -> SENSOR)

I  |  D  | LF
| --- | --- | ---|

#### (SENSOR -> HOST)

I  |  D  | Bit Rate (6 bytes) | Laser state | Mode | Diagnostic | Motor Speed (2 bytes) | Sample Rate (4 bytes) | LF
| --- | --- | ---| --- | ---  | --- | --- | --- | --- |

Example:  
    IV115200110050500

---
    
#### RR - Reset Device
* Reset Scanner

#### (HOST -> SENSOR)

R | R | LF
| --- | --- | ---|

#### (SENSOR -> HOST)

R | R | Status | Sum | LF
| --- | --- | ---| --- | ---  |

