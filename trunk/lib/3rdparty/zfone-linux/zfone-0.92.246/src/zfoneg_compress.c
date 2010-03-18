#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <zlib.h>
#include <zrtp.h>

#define REPORT_FILE_NAME_TMP	"report_tmp"
#define REPORT_FILE_NAME_RES	"zfone-bug-report"
#define REPORT_TMP_DIR			"report"

#define CHUNK 		16384
#define PATH_LEN	128

typedef struct zfone_file_info
{
	char		name[64];
	long long	length;
} zfone_file_info_t;

uint64_t swap64(uint64_t x)
{
	uint64_t res;
	res =  (x >> 8  & 0x00000000ff000000ll) | (x << 8  & 0x000000ff00000000ll);
	res |= (x >> 24 & 0x0000000000ff0000ll) | (x << 24 & 0x0000ff0000000000ll);
	res |= (x >> 40 & 0x000000000000ff00ll) | (x << 40 & 0x00ff000000000000ll);
	res |= (x >> 56 & 0x00000000000000ffll) | (x << 56 & 0xff00000000000000ll);
	return res;
}

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
static int compress_file(char *source_name, char *dest_name, int level)
{
    FILE *source, *dest;
	int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return -1;

	source = fopen(source_name, "rb");
	if (!source)
	{
		(void)deflateEnd(&strm);
		return -1;
	}

	dest = fopen(dest_name, "wb");
	if (!dest)
	{
		(void)deflateEnd(&strm);
		fclose(source);
		return -1;
	}

	/* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) 
		{
            (void)deflateEnd(&strm);
			fclose(source);
			fclose(dest);
            return -1;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) 
			{
                (void)deflateEnd(&strm);
				fclose(source);
				fclose(dest);
                return -1;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
	fclose(source);
	fclose(dest);
    return 0;
}

/*
static int decompress_file(char *source_name, char *dest_name)
{
    FILE *source, *dest;
    int ret;
	unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return -1;

	source = fopen(source_name, "rb");
	if (!source)
	{
		(void)deflateEnd(&strm);
		return -1;
	}

	dest = fopen(dest_name, "wb");
	if (!dest)
	{
		(void)deflateEnd(&strm);
		fclose(source);
		return -1;
	}

    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) 
		{
            (void)inflateEnd(&strm);
			fclose(source);
			fclose(dest);
            return -1;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR); 
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
				fclose(source);
				fclose(dest);
                return -1;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
				fclose(source);
				fclose(dest);
                return -1;
            }
        } while (strm.avail_out == 0);

    } while (ret != Z_STREAM_END);

    (void)inflateEnd(&strm);
	fclose(source);
	fclose(dest);
    return ret == Z_STREAM_END ? 0 : -1;
}
*/
static int add_file(FILE *file, char *path)
{
	struct stat st;
	int count;		
	zfone_file_info_t info;
	FILE *f;
	char *ptr;
	
	char buffer[CHUNK];

	if (!file)
		return -1;

	if (stat(path, &st) < 0)
		return -1;

	f = fopen(path, "rb");
	if ( !f )
		return -1;
			
	ptr = strrchr(path, '/');
	if (!ptr)
		ptr = path;
	else 
		ptr++;
	strncpy(info.name, ptr, sizeof(info.name)-1);
	info.length = swap64(st.st_size);

	fwrite(&info, 1, sizeof(info), file);
	while ( (count = fread(buffer, 1, CHUNK, f)) )
	{
		fwrite(buffer, 1, count, file);
	}
	fclose(f);
	return 0;
}

static int create_file(char *name)
{
	FILE *fo;
	if ( !(fo = fopen(name, "wb")) )
	{
		return -1;
	}

	if ( add_file(fo, "/var/log/zfone_all.log") < 0 )
	{
		fclose(fo);
		return -1;
	}

	system("uname -a > sys.rep");
	system("echo '-----------------------' >> sys.rep");
	system("ifconfig >> sys.rep");
	system("echo '-----------------------' >> sys.rep");
	system("lsmod >> sys.rep");
	system("echo '-----------------------' >> sys.rep");
	system("ps -ax >> sys.rep");
	system("echo '-----------------------' >> sys.rep");
#if ZRTP_PLATFORM == ZP_LINUX
	system("iptables -L >> sys.rep");
#else
	system("ipfw list >> sys.rep");
#endif
	add_file(fo, "sys.rep");

	unlink("sys.rep");

	fclose(fo);
	return 0;
}

/*
static int extract_dir(char *dir, char *file_name)
{
	char buffer[CHUNK], path[PATH_LEN];
	FILE *fi = NULL;
	zfone_file_info_t info;

	mkdir(dir, 0);

	fi = fopen(file_name, "rb");
	if ( !fi )
		return -1;

	do 
	{
		FILE *fo = NULL;
		int count = fread(&info, 1, sizeof(info), fi);
		if (count < sizeof(info))
			break;
	
		info.length = swap64(info.length);
		strncpy(path, dir, PATH_LEN);
		strncat(path, "/", PATH_LEN);
		strncat(path, info.name, PATH_LEN);
		if ( (fo = fopen(path, "wb")) )
		{
			do {
				count = (int)fread(buffer, 1, CHUNK < info.length ? CHUNK : info.length, fi);
				if (fo)
					fwrite(buffer, 1, count, fo);
				info.length -= count;
			} while(info.length > 0 && count > 0);
			fclose(fo);
		}
		else 
			break;
	} while (1);

	fclose(fi);
	return 0;	
}
*/

static int do_create_report(char *report_name)
{
	char path[PATH_LEN];
	int length;
	struct stat st;

	strncpy(path, getenv("HOME"), PATH_LEN);
	strncat(path, "/", PATH_LEN);
	length = strlen(path);
	strncat(path, "Desktop", PATH_LEN);
	if ( stat(path, &st) < 0 )
		path[length] = 0;
	else
		strncat(path, "/", PATH_LEN);
	strncat(path, report_name, PATH_LEN);
			
	if ( create_file(REPORT_FILE_NAME_TMP) < 0 )
	{
		unlink(REPORT_FILE_NAME_TMP);
		return -1;
	}

	if (compress_file(REPORT_FILE_NAME_TMP, path, Z_BEST_COMPRESSION) < 0)
	{
		
		unlink(REPORT_FILE_NAME_TMP);
		unlink(path);
		return -1;
	}

	unlink(REPORT_FILE_NAME_TMP);
	return 0;
}

int create_report()
{
	char report_name[PATH_LEN];
	time_t	t;
	struct tm *lt;

	time(&t);
	lt = localtime(&t);
	
	if ( !lt )
		return -1;
		
	snprintf(report_name, PATH_LEN, "%s-%.2d-%.2d-%.4d.rep", REPORT_FILE_NAME_RES, lt->tm_mon+1, lt->tm_mday, lt->tm_year+1900);
	return do_create_report(report_name);
}

