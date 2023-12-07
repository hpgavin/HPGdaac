# PREEMMPT_RT install log for 32-bit PiOS release 2023-10-10 
 
Instructions at raspberrypi.com for compiling the kernel actually now include an example for compiling with the PREEMPT_RT patch!   thank you!! 

<https://www.raspberrypi.com/documentation/computers/linux_kernel.html#cross-compiling-the-kernel>

In the PREEMPT_RT 6.0 kernel, the CONFIG_PREEMPT_RT_FULL configuration option was still available. This option provided a fully preemptible real-time kernel, allowing for low-latency and deterministic behavior. 

The CONFIG_PREEMPT_RT_FULL configuration option was removed in the PREEMPT_RT 6.1 kernel. As the developers found it to have some limitations and issues they decided to remove it and focus on improving the mainline PREEMPT_RT patch instead. The mainline patch provides a good balance between real-time capabilities and general-purpose functionality. 

Perhaps a future version of the PREEMPT_RT patch will provide the performance enabled by CONFIG_PREEMPT_RT_FULL.   
In the meantime a version of this document for kernel 5.10 (Debian Bullseye) and the PREEMPT_RT patch 5.10-rt (supported until December 2026) will be forthcoming.  

## 0.  The installation of PREEMPT-RT on Raspberry PI involves ... 

 * collecting Raspberry Pi kernel sources and the PREEMPT_RT patch with version numbers and patch levels that match your desktop linux installation
 * patching the Raspberry PI linux kernel sources with the PREEMTP-RT patch
 * cross compiling the patched Raspberry PI Linux kernel source on a linux computer (e.g., a computer running ubuntu) 
 * transferring and installing the patched and compiled Raspberry PI linux kernel on the Raspbery PI

  For this to work the kernel versions of:  
    - the computer-based linux installation (e.g., ubuntu)
    - the Rapsberry Pi OS linux kernel, and  
    - the PREEMPT-RT patch,
  all need to match.

  Check the kernel version of your current RPi installation and your current Linux desktop installation with 

```
uname -a
```

  The versions used to build a new RT kernel today, 2023-12-03, are

  * ubuntu desktop
    Linux echo 6.2.0-37-generic #38~22.04.1-Ubuntu SMP PREEMPT_DYNAMIC Thu Nov  2 18:01:13 UTC 2 x86_64 x86_64 x86_64 GNU/Linux

  * Raspberry Pi OS on RPi 4B     : kernel version   6.1.0
    <https://github.com/raspberrypi/linux>
    <https://github.com/raspberrypi/linux/tree/rpi-6.1.y>

  * PREEMPT_RT patch              : patch version    6.1.
    <https://wiki.linuxfoundation.org/realtime/preempt_rt_versions>
    <https://cdn.kernel.org/pub/linux/kernel/projects/rt/6.1/>

## 1.  Install tools required for cross-compiling the kernel 

```
sudo apt install  rpi-imager
sudo apt install  build-essential binutils automake 
sudo apt install  git bc bison flex libssl-dev make libc6-dev libncurses5-dev
sudo apt install crossbuild-essential-armhf      # ... for 32 bit RPi kernel
sudo apt install crossbuild-essential-arm64      # ... for 64 bit RPi kernel
```

* <https://www.raspberrypi.com/documentation/computers/linux_kernel.html#cross-compiling-the-kernel>
* <https://wiki.linuxfoundation.org/realtime/documentation/howto/applications/preemptrt_setup>
* <https://github.com/raspberrypi/tools> ... read the README.md

## 2.  Install the desired RPi OS version on your RPi SD card (if needed) 

If needed, backup your user data from your Raspberry Pi ~ filesystem 

Use a MicroSD card reader ... e.g., CanaKit Mini MicroSD USB Reader SKU: CK-USB-READER

Install the 32-bit version of Raspberry Pi OS that matches your
desktop installation to SD card via the Raspberry Pi SD card imager app from ...
<https://www.raspberrypi.com/software/>


Start RaspberryPiImager ... 
```
rpi-imager
```

Reformat the SD card to a single fat32 partition using the drop-down menu in rpi-imager. 

Select the RASPBERRY PI OS (32-bit) that matches your Ubuntu OS version 

config: enable SSH

