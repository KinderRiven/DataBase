#include <errno.h>
#include <string.h>

#include "db.h"
#include "db_api.h"

#ifdef _WIN32
#define fwrite	_fwrite_nolock
#define fputc	_fputc_nolock
#define getc	_getc_nolock
#else
#undef getc
#define fputc	fputc_unlocked
#define fwrite	fwrite_unlocked
#define getc	getc_unlocked
#endif

#ifndef unix
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>

double getCpuTime(int type)
{
FILETIME crtime[1];
FILETIME xittime[1];
FILETIME systime[1];
FILETIME usrtime[1];
SYSTEMTIME timeconv[1];
double ans = 0;

	memset (timeconv, 0, sizeof(SYSTEMTIME));

	switch( type ) {
	case 0:
		GetSystemTimeAsFileTime (xittime);
		FileTimeToSystemTime (xittime, timeconv);
		ans = (double)timeconv->wDayOfWeek * 3600 * 24;
		break;
	case 1:
		GetProcessTimes (GetCurrentProcess(), crtime, xittime, systime, usrtime);
		FileTimeToSystemTime (usrtime, timeconv);
		break;
	case 2:
		GetProcessTimes (GetCurrentProcess(), crtime, xittime, systime, usrtime);
		FileTimeToSystemTime (systime, timeconv);
		break;
	}

	ans += (double)timeconv->wHour * 3600;
	ans += (double)timeconv->wMinute * 60;
	ans += (double)timeconv->wSecond;
	ans += (double)timeconv->wMilliseconds / 1000;
	return ans;
}
#else
#include <time.h>
#include <sys/resource.h>

double getCpuTime(int type)
{
struct rusage used[1];
struct timeval tv[1];

	switch( type ) {
	case 0:
		gettimeofday(tv, NULL);
		return (double)tv->tv_sec + (double)tv->tv_usec / 1000000;

	case 1:
		getrusage(RUSAGE_SELF, used);
		return (double)used->ru_utime.tv_sec + (double)used->ru_utime.tv_usec / 1000000;

	case 2:
		getrusage(RUSAGE_SELF, used);
		return (double)used->ru_stime.tv_sec + (double)used->ru_stime.tv_usec / 1000000;
	}

	return 0;
}
#endif

//  Interface function to create a document key
//	from a document and a key length

typedef struct {
	int keyLen;
} KeySpec;

uint16_t keyGenerator (uint8_t *key, Document *doc, Object *spec) {
KeySpec *keySpec = (KeySpec *)(spec + 1);
uint16_t keyLen = 0;

	if (!(keyLen = keySpec->keyLen))
		keyLen = doc->size;

	memcpy(key, doc + 1, keyLen);
	return keyLen;
}

typedef struct {
	char idx;
	char *cmds;
	char *inFile;
	char *maxKey;
	char *minKey;
	DbHandle *database;
	int useTxn, noDocs;
	int bits, xtra, onDisk;
	int num, idxType, keyLen;
} ThreadArg;

char *indexNames[] = {
"ARTreeIdx",
"Btree1Idx",
"Btree2Idx"
};

HandleType indexType[] = {
ARTreeIndexType,
Btree1IndexType,
Btree2IndexType
};

//  standalone program to index file of keys
//  then list them onto std-out

