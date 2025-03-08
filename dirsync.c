/**
 * $Id: dirsync.c,v 1.30 2006/04/11 16:42:01 mviara Exp $
 * $Name:  $
 * 
 * Directories syncronizer. Derived from dirimage.
 *
   Revision 1.27  2006/02/27 14:54:03  raj
   Added mode 3 of copying - don't check mtimes on symlinks (useful when
   doing a backward synchronization)
 
   Revision 1.26  2006/02/26 22:00:27  raj (raj@ap.krakow.pl)
   Release 1.11
   Changed C++ style comments (// ...) to regular C-style comments to avoid
   syntax errors in some compilers (eg. Sun's cc)
   Fixed coredump in ArraySearch function when the table was empty
   Made the program set access/modify times on directories too (under Unix)
   Made the program set the ownership of files/dirs if run by root (Unix)
   Made the program duplicate symlinks instead of copy their contents,
   unless -L switch is specified (Unix)
   Changed mode 2 to show also files with different mtimes, not only with
   different sizes
 *
 * Revision 1.25  2005/11/26 14:59:12  mviara
 *
 * Tested in linux.
 *
 * Revision 1.24  2005/11/26 14:39:49  mviara
 * Added support for mode 2 and log.
 *
 * Revision 1.23  2005/11/05 20:54:16  mviara
 * Release 1.08
 *
 * Revision 1.22  2005/11/04 08:06:28  mviara
 * Added support for verify copied data.
 *
 * Revision 1.21  2005/06/04 05:45:51  mviara
 *
 * Begin version 1.08, added supporto for ignore block devices.
 * Fixed bug in linux manual.
 *
 * Revision 1.20  2004/12/28 15:09:55  mviara
 * Added warning when file size change during a copy.
 *
 * Revision 1.19  2004/12/07 00:15:47  mviara
 * Improved statistics.
 *
 * Revision 1.18  2004/11/30 17:09:43  mviara
 * Removed unnecessary printf.
 *
 * Revision 1.17  2004/11/25 19:17:38  mviara
 * Tested in win32 enviroment.
 *
 * Revision 1.16  2004/11/25 18:58:33  mviara
 *
 * Added the "Henry Spencer's regular expression library" to support regular
 * expression matching in exclude files/directories.
 *
 * Revision 1.15  2004/11/10 18:14:37  mviara
 * Version 1.05 Win32
 *
 * Revision 1.14  2004/11/05 16:23:44  mviara
 *
 * Version 1.05
 *
 * Revision 1.13  2004/10/26 11:22:41  mviara
 * Bug fixed on handling root file system.
 *
 * Revision 1.12  2004/10/25 08:45:58  mviara
 * Corrected error on file count.
 *
 * Revision 1.11  2004/10/18 08:49:48  mviara
 * Version 1.03
 *
 * Revision 1.10  2004/10/16 09:04:53  mviara
 * Corrected a bug in source or dest ending in \ or /
 *
 * Revision 1.9  2004/10/08 18:15:42  mviara
 *
 * Added option from a text file.
 *
 * Revision 1.8  2004/10/07 13:59:58  mviara
 *
 * Improved compatibility (removed <lcms.h>)
 *
 * Revision 1.7  2004/10/07 13:55:33  mviara
 *
 * Added install for linux.
 * Added #include <unistd.h> to compile in athore linux system.
 *
 * Revision 1.6  2004/10/07 13:40:40  mviara
 * Added support for MSDOS.
 *
 * Revision 1.5  2004/10/06 03:31:57  mviara
 * Version 1.01
 *
 * Revision 1.4  2004/10/02 04:59:04  mviara
 * Improved performance on large directory.
 *
 * Revision 1.3  2004/09/30 19:32:46  mviara
 * Added -r and -m options.
 *
 * Revision 1.2  2004/09/30 18:05:39  mviara
 * Corrected some english error.
 *
 * Revision 1.1.1.1  2004/09/27 13:12:59  mviara
 * First imported version
 *
 */
static char rcsinfo[] = "$Id: dirsync.c,v 1.30 2006/04/11 16:42:01 mviara Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#ifndef __LINUX__
#include <io.h>
#include <sys/utime.h>
#include <limits.h>
#ifdef __WATCOMC__
#include <direct.h>
#else
#include "dirent.h"
#endif
#include "getopt.h"
#endif

#ifdef __LINUX__
#include <utime.h>
#include <dirent.h>
#include <unistd.h>
#endif

#include "regex.h"

#ifdef __LINUX__
#define DIRSEP	'/'
#else
#define DIRSEP	'\\'
#endif

/**
 * MSDOS compatibility
 */
#ifndef O_BINARY
#define	O_BINARY	0
#endif

/**
 * If not defined define the max path len
 */
