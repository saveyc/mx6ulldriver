KERNELDIR = /home/k/linux/kernel
CURRENT_PATH := $ (shell pwd)
obj-m := icm20608.o

build : kernel_Modules

kernel_Modules:
	$(make) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(make) -C $(KERNELDIR) M=$(CURRENT_PATH) clean