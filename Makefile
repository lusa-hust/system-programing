TARGET_MODULE:=ictRedis
BUILDSYSTEM_DIR:=/usr/src/linux-headers-$(shell uname -r)
PWD:=$(shell pwd)
MAKE:=make

ifneq ($(KERNELRELEASE),)
	obj-m := $(TARGET_MODULE).o
else
all:
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) clean
load:
	insmod ./$(TARGET_MODULE).ko
unload:
	rmmod ./$(TARGET_MODULE).ko
endif