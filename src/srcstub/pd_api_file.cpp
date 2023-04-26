#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "pd_api/pd_api_display.h"
#include "pd_api/pd_api_file.h"
#include "gamestubcallbacks.h"
#include "defines.h"

const char* pd_api_file_geterr(void)
{
    return strerror(errno);
}

int	pd_api_file_stat(const char* path, FileStat* stats)
{
    int result = -1;
    struct stat lstats;
    char filename[MAXPATH];
    sprintf(filename, "./saveddata/%s", path);
    if(stat(filename, &lstats) == 0)
    {
        stats->isdir = S_ISDIR(lstats.st_mode);
        stats->size = lstats.st_size;
        struct tm dt;
        dt = *(gmtime(&lstats.st_mtime));
        stats->m_day = dt.tm_mday;
        stats->m_hour = dt.tm_hour;
        stats->m_minute = dt.tm_min;
        stats->m_month = dt.tm_mon;
        stats->m_second = dt.tm_sec;
        stats->m_year = 1900+dt.tm_year;
        result = 0;
    }
    else
    {
        sprintf(filename, "./%s", path);
        if(stat(filename, &lstats) == 0)
        {
            stats->isdir = S_ISDIR(lstats.st_mode);
            stats->size = lstats.st_size;
            struct tm dt;
            dt = *(gmtime(&lstats.st_mtime));
            stats->m_day = dt.tm_mday;
            stats->m_hour = dt.tm_hour;
            stats->m_minute = dt.tm_min;
            stats->m_month = dt.tm_mon;
            stats->m_second = dt.tm_sec;
            stats->m_year = 1900+dt.tm_year;
            result = 0;
        }   
    }
    return result;
}


int	pd_api_file_listfiles(const char* path, void (*callback)(const char* path, void* userdata), void* userdata, int showhidden)
{
    char filename[MAXPATH];
    char filename2[(2*MAXPATH)+1];
    int listsize = ARRAY_LIST_INCSIZES;
    int listcount = 0;
    struct dirent *entry;
    struct stat lstats;

    char **filenamelist = (char **) malloc(listsize * sizeof (*filenamelist));

    //saved data folder
    sprintf(filename, "./saveddata/%s", path);
    DIR *dir = opendir(filename);
    if (dir != NULL) 
    {
        while ((entry = readdir(dir)) != NULL) 
        {
            if((strcmp(entry->d_name, ".") == 0) ||
               (strcmp(entry->d_name, "..") == 0))
                continue;

            sprintf(filename2, "%s/%s", filename, entry->d_name);           
            if(stat(filename2, &lstats) == 0)
            {
                if(S_ISDIR(lstats.st_mode))
                    sprintf(filename2, "%s/", entry->d_name);
                else
                    sprintf(filename2, "%s", entry->d_name);
                callback(filename2, userdata);
                //keep found values in in a list so we won't send duplicate names
                if (listcount >= listsize)
                {
                    listsize += ARRAY_LIST_INCSIZES;
                    filenamelist = (char **) realloc(filenamelist, listsize * sizeof(*filenamelist));
                }

                filenamelist[listcount] = (char *) malloc(261 * sizeof(char));
                strcpy(filenamelist[listcount++], filename2);
            }
        }
        closedir(dir);
    }

    //current folder
    sprintf(filename, "./%s", path);
    dir = opendir(filename);
    if (dir != NULL) 
    {
        while ((entry = readdir(dir)) != NULL) 
        {
            if((strcmp(entry->d_name, ".") == 0) ||
               (strcmp(entry->d_name, "..") == 0))
                continue;
            sprintf(filename2, "%s/%s", filename, entry->d_name);           
            if(stat(filename2, &lstats) == 0)
            {
                if(S_ISDIR(lstats.st_mode))
                    sprintf(filename2, "%s/", entry->d_name);
                else
                    sprintf(filename2, "%s", entry->d_name);
                bool bfound = false;
                for (int i = 0; i < listcount; i++)
                {
                    bfound = strcmp(filename2, filenamelist[i]) == 0;
                    if(bfound)
                    {
                        break;
                    }
                }

                if(!bfound)
                {
                    callback(filename2, userdata);
                }
            }
        }
        closedir(dir);
    }
    for (int i = 0; i < listcount; i++)
    {
        free(filenamelist[i]);
    }
    free(filenamelist);
    return 0;
}

int	pd_api_file_mkdir(const char* path)
{
    char filename[MAXPATH];
    sprintf(filename, "./saveddata/%s", path);
    #if defined(_WIN32)
    return mkdir(filename);
    #else 
    return mkdir(filename, 0777);
    #endif
}