#ifndef PATH_MAX
#define PATH_MAX	1024
#endif


/**
 * Structure to hold a link queue
 */
typedef struct Link_S
{
	struct Link_S * next;
} Link_T;


/**
 * Directory entry
 */
typedef struct Entry_S
{
	Link_T	link;
	
/*	// Name without path      */
	char *		name;
	
/*	// Stat buffer            */
	struct stat	statb;
	
} Entry_T;

/**
 * Regular expression entry
 */
typedef struct
{
	Link_T	link;
	regex_t regex;
	char *  name;
} RegexEntry_T;


typedef struct FileArray_S
{
	Link_T	head;
	long	count;
	Entry_T	**entry;
} FileArray_T;

typedef struct Directory_S
{
	FileArray_T	files;
	FileArray_T	dirs;
} Directory_T;

#ifdef __MSDOS__
#define DEFAULT_BUFFER_SIZE	(1024*64)
#else
#define DEFAULT_BUFFER_SIZE	(1024*1024)
#endif

#define DEFAULT_VERBOSE	2

static long	filesCopied = 0;
static long	filesDeleted = 0;
static long	filesChecked = 0;
static long	errors = 0;
static long	kbCopied = 0;
static int	mode = 0;
static int	dontRemove = 0;
static int	fileVerify = 0;
static Link_T	excludedDirs;
static Link_T	excludedFiles;
static Link_T	excludedRegex;
static FILE *	log = 0;
static char	* buffer,* buffer1;
static int	bufferSize = DEFAULT_BUFFER_SIZE;
static int	verbose = DEFAULT_VERBOSE;
/* (JR) add */
static int	followSymlinks = 0;

#define MSG_PATH(msg) if (*msg) \
					  printf(msg);\
					  *msg=0;


static void QueueInit(Link_T * link)
{
	link->next = link;
}

static void QueueAdd(Link_T * head,Link_T * link)
{
	link->next = head->next;
	head->next = link;
}

static char dateTimeFormat[128] = "%d/%m/%Y %H:%M:%S";

static char * GetDateAndTime()
{
	time_t t;
	static char  buffer[32];
	struct tm * tmp;
	
	time(&t);
	tmp =localtime(&t);
	
	strftime(buffer,sizeof(buffer),dateTimeFormat,tmp);

	return buffer;
}

static char dateFormat[128] = "%d-%m-%Y";

static char * GetDate()
{
	time_t t;
	static char  buffer[32];
	struct tm * tmp;

	time(&t);
	tmp =localtime(&t);

	strftime(buffer,sizeof(buffer),dateFormat,tmp);

	return buffer;
}


static void LogDateAndTime()
{
	fprintf(log,"%s ",GetDateAndTime());
}

static void PrintError(char *cmd,char *obj)
{
	int code = errno;
	char *msg = strerror(code);
	if (code == 0)
		msg = "";
	fprintf(stderr,"\ndirsync: %s (%s) %s (%d)\n",cmd,obj,msg,code);
	errors++;

	if (log)
	{
		LogDateAndTime();
		fprintf(log,"%s (%s) %s (%d)\n",cmd,obj,msg,code);
	}
}

static char * FilePath(char * dest,char * dir,char * file)
{
	int len;

	len = strlen(dir);

	if (dir[len - 1] != DIRSEP)
		sprintf(dest,"%s%c%s",dir,DIRSEP,file);
	else
		sprintf(dest,"%s%s",dir,file);
	
	return dest;
}


static void CopyAttribute(char *path,struct stat * statb)
{
	struct utimbuf time;

/* (JR) mod - set attributes on directories too! */
#ifndef __LINUX__
	if ((statb->st_mode & S_IFMT) == S_IFREG)
#endif
	{
		time.actime = statb->st_atime;
		time.modtime = statb->st_mtime;
		if (utime(path,&time))
		{
			PrintError("utime",path);
		}
	}

#ifdef __LINUX__
	/* (JR) if run as root, set ownership too */
	if (geteuid() == 0) {
	  if (chown(path,statb->st_uid,statb->st_gid))
	    PrintError("chown",path);
	}
#endif

	if (chmod(path,statb->st_mode))
	{
		PrintError("chmod",path);
	}

}

static void FileUnlink(char * path)
{
	
	if (unlink(path))
	{
#ifdef UNSECURE
		chmod(path,0777);
		if (unlink(path))
#endif
			PrintError("unlink",path);
	}

}

