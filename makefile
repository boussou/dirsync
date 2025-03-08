TARGET=dirsync-1_01
make:
	@echo Please select your system and issue the command : make system
	@echo Supported system are : watcomc,visualc,linux.

watcomc:
	make dirsync.exe EXE=dirsync.exe "SOURCE=dirsync.c getopt.c" CC="wcl386 -D__NT__ -wx -ox"

visualc:
	make dirsync.exe EXE=dirsync.exe "SOURCE=dirsync.c getopt.c dirent.c" CC="cl -Ox -D__NT__"

linux:
	make dirsync EXE=dirsync SOURCE=dirsync.c CC="cc -D__LINUX__ -O -o dirsync"

${EXE}: ${SOURCE} makefile
	${CC} -DNDEBUG ${SOURCE}

tar: dirsync.exe dirsync dirsync.c getopt.c dirent.h dirent.c makefile readme.txt \
	dirsync.html
	rm -f ${TARGET}.tar ${TARGET}.tar.gz ${TARGET}.zip
	tar cf ${TARGET}.tar $+
	gzip ${TARGET}.tar
	pkzip25 -add ${TARGET}.zip $+