int	pd_api_file_unlink(const char* name, int recursive)
{
    int result = 0;
    if (strlen(name) > 0)
    {
        if((strcmp(name, ".") == 0) ||
           (strcmp(name, "..") == 0))
            return 0;

        struct stat lstats;
        char filename[MAXPATH];
        char filename2[(2*MAXPATH) + 1];
        char filename3[(2*MAXPATH) + 1];
        sprintf(filename, "./saveddata/%s", name);
        if(stat(filename, &lstats) == 0)
        {
            if(S_ISDIR(lstats.st_mode))
            {
                if(recursive)
                {
                    struct dirent *entry;            
                    DIR *dir = opendir(name);
                    dir = opendir(filename);
                    if (dir != NULL) 
                    {
                        while ((entry = readdir(dir)) != NULL) 
                        {
                            if((strcmp(entry->d_name, ".") == 0) ||
                            (strcmp(entry->d_name, "..") == 0))
                                continue;
                            sprintf(filename2, "%s/%s", filename, entry->d_name);
                            if(stat(filename2, &lstats) == 0)
                            {
                                if(S_ISDIR(lstats.st_mode))
                                {
                                    sprintf(filename3, "%s/%s", name, entry->d_name);
                                    if(pd_api_file_unlink(filename3, recursive) == -1)
                                    {
                                        result = -1;
                                    }
                                }
                                else
                                {
                                    if(unlink(filename2) == -1)
                                    {
                                        result = -1;
                                    }
                                }                 
                            }
                        }
                        closedir(dir);
                    }
                }
                if(rmdir(filename) == -1)
                {
                    result = -1;
                }

            }
            else
            {
                if(unlink(filename) == -1)
                {
                    result = -1;
                }
            }
        }
    }
    return result;
}

int	pd_api_file_rename(const char* from, const char* to)
{
    char filename[MAXPATH];
    char filenameTo[MAXPATH];
    sprintf(filename, "./saveddata/%s", from);
    sprintf(filenameTo, "./saveddata/%s", to);
    return rename(filename, filenameTo);
}

SDFile*	pd_api_file_SDFileopen(const char* name, FileOptions mode)
{
    char modestr[4];
    char filename[MAXPATH];
    FILE *LastFile = NULL;
    if (((mode & kFileWrite) == kFileWrite) || ((mode & kFileAppend) == kFileAppend) || ((mode & kFileReadData) == kFileReadData))
    {
        pd_api_file_mkdir("");
        sprintf(filename, "./saveddata/%s", name);
    }
    else
    {
        sprintf(filename, "./%s", name);
    }

    if ((mode & kFileWrite) == kFileWrite)
    {
        modestr[0] = 'w';
        modestr[1] = '+';
        modestr[2] = 'b';
        modestr[3] = '\0';
        LastFile = fopen(filename, modestr);
    }
    else
    {
        if ((mode & kFileAppend) == kFileAppend)
        {
            modestr[0] = 'a';
            modestr[1] = '+';
            modestr[2] = 'b';
            modestr[3] = '\0';
            LastFile = fopen(filename, modestr);
        }
        else
        {
            //both read and readdata, data gets preference
            if (((mode & kFileReadData) == kFileReadData) && ((mode & kFileRead) == kFileRead))
            {
                modestr[0] = 'r';
                modestr[1] = '+';
                modestr[2] = 'b';
                modestr[3] = '\0';
                LastFile = fopen(filename, modestr);
                if(!LastFile)
                {
                    sprintf(filename, "./%s", name);
                    LastFile = fopen(filename, modestr);
                }
            }
            else
            {
                //read mode, path has been set in beginning of function then
                if(((mode & kFileRead) == kFileRead) || ((mode & kFileReadData) == kFileReadData))
                {
                    modestr[0] = 'r';
                    modestr[1] = '+';
                    modestr[2] = 'b';
                    modestr[3] = '\0';
                    LastFile = fopen(filename, modestr);
                }
            }
        }
    }
    return LastFile;
}

int	pd_api_file_SDFileclose(SDFile* file)
{
    return fclose((FILE*)file);
}

int	pd_api_file_SDFileread(SDFile* file, void* buf, unsigned int len)
{
    return fread(buf, 1, len, (FILE*)file);
}

int	pd_api_file_SDFilewrite(SDFile* file, const void* buf, unsigned int len)
{
    return fwrite(buf, 1, len, (FILE*)file);
}

int	pd_api_file_SDFileflush(SDFile* file)
{
    return fflush((FILE*)file);
}

int	pd_api_file_SDFiletell(SDFile* file)
{
    return ftell((FILE*)file);
}

int	pd_api_file_SDFileseek(SDFile* file, int pos, int whence)
{
    return fseek((FILE*)file, pos, whence);
}

playdate_file* pd_api_file_Create_playdate_file()
{
    playdate_file *Tmp = (playdate_file*) malloc(sizeof(*Tmp));
   	Tmp->geterr = pd_api_file_geterr;
	Tmp->listfiles = pd_api_file_listfiles;
	Tmp->stat = pd_api_file_stat;
	Tmp->mkdir = pd_api_file_mkdir;
	Tmp->unlink = pd_api_file_unlink;
	Tmp->rename = pd_api_file_rename;
	
	Tmp->open = pd_api_file_SDFileopen;
	Tmp->close = pd_api_file_SDFileclose;
	Tmp->read = pd_api_file_SDFileread;
	Tmp->write = pd_api_file_SDFilewrite;
	Tmp->flush = pd_api_file_SDFileflush;
	Tmp->tell = pd_api_file_SDFiletell;
	Tmp->seek = pd_api_file_SDFileseek;
    return Tmp;
}