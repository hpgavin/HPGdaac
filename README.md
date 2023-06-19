# HPGdaac

High Performance Graphical data acquisition and control:

An open-source command-line interface between Raspberry Pi and the
[WaveShare High Performance Analog-Digital Digital-Analog](https://www.waveshare.com/high-precision-ad-da-board.htm)
hat for the Raspbery Pi.  

---------------------------------

## HPGdaac Installation 


1. patch the RPi kernel with PREEMPT-RT 
    following instructions in ... PREEMPT-RT-install-log  

2. install GPIO driver source codes
    following instructions in ... bcm2835-software-install

3. include the xcb development sources
```
sudo apt install libx11-xcb-dev
```

4. clone software from github to your RPi, e.g., to your ~/Code/ directory
 
```
mkdir ~/Code
cd ~/Code
git clone https://github/hpgavin/HPGnumlib 
git clone https://github/hpgavin/HPGxcblib 
git clone https://github/hpgavin/HPGdaac  
```

5. make and make install

```
cd ~/Code/HPGdaac
make clean
make
sudo make install
```

---------------------------------

## HPGdaac Usage


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
ch0: acceleromter differential  ch1: geophone differential
Sensor Configuration File Name             : snsrs.cfg
Number of Control Constants                : 0
D/A 0 data file name                       : DA-files/chirp0.dat
D/A 1 data file name                       : DA-files/chirp1.dat
```

### Sensor configuration file

Uers may edit the first line (containing a descriptive title) and the eight line and following lines (with detailed sensor configurations) in their entirety.   Values following the colon (:) may be edited in all other lines.  

Example test configuration file:

 * The 'Channel' column corresponds to the pair of Channel positive and Channel negative pins in the Test Configuration file.  
 * The 'Label'   column is a brief text description of the sensor.
 * The 'Sensitivity' column is the numerical value of the volts-per-physical-unit of the sensor.  
 * The 'V/Unit' column indicates the 'physical unit'
 * The 'DeClip' column indicates how the scaling operation will deal with clipped data
 * The 'Detrend' column indicates how the scaling operation will deal with biased or trending data
 * The 'Smooth' column indicates how much the scaling operation will smooth the data


```
Geophone sensitivity
Xlabel : "seconds"
Ylabel : "volts"
integrate channel     : -1
differentiate channel : -1
Channel Label             Sensitivity   V/Unit          DeClip  Detrend Smooth
===============================================================================
 0      "geophone delta V_d" 1.0           "V"          0       4       0.1
 1      "accelerometer"      1.0           "V"          0       4       0.1
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

---------------------------------

## HPGdaac Performance

... quantitative information to be added soon ...