static int FileVerify(char * msg,char *source,char * dest,Entry_T *e)
{
	int fds,fdd;
	int nread;
	int result;
	size_t count = 0;
	char sourcePath[PATH_MAX+1];
	char destPath[PATH_MAX+1];
	size_t perc,oldPerc=101;

	FilePath(sourcePath,source,e->name);
	FilePath(destPath,dest,e->name);


	fds = open(sourcePath,O_RDONLY|O_BINARY);

	if (fds < 0)
	{
		PrintError("open",sourcePath);
		return 1;
	}

	fdd = open(destPath,O_RDONLY|O_BINARY);


	if (fdd < 0)
	{
		PrintError("open",destPath);
		close(fds);
		return 1;
	}


	if (verbose > 1)
	{
		MSG_PATH(msg);
		printf("\rVERIFY %-50.50s %10ld Byte ",e->name,e->statb.st_size);
		fflush(stdout);
	}

	if (log)
	{
		LogDateAndTime();
		fprintf(log,"VERIFY %s %10lu Bytes\n",e->name,e->statb.st_size);
	}
	

	result = 0;
	
	for (;;)
	{
		nread = read(fds,buffer,bufferSize);
		
		if (nread < 0)
		{
			PrintError("read",sourcePath);
			result = 1;
			break;
		}

		if (nread == 0)
			break;

		if (read(fdd,buffer1,nread) != nread)
		{
			PrintError("read",destPath);
			result = 1;
			break;
		}

		if (memcmp(buffer,buffer1,nread))
		{
			errno = 0;
			PrintError("Verify",destPath);
			result = 1;
		}

		count += nread;

		if (verbose > 1)
		{
			if (count == e->statb.st_size)
				perc = 100;
			else
				perc = (size_t)(((double)count * 100.0) / (double)e->statb.st_size);

			if (perc > 100)
				perc = 100;

			if (perc != oldPerc)
			{
				printf("%%%03lu\010\010\010\010",perc);
				fflush(stdout);
				oldPerc = perc;
			}
		}
	}

	close(fdd);
	close(fds);

	printf("\r                                                                              \r");
	
	return result;

}


static void FileCopy(char * msg,char *source,char * dest,Entry_T *e)
{
	int fds,fdd;
	char sourcePath[PATH_MAX+1];
	char destPath[PATH_MAX+1];
#ifdef __LINUX__
	char symlinkPath[PATH_MAX+1];
	int dd;
#endif
	int nread;
	size_t count = 0;
	size_t perc,oldPerc=101;
	

	if (log)
	{
		LogDateAndTime();
		fprintf(log,"COPY   %s %10lu Bytes\n",e->name,e->statb.st_size);
	}
	
	if (verbose > 1)
	{
		MSG_PATH(msg);
		printf("COPY   %-50.50s %10lu Byte ",e->name,e->statb.st_size);
		fflush(stdout);
	}
	
	FilePath(sourcePath,source,e->name);
	FilePath(destPath,dest,e->name);
	
#ifdef __LINUX__
        /* (JR) - duplicate symlinks instead of following them */
	if ((e->statb.st_mode & S_IFMT) == S_IFLNK && !followSymlinks) {
	  count=readlink(sourcePath,symlinkPath,PATH_MAX);
	  if (count<0) {
	    PrintError("symlink read",sourcePath);
	    return;
	  }
	  /* add null byte */
	  symlinkPath[count]=0;
	  if (dd = symlink(symlinkPath,destPath) < 0) {
	    if (errno==EEXIST) {
	      FileUnlink(destPath);
	      dd = symlink(symlinkPath,destPath);
	    }
	  }
	  if (dd < 0) {
	    PrintError("symlink create",destPath);
	    return;
	  }
	  /* set ownership; can't set times nor permissions on a symlink */
	  if (geteuid() == 0) {
	    if (lchown(destPath,e->statb.st_uid,e->statb.st_gid))
	    PrintError("chown",destPath);
	  }
	} else {
#endif
	
	fds = open(sourcePath,O_RDONLY|O_BINARY);

	if (fds < 0)
	{
		PrintError("open",sourcePath);
		return;
	}
	
	fdd = open(destPath,O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,0777);

	
	if (fdd < 0)
	{
		FileUnlink(destPath);
		fdd = open(destPath,O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,0777);
		if (fdd < 0)
		{
			PrintError("open",destPath);
			close(fds);
			return;
		}
	}

	for (;;)
	{
		nread = read(fds,buffer,bufferSize);
		if (nread < 0)
		{
			PrintError("read",sourcePath);
			break;
		}

		if (nread == 0)
			break;

		
		if (write(fdd,buffer,nread) != nread)
		{
			PrintError("write",destPath);
			break;
			
		}

		count += nread;

		if (verbose > 1)
		{
			if (count == e->statb.st_size)
				perc = 100;
			else
				perc = (size_t)(((double)count * 100.0) / (double)e->statb.st_size);
			
			if (perc > 100)
				perc = 100;
			
			if (perc != oldPerc)
			{
				printf("%%%03lu\010\010\010\010",perc);
				fflush(stdout);
				oldPerc = perc;
			}
		}
	}

	if (close(fdd))
	{
		PrintError("close",destPath);
	}

	if (close(fds))
	{
		PrintError("close",sourcePath);
	}

	if (fileVerify)
		FileVerify(msg,source,dest,e);
	


	CopyAttribute(destPath,&e->statb);
	
#ifdef __LINUX__
	}   /* (JR) end of symlink if ... */
#endif

	if (verbose > 1)
		printf("\r");

	filesCopied++;

	if (count !=  e->statb.st_size)
	{
		fprintf(stderr,"dirsync: Size of %s changed, expected %lu copied %lu\n",
			  sourcePath,e->statb.st_size,count);
	}
	
	count /= 1024;
	kbCopied += count;

	
}


