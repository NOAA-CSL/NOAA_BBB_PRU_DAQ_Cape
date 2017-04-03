Pin use for the NOAA BeagleBone Black PRU and DAQ Cape
======================================================

P8 46-Pin Connector
-------------------

|*Pin*|*$PINS*|*Address*|*Mode*|*Name*|*Use*|
|---|---|---|---|---|---|
|P8.1| | | |DGND|Digital Ground|
|P8.2| | | |DGND|Digital Ground|
|P8.3| | | | |(allocated emmc2)|
|P8.4| | | | |(allocated emmc2)|
|P8.5| | | | |(allocated emmc2)|
|P8.6| | | | |(allocated emmc2)|
|P8.7| | | | |(unused)|
|P8.8| | | | |(unused)|
|P8.9|39 |0x89c/09c |7 |GPIO 69 In |Available |
|P8.10|38 |0x898/098 |7 |GPIO 68 In |Available |
|P8.11|13 |0x834/034 |7 |GPIO 45 In |Available |
|P8.12|12 |0x830/030 |7 |GPIO 44 Out |ms timing test |
|P8.13|9 |0x824/024 |7 |GPIO 23 Out |1 s timing test |
|P8.14|10 |0x828/028 |7 |GPIO 26 Out |AI out of range  |
|P8.15| | | | |(unused) |
|P8.16| | | | |(unused) |
|P8.17| | | | |(unused) |
|P8.18| | | | |(unused) |
|P8.19| | | | |(unused) |
|P8.20| | | | |(allocated emmc2) |
|P8.21| | | | |(allocated emmc2) |
|P8.22| | | | |(allocated emmc2) |
|P8.23| | | | |(allocated emmc2) |
|P8.24| | | | |(allocated emmc2) |
|P8.25| | | | |(allocated emmc2) |
|P8.26| | | | |(unused) |
|P8.27|31 |0x8e0/0e0 |6 |pr1_pru1_pru_r31_8 |Data ready |
|P8.28|56 |0x8d8/0d8 |6 |pr1_pru1_pru_r31_10 |Spare, pulled high |
|P8.29|58 |0x8e4/0e4 |6 |pr1_pru1_pru_r31_9 |Over/under flow |
|P8.30|57 |0x8ec/0ec |6 |pr1_pru1_pru_r31_11 |STOP |
|P8.31| | | | |(unused) |
|P8.32| | | | |(unused) |
|P8.33| | | | |(unused) |
|P8.34| | | | |(unused) |
|P8.35| | | | |(unused) |
|P8.36| | | | |(unused) |
|P8.37| | | | |(unused) |
|P8.38| | | | |(unused) |
|P8.39|46 |0x8b8/0b8 |6 |pr1_pru1_pru_r31_6 |Low byte, bit 6 |
|P8.40|47 |0x8bc/0bc |6 |pr1_pru1_pru_r31_7 |Low byte, bit 7 |
|P8.41|44 |0x8b0/0b0 |6 |pr1_pru1_pru_r31_4 |Low byte, bit 4 |
|P8.42|45 |0x8b4/0b4 |6 |pr1_pru1_pru_r31_5 |Low byte, bit 5 |
|P8.43|42 |0x8a8/0a8 |6 |pr1_pru1_pru_r31_2 |Low byte, bit 2 |
|P8.44|43 |0x8ac/0ac |6 |pr1_pru1_pru_r31_3 |Low byte, bit 3 |
|P8.45|40 |0x8a0/0a0 |6 |pr1_pru1_pru_r31_0 |Low byte, bit 0 |
|P8.46|41 |0x8a4/0a4 |6 |pr1_pru1_pru_r31_1 |Low byte, bit 1 |

P9 46-Pin Connector
-------------------

|*Pin*|*$PINS*|*Address*|*Mode*|*Name*|*Use*|
|---|---|---|---|---|---|
|P9.1| | | |GND|Ground|
|P9.2| | | |GND|Ground|
|P9.3| | | |DC 3.3 V | |
|P9.4| | | |DC 3.3 V | |
|P9.5| | | |VDD_5 V | |
|P9.6| | | |VDD_5 V | |
|P9.7| | | |SYS_5 V | |
|P9.8| | | |SYS_5 V | |
|P9.9| | | |PWR_BUT |Power Button |
|P9.10| | | |SYS_RESETn|Reset Button |
|P9.11| | | | |(unused) |
|P9.12| | | | |(unused) |
|P9.13|29 |0x874/074 |7 |GPIO 31 Out |SPI SCLK |
|P9.14|18 |0x848/048 |7 |GPIO 50 Out |SPI CBS |
|P9.15|16 |0x840/040 |7 |GPIO 48 In |SPI SDO |
|P9.16|19 |0x84c/04c |7 |GPIO 51 Out |SPI SDI |
|P9.17| | | | |(unused) |
|P9.18| | | | |(unused) |
|P9.19|95 |0x97c/17c |3 |I2C2_SCL |/dev/i2c-1 |
|P9.20|94 |0x978/178 |3 |I2C2_SDA |/dev/i2c-1 |
|P9.21|85 |0x954/154 |1 |uart2_txd |RS232 port 2 txd |
|P9.22|84 |0x950/150 |1 |uart2_rxd |RS232 port 2 rxd |
|P9.23| | | | |(unused) |
|P9.24|97 |0x984/184 |0 |uart1_txd |RS232 port 1 & TTL port1 txd |
|P9.25|107 |0x9ac/1ac |6 |pr1_pru0_pru_r31_7 |High byte, bit 7 |
|P9.26|96 |0x980/180 |0 |uart1_rxd |RS232 port 1 & TTL port 1 rxd |
|P9.27|105 |0x9a4/1a4 |6 |pr1_pru0_pru_r31_5 |High byte, bit 5 |
|P9.28|103 |0x99c/19c |6 |pr1_pru0_pru_r31_3 |High byte, bit 3 |
|P9.29|101 |0x994/194 |6 |pr1_pru0_pru_r31_1 |High byte, bit 1 |
|P9.30|102 |0x998/198 |6 |pr1_pru0_pru_r31_2 |High byte, bit 2 |
|P9.31|100 |0x990/190 |6 |pr1_pru0_pru_r31_0 |High byte, bit 0 |
|P9.32| | | |VADC |1.8 V Reference |
|P9.33| | | |AIN4 |LD_Mon |
|P9.34| | | |AGND |ADC Ground |
|P9.35| | | |AIN6 |BatV |
|P9.36| | | |AIN5 |Temp |
|P9.37| | | |AIN2 |LD_Temp |
|P9.38| | | |AIN3 |Laser_FB |
|P9.39| | | |AIN0 |POPS_Flow |
|P9.40| | | |AIN1 |Pump_FB |
|P9.41B|109 |0x9a8/1a8 |6 |pr1_pru0_pru_r31_6 |High byte, bit 6 |
|P9.42B|80 |0x9a0/1a0 |6 |pr1_pru0_pru_r31_4 |High byte, bit 4 |
|P9.43| | | |GND | |
|P9.44| | | |GND | |
|P9.45| | | |GND | |
|P9.46| | | |GND | |