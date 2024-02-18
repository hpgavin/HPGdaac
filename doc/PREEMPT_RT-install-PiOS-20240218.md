# Patching and building Rapsberry Pi OS with PREEMPT_RT ... on 2024-02-18 
 
These instructions are modified from Raspberry Pi kernel compilation documentation
which includes an example of patching with PREEMPT_RT and building the kernel.   (thank you!!)

* <https://www.raspberrypi.com/documentation/computers/linux_kernel.html#building-the-kernel-locally>

Note: as of now, HPGdaac is confired to work on 32 bit RPi OS with PREEMPT_RT patches.    It is not yet clear if the HPADDA hardware can work on 64 bit RPi with PREEMPT_RT patches.  

## 0.  Patching, configuring and building Raspberry Pi OS with PREEMPT_RT involves ... 

 * downloading Raspberry Pi OS kernel sources and the PREEMPT_RT patch matching your current Raspberry Pi OS version number 
 * patching the Raspberry Pi OS kernel sources with the matching PREEMPT_RT patch
 * configuring the patched Raspberry Pi OS kernel source on the Raspberry Pi 
 * building the patched Raspberry Pi OS kernel on the Raspberry Pi 
 * installing the built Raspberry Pi OS kernel onto the Raspbery Pi

First, update and upgrade your OS distribution and install required packages
```
sudo apt update
sudo apt upgrade
<reboot>
sudo apt install git bc bison flex libssl-dev make libncurses-dev
```

To check the kernel version, distribution name, and bits of your current Raspberry Pi OS installation ...
```
uname -a
hostnamectl
getconf LONG_BIT  # confirm the  PiOS is a 32 bit or a 64 bit OS
```

  The versions used to successfully build a PREEMPT_RT patched kernel today, 2024-02-18, are

  * Raspberry Pi OS on RPi 4B     : kernel version   6.1.77
    <https://github.com/raspberrypi/linux/tree/rpi-6.1.y>

  * PREEMPT_RT patch              : patch version    6.1.77
    <https://wiki.linuxfoundation.org/realtime/preempt_rt_versions>

## 1.  Download the Raspberry Pi OS kernel sources and the PREEMPT_RT patch for your current Raspberry Pi installation

```
mkdir Code/RPi-rt
cd    Code/RPi-rt
git clone --depth=1 --branch rpi-6.1.y https://github.com/raspberrypi/linux
wget https://mirrors.edge.kernel.org/pub/linux/kernel/projects/rt/6.1/patch-6.1.77-rt24.patch.gz
```
Confirm the RPi kernel source version, patch level, and sublevel ... 
```
cd    Code/RPi-rt/linux
head  Makefile  -n 4   #  confirm the VERSION, PATCHLEVEL, SUBLEVEL
# ... results in ... 
   # SPDX-License-Identifier: GPL-2.0
   VERSION = 6
   PATCHLEVEL = 1
   SUBLEVEL = 77
```
The VERSION and PATCHLEVEL of the linux source need to match the output of hostnamectl, above ...

* <https://wiki.linuxfoundation.org/realtime/preempt_rt_versions>
* <https://cdn.kernel.org/pub/linux/kernel/projects/rt/6.1/older/>

## 2.  Patch the kernel sources with the PREEMPT_RT patch 

```
cd Code/RPi-rt/linux
gunzip patch-6.1.77-rt24.patch.gz
cat patch-6.1.77-rt24.patch | patch -p1 --dry-run    # check that the patch fits
cat patch-6.1.77-rt24.patch | patch -p1 
```

## 3.  Configure the patched kernel sources for your Raspberry Pi OS version 

```
cd  Code/RPi-rt/linux 
make clean
#export KERNEL=kernel7l        # for 32 bit OS on a RPi 4B
export KERNEL=kernel8         # for 64 bit OS on a RPi 4B
#export KERNEL=kernel_2712     # for 64 bit OS on a RPi 5
#export ARCH=arm               # for 32 bit OS 
export ARCH=arm64             # for 64 bit OS 
make bcm2711_defconfig        # ... apply the default Configuration for RPi 4B
#make bcm2712_defconfig        # ... apply the default Configuration for RPi 5
make menuconfig               # ... edit the configuration for PREEMPT_RT
```

  navigate to ...

  General setup ---> Preemtion Model --->
  select Fully Preemptible Kernel (Real-Time) 

  < Exit >

  Kernel Features ---> Timer Frequency
  select 1000 Hz
    
  < Exit >  < Exit >  and  < Save >  to .config 

