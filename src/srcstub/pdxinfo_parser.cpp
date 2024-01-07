#include <stdio.h>
#include <string>
#include <cstring>
#include <stdlib.h>
#include "pdxinfo_parser.h"


const int MAXFIELDSIZE = 4096;

typedef struct _pdx_info_parser_entry _pdx_info_parser_entry;
struct _pdx_info_parser_entry {
	bool retrieved;
	char value[MAXFIELDSIZE];
	_pdx_info_parser_entry() : retrieved(false), value("") {}
};

const int _pdx_info_parser_imagePath_ID  = 0;
const int _pdx_info_parser_buildNumber_ID  = 1;
const int _pdx_info_parser_version_ID  = 2;
const int _pdx_info_parser_description_ID  = 3;
const int _pdx_info_parser_bundleID_ID  = 4;
const int _pdx_info_parser_author_ID  = 5;
const int _pdx_info_parser_name_ID  = 6;
const int _pdx_info_parser_ID_Count  = 7;

_pdx_info_parser_entry _pdx_info_parser_cache[_pdx_info_parser_ID_Count];


//https://stackoverflow.com/a/27304609
char* stristr( const char* str1, const char* str2 )
{
    const char* p1 = str1 ;
    const char* p2 = str2 ;
    const char* r = *p2 == 0 ? str1 : 0 ;

    while( *p1 != 0 && *p2 != 0 )
    {
        if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) )
        {
            if( r == 0 )
            {
                r = p1 ;
            }

            p2++ ;
        }
        else
        {
            p2 = str2 ;
            if( r != 0 )
            {
                p1 = r + 1 ;
            }

            if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) )
            {
                r = p1 ;
                p2++ ;
            }
            else
            {
                r = 0 ;
            }
        }

        p1++ ;
    }

    return *p2 == 0 ? (char*)r : 0 ;
}

char* _pdx_info_parser_get(const char* field, char *buffer, size_t bufsize)
{
	FILE* fp;
	char* result = NULL;
	char* foundstr = NULL;
	memset(buffer, 0, bufsize);
	fp = fopen("./pdxinfo", "r");
	if(fp)
	{	
		//find out filesize
		fseek(fp, 0l, SEEK_END);
		long fileSize = ftell(fp);
		fseek(fp, 0l, SEEK_SET);
		char buf[fileSize+1];
		memset(buf, 0, fileSize+1);
		
		//read whole file
		fread(buf, fileSize, 1, fp);
	
		//find our string
		foundstr = stristr(buf, (std::string(field) + "=").c_str());
		if(foundstr)
		{
			//find windows endline
			char* endline = strstr(foundstr, "\r\n");
			if (endline)
			{
				endline[0] = '\0';
			}
			else
			{
				//find mac endline
				endline = strstr(foundstr, "\r");
				if (endline)
				{
					endline[0] = '\0';
				}
				else
				{
					//find unix endline
					endline = strstr(foundstr, "\n");
					if (endline)
					{
						endline[0] = '\0';
					}
				}
			}
			
			//remove field=
			foundstr = foundstr + strlen(field) + 1;
			result = strncpy(buffer, foundstr, bufsize);				
		}
		fclose(fp);
	}
	return result;
}

char * _pdx_info_parser_retrieve(int id)
{
	if(!_pdx_info_parser_cache[id].retrieved)
	{
		_pdx_info_parser_cache[id].retrieved = true;
		switch(id)
		{
			case _pdx_info_parser_author_ID:
				_pdx_info_parser_get("author", _pdx_info_parser_cache[id].value, MAXFIELDSIZE);
				break;
			case _pdx_info_parser_name_ID:
				_pdx_info_parser_get("name", _pdx_info_parser_cache[id].value, MAXFIELDSIZE);
				break;
			case _pdx_info_parser_buildNumber_ID:
				_pdx_info_parser_get("buildNumber", _pdx_info_parser_cache[id].value, MAXFIELDSIZE);
				break;
			case _pdx_info_parser_version_ID:
				_pdx_info_parser_get("version", _pdx_info_parser_cache[id].value, MAXFIELDSIZE);
				break;
			case _pdx_info_parser_bundleID_ID:
				_pdx_info_parser_get("bundleID", _pdx_info_parser_cache[id].value, MAXFIELDSIZE);
				break;
			case _pdx_info_parser_imagePath_ID:
				_pdx_info_parser_get("imagePath", _pdx_info_parser_cache[id].value, MAXFIELDSIZE);
				break;
			case _pdx_info_parser_description_ID:
				_pdx_info_parser_get("description", _pdx_info_parser_cache[id].value, MAXFIELDSIZE);
				break;
			default:
				break;
		}
	
	}

	return _pdx_info_parser_cache[id].value;
}

char * pdx_info_parser_name()
{
	return _pdx_info_parser_retrieve(_pdx_info_parser_name_ID);
}

char * pdx_info_parser_author()
{
	return _pdx_info_parser_retrieve(_pdx_info_parser_author_ID);
}

char * pdx_info_parser_bundleID()
{
	return _pdx_info_parser_retrieve(_pdx_info_parser_bundleID_ID);
}

char * pdx_info_parser_description()
{
	return _pdx_info_parser_retrieve(_pdx_info_parser_description_ID);
}

char * pdx_info_parser_version()
{
	return _pdx_info_parser_retrieve(_pdx_info_parser_version_ID);
}

char * pdx_info_parser_buildNumber()
{
	return _pdx_info_parser_retrieve(_pdx_info_parser_buildNumber_ID);
}

char * pdx_info_parser_imagePath()
{
	return _pdx_info_parser_retrieve(_pdx_info_parser_imagePath_ID);
}
