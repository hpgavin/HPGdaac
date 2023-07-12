# ========================================================================= #
#	                                                                    #
#	Makefile for building: HPGdaac                                      #
#                                                                           #
#	High Performance Graphical data acquisition and cotrol              #
#	using	the WaveShare HPADDA on a Raspberry Pi                      #
#                                                                           #
# ========================================================================= #


SHELL = /bin/sh

VERSION = date "+%Y.%m%d%" 

DIR_C := ./src
DIR_N := ../HPGnumlib
DIR_X := ../HPGxcblib
DIR_O := ./obj
 
CC      = gcc
DEBUG   = -Wall
CFLAGS  = -g -O0  
CFLAGS += $(DEBUG)   
LFLAGS  = -l bcm2835  -l xcb  -l m  -l rt 

TARGET  = HPGdaac

$(DIR_O)/%.o : $(DIR_N)/%.c
	$(CC) $(CFLAGS) -c  $< -o   $@  

$(DIR_O)/%.o : $(DIR_X)/%.c
	$(CC) $(CFLAGS) -c  $< -o   $@  

$(DIR_O)/%.o : $(DIR_C)/%.c
	$(CC) $(CFLAGS) -c  $< -o   $@  

$(TARGET) : $(DIR_O)/HPGdaac.o $(DIR_O)/HPGutil.o $(DIR_O)/NRutil.o $(DIR_O)/HPGxcb.o $(DIR_O)/HPADDAlib.o $(DIR_O)/HPGcontrol.o
	$(CC) $(CFLAGS)  $^ -o   $@  $(LFLAGS)

install:
	chown root $(TARGET); chmod u+s $(TARGET); mv $(TARGET) /usr/local/bin/.

clean:
	rm $(DIR_O)/*.o 