static void FileRemove(char * msg,char *dir,char *file)
{
	char  path[PATH_MAX+1];
	filesDeleted ++;
	
	FilePath(path,dir,file);

	if (log)
	{
		LogDateAndTime();
		fprintf(log,"RM     %s\n",path);
	}
	
	if (verbose > 0)
	{
		MSG_PATH(msg);
		printf("RM     %s\n",path);
	}
	
	FileUnlink(path);
}


static void * CheckedAlloc(int size)
{
	void *p;

	
	if (!size)
		return 0;
	
	p = malloc(size);

	if (p == 0)
	{
		fprintf(stderr,"\ndirsync: Out of memory\n");
		exit(1);
	}

	return p;
}

static int RegexSearch(Link_T * head,char *name)
{
	Link_T * link;
	RegexEntry_T * e;

	
	for (link = head->next ; link != head ; link=link->next)
	{
		e = (RegexEntry_T *)link;
/*		//printf("regsearch %s with %s\n",e->name,name);   */

		if (regexec(&e->regex,name,0,NULL,0) == 0)
		{
/*			//printf("%s matched by %s \n",name,e->name);   */

			return 1;

		}
		
	}

	return 0;
}

static Entry_T * EntrySearch(Link_T * head,char *name)
{
	Link_T * link;
	Entry_T * e;

	
	
	for (link = head->next ; link != head ; link=link->next)
	{
		e = (Entry_T *)link;
		if (!strcmp(name,e->name))
		{
			return e;
		}
	}

	return 0;
}

static void EntryAdd(Link_T * queue,char *name,struct stat * statb)
{
	int len = strlen(name)+1;
	Entry_T * entry;

	entry = CheckedAlloc(sizeof(Entry_T));
	entry->name = (char *)CheckedAlloc(len);
	memcpy(entry->name,name,len);
	if (statb != NULL)
		entry->statb = * statb;

	QueueAdd(queue,&entry->link);
}

static void RegexAdd(Link_T * queue,char *name)
{
	int len = strlen(name)+1;
	RegexEntry_T * entry;

	entry = CheckedAlloc(sizeof(RegexEntry_T));
	entry->name = (char *)CheckedAlloc(len);
	memcpy(entry->name,name,len);
	if (regcomp(&entry->regex,name,REG_NOSUB) != 0)
	{
		PrintError("regex",name);
	}
	else
		QueueAdd(queue,&entry->link);
}

static int ArrayCompare(const void *v1,const void *v2)
{
	Entry_T * e1 = *( Entry_T **)v1;
	Entry_T * e2 = *( Entry_T **)v2;

	return strcmp(e1->name,e2->name);
}

static Entry_T *  ArraySearch(FileArray_T *a,Entry_T *e)
{
	Entry_T ** result;
	if (a->count > 0) {      /* dumps core if empty table! - (JR) */
	  result =  (Entry_T **)bsearch(&e,a->entry,a->count,sizeof(Entry_T *),ArrayCompare);
	  if (!result)
		return 0;
	  return *result;
	} else
	  return 0;
}


static Entry_T **  ArrayCreate(FileArray_T * a)
{
	long i;
	Link_T * link;
	Entry_T * e;

	
	for (a->count = 0,link = a->head.next ; link != &a->head ; link=link->next)
		a->count++;

	a->entry = CheckedAlloc(a->count*sizeof(Entry_T *));

	for (i = 0,link = a->head.next ; link != &a->head ; link=link->next)
	{
		e = (Entry_T *)link;
		a->entry[i++] = e;
	}

	qsort(a->entry,a->count,sizeof(Entry_T *),ArrayCompare);
	
	return 0;
}

