
PROGS=join_images peak_count
SITE=https://dds.cr.usgs.gov/srtm/version2_1/SRTM3/Eurasia/

CFLAGS=-Wall 

all: ${PROGS}

%.hgt.zip:
	wget -b ${SITE}$@

%.hgt: %.hgt.zip
	unzip $<

%.0.0.png: %.hgt
	${HGT2PNG} a $< ./ 1201 1201

%.pgm: %.0.0.png
	convert $^ $@

join_images: join_images.o
	gcc -o $@ -l netpbm -lm $@.o
	
peak_count: peak_count.o
	gcc -o $@ -l netpbm $@.o
	
%.o: %.c
	gcc -c ${CFLAGS} $<

push:
	git add .
	git commit -m "update"
	git push

test_v: N41W009.pgm N40W009.pgm
	./join_images $^ out_v1.pgm out_v2.pgm

test_h: N41W009.pgm N41W008.pgm
	./join_images -s $^ out_v1.pgm out_v2.pgm

test: test_v test_h

	