check .config to confirm it contains these lines (32 bit OS for RPi 4) ...
```
 CONFIG_LOCALVERSION="-v8"
 CONFIG_HIGH_RES_TIMERS=y
 CONFIG_PREEMPT_RT=y
 CONFIG_HZ_1000=y
 CONFIG_HZ=1000
```
* if you are running a 32 bit OS, and CONFIG_LOCALVERSION is not -v7l then something went wrong with this configuration step.  re-do step 3.
* if you are running a 64 bit OS, and CONFIG_LOCALVERSION is not -v8 then something went wrong with this configuration step.  re-do step 3.
* otherwise, for a 32 bit OS, edit .config to read ...
```
CONFIG_LOCALVERSION="-v7l-rt"
```
* otherwise, for a 64 bit OS, edit .config to read ...
```
CONFIG_LOCALVERSION="-v8-rt"
```
* <https://www.raspberrypi.com/documentation/computers/linux_kernel.html#configuring-the-kernel>
* <https://www.raspberrypi.com/documentation/computers/linux_kernel.html#configuring-the-kernel>
* <https://lemariva.com/blog/2018/07/raspberry-pi-preempt-rt-patching-tutorial-for-kernel-4-14-y>
* <https://www.raspberrypi.com/documentation/computers/linux_kernel.html#cross-compiling-the-kernel>
* <https://robskelly.com/2020/10/14/raspberry-pi-4-with-64-bit-os-and-preempt_rt/>

## 4.  Build the patched kernel 

```
nproc                         # the number of processors 
cd /tmp/RPi-rt/linux
make help                    
# for 32 bit confirm that zImage (or Image), modules, and dtbs are marked with a "*"
# for 64 bit confirm that vmlinux, modules, dtbs, and Image.gz are marked with a "*"
make -j4 all # this will take about three hours on a RPi 4
sudo make modules_install
```
   At the end of modules_install output,
   the last section of DEPMOD reports the version of your new RT kernel ...   
```
  DEPMOD  /tmp/RPi-rt/lib/modules/6.1.77-rt24-v7l-rt+   # 32 bit kernel
  DEPMOD  /lib/modules/6.1.77-rt24-v8-rt+               # 64 bit kernel
```
## 5.  Install the patched and built kernel onto your Raspberry Pi 

```
cd Code/RPi-rt/linux

 # for Bullseye ... 
sudo cp -v arch/arm/boot/dts/*.dtb /boot/
sudo cp -v arch/arm/boot/dts/overlays/*.dtb* /boot/overlays/
sudo cp -v arch/arm/boot/dts/overlays/README /boot/overlays/
sudo cp -v arch/arm/boot/zImage /boot/$KERNEL.img

 # for Bookworm  ... 
sudo cp -v arch/arm64/boot/dts/broadcom/*.dtb /boot/firmware/
sudo cp -v arch/arm64/boot/dts/overlays/*.dtb* /boot/firmware/overlays/
sudo cp -v arch/arm64/boot/dts/overlays/README /boot/firmware/overlays/
sudo cp -v arch/arm64/boot/Image.gz /boot/firmware/$KERNEL.img
```
For 32 bit OS, at the top of /boot/config.txt ... specify the .img file in /boot/ (or /boot/firmware/) to use ...
```
# kernel with PREEMPT_RT
kernel=kernel7l.img
```
In the [pi4] section of /boot/config.txt ... add ...
```
arm_64bit=0

For 64 bit OS, at the top of /boot/firmware/config.txt ... specify the .img file in /boot/ (or /boot/firmware/) to use ...
```
# kernel with PREEMPT_RT
kernel=kernel8.img
```
Confirm that /boot/firmware/config.txt includes the line ... arm_64bit=1 

To enable SPI, uncomment  dtparam=spi=on

```
reboot
```
sudo shutdown -r now 
```
After rebooting ... 
```
uname -a
```
For 32 bit kernels, this should indicate PREEMPT_RT like ... 
```
Linux hpg-rpi-00 6.1.77-rt24-v7l-rt+ #1 SMP PREEMPT_RT Sat Feb 17 14:49:50 EST 2024 armv7l GNU/Linux
```
For 64 bit kernels, this should indicate PREEMPT_RT like ... 
```
Linux hpg-rpi-00 6.1.77-rt24-v8-rt+ #1 SMP PREEMPT_RT Sun Feb 18 15:04:44 EST 2024 aarch64 GNU/Linux
```


* <https://www.raspberrypi.com/documentation/computers/config_txt.html#kernel>
* <https://www.raspberrypi.com/documentation/computers/linux_kernel.html>
* <https://github.com/raspberrypi/linux/issues/3249>
* <https://github.com/iamroot12CD/linux/blob/master/scripts/mkknlimg>
* <https://lemariva.com/blog/2019/09/raspberry-pi-4b-preempt-rt-kernel-419y-performance-test>
* <https://lemariva.com/blog/2018/07/raspberry-pi-preempt-rt-patching-tutorial-for-kernel-4-14-y>

## 6.  RT-Tests suite

```
sudo apt install rt-tests

sudo cyclictest  -p 90  -i 1000

sudo cyclictest -l50000000 -m -S -p90 -i200 -h400 -q > output.txt
grep -v -e "^#" -e "^$" output.txt | tr " " "," | tr "\t" "," >histogram.csv
sed -i '1s/^/time,core1,core2,core3,core4\n /' histogram.csv
```

## 7. Programming for realtime performance ...

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