static int ScanDir(char *dirname,Directory_T * queue,struct stat * statb)
{
	struct dirent * d;
	DIR * dir;
	long dirCount,fileCount;
	char path[PATH_MAX+1];
	struct stat s;
	
	fileCount = dirCount = 0;
	
	QueueInit(&queue->files.head);
	QueueInit(&queue->dirs.head);
	
	if (stat(dirname,statb))
	{
		PrintError("stat",dirname);
		return 1;
	}

	if ((statb->st_mode & S_IFMT) != S_IFDIR)
	{
		fprintf(stderr,"\ndirsync: %s is not a directory\n",dirname);
		errors++;
		return 1;
	}

	if (verbose > 0)
	{
		printf("SCAN   %-60.60s  ",dirname);
		
		fflush(stdout);
	}
	
	dir = opendir(dirname);

	if (dir == 0)
	{
		PrintError("opendir",dirname);
		return 1;
	}

	while ((d = readdir(dir)) != 0)
	{
		FilePath(path,dirname,d->d_name);
/*test	printf("%s\n",d->d_name);  */

		if (RegexSearch(&excludedRegex,path))
			continue;
		
#ifdef __LINUX__
		if (followSymlinks ? stat(path,&s) : lstat(path,&s))
#else
		if (stat(path,&s))
#endif
		{
			PrintError("stat",path);
			continue;
		}

/*		//printf("%s\n",d->d_name);   */
		switch (s.st_mode & S_IFMT)
		{
			case	S_IFDIR:
					if (EntrySearch(&excludedDirs,d->d_name) == NULL)
					{
						EntryAdd(&queue->dirs.head,d->d_name,&s);
						dirCount++;
					}
					break;
					
			case	S_IFREG:
#ifdef S_IFLNK
			case	S_IFLNK:
#endif			
					if (EntrySearch(&excludedFiles,d->d_name) == NULL)
					{
						EntryAdd(&queue->files.head,d->d_name,&s);
						fileCount++;
					}
					break;

#ifdef S_IFBLK
			case	S_IFBLK:
					fprintf(stderr,"\ndirsync: Ignored block device %s in %s\n",
							d->d_name,dirname);
					break;
#endif
					
#ifdef S_IFCHR
			case	S_IFCHR:
					fprintf(stderr,"\ndirsync: Ignored char device %s in %s\n",
							d->d_name,dirname);
					break;
#endif
					
#ifdef	S_IFFIFO
			case	S_IFFIFO:
					fprintf(stderr,"\ndirsync: Ignored fifo %s in %s\n",
						d->d_name,dirname);
					break;
#endif
			default:
					fprintf(stderr,"\ndirsync: Ignored unknown  %s in directory %s\n",
							d->d_name,dirname);
					break;

		}

	}

	closedir(dir);

	
	if (verbose > 0)
	{
		printf("*");
		fflush(stdout);
	}

	/**
	 * Now create one sorted array for files and directory
	 */
	ArrayCreate(&queue->files);
	ArrayCreate(&queue->dirs);
	
	if (verbose > 0)
	{
		printf("\010 \010\r");
		fflush(stdout);
	}
	
	return 0;
}

static void Mkdir(char * msg,char * path)
{
	if (verbose > 1)
	{
		MSG_PATH(msg);
		printf("MKDIR  %s\n",path);
	}
	
#ifdef __LINUX__
	if (mkdir(path,0777))
#else
	if (mkdir(path))
#endif
	{
		PrintError("mkdir",path);
	}

}

static void Rmdir(char * msg,char *dirname,char *file)
{
	char  path[PATH_MAX+1];

	FilePath(path,dirname,file);

	if (log)
	{
		LogDateAndTime();
		fprintf(log,"RMDIR  %s\n",path);
		
	}
	
	if (verbose > 0)
	{
		MSG_PATH(msg);
		printf("RMDIR  %s\n",path);
	}

	if (rmdir(path))
	{
#ifdef UNSECURE
		chmod(path,0777);
		if (rmdir(path))
#endif
		{
			PrintError("rmdir",path);
		}
	}
}

static void FreeArray(FileArray_T * a)
{
	long i;
	Entry_T * e;

	for (i = 0 ; i < a->count ; i++)
	{
		e = a->entry[i];
		free(e->name);
		free(e);
	}

	if (a->entry)
		free(a->entry);
}

static void FreeDirectory(Directory_T * dir)
{
	FreeArray(&dir->files);
	FreeArray(&dir->dirs);
}
	
static void RmdirAll(char *msg,char *dirname,char *file)
{
	char path[PATH_MAX+1];
	struct stat statb;
	Directory_T dir;
	Entry_T * e;
	long i;
	
	FilePath(path,dirname,file);		
	
	if (ScanDir(path,&dir,&statb))
		return;
	
/*	// Step 1. Remove all file       */
	for (i = 0 ; i  < dir.files.count ; i++)
	{
		e = dir.files.entry[i];

		FileRemove(msg,path,e->name);
	}

/*	// Step 2. Remove all dir        */
	for (i = 0 ; i < dir.dirs.count ; i++)
	{
		e = dir.dirs.entry[i];

		RmdirAll(msg,path,e->name);
	}

/*	// Step 3. Remove dir            */

	Rmdir(msg,dirname,file);

	FreeDirectory(&dir);
}

