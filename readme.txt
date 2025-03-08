About
============================================================================

DirSync is a Directory Synchronizer for Linux & Windows.
Derived from an obsolete project named DirImage created in 1994.

description
============================================================================
DirSync is a Directory Synchronizer. This utility take two arguments --  the source directory and the destination directory -- and recursively ensures that the two directories are identical. DirSync can be used to create incremental copies of large chunks of data. For example, if your file server's contents are in the directory /data, you can make a copy in a directory called /backup with the command "dirsync /data /backup."

The first time you run dirsync, all data will be copied. When you issue the command again, though, only the changed files are copied. Both source code and an executable for Win32 and Linux are included in the distribution. Also it is possible compile dirsync for MS-DOS environment with OpenWatcom. It's very simple to compile DirSync for other Unix-like systems, too. Because the program works in all Win32 environments, you can run it with WinNt, Win2K and WinXP.

Changes
============================================================================
1.11	11/04/2006
	Thanks to Jaroslaw Rafa for the following modification :

	- Fixed bug in ArraySearch()
	- Convert all comment from c++ to c style to compile on sun machine.
	- Set modify/access time also for directories and not only files.
	- Set uid/gid of copied files.
	- Handle of symbolic links.
	- Added mode 3 with like mode 0 but do not check date/time on
	 symbolic links.

1.10	28/01/2006
	Fixed bug in the distribution package.

1.09	20/01/2006
	Added support for mode 2 (-m 2) in this mode dirsync do not copy or
	remove any file/directories but only show the difference 
	using this code :
		0 name - Directory not present in the source
		1 name - Directory not present int the destination
		2 name - File with different size
		4 name - File not equal.
		5 name - File not present in the source
	If this option it is used with -l in the log will be registred the
	word ERROR the file and the description.

	This option work better with -v 0

	Added option -D format (in strftime format) to change the date format
	used for name the log file . Default is %d-%m-%Y

	Added option -T format (in strftime format) to change the format used
	to log the time.  Default %d/%m/%Y %H:%M:%S.

	Added option -l file to log all the operation in the specified file
	if the file name is - the name will be the current date with the format
	specified in -D option and extension .log.
	All the operation are logged in the file with the current time using
	the format specified in -T option.

1.08	11/21/2005
	Update manual.
	Added support for option -V to verify copied data. After the copy all 
	data are read from the destination and compared with the source. This
	operation can be time consuming on large file.

1.07	12/28/2004
	Added warning when file size change during a copy.
	Improved statistics of copied file(s).
	Removed a printf in mached regular expression.

1.06	11/26/2004
	Added thre "Henry Spencer's regular expression library" to support
	the option -e to exclude files/directories based on regular expression.
	Bug fixed in the option -x and -X with a file argument.

1.05	11/10/2004
	Corrected a bug in reading file without permission.
	Corrected bug in MSDOS version.

1.04	10/26/2004
	Corrected error on file copy counter when an error occours.
	Bug fixed on handling root file system.

1.03	10/18/2004
	Added a getopt.h to the source distribution.
	Corrected a bug when the source or destination end in \ 
	thanks to Jim Cassidy.
	Corrected a bug in display file copy percentage of file bigger
	that 200 MB.

1.02	10/08/2004
	Added support for MSDOS version (see makefile).
	Added man page and install for linux.
	Removed html documentation.

1.01	10/06/2004
	Removed some unused code.
	Improved performance for large directories (more than 10000 entries).
	Added -m and -r options.
	Corrected some English translation.
	Corrected error in getopt error message.

1.00	09/22/2004	
	Fist public release.
============================================================================


History
============================================================================

DirSync is derived from an obsolete project named DirImage created in 1994. DirImage is still downloadable here for the
operating systems listed below; sorry, but all the programs are in Italian.

Notes
============================================================================

original web address (no longer reachable):
http://www.viara.eu/en/dirsync.htm

For any comment or suggestion : mario __at__ viara __dot__ eu
