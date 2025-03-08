TARGET=dirsync-1_09
STDSRC=dirsync.c regcomp.c regexec.c regfree.c

make:
	@echo Please select your system and issue the command : make system
	@echo Supported system are : watcomc,visualc,linux,msdos. The msdos
	@echo version require a OpenWatcom compiler.
	@echo The linux version can be installed with : make install

#
# MSDOS require watcomc
#
msdos:
	make dirsync.exe EXE=dirsync.exe "SOURCE=${STDSRC} getopt.c" CC="wcl386 -D__MSDOS__ -wx -ox -bt=dos"

#
# WIN32 version with WATCOMC
#
watcomc:
	make dirsync.exe EXE=dirsync.exe "SOURCE=${STDSRC} getopt.c" CC="wcl386 -D__NT__ -wx -ox"

#
# WIN32 Version with VisualC 
# If regex are used optimization must be off!
#
visualc:
	make dirsync.exe EXE=dirsync.exe "SOURCE=${STDSRC} getopt.c dirent.c" CC="cl -D__NT__"

#
# LINUX Version may be work with many Unix system
#
linux:
	make dirsync EXE=dirsync "SOURCE=${STDSRC}" CC="cc -D__LINUX__ -O -o dirsync"


${EXE}: ${SOURCE} makefile
	${CC} -DNDEBUG ${SOURCE}

tar: dirsync.exe dirsync \
     dirsync.c getopt.h getopt.c dirent.h dirent.c \
     regcomp.c  regexec.c  cclass.h cname.h \
     engine.c engine.ih  regcomp.ih  regerror.c regerror.ih \
     regex.h regex2.h regexec.c regfree.c \
     makefile readme.txt dirsync.1 dirsync.xml
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