static int dirsync(char *source,char *dest)
{
	char sourcePath[PATH_MAX+1],destPath[PATH_MAX+1];
	char msg[80*2+5];
	Directory_T dirSource,dirDest;
	Entry_T * se , *de;
	int needCopy;
	struct stat statSource,statDest;
	int firstTime = 1;
	long i;
/* (JR) add */
	int isLink = 0;
	
	sprintf(msg,"FROM   %-70.70s\nTO     %-70.70s\n",source,dest);

	if (log)
	{
		LogDateAndTime();
		fprintf(log,"FROM   %s\n",source);
		LogDateAndTime();
		fprintf(log,"TO     %s\n",dest);
	}
	
	if (ScanDir(source,&dirSource,&statSource))
		return 1;

	if (ScanDir(dest,&dirDest,&statDest))
	{
		FreeDirectory(&dirSource);
		return 1;
	}
	
/*	// Step 1. Delete all file not present in the source dir    */
	for (i = 0 ; i < dirDest.files.count ; i++)
	{
		de = dirDest.files.entry[i];
		se = ArraySearch(&dirSource.files,de);

		if (se == NULL)
		{
			if (!dontRemove)
			{
				if (mode == 2)
				{
					MSG_PATH(msg);
					printf("\t5 %s\n",de->name);
					if (log)
					{
						LogDateAndTime();
						fprintf(log,"ERROR  %s - File not present in the source\n",de->name);
					}
				}
				else
				{
					if (firstTime)
					{
						firstTime = 0;
#ifdef UNSECURE
						chmod(dest,0777);
#endif
					}
					FileRemove(msg,dest,de->name);
				}
			}
		}
	}

/*	// Step 2. Delete all directories not present in the source    */
	for (i = 0 ; i < dirDest.dirs.count; i++)
	{
		de = dirDest.dirs.entry[i];
		se = ArraySearch(&dirSource.dirs,de);

		if (se == NULL)
		{
			if (!dontRemove)
			{
				if (mode == 2)
				{
					MSG_PATH(msg);
/*					printf("\t0%s\n",de->name); */
					printf("\t0 %s\n",de->name);
					if (log)
					{
						LogDateAndTime();
						fprintf(log,"ERROR  %s - Directory not present in the source\n",de->name);
					}
					
				}
				else
				{
					if (firstTime)
					{
						firstTime = 0;
#ifdef UNSECURE
						chmod(dest,0777);
#endif
					}					
					RmdirAll(msg,dest,de->name);
				}
			}
		}
	}
	
/*	// Step 3. Copy all file changed form source to dest    */
	for (i = 0 ; i < dirSource.files.count ; i++)
	{
		se = dirSource.files.entry[i];
		de = ArraySearch(&dirDest.files,se);
/* (JR) add */
#ifdef S_IFLNK
		isLink = ((se->statb.st_mode & S_IFMT) == S_IFLNK);
#endif
/* (JR) end */

		needCopy = 0;
		filesChecked++;
		
/*		// File not present in the destination          */
		if (de == NULL)
			needCopy = 1;
		else switch (mode)
		{
			case	0:
					if (se->statb.st_size != de->statb.st_size ||
/*					  se->statb.st_mtime != de->statb.st_mtime)  */
/* (JR) mod */
					  isLink ? (se->statb.st_mtime > de->statb.st_mtime)
						 : (se->statb.st_mtime != de->statb.st_mtime))
						needCopy = 1;
					break;
			case	1:
					if (se->statb.st_mtime > de->statb.st_mtime)
						needCopy = 1;
					break;
			case	2:
/*					if (se->statb.st_size != de->statb.st_size)  */
/* (JR) mod */
					if (se->statb.st_size != de->statb.st_size ||
					  isLink ? (se->statb.st_mtime > de->statb.st_mtime)
						 : (se->statb.st_mtime != de->statb.st_mtime))
						needCopy = 1;
					break;
/* (JR) add */
			case	3:
					if (se->statb.st_size != de->statb.st_size ||
					  (!isLink && se->statb.st_mtime != de->statb.st_mtime))
						needCopy = 1;
					break;
					
		}
							
		if (needCopy)
		{
			if (mode == 2)
			{
				MSG_PATH(msg);
				printf("\t2 %s\n",se->name);
				if (log)
				{
					LogDateAndTime();
					fprintf(log,"ERROR  %s - Need to be copied\n",se->name);
				}
				
			}
			else
			{
/*				FileCopy(msg,source,dest,se);  */
				if (firstTime)
				{
					firstTime = 0;
#ifdef UNSECURE
					chmod(dest,0777);
#endif
				}
				/* this rather should be here? - (JR) */
				FileCopy(msg,source,dest,se);
				
			}
		}
		else
		{
			if (mode == 2)
				if (FileVerify(msg,source,dest,se))
				{
					MSG_PATH(msg);
					printf("\t4 %s\n",se->name);
					if (log)
					{
						LogDateAndTime();
						fprintf(log,"ERROR  %s - Verify error\n",se->name);
					}
					
				}
		}
			
	}

/*	// Step 4. Call dirsync in all directory        */
	for (i = 0 ; i < dirSource.dirs.count ; i++)
	{

		se = dirSource.dirs.entry[i];
		de = ArraySearch(&dirDest.dirs,se);
		FilePath(destPath,dest,se->name);
		FilePath(sourcePath,source,se->name);

		if (de == NULL)
		{
			if (mode == 2)
			{
				MSG_PATH(msg);
				printf("\t1 %s\n",se->name);
				if (log)
				{
					LogDateAndTime();
					fprintf(log,"ERROR  %s - Directory not present in destinatio\n",se->name);
				}
				
			}
			else
				/* (JR) add */
				if (firstTime)
				{
					firstTime = 0;
#ifdef UNSECURE
					chmod(dest,0777);
#endif
				}
				/* (JR) end */
				Mkdir(msg,destPath);
				/* another (JR) add, in case of an empty dir */
				CopyAttribute(destPath,&se->statb);
		}

		if (de != NULL || mode != 2)
			dirsync(sourcePath,destPath);
	}

	/* Set destination directory attributes */
	if (firstTime == 0 && mode != 2)
		CopyAttribute(dest,&statSource);

	FreeDirectory(&dirDest);
	FreeDirectory(&dirSource);
	
	return 0;
}