#ifdef unix
void *index_file (void *arg)
#else
unsigned __stdcall index_file (void *arg)
#endif
{
uint64_t line = 0, cnt = 0;
Params params[MaxParam];
unsigned char key[4096];
int ch, len = 0, slot;
ThreadArg *args = arg;
KeySpec keySpec[1];
HandleType idxType;
DbHandle database[1];
DbHandle docHndl[1];
DbHandle cursor[1];
DbHandle index[1];
DbHandle *parent;
char *idxName;
bool found;

uint32_t maxLen = 0;
uint32_t keyLen = 0;
uint8_t *keyPtr;
Document *doc;
int idx, stat;
ObjId objId;
ObjId txnId;
FILE *in;

	cloneHandle(database, args->database);

	if (args->maxKey)
		maxLen = strlen(args->maxKey);
	
	idxType = indexType[args->idxType];
	idxName = indexNames[args->idxType];
	docHndl->handle.bits = 0;

	txnId.bits = 0;

	memset (params, 0, sizeof(params));
	params[OnDisk].boolVal = args->onDisk;
	params[Btree1Bits].intVal = args->bits;
	params[Btree1Xtra].intVal = args->xtra;

	if( args->idx < strlen (args->cmds) )
		ch = args->cmds[args->idx];
	else
		ch = args->cmds[strlen(args->cmds) - 1];

	switch(ch | 0x20)
	{
/*
	case 'd':
		fprintf(stderr, "started delete for %s\n", args->inFile);

		keySpec->keyLen = args->keyLen;

		if (args->noDocs)
			parent = database;
		else {
			openDocStore(docHndl, database, "documents", strlen("documents"), params);
			parent = docHndl;
		}

		if ((stat = createIndex(index, parent, idxType, idxName, strlen(idxName), keySpec, sizeof(keySpec), params)))
		  fprintf(stderr, "createIndex Error %d name: %s\n", stat, idxName), exit(0);

		if (docHndl->handle->bits)
			addIndexKeys(docHndl);

		if( in = fopen (args->inFile, "rb") )
		  while( ch = getc(in), ch != EOF )
			if( ch == '\n' )
			{
#ifdef DEBUG
			  if (!(line % 100000))
				fprintf(stderr, "line %lld\n", line);
#endif
			  line++;

			  if ((stat = delDocument (docHndl, key, len, &objId, txnId)))
				  fprintf(stderr, "Del Document Error %d Line: %lld\n", stat, line), exit(0);
			  len = 0;
			  continue;
			}

			else if( len < 4096 )
				key[len++] = ch;

		fprintf(stderr, "finished %s for %d keys: %d reads %d writes %d found\n", args->inFile, line, bt->reads, bt->writes, bt->found);
		break;
*/

	case 'w':
		fprintf(stderr, "started indexing for %s\n", args->inFile);

		keySpec->keyLen = args->keyLen;

		if (args->noDocs)
			parent = database;
		else {
			openDocStore(docHndl, database, "documents", strlen("documents"), params);
			parent = docHndl;
		}

		if ((stat = createIndex(index, parent, idxType, idxName, strlen(idxName), keySpec, sizeof(keySpec), params)))
		  fprintf(stderr, "createIndex Error %d name: %s\n", stat, idxName), exit(0);

		if (docHndl->handle.bits)
			addIndexKeys(docHndl);

		if( in = fopen (args->inFile, "r") )
		  while( ch = getc(in), ch != EOF )
			if( ch == '\n' )
			{
#ifdef DEBUG
			  if (!(line % 100000))
				fprintf(stderr, "line %lld\n", line);
#endif
			  line++;

			  if (docHndl->handle.bits) {
				if ((stat = addDocument (docHndl, key, len, &objId, txnId)))
				  fprintf(stderr, "Add Document Error %d Line: %lld\n", stat, line), exit(0);
			  } else
				if ((stat = insertKey(index, key, len)))
				  fprintf(stderr, "Insert Key Error %d Line: %lld\n", stat, line), exit(0);
			  len = 0;
			}
			else if( len < 4096 )
				key[len++] = ch;

		fprintf(stderr, " Total keys indexed %lld\n", line);
		break;

	case 'f':
		fprintf(stderr, "started finding keys for %s\n", args->inFile);

		keySpec->keyLen = args->keyLen;

		if (args->noDocs)
			parent = database;
		else {
			openDocStore(docHndl, database, "documents", strlen("documents"), params);
			parent = docHndl;
		}

		if ((stat = createIndex(index, parent, idxType, idxName, strlen(idxName), keySpec, sizeof(keySpec), params)))
		  fprintf(stderr, "createIndex Error %d name: %s\n", stat, idxName), exit(0);

		if (docHndl->handle.bits)
			addIndexKeys(docHndl);

		createCursor (cursor, index, txnId, 'f');

		if( in = fopen (args->inFile, "rb") )
		  while( ch = getc(in), ch != EOF )
			if( ch == '\n' )
			{
#ifdef DEBUG
			  if (!(line % 100000))
				fprintf(stderr, "line %lld\n", line);
#endif
			  line++;

			  if (keySpec->keyLen)
				len = keySpec->keyLen;

			  stat = positionCursor (cursor, key, len);

			  if (args->noDocs) {
				if (stat == CURSOR_notfound)
				  fprintf(stderr, "findKey not Found: line: %lld expected: %.*s \n", line, len, key), exit(0);
			  } else {
				if ((stat = nextDoc (cursor, &doc, args->maxKey, maxLen)))
				  fprintf(stderr, "findDocument Error %d Syserr %d Line: %lld\n", stat, errno, line), exit(0);
				if (memcmp(doc + 1, key, len))
				  fprintf(stderr, "findDoc Error: line: %lld expected: %.*s found: %.*s.\n", line, len, key, doc->size, (char *)(doc + 1)), exit(0);
			  }

			  cnt++;
			  len = 0;
			}
			else if( len < 4096 )
				key[len++] = ch;

		fprintf(stderr, "finished %s for %lld keys, found %lld\n", args->inFile, line, cnt);
		break;

	case 's':
		fprintf(stderr, "started scanning");

		if (args->minKey)
			fprintf(stderr, " min key: %s", args->minKey);

		if (args->maxKey)
			fprintf(stderr, " max key: %s", args->maxKey);

		fprintf(stderr, "\n");

		if (args->noDocs)
			parent = database;
		else {
			openDocStore(docHndl, database, "documents", strlen("documents"), params);
			parent = docHndl;
		}

		if ((stat = createIndex(index, parent, idxType, idxName, strlen(idxName), keySpec, sizeof(keySpec), params)))
		  fprintf(stderr, "createIndex Error %d name: %s\n", stat, idxName), exit(0);

		if (docHndl->handle.bits)
			addIndexKeys(docHndl);

		// create forward cursor

		createCursor (cursor, index, txnId, 'f');

		if (args->minKey)
			positionCursor (cursor, args->minKey, strlen(args->minKey));

		if (args->noDocs)
		  while (!(stat = nextKey (cursor, &keyPtr, &keyLen, args->maxKey, maxLen))) {
			fwrite (keyPtr, keyLen, 1, stdout);
			fputc ('\n', stdout);
			cnt++;
		  }
		else
		  while (!(stat = nextDoc(cursor, &doc, args->maxKey, maxLen))) {
            fwrite (doc + 1, doc->size, 1, stdout);
            fputc ('\n', stdout);
            cnt++;
		  }

		if (stat != ERROR_endoffile)
		  fprintf(stderr, "fwdScan: Error %d Syserr %d Line: %lld\n", stat, errno, cnt), exit(0);

		fprintf(stderr, " Total keys read %lld\n", cnt);
		break;

	case 'r':
		fprintf(stderr, "started reverse scanning");

		if (args->minKey)
			fprintf(stderr, " min key: %s", args->minKey);

		if (args->maxKey)
			fprintf(stderr, " max key: %s", args->maxKey);

		fprintf(stderr, "\n");

		if (args->noDocs)
			parent = database;
		else {
			openDocStore(docHndl, database, "documents", strlen("documents"), params);
			parent = docHndl;
		}

		if ((stat = createIndex(index, parent, idxType, idxName, strlen(idxName), keySpec, sizeof(keySpec), params)))
		  fprintf(stderr, "createIndex Error %d name: %s\n", stat, idxName), exit(0);

		if (docHndl->handle.bits)
			addIndexKeys(docHndl);

		// create reverse cursor

		createCursor (cursor, index, txnId, 'r');

		if (args->minKey)
			positionCursor (cursor, args->minKey, strlen(args->minKey));

		if (args->noDocs)
		  while (!(stat = prevKey (cursor, &keyPtr, &keyLen, args->maxKey, maxLen))) {
			fwrite (keyPtr, keyLen, 1, stdout);
			fputc ('\n', stdout);
			cnt++;
		  }
		else
		  while (!(stat = prevDoc(cursor, &doc, args->maxKey, maxLen))) {
            fwrite (doc + 1, doc->size, 1, stdout);
            fputc ('\n', stdout);
            cnt++;
		  }

		if (stat != ERROR_endoffile)
		  fprintf(stderr, "revScan: Error %d Syserr %d Line: %lld\n", stat, errno, cnt), exit(0);

		fprintf(stderr, " Total keys read %lld\n", cnt);
		break;

	case 'c':
		fprintf(stderr, "started counting");

		if (args->minKey)
			fprintf(stderr, " min key: %s", args->minKey);

		if (args->maxKey)
			fprintf(stderr, " max key: %s", args->maxKey);

		fprintf(stderr, "\n");

		if (args->noDocs)
			parent = database;
		else {
			openDocStore(docHndl, database, "documents", strlen("documents"), params);
			parent = docHndl;
		}

		if ((stat = createIndex(index, parent, idxType, idxName, strlen(idxName), keySpec, sizeof(keySpec), params)))
		  fprintf(stderr, "createIndex Error %d name: %s\n", stat, idxName), exit(0);

		//  create forward cursor

		createCursor (cursor, index, txnId, 'f');

		if (args->noDocs)
	  	  while (!(stat = nextKey(cursor, NULL, NULL, args->maxKey, maxLen)))
			cnt++;
		else
	  	  while (!(stat = nextDoc(cursor, &doc, args->maxKey, maxLen)))
			cnt++;

		fprintf(stderr, " Total keys counted %lld\n", cnt);
		break;
	}

#ifdef unix
	return NULL;
#else
	return 0;
#endif
}

