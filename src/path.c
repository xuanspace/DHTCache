/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * file path utilitis
 *
  * Author(s): wxlin    <weixuan.lin@sierraatlantic.com>
  *
 * $Id: path.c,v 1.3 2008-12-23 07:16:33 wxlin Exp $
 *
 */

static void reduce(char *dir)
{
    size_t i = strlen(dir);
    while (i > 0 && dir[i] != SEP)
        --i;
    dir[i] = '\0';
}

/* Is file, not directory */
static int isfile(char *filename)          
{
    struct stat buf;
    if (stat(filename, &buf) != 0)
        return 0;
    if (!S_ISREG(buf.st_mode))
        return 0;
    return 1;
}

/* Is executable file */
static int isxfile(char *filename)         
{
    struct stat buf;
    if (stat(filename, &buf) != 0)
        return 0;
    if (!S_ISREG(buf.st_mode))
        return 0;
    if ((buf.st_mode & 0111) == 0)
        return 0;
    return 1;
}

/* Is directory */
static int isdir(char *filename)  
{
    struct stat buf;
    if (stat(filename, &buf) != 0)
        return 0;
    if (!S_ISDIR(buf.st_mode))
        return 0;
    return 1;
}

/* Make directory */
static void mkpath(char *filename)
{
    char *p;
	char *path = alloca(strlen(filename) + 1);
	
    strcpy(path, filename);
    for (p = path; *p; p++) {
        if (*p == '/' && p != path) {
            *p = '\0';
            mkdir(path, 0755);
            *p = '/';
        }
    }
}

