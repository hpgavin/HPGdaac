# ========================================================================= #
#	                                                                    #
#	Makefile for building: HPGdaac                                      #
#                                                                           #
#	high performance graphical data acquisition and cotrol              #
#	using	High-Precision Graphical Digital and Digital to Analog      #
#                                                                           #
# ========================================================================= #


SHELL = /bin/sh

VERSION = date "+%Y.%m%d%" 

DIR_C := ./src
DIR_N := ../../HPGnumlib
DIR_X := ../../HPGxcblib
DIR_O := ./obj

SRC_C := $(wildcard $(DIR_C)/*.c)
SRC_N := $(wildcard $(DIR_N)/*.c)
SRC_X := $(wildcard $(DIR_X)/*.c)
SRC_O := $(patsubst $(DIR_N)/%.c, $(DIR_O)/%.o, $(notdir $(SRC_N))) \
         $(patsubst $(DIR_X)/%.c, $(DIR_O)/%.o, $(notdir $(SRC_X))) \
         $(patsubst $(DIR_C)/%.c, $(DIR_O)/%.o, $(notdir $(SRC_C))) 

TARGET = HPGdaac

CC = gcc

DEBUG   = -Wall
CFLAGS  = -g -O0  
CFLAGS += $(DEBUG)   
LFLAGS  = -l bcm2835  -l xcb  -l m  -l rt 


$(TARGET):$(SRC_O)
	$(CC) $(CFLAGS) $(SRC_O) -o $@  $(LFLAGS)

$(DIR_O)/%.o : $(DIR_C)/%.c
	$(CC) $(CFLAGS) -c  $< -o   $@  $(LFLAGS)

$(DIR_O)/%.o : $(DIR_N)/%.c
	$(CC) $(CFLAGS) -c  $< -o   $@  $(LFLAGS)

$(DIR_O)/%.o : $(DIR_X)/%.c
	$(CC) $(CFLAGS) -c  $< -o   $@  $(LFLAGS)

clean:
	rm $(DIR_O)/*.* 

install:
	chown root $(TARGET); chmod u+s $(TARGET); mv $(TARGET) /usr/local/bin/.
