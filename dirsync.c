/**
 * Directories syncronizer
 *
 * $Id: dirsync.c,v 1.1.1.1 2004/09/27 13:12:59 mviara Exp $
 * $Name:  $
 *
 * $Log: dirsync.c,v $
 * Revision 1.1.1.1  2004/09/27 13:12:59  mviara
 * First imported version
 *
 */
static char rcsinfo[] = "$Id: dirsync.c,v 1.1.1.1 2004/09/27 13:12:59 mviara Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#ifdef __NT__
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
#include <lcms.h>
#endif

#ifndef O_BINARY
#define	O_BINARY	0
#endif

#ifndef PATH_MAX
#define PATH_MAX	1024
#endif


//#include "dirent.h"

typedef struct Link_S
{
	struct Link_S * next, * prev;
} Link_T;


typedef struct Entry_S
{
	Link_T	link;
	
	// Name without path
	char *		name;
	
	// Stat buffer
	struct stat	statb;
	
} Entry_T;

typedef struct Directory_S
{
	Link_T	listFiles;
	Link_T	listDirs;
	long	fileCount;
	long	dirCount;
} Directory_T;

#define DEFAULT_BUFFER_SIZE	(1024*1024)
#define DEFAULT_VERBOSE	2

long	filesCopied = 0;
long	filesDeleted = 0;
long	filesChecked = 0;
long	errors = 0;

Link_T	excludedDirs;
Link_T	excludedFiles;
char	* buffer;
int		bufferSize = DEFAULT_BUFFER_SIZE;
int		verbose = DEFAULT_VERBOSE;

#define MSG_PATH(msg) if (*msg) \
					  printf(msg);\
					  *msg=0;


void QueueInit(Link_T * link)
{
	link->next = link->prev = link;
}

void QueueAdd(Link_T * head,Link_T * link)
{
	link->prev	= head->prev;
	link->next	= head;
	head->prev->next = link;
	head->prev  = link;
}

void QueueUnlink(Link_T * link)
{
	link->prev->next = link->next;
	link->next->prev = link->prev;
}

int QueueEmpty(Link_T * head)
{
	return head->next == head ? 1 : 0;
}

void PrintError(char *cmd,char *obj)
{
	int code = errno;
	char *msg = strerror(code);
	
	fprintf(stderr,"\ndirsync: %s (%s) %s (%d)\n",cmd,obj,msg,code);
	errors++;
}

char * FilePath(char * dest,char * dir,char * file)
{

#ifdef __NT__
	sprintf(dest,"%s\\%s",dir,file);
#else
	sprintf(dest,"%s/%s",dir,file);
#endif
	
	
	return dest;
}

char * Size2String(long size)
{
	static char tmp[255];

	if (size < 1024)
		sprintf(tmp,"%ld Bytes");
	else if (size < 1024 * 1024)
		sprintf(tmp,"%ld KB",size/1024);
	else
		sprintf(tmp,"%ld MB",size/(1024*1024));

	return tmp;
}

void CopyAttribute(char *path,struct stat * statb)
{
	struct utimbuf time;




	if ((statb->st_mode & S_IFMT) == S_IFREG)
	{
		time.actime = statb->st_atime;
		time.modtime = statb->st_mtime;
		if (utime(path,&time))
		{
			PrintError("utime",path);
		}
	}

	if (chmod(path,statb->st_mode))
	{
		PrintError("chmod",path);
	}

}

void FileUnlink(char * path)
{
	if (unlink(path))
	{
		chmod(path,0777);
		if (unlink(path))
			PrintError("unlink",path);
	}

}

void FileCopy(char * msg,char *source,char * dest,Entry_T *e)
{
	int fds,fdd;
	char sourcePath[PATH_MAX+1];
	char destPath[PATH_MAX+1];
	int nread;
	size_t count = 0;
	size_t perc,oldPerc=101;
	
	filesCopied++;
	
	if (verbose > 1)
	{
		MSG_PATH(msg);
		printf("COPY   %-50.50s %10ld Byte ",e->name,e->statb.st_size);
		fflush(stdout);
	}
	
	FilePath(sourcePath,source,e->name);
	FilePath(destPath,dest,e->name);
	
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
				perc = (count * 100) / e->statb.st_size;
			
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

	if (close(fds))
	{
		PrintError("close",sourcePath);
	}

	if (close(fdd))
	{
		PrintError("close",destPath);
	}

	CopyAttribute(destPath,&e->statb);
	
	if (verbose > 1)
		printf("\n");
}


void FileRemove(char * msg,char *dir,char *file)
{
	char  path[PATH_MAX+1];
	filesDeleted ++;
	
	FilePath(path,dir,file);
	if (verbose > 0)
	{
		MSG_PATH(msg);
		printf("RM     %s\n",path);
	}
	
	FileUnlink(path);
}


