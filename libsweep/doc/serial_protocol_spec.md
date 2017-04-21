# Communication Protocol Spec:
This document outlines the communication protocol for the Scanse Sweep scanning LiDAR. For more information visit [scanse.io](http://scanse.io/).

---
### Interface

**UART Settings**:
- Bit Rate    :115.2 Kbps  (115200)
- Parity      :None  
- Data Bit    :8  
- Stop Bit    :1  
- Flow Control    :None  

---
### Measurement Direction and Data Points

`Rotation Direction`: Counterclockwise  
`Detection Range`: 360 degrees

---
### Naming Convention
`Sensor`: Sweep Device  
`Host`:Controller (computer, microcontroller etc)

In general, Sweep's communication protocol can be thought of as a set of `commands` and `receipts`.  
`Command:` the term given to messages sent from the host to the sensor (host->sensor)  
`Receipt:` the term given to messages sent from the sensor to the host (sensor -> host). 

---
### Data Encoding and Decoding

The protocol design specifies that all transmitted bytes (both commands and receipts) are legible ASCII characters, in addition to CR and LF. This means that numeric codes (ex: motor speed code '05') are also transmitted as ASCII characters, and not decimal values. This design was chosen so that commands and receipts could be read directly from the terminal.  

**Note:** The only exception is the `Data Block` receipt, which must permit arbitrary byte values. See the Data Block receipt section for encoding. 

---
### General Communication Format

#### (HOST -> SENSOR)
Command with no parameter

Command Symbol (2 bytes) | Line Feed(LF)
| --- | --- |

Command with parameter

Command Symbol (2 bytes) | Parameter (2 bytes) | Line Feed(LF)
| --- | --- | ---|


#### (SENSOR -> HOST)

Response with no parameter echoed

Command Symbol (2 bytes) | Status (2 bytes) | Sum of Status | Line Feed(LF)  
| --- | --- | ---| --- |  

Response with parameter echoed

Command Symbol (2 bytes) | Parameter (2 bytes) | Line Feed(LF) | Status (2 bytes) | Sum of Status | Line Feed(LF)  
| --- | --- | ---| --- | --- | --- |    

- `Command Symbol`: 2 byte code at the beginning of every command (ASCII)
- `Parameter`: Information that is needed to change sensor settings (ASCII).. ex: a motor speed code 05 transmitted as ASCII parameter '05', which has byte values [48, 53] in decimal.
- `Line Feed (LF) or Carriage Return (CR)`: Terminating code. Command can have LF or CR or both as termination code but receipt will always have LF as its termination code.
- `Status`: 2 bytes of data used to convey the normal/abnormal processing of a command. ASCII byte values of `'00'` or `'99'` indicate that the sensor received and processed the command normally. Value of `'11'` specifies an invalid parameter was included in the command. Any other byte values are reserved for errors, which will convey that an undefined, invalid or incomplete command was received by the sensor. 
- `Sum of Status`: 1 byte of data used to check for corrupted transmission. See Appendix for instructions for authenticating receipts.

---
## Sensor Commands

**DS** - Start data acquisition  
**DX** - Stop data acquisition  
**MS** - Adjust Motor Speed  
**LR** - Adjust LiDAR Sample Rate  
**LI** - LiDAR Info  
**MI** - Motor Information  
**MZ** - Motor Ready
**IV** - Version Info  
**ID** - Device Info  
**RR** - Reset Device  

---

## DS - Start data acquisition
* Initiates scanning
* Sensor responds with header containing status.  
* Sensor begins sending constant stream of Data Blocks, each containing a single sensor readings. The stream continues indenfinitely until the host sends a DX command.  

### (HOST -> SENSOR)

D  |  S  |  LF
| --- | --- | ---|

### (SENSOR -> HOST)
Header response

D  |  S  |  Status  |  SUM  |  LF
| --- | --- | ---| --- | --- |

DS command is not guaranteed to succeed. There are a few conditions where it will fail. In the event of a failure, the two status bytes are used to communicate the failure.

Status Code (2 byte ASCII code):
- `'00'`: Successfully processed command. Data acquisition effectively initiated.
- `'12'`: Failed to process command. Motor speed has not yet stabilized. Data acquisition NOT initiated. Wait until motor speed has stabilized before trying again.
- `'13'`: Failed to process command. Motor is currently stationary (0Hz). Data acquisition NOT initiated. Adjust motor speed before trying again.

### (SENSOR -> HOST)
Data Block (7 bytes) - repeat indefinitely

sync/error (1byte)  |  Azimuth - degrees(float) (2bytes)  |  Distance - cm(int) (2bytes)  |  Signal Strength (1byte)  | Checksum (1byte)
| ------ | ------ | ------ | ------ | ------ |

### Data Block Structure:
The `Data Block` receipt is 7 bytes long and contains all the information about a single sensor reading.

- **Sync/Error Byte**: The sync/error byte is multi-purpose, and encodes information about the rotation of the Sweep sensor, as well as any error information. Consider the individuals bits:
    
    e6 | e5 | e4 | e3 | e2 | e1 | e0 | sync
    | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ |

   - **Sync bit**: least significant bit (LSB) which carries the sync value. A value of `1` indicates that this Data Block is the first acquired sensor reading since the sensor passed the 0 degree mark. Value of `0` indicates all other measurement packets.
    - **Error bits**: 7 most significant bits (e0-6) are reserved for error encoding. The bit `e0` indicates a communication error with the LiDAR module with the value `1`. Bits `e1:6` are reserved for future use.

- **Azimuth**: Angle that ranging was recorded at (in degrees). Azimuth is a float value, transmitted as a 16 bit int. This needs to be converted from 16bit int to float. Use instructions in the Appendix. Note: the lower order byte is received first, higher order byte is received second.
- **Distance**: Distance of range measurement (in cm). Distance is a 16 bit integer value. Note: the lower order byte is received first, higher order byte is received second. Use instructions in the Appendix.
- **Signal strength** : Signal strength of current ranging measurement. Larger is better. 8-bit unsigned int, range: 0-255
- **Checksum**: Calculated by adding the 6 bytes of data then dividing by 255 and keeping the remainder. (Sum of bytes 0-5) % 255 ... Use the instructions in the Appendix.  



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
Adjusts motor speed setting to the specified code indicating a motor speed between 0Hz and 10Hz. This sets the target speed setting, but the motor will take time (~6 seconds) to adjust and stabilize to the new setting. The blue LED on the device will flash while the speed is stabilizing.

#### (HOST -> SENSOR)

M |  S |  Speed Code (2 bytes)  |  LF
| --- | --- | ---| --- |

#### (SENSOR -> HOST)

M |  S |  Speed Code (2 bytes)  |  LF  |  Status  |  Sum  |  LF
| --- | --- | ---| --- | --- | ---| --- |

Speed Code (2 byte ASCII code):  
- `'00'` = 0Hz
- `'01'` = 1Hz
- `'02'` = 2Hz
- `'03'` = 3Hz
- `'04'` = 4Hz
- `'05'` = 5Hz
- `'06'` = 6Hz
- `'07'` = 7Hz
- `'08'` = 8Hz
- `'09'` = 9Hz
 - `'10'`= 10Hz 

(Note: codes are ASCII encoded, ie: in '05' = 0x3035)

MS command is not guaranteed to succeed. There are a few conditions where it will fail. In the event of a failure, the two status bytes are used to communicate the failure.

Status Code (2 byte ASCII code):
- `'00'`: Successfully processed command. Motor speed setting effectively changed to new value.
- `'11'`: Failed to process command. The command was sent with an invalid parameter. Use a valid parameter when trying again.
- `'12'`: Failed to process command. Motor speed has not yet stabilized to the previous setting. Motor speed setting NOT changed to new value. Wait until motor speed has stabilized before trying to adjust it again.

---
#### LR - Adjust LiDAR Sample Rate
Default Sample Rate - 500-600Hz  

#### (HOST -> SENSOR)

L |  R |  Sample Rate Code (2 bytes)  |  LF
| --- | --- | ---| --- |


#### (SENSOR -> HOST)

L |  R |  Sample Rate Code (2 bytes)  |  LF  |  Status  |  Sum  |  LF
| --- | --- | ---| --- | ---| --- | --- | 

Sample Rate Code (2 byte ASCII code):  
- `'01'` = 500-600Hz  
- `'02'` = 750-800Hz  
- `'03'` = 1000-1050Hz  

(Note: codes are ASCII encoded, ie: '02' = 0x3032)

LR command is not guaranteed to succeed. There are a few conditions where it will fail. In the event of a failure, the two status bytes are used to communicate the failure.

Status Code (2 byte ASCII code):
- `'00'`: Successfully processed command. Sample Rate setting effectively changed to new value.
- `'11'`: Failed to process command. The command was sent with an invalid parameter. Use a valid parameter when trying again.

(Note: codes are ASCII encoded, ie: '11' = 0x3131)

---
#### LI - LiDAR Information
Returns the current LiDAR Sample Rate Code. 

#### (HOST -> SENSOR)

L  |  I  |  LF
| --- | --- | ---|

#### (SENSOR -> HOST)

L  |  I   |  Sample Rate Code (2 bytes)  |  LF  |
| --- | --- | --- | --- |  

Sample Rate Code (2 byte ASCII code):  
- `'01'` = 500-600Hz  
- `'02'` = 750-800Hz  
- `'03'` = 1000-1050Hz  

(Note: codes are ASCII encoded, ie: '02' = 0x3032)

---
#### MI - Motor Information
Returns current motor speed code representing the rotation frequency (in Hz) of the current target motor speed setting. This does not mean that the motor speed is stabilized yet.

#### (HOST -> SENSOR)

M  |  I  |  LF
| --- | --- | ---|

#### (SENSOR -> HOST)

M  |  I   |  Speed Code (Hz) (2 bytes)  |  LF  |
| --- | --- | ---| --- |

Speed Code (2 byte ASCII code):  
- `'00'` = 0Hz
- `'01'` = 1Hz
- `'02'` = 2Hz
- `'03'` = 3Hz
- `'04'` = 4Hz
- `'05'` = 5Hz
- `'06'` = 6Hz
- `'07'` = 7Hz
- `'08'` = 8Hz
- `'09'` = 9Hz
 - `'10'`= 10Hz 

(Note: codes are ASCII encoded, ie: in '05' = 0x3035)

---
#### MZ - Motor Ready/Stabilized
Returns a ready code representing whether or not the motor speed has stabilized.

#### (HOST -> SENSOR)

M  |  Z  |  LF
| --- | --- | ---|

#### (SENSOR -> HOST)

M  |  Z   |  Ready Code (2 bytes)  |  LF  |
| --- | --- | --- | --- |

Ready Code (2 byte ASCII code):  

- `'00'` = motor speed has stabilized.
- `'01'` = motor speed has not yet stabilized.

(Note: codes are ASCII encoded, ie: '01' = 0x3031)

While adjusting motor speed, the sensor will NOT be able to accomplish certain actions such as `DS` or `MS`. After powering on the device or adjusting motor speed, the device will allow ~6 seconds for the motor speed to stabilize. The `MZ` command allows the user to repeatedly query the motor speed state until the return code indicates the motor speed has stabilized. After the motor speed is noted as stable, the user can safely send commands like `DS` or `MS`.

---
#### IV - Version Details
Returns details about the device's version information.
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
Returns details about the device's current state/settings.
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
Resets the device. Green LED indicates the device is resetting and cannot receive commands. When the LED turns blue, the device has successfully reset.

#### (HOST -> SENSOR)

R | R | LF
| --- | --- | ---|

#### (SENSOR -> HOST)

No response

---
## Appendix:
#### Authenticating Receipts (Does not apply to Data Block)
Authentification of a valid receipt is accomplished by a checksum, which uses the `Status`(2 bytes) and `Sum of Status` (1 byte) from the receipt. To perform the checksum, the received `Sum of Status` bytes is checked against a calculated sum of the 2 `Status` bytes. The protocol design allows for performing the checksum visually in a terminal, which requires  all bytes in a receipt to be legible ASCII values (ex: `'00P'`).  Therefore performing the checksum in code is not intuitive. It works like this:

- The status bytes are summed
- The lower 6 bits (Least Significant) of that sum are then added to 30Hex
- The resultant value is compared with the checksum byte
```javascript
//statusByte#1 + statusByte#2
let sumOfStatusBytes = status1_byteValue + status2_byteValue; 
//grab the lower (least significant) 6 bits by performing a bit-wise AND with 0x3F (ie: 00111111)
let lowerSixBits = sumOfStatusBytes & 0x3F; 
//add 30Hex to it
let sum = lowerSixBits + 0x30; 
return ( sum === checkSumByteValue );
```
ex: Consider the common case of `'00P'` ( decimal -> `[48, 48, 80]`, hex -> `[0x30, 0x30, 0x50]`)
```
0x30 + 0x30 = 0x60 // sum of the status bytes
0x60 & 0x3F = 0x20 // retrieve only the lower 6 bits
0x20 + 0x30 = 0x50 // calculate the ASCII legible sum
0x50 = 'P'         // translate to ASCII
```

#### Parsing Data Block 16-bit integers and floats
The `Data Block` receipt includes int-16 and float values (distance & azimuth). In the case of distance, the value is a 16-bit integer. In the case of the azimuth, the value is a float which is sent as a 16bit integer and must be converted back to a float. 

In either case, the 16-bit int is sent as two individual bytes. The lower order byte is received first, and the higher order byte is received second. For example, parsing the distance:
```javascript
//assume dataBlock holds the DATA_BLOCK byte array
//such that indices 3 & 4 correspond to the two distance bytes 
let distance = (dataBlock[4] << 8) + (dataBlock[3]);
```
For floats, start by using the same technique to acquire the 16-bit int. Once you have it, you can perform the conversion to float like so:
```javascript
//assume dataBlock holds the DATA_BLOCK byte array, 
//such that indices 1 & 2 correspond to the two azimuth bytes 
let angle_int = (dataBlock[2] << 8) + (dataBlock[1]);
let degree = 1.0 * ( (angle_int >> 4) + ( (angle_int & 15) / 16.0 ) );
```

#### Performing Data Block Checksum
The last byte of a `Data Block` receipt is a checksum byte. It represents the sum of all bytes up until the checksum byte (ie: the first 6 bytes). Obviously a single byte caps at 255, so the checksum can't reliably hold the sum of 6 other bytes. Instead, the checksum byte uses the modulo operation and effectively contains the remainder after dividing the sum by 255 (this is the same as saying sum % 255). 
Validating the checksum looks like:
```javascript
//ONLY applies to receipts of type ReceiptEnum.DATA_BLOCK
//calculate the sum of the specified status or msg bytes
let calculatedSum = 0;
for(let i = 0; i < 6; i++){
    //add each status byte to the running sum
    calculatedSum += dataBlock[i];
}
calculatedSum = calculatedSum % 255;
let expectedSumVal = dataBlock[6];
return calculatedSum === expectedSumVal;
```
