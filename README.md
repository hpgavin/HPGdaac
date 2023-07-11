# HPGdaac

High Performance Graphing data acquisition and control:

An open-source command-line interface between a Raspberry Pi and a
[WaveShare High Performance Analog-Digital Digital-Analog](https://www.waveshare.com/high-precision-ad-da-board.htm)
(24 bit, 8 channel, 30 kHz/chnl) hat for the Raspbery Pi.  

---------------------------------

## WaveShare High Performance Analog-Digital, Digital Analog hardware configuration

To configure the WaveShare HPADDA board for use with the **HPGdaac**, 
* on the 2x6 pin block, 
  * connect 'VCC' to '5V'
  * connect 'VREF' to '5V'
* on the 2x1 pin block
  * connect AGND to GND
* on the 1x13 screw terminal block, to help prevent accidentally shorting 'VCC' to a signal lead, 
  * mark the 'AGND' terminal with black ink
  * mark the 'VCC' terminal with red ink

---------------------------------

## Installation 

1. patch the RPi kernel with PREEMPT-RT 
    following instructions in ... doc/PREEMPT-RT-install-log  

2. install GPIO driver source codes
    following instructions in ... doc/bcm2835-software-install

3. install the xcb development sources
```
sudo apt install libx11-xcb-dev
```
4. install gnuplot for simple data plotting after the test
```
sudo apt install gnuplot
```

5. clone software from github to your RPi, e.g., to your ~/Code/ directory
```
mkdir ~/Code
cd ~/Code
git clone https://github.com/hpgavin/HPGnumlib 
git clone https://github.com/hpgavin/HPGxcblib 
git clone https://github.com/hpgavin/HPGdaac  
git clone https://github.com/hpgavin/HPGdaac-xtra  
```

6. make and make install
```
cd ~/Code/HPGdaac
make clean
make
sudo make install
```

---------------------------------

## Usage

**HPGdaac** is a command line program for digitizing analog signals while simultaneously sending analog outputs and potentiall performing real-time calculations at each time step.   **HPGdaac** is configured via a test configuration file and a sensor configuration file.   

After installation, **HPGdaac** is run from the command line using:
```
HPGdaac <test configuration filename> <measured data filename> 
```

### Test configuration file

Uers may edit the first line (containing a descritive title) and the ninth line (summarizing sensor configurations) in their entirety.   Values following the colon (:) may be edited in all other lines.  

Example test configuration file:

```
Title: data acquisition via HPGdaac
Acquisition Time (duration of test, sec)   :   33.0
Channel scan rate (scans per sec)          :  200.0
Digitization rate 1000, 2000, 3750, 7500   : 2000.0
Number of Channels   [1 to 8]              :   2
Channel Positive Pin [0 to 7]              :   0   2   2   3   4   5   6   7
Channel Negative Pin [0 to 7] or -1        :   1   3  -1  -1  -1  -1  -1  -1
Voltage Range [0.05 to 5.00] (volts)       : 2.5 1.2 5.0 5.0 5.0 5.0 5.0 5.0
ch0: voltage input Vi   ch1: sensor output Vo
Sensor Configuration File Name             : snsrs.cfg
Number of Control Constants                : 0
D/A 0 data file name                       : DA-files/chirp0.dat
D/A 1 data file name                       : DA-files/chirp1.dat
```

### Sensor configuration file

Uers may edit the first line (containing a descriptive title) and the eight line and following lines (with detailed sensor configurations) in their entirety.   Values following the colon (:) may be edited in all other lines.  

Example sensor configuration file:

 * The 'Channel' column corresponds to the pair of Channel positive and Channel negative pins in the Test Configuration file.  
 * The 'Label'   column is a brief text description of the sensor.
 * The 'Sensitivity' column is the numerical value of the volts-per-physical-unit of the sensor.  
 * The 'V/Unit' column indicates the 'physical unit'
 * The 'DeClip' column indicates how the scaling operation will deal with clipped data
 * The 'Detrend' column indicates how the scaling operation will deal with biased or trending data
 * The 'Smooth' column indicates how much the scaling operation will smooth the data

```
A template sensor configuration file for HPGdaac
Xlabel : "seconds"
Ylabel : "volts"
integrate channel     : -1
differentiate channel : -1
Channel Label             Sensitivity   V/Unit          DeClip  Detrend Smooth
===============================================================================
 0      "voltage input Vi"   1.0           "V"          0       4       0.1
 1      "sensor output Vo"   1.0           "V"          0       4       0.1
 2      "sensor 2"           1.234         "mm"         3       3       0.1
 3      "sensor 3"           5.678         "g"          3       3       0.1
 4      "sensor 4"           9.876         "g"          3       3       0.1
 5      "sensor 5"           5.432         "g"          3       3       0.1
 6      "sensor 6"           1.234         "g"          3       3       0.1
 7      "sensor 7"           5.678         "g"          3       3       0.1
```


 Clip Correction Types:

          0:  no clip correction
          3:  fit a cubic polynomial to the clipped region
          5:  fit a fifth order polynomial to the clipped region


 Detrending Types: 

          0:   none       - no detrending
          1:   debias     - subtract the average value of each time series
          2:   detrend    - subtract the ordinary least squares straight line through the data
          3:   baseline   - subtract a line passing through the fist point and the last point
          4:   first_pt   - subtract the first point
          5:   peak_peak  - make the max equal to the negative of the min

 Smoothing level value (a number between 0 and 1) 

           0     :  no smoothing
       0 <  < 1  : intermediate smoothing
           1     :  maximum smoothing

---------------

After executing the command line ...

```
HPGdaac <test configuration filename> <measured data filename> 
```



... **HPGdaac** configures the internal parameters of the HPADDA, opens a window for plotting the data in real time, and asks if the user is ready.  
Pressing "[enter]" or "Y [enter]" intiates the test.   Measured data is displayed to the screen the instant it is measured,
and when the test is complete
**HPGdaac** writes the data to the measured data file (a plain text file) in which the provided *measured data filename* is appended by a date-time stamp of the test.   The user may then choose to delete or retain the data file.   

### Measured data file header and format

Every data file created by **HPGdaac** has a standard twelve-line header and columns of space delimited data in units of least significant bit (LSB). 

For example, running ...
```
HPGdaac test.cfg  data123  
```
... at 3:14:16 on Tuesday March 14, 2023, with the *test configuration file* shown above, results in a **raw** data file which has a header of 12 lines ...

```
% Tue  Mar 14 03:14:16 2023
% Title: data acquisition via HPGdaac
% Data file 'data123.20230314.031416' created using configuration 'test.cfg'
% 6600 scans of 2 channels at 200 scans per second in 33.000 seconds
% ch0: voltage input Vi   ch1: sensor output Vo
% voltage ranges
%   2.5000    1.2000
% pre-test sample average
%     -564    125294
% pre-test sample rms
%       54        92
%   chn  0    chn  1
      1132    134224
     -1124    203892
   -211952  -7978718
   -166210  -4587630
    -51298  -1844884
      6090   2242276
```

* line 1: date-time 
* line 2: line 1 of the test configuration file
* line 3: the data file name (with the time stamp) and the configuration file
* line 4: the number of channel scans, scan rate, and collection duration
* line 5: line 9 of the test configuration file
* line 6-7: volage ranges from the test configuration file for each measured channel
* line 8-9: initial offest for each measured channel
* line 10-11: initial root mean square for each measured channel
* line 12: header data for each measured channel

The subsequent space delimitted columns of data are in units of least significant bit (LSB).   
This is the most compact and precice way to store the data.   

### Scaling the measured data file to desired units

The program **scale** from the [HPGdaac-xtra](https://www.github.org/hpgavin/HPGdaac-xtra) repository uses the *sensor configuration file* to convert the measured data from units of LSB to the units specified in *sensor configuration file.*

Usage ...
```
scale <sensor config file> <raw data file> <scaled data file> <data stats file> 
```

For example, running ...
```
scale  snsrs.cfg  data123.20230314.031416  data123.20230314.031416.scl  dataStats 
```
... with the snsrs.cfg being the  *sensor configuration file* shown above, 
results in the named **scaled** data file (*data123.20230314.031416.scl*)
which has a header of 19 lines  ... 
```
% Tue  Mar 14 03:14:16 2023
% Title: data acquisition via HPGdaac
% Data file 'data123.20230314.031416' created using configuration 'test.cfg'
% 6600 scans of 2 channels at 200 scans per second in 33.000 seconds
% ch0: voltage input Vi   ch1: sensor output Vo
% voltage ranges
%   2.5000    1.2000
% pre-test sample average
%     -564    125294
% pre-test sample rms
%       54        92
% sensor configuration file name: snsrs.cfg
% A template sensor configuration file for HPGdaac
% sensitivity: 1.00000000e+00   1.00000000e+00
% clip-corr. :  0                0
% detrending : firstPoint       firstPoint
% smoothing  : 0.100            0.100
%  time         chn  0           chn  1
%  seconds      V                V
  5.0000e-03   2.95856036e-02   6.83738828e-01
  1.0000e-02   2.82409228e-02   7.18620121e-01
  1.5000e-02  -9.74223763e-02  -3.37159777e+00
  2.0000e-02  -7.01580197e-02  -2.45410538e+00
  2.5000e-02  -1.66511536e-03  -7.57470250e-01
  3.0000e-02   3.25408019e-02   1.55045390e+00
  3.5000e-02   3.18899192e-02   1.40107858e+00
  4.0000e-02   1.35447998e-02   6.61691427e-01
  4.5000e-02   9.27710719e-03   8.72385323e-01

   :            :                :

  2.9995e+01   3.40106525e-02   6.96677923e-01
  3.0000e+01   3.28900851e-02   6.81472659e-01
% MINIMUM     -1.04121006e+00  -9.35091591e+00
% MAXIMUM      1.21445441e+00   1.07766724e+01
% R.M.S.       1.90395564e-01   2.30585074e+00

```

* line 1-11: a copy of lines 1-11 of the raw data file
* line 12: the sensor configuration file name 
* line 13: line 1 of the sensor configuration file
* line 14: sensor voltage sensitivity of each channel
* line 15-17: columns 5, 6, and 7 of the sensor configuration file 
* line 18:  channel numbers for each column
* line 19:  scaled units from column for of the sensor configuration file 

The subsequent space delimitted columns of data are scaled to the units specified in the sensor configuration file. 
The last lines of the scaled data file provide the maximum, minimum, and root mean square (RMS) in the scaled units. 

In addition to scaling the raw data to the desired units, the **scale** corrects for channel-to-channel skew, 
and optionally interpolates clipped data, applies some smoothing, and detrends the data.   

**scale** appends the *data stats file* with the summary of the maximum, minimum, and root mean square (RMS) of the scaled data.  

---------------------------------

## Performance

... quantitative information to be added soon ...

---------------------------------

## Realtime feedback control

... quantitative information to be added soon ...

---------------------------------

## Acknowledgements 

* [WaveShsare](https://www.waveshare.com/)
* [The Curious Scientist](https://www.youtube.com/c/CuriousScientist)
* [HPADDAlibrary](https://github.com/shujima/HPADDAlibrary)
