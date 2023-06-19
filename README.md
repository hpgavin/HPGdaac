#HPGdaac

High Performance Graphical data acquisition and control:
A command-line interface between Raspberry Pi and the WaveShare High Performance Analog-Digital Digital-Analog hat for the Raspbery Pi.  

---------------------------------
Software Installation for HPGdaac
---------------------------------

[1] patch the RPi kernel with PREEMPT-RT 
    following instructions in ... PREEMPT-RT-install-log  

[2] install GPIO driver source codes
    following instructions in ... bcm2835-software-install

[3] include the xcb development sources
    sudo apt install libx11-xcb-dev

[4] clone software from github to your RPi, e.g., to ~/Code/
 
    mkdir ~/Code
    cd ~/Code
    git clone https://github/hpgavin/HPGnumlib 
    git clone https://github/hpgavin/HPGxcblib 
    git clone https://github/hpgavin/HPGdaac  

[5] cd ~/Code/HPGdaac
    make clean
    make
    sudo make install


---------------------------------
Usage for HPGdaac
---------------------------------

[1] patch the RPi kernel with PREEMPT-RT 