typedef struct timeval timer;

int main (int argc, char **argv)
{
int idx, cnt, len, slot, err;
int useTxn = 0, noDocs = 0;
int xtra = 0, bits = 16;
char *minKey = NULL;
char *maxKey = NULL;
int keyLen = 10;
int idxType = 0;
char *dbName;
char *cmds;

double start, stop;
#ifdef unix
pthread_t *threads;
#else
SYSTEM_INFO info[1];
HANDLE *threads;
#endif
ThreadArg *args;
float elapsed;
int num = 0;
char key[1];
Params params[MaxParam];
bool onDisk = true;
DbHandle database[1];
DbHandle docHndl[1];
DbHandle index[1];

#ifdef _WIN32
	GetSystemInfo(info);
	fprintf(stderr, "PageSize: %d, # Processors: %d, Allocation Granularity: %d\n\n", info->dwPageSize, info->dwNumberOfProcessors, info->dwAllocationGranularity);
#endif
	if( argc < 3 ) {
		fprintf (stderr, "Usage: %s db_name -cmds=[crwsdf]... -idxType=[012] -bits=# -xtra=# -inMem -txns -noDocs -keyLen=# -minKey=abcd -maxKey=abce src_file1 src_file2 ... ]\n", argv[0]);
		fprintf (stderr, "  where db_name is the prefix name of the database file\n");
		fprintf (stderr, "  cmds is a string of (c)ount/(r)ev scan/(w)rite/(s)can/(d)elete/(f)ind, with a one character command for each input src_file, or a no-input command.\n");
		fprintf (stderr, "  idxType is the type of index: 0 = ART, 1 = btree1, 2 = btree2\n");
		fprintf (stderr, "  keyLen is key size, zero for whole line\n");
		fprintf (stderr, "  bits is the btree page size in bits\n");
		fprintf (stderr, "  xtra is the btree leaf page extra bits\n");
		fprintf (stderr, "  inMem specifies no disk files\n");
		fprintf (stderr, "  noDocs specifies keys only\n");
		fprintf (stderr, "  txns indicates use of transactions\n");
		fprintf (stderr, "  minKey specifies beginning cursor key\n");
		fprintf (stderr, "  maxKey specifies ending cursor key\n");
		fprintf (stderr, "  src_file1 thru src_filen are files of keys/documents separated by newline\n");
		exit(0);
	}

	// process database name

	dbName = (++argv)[0];
	argc--;

	// process configuration arguments

	while (--argc > 0 && (++argv)[0][0] == '-')
	  if (!memcmp(argv[0], "-xtra=", 6))
			xtra = atoi(argv[0] + 6);
	  else if (!memcmp(argv[0], "-keyLen=", 8))
			keyLen = atoi(argv[0] + 8);
	  else if (!memcmp(argv[0], "-bits=", 6))
			bits = atoi(argv[0] + 6);
	  else if (!memcmp(argv[0], "-cmds=", 6))
			cmds = argv[0] + 6;
	  else if (!memcmp(argv[0], "-idxType=", 9))
			idxType = atoi(argv[0] + 9);
	  else if (!memcmp(argv[0], "-inMem", 6))
			onDisk = 0;
	  else if (!memcmp(argv[0], "-txns", 5))
			useTxn = 1;
	  else if (!memcmp(argv[0], "-noDocs", 7))
			noDocs = 1;
	  else if (!memcmp(argv[0], "-minKey=", 8))
			minKey = argv[0] + 8;
	  else if (!memcmp(argv[0], "-maxKey=", 8))
			maxKey = argv[0] + 8;
	  else
			fprintf(stderr, "Unknown option %s ignored\n", argv[0]);

	cnt = argc;
	initialize();

	start = getCpuTime(0);

#ifdef unix
	threads = malloc (cnt * sizeof(pthread_t));
#else
	threads = GlobalAlloc (GMEM_FIXED|GMEM_ZEROINIT, cnt * sizeof(HANDLE));
#endif
	args = malloc ((cnt ? cnt : 1) * sizeof(ThreadArg));

	params[OnDisk].boolVal = onDisk;
	openDatabase(database, dbName, strlen(dbName), params);

	//	fire off threads

	idx = 0;

	do {
	  args[idx].database = database;
	  args[idx].inFile = argv[idx];
	  args[idx].idxType = idxType;
	  args[idx].minKey = minKey;
	  args[idx].maxKey = maxKey;
	  args[idx].keyLen = keyLen;
	  args[idx].noDocs = noDocs;
	  args[idx].useTxn = useTxn;
	  args[idx].onDisk = onDisk;
	  args[idx].cmds = cmds;
	  args[idx].bits = bits;
	  args[idx].xtra = xtra;
	  args[idx].num = num;
	  args[idx].idx = idx;

	  if (cnt > 1) {
#ifdef unix
		if( err = pthread_create (threads + idx, NULL, index_file, args + idx) )
		  fprintf(stderr, "Error creating thread %d\n", err);
#else
		while ( (int64_t)(threads[idx] = (HANDLE)_beginthreadex(NULL, 65536, index_file, args + idx, 0, NULL)) < 0LL)
		  fprintf(stderr, "Error creating thread errno = %d\n", errno);

#endif
		continue;
	  } else
	  	//  if zero or one files specified,
	  	//  run index_file once

	  	index_file (args);
	} while (++idx < cnt);

	// 	wait for termination

#ifdef unix
	if (cnt > 1)
	  for( idx = 0; idx < cnt; idx++ )
		pthread_join (threads[idx], NULL);
#else
	if (cnt > 1)
	  WaitForMultipleObjects (cnt, threads, TRUE, INFINITE);

	if (cnt > 1)
	  for( idx = 0; idx < cnt; idx++ )
		CloseHandle(threads[idx]);
#endif

	elapsed = getCpuTime(0) - start;
	fprintf(stderr, " real %dm%.3fs\n", (int)(elapsed/60), elapsed - (int)(elapsed/60)*60);
	elapsed = getCpuTime(1);
	fprintf(stderr, " user %dm%.3fs\n", (int)(elapsed/60), elapsed - (int)(elapsed/60)*60);
	elapsed = getCpuTime(2);
	fprintf(stderr, " sys  %dm%.3fs\n", (int)(elapsed/60), elapsed - (int)(elapsed/60)*60);

}
