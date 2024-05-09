obj-m += sj201-soundcard.o
obj-m += tas5806md.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