static void Usage()
{
	printf("usage : dirsync [option(s)] source dest\n");
	printf("\tsource\tsoure directory (must exist)\n");
	printf("\tdest\tdestination directory (must exist)\n");
	printf("\t-h\tDisplay this message\n");
	printf("\t-v lvl\tset verbose level (default %d,min 0,max 9)\n",DEFAULT_VERBOSE);
	printf("\t-q\tQuiet mode (same result of -v 0)\n");
	printf("\t-V\tVerify copied data\n");
	printf("\t-X name\tExclude directory name (1) from scanning (default . ..)\n");
	printf("\t-x name\tExclude file name (1) from scanning\n");
	printf("\t-e name\tExclude regular expression name (1) from scanning\n");
	printf("\t-b size\tSet the read/write buffer size (default %d)\n",DEFAULT_BUFFER_SIZE);
	printf("\t-m mode\tSet copy file mode,always missing file are copied\n");
	printf("\t\t0 - Copy file if have different size or date (default)\n");
	printf("\t\t1 - Copy file if the destination is older than source\n");
	printf("\t\t2 - Do not copy any file only show the difference \n");
	printf("\t\t    with this code :  0 Directory not present in source\n");
	printf("\t\t                   :  1 Directory nont present in destination\n");
	printf("\t\t                      2 Different size, 4 Not equal\n");
	printf("\t\t3 - Like mode 0 but do not check size/time in symbolics link\n");
/* (JR) add */
	printf("\t\t3 - Like mode 0, but don't check date on symlinks\n");
/* (JR) end */
	printf("\t-T fmt\tSet date and time format, default %s\n",dateTimeFormat);
	printf("\t-D fmt\tSet date format, default %s\n",dateFormat);
	printf("\t-l file\tLog operation in file, if - use date format .log (see -D)\n");
	printf("\t-r\tDon't remove file/directory missed in the source\n");
/* (JR) add */
	printf("\t-L\tFollow symlinks (copy contents instead of duplicating link)\n");
/* (JR) end */
	printf("\n\t\t(1) If name begin with @ for example @list the names will be\n");
	printf("\t\tread from a text file named list.\n");
	
}

static void AddRegexOption(Link_T * queue,char *name)
{
	FILE * fd;
	char buffer[PATH_MAX+1];

	if (*name != '@')
	{
		RegexAdd(queue,name);
		return;
	}
	name++;

	if ((fd = fopen(name,"rt")) == NULL)
	{
		PrintError("fopen",name);
		exit(1);
	}

	while (fscanf(fd,"%s",buffer) == 1)
	{
		RegexAdd(queue,name);
	}

	fclose(fd);


}

static void AddEntryOption(Link_T * queue,char *name)
{
	FILE * fd;
	char buffer[PATH_MAX+1];
	
	if (*name != '@')
	{
		EntryAdd(queue,name,NULL);
		return;
	}
	name++;

	if ((fd = fopen(name,"rt")) == NULL)
	{
		PrintError("fopen",name);
		exit(1);
	}

	while (fscanf(fd,"%s",buffer) == 1)
	{
		EntryAdd(queue,name,NULL);
	}

	fclose(fd);
	
}