void * CheckedAlloc(int size)
{
	void *p = malloc(size);

	if (p == 0)
	{
		fprintf(stderr,"dirsync: Out of memory\n");
		exit(1);
	}

	return p;
}

Entry_T * EntrySearch(Link_T * head,char *name)
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

void EntryFree(Entry_T * e)
{
	QueueUnlink(&e->link);
	free(e->name);
	free(e);
	
}

void EntryAdd(Link_T * queue,char *name,struct stat * statb)
{
	Entry_T * entry;

	entry = CheckedAlloc(sizeof(Entry_T));
	entry->name = (char *)CheckedAlloc(strlen(name)+1);
	strcpy(entry->name,name);
	if (statb != NULL)
		entry->statb = * statb;

	QueueAdd(queue,&entry->link);
}

int ScanDir(char *dirname,Directory_T * queue,struct stat * statb)
{
	struct dirent * d;
	DIR * dir;
	long dirCount,fileCount;
	char path[PATH_MAX+1];
	struct stat s;
	
	fileCount = dirCount = 0;
	
	QueueInit(&queue->listFiles);
	QueueInit(&queue->listDirs);
	
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
		
		if (stat(path,&s))
		{
			PrintError("stat",path);
			continue;
		}

		//printf("%s\n",d->d_name);
		switch (s.st_mode & S_IFMT)
		{
			case	S_IFDIR:
					if (EntrySearch(&excludedDirs,d->d_name) == NULL)
					{
						EntryAdd(&queue->listDirs,d->d_name,&s);
						dirCount++;
					}
					break;
			case	S_IFREG:
					if (EntrySearch(&excludedFiles,d->d_name) == NULL)
					{
						EntryAdd(&queue->listFiles,d->d_name,&s);
						fileCount++;
					}
					break;

			case	S_IFCHR:
					fprintf(stderr,"\ndirsync: Ignored char device %s in %s\n",
							d->d_name,dirname);
					break;
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
		printf("\r");
	

	return 0;
}

void Mkdir(char * msg,char * path)
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
void Rmdir(char * msg,char *dirname,char *file)
{
	char  path[PATH_MAX+1];

	FilePath(path,dirname,file);

	if (verbose > 0)
	{
		MSG_PATH(msg);
		printf("RMDIR  %s\n",path);
	}

	if (rmdir(path))
	{
		chmod(path,0777);
		if (rmdir(path))
		{
			PrintError("rmdir",path);
		}
	}
}

void RmdirAll(char *msg,char *dirname,char *file)
{
	char path[PATH_MAX+1];
	struct stat statb;
	Directory_T dir;
	Link_T * link,* next;
	Entry_T * e;

	FilePath(path,dirname,file);		
	
	if (ScanDir(path,&dir,&statb))
		return;
	
	// Step 1. Remove all file
	for (link = dir.listFiles.next ; link != &dir.listFiles ; link=next)
	{
		next = link->next;
		e = (Entry_T *)link;

		FileRemove(msg,path,e->name);
		EntryFree(e);
	}

	// Step 2. Remove all dir
	for (link = dir.listDirs.next ; link != &dir.listDirs ; link=next)
	{
		next = link->next;
		e = (Entry_T *)link;

		RmdirAll(msg,path,e->name);
		EntryFree(e);
	}

	// Step 3. Remove dir

	Rmdir(msg,dirname,file);
	
	assert(QueueEmpty(&dir.listFiles));
	assert(QueueEmpty(&dir.listDirs));
}

int dirsync(char *source,char *dest)
{
	char sourcePath[PATH_MAX+1],destPath[PATH_MAX+1];
	char msg[80*2+5];
	Directory_T dirSource,dirDest;
	Link_T * link,*next;
	Entry_T * se , *de;
	int needCopy;
	struct stat statSource,statDest;
	int firstTime = 1;
	
	sprintf(msg,"FROM   %-70.70s\nTO     %-70.70s\n",source,dest);
	
	if (ScanDir(source,&dirSource,&statSource))
		return 1;

	// FIXME dealloc dirSource
	if (ScanDir(dest,&dirDest,&statDest))
		return 1;

	
	// Step 1. Delete all file not present in the source dir
	for (link = dirDest.listFiles.next ; link != &dirDest.listFiles ; link=next)
	{
		next = link->next;
		de = (Entry_T *)link;
		se = EntrySearch(&dirSource.listFiles,de->name);

		if (se == NULL)
		{
			if (firstTime)
			{
				firstTime = 0;
				chmod(dest,0777);
			}				
			FileRemove(msg,dest,de->name);
			EntryFree(de);
		}
	}

	// Step 2. Delete all directories not present in the source
	for (link = dirDest.listDirs.next ; link != &dirDest.listDirs ; link=next)
	{
		next = link->next;
		de = (Entry_T *)link;
		se = EntrySearch(&dirSource.listDirs,de->name);

		if (se == NULL)
		{
			if (firstTime)
			{
				firstTime = 0;
				chmod(dest,0777);
			}				

			RmdirAll(msg,dest,de->name);
			EntryFree(de);
		}
	}
	
	// Step 3. Copy all file changed form source to dest
	for (link = dirSource.listFiles.next ; link != &dirSource.listFiles ; link=next)
	{
		next = link->next;
		
		se = (Entry_T *)link;
		de = EntrySearch(&dirDest.listFiles,se->name);

		needCopy = 0;
		filesChecked++;
		// File not prsent in the destination
		if (de == NULL)
			needCopy = 1;
		else if (se->statb.st_size != de->statb.st_size ||
				 se->statb.st_mtime != de->statb.st_mtime)
			needCopy = 1;
/*
		printf("size %ld,%ld mtime %ld,%ld\n",se->statb.st_size,
											  de->statb.st_size,
										      se->statb.st_mtime,
										      de->statb.st_mtime);*/
		if (needCopy)
		{
			if (firstTime)
			{
				firstTime = 0;
				chmod(dest,0777);
			}				

			FileCopy(msg,source,dest,se);
		}
		EntryFree(se);

		if (de != NULL)
			EntryFree(de);
			
	}

	// Step 4. Call dirsync in all directory
	for (link = dirSource.listDirs.next ; link != &dirSource.listDirs ; link=next)
	{
		next = link->next;

		se = (Entry_T *)link;
		de = EntrySearch(&dirDest.listDirs,se->name);
		FilePath(destPath,dest,se->name);
		FilePath(sourcePath,source,se->name);

		if (de == NULL)
		{
			Mkdir(msg,destPath);
		}
		else
			EntryFree(de);


		EntryFree(se);

		dirsync(sourcePath,destPath);
	}

	/* Set destination directory attributes */
	if (firstTime == 0)
		CopyAttribute(dest,&statSource);

	/* Onlyfor debug */
	assert(QueueEmpty(&dirDest.listDirs));
	assert(QueueEmpty(&dirDest.listFiles));
	assert(QueueEmpty(&dirSource.listDirs));
	assert(QueueEmpty(&dirSource.listFiles));
	
	return 0;
}


void Usage()
{
	printf("usage : dirsync [option(s)] source dest\n");
	printf("\tsource\tsoure directory (must exist)\n");
	printf("\tdest\tdestination directory (must exist)\n");
	printf("\t-h\tDisplay this message\n");
	printf("\t-v lvl\tset verbose level (default %d,min 0,max 9)\n",DEFAULT_VERBOSE);
	printf("\t-q\tQuiet mode (same result of -v 0)\n");
	printf("\t-X name\tExclude directory 'name' from scanning (defualt . ..)\n");
	printf("\t-x name\tExclude file 'name' from scanning\n");
	printf("\t-b size\tSet the read/write buffer size (default %d)\n",DEFAULT_BUFFER_SIZE);
}

int main(int argc,char **argv)
{
	time_t start,end;
	int o;

	printf("dirsync 1.0 by mario@viara.cn\n\n");
	
	QueueInit(&excludedDirs);
	QueueInit(&excludedFiles);
	
	// Add default excluded dir
	EntryAdd(&excludedDirs,".",NULL);
	EntryAdd(&excludedDirs,"..",NULL);



	while (( o = getopt(argc,argv,"qhv:x:X:b:")) != -1)
	{
		switch (o)
		{
			case	'b':
					bufferSize = atoi(optarg);
					break;
			case	'x':
					EntryAdd(&excludedFiles,optarg,NULL);
					break;
			case	'X':
					EntryAdd(&excludedDirs,optarg,NULL);
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
		}
	}

	if (optind + 2 != argc)
	{
		printf("dirsync: source and destination not specified\nplease try dirsync -h for usage\n");
		exit(1);
	}
	
	buffer = (char *)CheckedAlloc(bufferSize);

	time(&start);
	dirsync(argv[optind],argv[optind+1]);
	time(&end);

	if (verbose)
	{
		printf("%ld file(s) checked,%ld copied,%ld deleted in %ld second(s)",
			   filesChecked,filesCopied,filesDeleted,end -start);
		if (errors)
			printf(",%ld error(s)",errors);
		printf("\n");
	}

	return errors == 0 ? 0 : 2;
}