Boot Raspberry Pi with new SD card and check the kernel version
```
uname -a
```

... Linux hpg-rpi00 6.1.0-rpi4-rpi-v8 #1 SMP PREEMPT Debian 1:6.1.54-1-rpt2 (2023-10-05) aarchh64 GNU/Linux 

Confirm the  PiOS is a 32 bit OS
```
getconf LONG_BIT
```
... 32

## 3.  Download the corresponding Raspberry Pi OS kernel and the corresponding PREEMPT_RT patch 

  ...  ubuntu kernel version 6.2.0-37 generic #38~22.04.1-Ubuntu SMP PREEMPT_DYNAMIC Thu Nov 2 18:01:13 UTC x86_64 x86_64 GNU/Linux

```
mkdir /tmp/RPi-rt
cd    /tmp/RPi-rt
git clone --depth=1 --branch rpi-6.1.y https://github.com/raspberrypi/linux
cd    /tmp/RPi-rt/linux
head  Makefile  -n 4   #  confirm the VERSION, PATCHLEVEL, SUBLEVEL
```
Check the RPi kernel version, patch level, and sublevel ... 

```
   # SPDX-License-Identifier: GPL-2.0
   VERSION = 6
   PATCHLEVEL = 1
   SUBLEVEL = 64
```

Identify the patch with the corresponding version, patch level and sublevel from <https://mirrors.edge.kernel.org/pub/linux/kernel/projects/rt/> and download it
```
wget https://mirrors.edge.kernel.org/pub/linux/kernel/projects/rt/6.1/patch-6.1.64-rt17.patch.gz
```

* <https://wiki.linuxfoundation.org/realtime/preempt_rt_versions>
* <https://cdn.kernel.org/pub/linux/kernel/projects/rt/6.1/older/>

## 4.  Patch the kernel with the PREEMPT_RT patch 

```
cd /tmp/RPi-rt/linux
gunzip patch-6.1.64-rt17.patch.gz
zcat patch-6.1.64-rt17.patch | patch -p1 --dry-run    # check that the patch fits
zcat patch-6.1.64-rt17.patch | patch -p1 
```

## 5.  Configure the patched kernel for RPi 4B

```
cd  /tmp/RPi-rt/linux 
make clean
export KERNEL=kernel7l
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export INSTALL_MOD_PATH=/tmp/RPi-rt
export INSTALL_DTBS_PATH=/tmp/RPi-rt
```

Apply the Default Configuration 
```
make bcm2711_defconfig
```

  navigate to ...

  General setup ---> Preemtion Model --->
  select Fully Preemptible Kernel (Real-Time) 

  < Exit >

  Kernel Features ---> Timer Frequency
  select 1000 Hz
    
  < Exit >  < Exit >  and  < Save >  to .config 

  confirm that line 97 (or thereabouts) of .config is ...
    CONFIG_HIGH_RES_TIMERS=y
  confirm that line 117 (or thereabouts) of .config is ...
    CONFIG_PREEMPT_RT=y
  confirm that line 484-485 (or thereabouts) of .config is ...
    CONFIG_HZ_1000=y
    CONFIG_HZ=1000

* <https://www.raspberrypi.com/documentation/computers/linux_kernel.html#configuring-the-kernel>
* <https://lemariva.com/blog/2018/07/raspberry-pi-preempt-rt-patching-tutorial-for-kernel-4-14-y>
* <https://www.raspberrypi.com/documentation/computers/linux_kernel.html#cross-compiling-the-kernel>
* <https://robskelly.com/2020/10/14/raspberry-pi-4-with-64-bit-os-and-preempt_rt/>

## 6.  Build the patched kernel for RPi 4B 

```
nproc    # determine the number of processors 
cd /tmp/RPi-rt/linux
make -j8 zImage 
make -j8 modules 
make -j8 dtbs
make -j8 dtbs_install
make -j8 modules_install
```

   At the end of modules_install output,
   the last section of DEPMOD reports the version of your new RT kernel ...   
   DEPMOD  /tmp/RPi-rt/lib/modules/6.1.64-rt17-v7l+

