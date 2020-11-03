CC = mipsel-unknown-elf-gcc
PSX_ROOT = /usr/local/psxsdk
LIC_FILE = ${PSX_ROOT}/share/licenses/infoeur.dat
LIB = -L${PSX_ROOT}/lib
INCLUDE = -I${PSX_ROOT}/include
LDSCRIPT = -T${PSX_ROOT}/mipsel-unknown-elf/lib/ldscripts/playstation.x
CCFLAGS = -fsigned-char -msoft-float -mno-gpopt -fno-builtin -G0 ${LIB} ${INCLUDE} ${LDSCRIPT}


all: build hola.hsf
	mkpsxiso hola.hsf hola.bin ${LIC_FILE}

hola.hsf:
	mkisofs -o hola.hsf -V HOLA -sysid PLAYSTATION cd_root

build:
	${CC} ${CCFLAGS} -o hola.elf src/main.c -lm
	elf2exe hola.elf hola.exe 
	cp hola.exe cd_root
	systemcnf hola.exe > cd_root/system.cnf

clean:
	rm hola.bin
	rm hola.cue
	rm hola.hsf
	rm hola.elf
	rm hola.exe
	rm cd_root/hola.exe
	rm cd_root/system.cnf
