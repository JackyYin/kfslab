TARGET =


.PHONY: clean

all:
	make -C kmod

clean:
	make -C kmod clean
