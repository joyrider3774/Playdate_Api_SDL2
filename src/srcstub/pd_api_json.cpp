#include <string.h>
#include <dirent.h>
#include "pd_api/pd_api_json.h"
#include "pd_api.h"
#include "gamestubcallbacks.h"
#include "gamestub.h"
#include "defines.h"

void pd_api_json_startArray(struct json_encoder* encoder)
{

}

void pd_api_json_addArrayMember(struct json_encoder* encoder)
{

}

void pd_api_json_endArray(struct json_encoder* encoder)
{

}

void pd_api_json_startTable(struct json_encoder* encoder)
{

}

void pd_api_json_addTableMember(struct json_encoder* encoder, const char* name, int len)
{

}

void pd_api_json_endTable(struct json_encoder* encoder)
{

}

void pd_api_json_writeNull(struct json_encoder* encoder)
{

}

void pd_api_json_writeFalse(struct json_encoder* encoder)
{

}

void pd_api_json_writeTrue(struct json_encoder* encoder)
{

}

void pd_api_json_writeInt(struct json_encoder* encoder, int num)
{

}

void pd_api_json_writeDouble(struct json_encoder* encoder, double num)
{

}

void pd_api_json_writeString(struct json_encoder* encoder, const char* str, int len)
{

}



void pd_api_json_initEncoder(json_encoder* encoder, json_writeFunc* write, void* userdata, int pretty)
{
    encoder->startArray = pd_api_json_startArray;
    encoder->addArrayMember = pd_api_json_addArrayMember;
    encoder->endArray = pd_api_json_endArray;
    encoder->startTable = pd_api_json_startTable;
    encoder->addTableMember = pd_api_json_addTableMember;
    encoder->endTable = pd_api_json_endTable;
    encoder->writeNull = pd_api_json_writeNull;
    encoder->writeFalse = pd_api_json_writeFalse;
    encoder->writeTrue = pd_api_json_writeTrue;
    encoder->writeInt = pd_api_json_writeInt;
    encoder->writeDouble =  pd_api_json_writeDouble;
    encoder->writeString = pd_api_json_writeString;
    encoder->userdata = userdata;
    encoder->pretty = pretty;
    encoder->writeStringFunc = write;
}

int pd_api_json_decode(struct json_decoder* functions, json_reader reader, json_value* outval)
{
    return 0;
}

int pd_api_json_decodeString(struct json_decoder* functions, const char* jsonString, json_value* outval)
{
    return 0;
}


playdate_json* pd_api_json_Create_playdate_json()
{
    playdate_json *Tmp = (playdate_json*) malloc(sizeof(*Tmp));
	Tmp->initEncoder = pd_api_json_initEncoder;
	Tmp->decode = pd_api_json_decode;
	Tmp->decodeString = pd_api_json_decodeString;
    return Tmp;
};