TARGET=dirsync-1_03
make:
	@echo Please select your system and issue the command : make system
	@echo Supported system are : watcomc,visualc,linux,msdos. The msdos
	@echo versione require a OpenWatcom compiler.
	@echo The linux version can be installed with : make install

#
# MSDOS require watcomc
#
msdos:
	make dirsync.exe EXE=dirsync.exe "SOURCE=dirsync.c getopt.c" CC="wcl386 -wx -ox -bt=dos"

#
# WIN32 version with WATCOMC
#
watcomc:
	make dirsync.exe EXE=dirsync.exe "SOURCE=dirsync.c getopt.c" CC="wcl386 -D__NT__ -wx -ox"

#
# WIN32 Version with VisualC 6
#
visualc:
	make dirsync.exe EXE=dirsync.exe "SOURCE=dirsync.c getopt.c dirent.c" CC="cl -Ox -D__NT__"

#
# LINUX Version
#
linux:
	make dirsync EXE=dirsync SOURCE=dirsync.c CC="cc -D__LINUX__ -O -o dirsync"


${EXE}: ${SOURCE} makefile
	${CC} -DNDEBUG ${SOURCE}

tar: dirsync.exe dirsync dirsync.c getopt.h getopt.c dirent.h dirent.c makefile readme.txt dirsync.1 dirsync.xml
	rm -f ${TARGET}.tar ${TARGET}.tar.gz ${TARGET}.zip
	tar cf ${TARGET}.tar $+
	gzip ${TARGET}.tar
	pkzip25 -add ${TARGET}.zip $+


#
# Sample install tested on Suse 9.0
# 
install:
	install -D -m 755 dirsync $(DESTDIR)/usr/bin/dirsync
	install -D -m 644 dirsync.1 $(DESTDIR)/usr/local/man/man1/dirsync.1
