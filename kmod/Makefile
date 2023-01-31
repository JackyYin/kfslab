obj-m += labfs.o
labfs-objs := fs.o inode.o file_ops.o

ccflags-y += -g -Wall

KDIR ?= /lib/modules/`uname -r`/build

PWD := $(CURDIR)

build:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