static void RemoveDirsep(char * path)
{
	int len;
	len = strlen(path);
	
#ifdef __LINUX__
	if (len > 1 && path[len - 1] == DIRSEP)
		path[len - 1] = '\0';
#else
	if (len > 1 && path[len - 1] == DIRSEP && path[len - 2] != ':')
		path[len - 1] = '\0';
#endif
}

int main(int argc,char **argv)
{
	time_t start,end;
	long kbSec;
	int o;
	char logfile[PATH_MAX];
	char * version = "DirSync 1.11 author mario@viara.cn, mod by (JR)";

	printf("%s\n\n",version);
	
	QueueInit(&excludedDirs);
	QueueInit(&excludedFiles);
	QueueInit(&excludedRegex);
	
/*	// Add default excluded dir        */
	EntryAdd(&excludedDirs,".",NULL);
	EntryAdd(&excludedDirs,"..",NULL);



/*	while (( o = getopt(argc,argv,"T:D:l:Vqhv:x:X:b:m:re:")) != -1) */
/* (JR) mod */
	while (( o = getopt(argc,argv,"T:D:l:Vqhv:x:X:b:m:re:L")) != -1)
	{
		switch (o)
		{
			case	'T':
					strcpy(dateTimeFormat,optarg);
					break;
			case	'D':
					strcpy(dateFormat,optarg);
					break;
			case	'l':
					if (strcmp(optarg,"-"))
						strcpy(logfile,optarg);
					else
						sprintf(logfile,"%s.log",GetDate());
					log = fopen(logfile,"w");
					if (log == NULL)
					{
						PrintError("fopen",logfile);
						exit(1);
					}
					else
					{
						LogDateAndTime();
						fprintf(log,"%s\n",version);
					}
					
					if (verbose)
						printf("Log operation in %s\n",logfile);
					break;
			case	'V':
					fileVerify = 1;
					break;
			case	'r':
					dontRemove = 1;
					break;
					
			case	'm':
					mode = atoi(optarg);
					if (mode < 0 || mode > 3)
					{
						printf("dirsync: invalid -m %d\n",mode);
						Usage();
						return 2;
					}
					break;
			case	'b':
					bufferSize = atoi(optarg);
					break;

			case	'e':
					AddRegexOption(&excludedRegex,optarg);
					break;
			case	'x':
					AddEntryOption(&excludedFiles,optarg);
/*					//EntryAdd(&excludedFiles,optarg,NULL);  */
					break;
					
			case	'X':
					AddEntryOption(&excludedDirs,optarg);
/*					//EntryAdd(&excludedDirs,optarg,NULL);   */
					break;
			case	'q':
					verbose = 0;
					break;
			case	'v':
					verbose = atoi(optarg);
					break;
			case	'?':
			case	'h':
					Usage();
					return 2;
			/* (JR) add */
			case	'L':
					followSymlinks = 1;
					break;
		}
	}

	if (optind + 2 != argc)
	{
		printf("dirsync: source and destination not specified\nplease try dirsync -h for usage\n");
		exit(1);
	}

	/**
	 * Allocated buffer 1 to read and 2 to verify
	 */
	buffer  = (char *)CheckedAlloc(bufferSize);
	buffer1 = (char *)CheckedAlloc(bufferSize);
	
	time(&start);

	RemoveDirsep(argv[optind]);
	RemoveDirsep(argv[optind+1]);

	if (log)
	{
		LogDateAndTime();
		fprintf(log,"ARGS[%d]   %s %s\n",mode,argv[optind],argv[optind+1]);
	}
	
	dirsync(argv[optind],argv[optind+1]);
	time(&end);

	if (verbose)
	{
		if (end - start > 0)
			kbSec = kbCopied / (end -start);
		else
			kbSec = 0;
		printf("\r                                                                              \r");
				  
		printf("%ld files, %ld copied %lu KB %ld KB/s, %ld deleted, %ld s",
			   filesChecked,filesCopied,kbCopied,kbSec,filesDeleted,end -start);
		if (errors)
			printf(",%ld error(s)",errors);
		printf("\n");
	}

	if (log)
	{
		LogDateAndTime();
		fprintf(log,"       %ld files, %ld copied (%lu KB, %ld KB/s), %10ld deleted, %10ld second(s)",
		       filesChecked,filesCopied,kbCopied,kbSec,filesDeleted,end -start);
		if (errors)
			fprintf(log,",%ld error(s)",errors);
		fprintf(log,"\n");
		fclose(log);
	}

	return errors == 0 ? 0 : 2;
}
