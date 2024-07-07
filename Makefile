ifneq ($(KERNELRELEASE),)
	obj-m := chdr.o
else
	KERN_DIR ?= /usr/src/linux-headers-$(shell uname -r)/
	PWD := $(shell pwd)

.PHONY: default
default:
	$(MAKE) -C $(KERN_DIR) M=$(PWD) modules
endif

.PHONY: clean
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.mod modules.order *.symvers