```
cd /tmp/RPi-rt/linux

mkdir $INSTALL_MOD_PATH/boot
mkdir $INSTALL_MOD_PATH/dts
mkdir $INSTALL_MOD_PATH/boot/overlays
mkdir $INSTALL_MOD_PATH/lib

export KERNELrt=kernel7l_rt

cp arch/arm/boot/zImage $INSTALL_MOD_PATH/boot/$KERNELrt.img
cp arch/arm/boot/dts/*.dtb $INSTALL_MOD_PATH/boot/.
cp arch/arm/boot/dts/overlays/*.dtb* $INSTALL_MOD_PATH/boot/overlays/.
cp arch/arm/boot/dts/overlays/README $INSTALL_MOD_PATH/boot/overlays/.
```

* <https://github.com/raspberrypi/linux/issues/3249>
* <https://www.raspberrypi.com/documentation/computers/linux_kernel.html>
* <https://github.com/iamroot12CD/linux/blob/master/scripts/mkknlimg>
* <https://lemariva.com/blog/2019/09/raspberry-pi-4b-preempt-rt-kernel-419y-performance-test>
* <https://lemariva.com/blog/2018/07/raspberry-pi-preempt-rt-patching-tutorial-for-kernel-4-14-y>
* <https://www.raspberrypi.com/documentation/computers/linux_kernel.html#cross-compiling-the-kernel>

## 7.  Install the patched kernel image, modules, and device tree overlays to the RPi 4B SD card

Using the SD card reader, mount the RPi SD card on your ubuntu system

```
sudo cp -rd /tmp/RPi-rt/boot/*          /media/$USER/bootfs
sudo cp -d  /tmp/RPi-rt/*.dtb           /media/$USER/bootfs
sudo cp -rd /tmp/RPi-rt/lib/*           /media/$USER/rootfs/lib

sudo vi /media/$USER/bootfs/config.txt
```

Add these two lines to the ** TOP **  of /boot/config.txt ...

```
# kernel with PREEMPT_RT
kernel=kernel7l_rt.img
```

Unmount the RPi SD card from your ubuntu system and ensure it has been unmounted 

```
umount /media/$USER/rootfs
umount /media/$USER/bootfs
ls /media/$USER
```

Remove the RPi SD card from the USB SD card reader.
Insert the RPi SD card back into the RPi 4 and start your RPi 4B

```
uname -a
```

should return something like ... 
Linux hpg-rpi00 6.1.64-rt17-v7l+ #1 SMP PREEMPT_RT Sun Dec 3 12:16:23 EST 2023 armv7l GNU/Linux

## 8.  RT-Tests suite

```
sudo apt install rt-tests

sudo cyclictest  -p 90  -i 1000

sudo cyclictest -l50000000 -m -S -p90 -i200 -h400 -q > output.txt
grep -v -e "^#" -e "^$" output.txt | tr " " "," | tr "\t" "," >histogram.csv
sed -i '1s/^/time,core1,core2,core3,core4\n /' histogram.csv
```

## 9. Programming for realtime performace ...

* <https://lemariva.com/blog/2019/09/raspberry-pi-4b-preempt-rt-kernel-419y-performance-test>
* <https://taste.tuxfamily.org/wiki/index.php?title=Tricks_and_tools_for_PREEMPT-RT_kernel>
* <https://stackoverflow.com/questions/35766811/build-an-rt-application-using-preempt-rt>
* <https://rt.wiki.kernel.org/index.php/HOWTO:_Build_an_RT-application>
* <https://elinux.org/images/8/82/How_linux_preempt_rt_works_111207_1100.pdf>


## Other References 

* <https://www.raspberrypi.com/documentation/computers/linux_kernel.html#cross-compiling-the-kernel>
* <https://forums.raspberrypi.com/viewtopic.php?t=343387>
* <https://forums.raspberrypi.com/viewtopic.php?t=344994>
* <https://stackoverflow.com/questions/71645509/raspberry-pi4-kernel-64bit-with-rt-extension>
* <https://lemariva.com/blog/2018/02/raspberry-pi-rt-preempt-tutorial-for-kernel-4-14-y>
* <https://lemariva.com/blog/2019/09/raspberry-pi-4b-preempt-rt-kernel-419y-performance-test>
* <https://github.com/feecat/rpi-rt>
* <https://github.com/feecat/rpi-rt/releases>
* <https://www.instructables.com/64bit-RT-Kernel-Compilation-for-Raspberry-Pi-4B-/>

