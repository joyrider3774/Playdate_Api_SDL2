#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string>
#include <sstream>
#include <vector>
#include "pd_api.h"
#include "pd_api/pd_api_json.h"
#include "gamestubcallbacks.h"
#include "gamestub.h"
#include "defines.h"
#include "debug.h"
#include "nlohmann_json/json.hpp"

using json = nlohmann::ordered_json;

// ============================================================
//  Helpers: convert nlohmann json value -> Playdate json_value
// ============================================================

static json_value make_json_value(const json& j)
{
    json_value v;
    v.data.intval = 0;

    if (j.is_null())
    {
        v.type = kJSONNull;
    }
    else if (j.is_boolean())
    {
        v.type = j.get<bool>() ? kJSONTrue : kJSONFalse;
    }
    else if (j.is_number_integer())
    {
        v.type = kJSONInteger;
        v.data.intval = j.get<int>();
    }
    else if (j.is_number_float())
    {
        v.type = kJSONFloat;
        v.data.floatval = j.get<float>();
    }
    else if (j.is_string())
    {
        v.type = kJSONString;
        // caller must treat this as read-only / short-lived;
        // we strdup so it is at least a valid pointer.
        v.data.stringval = strdup(j.get<std::string>().c_str());
    }
    else if (j.is_array())
    {
        v.type = kJSONArray;
        v.data.arrayval = nullptr; // sub-lists are walked via callbacks
    }
    else // object / table
    {
        v.type = kJSONTable;
        v.data.tableval = nullptr;
    }
    return v;
}

// ============================================================
//  Recursive decoder walk
// ============================================================

static void walk_json(json_decoder* dec, const json& j,
                      const char* name, int arrayIndex, int depth)
{
    // Simulator uses "_root" for the outermost object/array name
    const char* effName = (name && name[0]) ? name : "_root";

    if (j.is_object())
    {
        if (dec->willDecodeSublist)
            dec->willDecodeSublist(dec, effName, kJSONTable);

        for (auto& [key, val] : j.items())
        {
            if (val.is_object() || val.is_array())
            {
                json_value_type subtype = val.is_array() ? kJSONArray : kJSONTable;

                // Save callbacks before child walk
                auto savedTableValue   = dec->didDecodeTableValue;
                auto savedArrayValue   = dec->didDecodeArrayValue;
                auto savedSublist      = dec->didDecodeSublist;

                walk_json(dec, val, key.c_str(), 0, depth + 1);

                // didDecodeSublist fires after child walk, before didDecodeTableValue
                if (dec->didDecodeSublist)
                    dec->didDecodeSublist(dec, key.c_str(), subtype);

                // Restore parent handlers
                dec->didDecodeTableValue = savedTableValue;
                dec->didDecodeArrayValue = savedArrayValue;
                dec->didDecodeSublist    = savedSublist;

                // didDecodeTableValue fires after didDecodeSublist
                if (dec->didDecodeTableValue)
                {
                    if (!dec->shouldDecodeTableValueForKey ||
                        dec->shouldDecodeTableValueForKey(dec, key.c_str()))
                    {
                        json_value sv;
                        sv.type = subtype;
                        sv.data.tableval = nullptr;
                        dec->didDecodeTableValue(dec, key.c_str(), sv);
                    }
                }
            }
            else
            {
                if (dec->didDecodeTableValue)
                {
                    if (!dec->shouldDecodeTableValueForKey ||
                        dec->shouldDecodeTableValueForKey(dec, key.c_str()))
                    {
                        json_value v = make_json_value(val);
                        dec->didDecodeTableValue(dec, key.c_str(), v);
                        if (v.type == kJSONString && v.data.stringval)
                            free(v.data.stringval);
                    }
                }
            }
        }
    }
    else if (j.is_array())
    {
        if (dec->willDecodeSublist)
            dec->willDecodeSublist(dec, effName, kJSONArray);

        int pos = 1; // Simulator uses 1-based positions
        for (auto& val : j)
        {
            if (val.is_object() || val.is_array())
            {
                // Build element name like simulator: "parentName[N]"
                char elemName[128];
                snprintf(elemName, sizeof(elemName), "%s[%d]", effName, pos);

                // shouldDecodeArrayValueAtIndex fires BEFORE the element walk —
                // including before willDecodeSublist — so games can use it to
                // track the current index (e.g. set indexLast) before willDecodeSublist
                // uses that index to set frameIndex etc.
                bool shouldDecode = true;
                if (dec->shouldDecodeArrayValueAtIndex)
                    shouldDecode = dec->shouldDecodeArrayValueAtIndex(dec, pos) != 0;

                if (shouldDecode)
                {
                    walk_json(dec, val, elemName, pos, depth + 1);

                    json_value_type subtype = val.is_array() ? kJSONArray : kJSONTable;

                    // didDecodeSublist fires after child walk
                    if (dec->didDecodeSublist)
                        dec->didDecodeSublist(dec, elemName, subtype);

                    // didDecodeArrayValue fires after didDecodeSublist
                    if (dec->didDecodeArrayValue)
                    {
                        json_value sv;
                        sv.type = subtype;
                        sv.data.tableval = nullptr;
                        dec->didDecodeArrayValue(dec, pos, sv);
                    }
                }
            }
            else
            {
                if (dec->didDecodeArrayValue)
                {
                    if (!dec->shouldDecodeArrayValueAtIndex ||
                        dec->shouldDecodeArrayValueAtIndex(dec, pos))
                    {
                        json_value v = make_json_value(val);
                        dec->didDecodeArrayValue(dec, pos, v);
                        if (v.type == kJSONString && v.data.stringval)
                            free(v.data.stringval);
                    }
                }
            }
            pos++;
        }
    }
    else
    {
        // bare root value (pos==0 means root per Playdate docs)
        if (dec->didDecodeArrayValue)
        {
            json_value v = make_json_value(j);
            dec->didDecodeArrayValue(dec, 0, v);
            if (v.type == kJSONString && v.data.stringval)
                free(v.data.stringval);
        }
    }
}

static int do_decode(json_decoder* functions, const std::string& input, json_value* outval)
{
    if (!functions)
        return 0;

    json j;
    try
    {
        j = json::parse(input);
    }
    catch (const json::parse_error& e)
    {
        if (functions->decodeError)
            functions->decodeError(functions, e.what(), 0);
        return 0;
    }

    walk_json(functions, j, nullptr, 0, 0);

    // Fire didDecodeSublist for root, matching simulator behaviour
    if (functions->didDecodeSublist)
    {
        json_value_type rootType = j.is_array() ? kJSONArray : kJSONTable;
        functions->didDecodeSublist(functions, "_root", rootType);
    }

    if (outval)
        *outval = make_json_value(j);

    return 1;
}

// ============================================================
//  Public decode API
// ============================================================

int pd_api_json_decode(struct json_decoder* functions, json_reader reader, json_value* outval)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_decode\n");

    if (!functions || !reader.read)
    {
        printfDebug(DebugTraceFunctions, "pd_api_json_decode end (null args)\n");
        return 0;
    }

    // Read all data from the reader into a string
    std::string buf;
    uint8_t tmp[512];
    int n;
    while ((n = reader.read(reader.userdata, tmp, sizeof(tmp))) > 0)
        buf.append(reinterpret_cast<char*>(tmp), n);

    int result = do_decode(functions, buf, outval);
    printfDebug(DebugTraceFunctions, "pd_api_json_decode end\n");
    return result;
}

int pd_api_json_decodeString(struct json_decoder* functions, const char* jsonString, json_value* outval)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_decodeString\n");

    if (!functions || !jsonString)
    {
        printfDebug(DebugTraceFunctions, "pd_api_json_decodeString end (null args)\n");
        return 0;
    }

    int result = do_decode(functions, std::string(jsonString), outval);
    printfDebug(DebugTraceFunctions, "pd_api_json_decodeString end\n");
    return result;
}

// ============================================================
//  Encoder state
//  The Playdate encoder is a streaming writer: the game calls
//  encoder->startTable(), encoder->addTableMember("key", len),
//  encoder->writeInt(42), encoder->endTable() etc. and each
//  call appends text to the output via writeStringFunc.
// ============================================================

// Helper: write a C string through the encoder's write function
static void enc_write(json_encoder* enc, const char* s)
{
    if (enc->writeStringFunc && s)
        enc->writeStringFunc(enc->userdata, s, (int)strlen(s));
}

static void enc_write_indent(json_encoder* enc)
{
    if (!enc->pretty) return;
    for (int i = 0; i < enc->depth; i++)
        enc_write(enc, "\t");
}

static void enc_write_separator(json_encoder* enc, bool isTable)
{
    // called before each member/element (not the first one)
    enc_write(enc, ",");
    if (enc->pretty)
        enc_write(enc, "\n");
}

// ---- array ----

void pd_api_json_startArray(struct json_encoder* encoder)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_startArray\n");
    enc_write(encoder, "[");
    if (encoder->pretty) enc_write(encoder, "\n");
    encoder->depth++;
    encoder->startedArray = 1;
    printfDebug(DebugTraceFunctions, "pd_api_json_startArray end\n");
}

void pd_api_json_addArrayMember(struct json_encoder* encoder)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_addArrayMember\n");
    // separator between elements; the first element needs no comma
    // We track via startedArray: 1 = just opened (no elements yet)
    if (!encoder->startedArray)
        enc_write_separator(encoder, false);
    encoder->startedArray = 0;
    enc_write_indent(encoder);
    printfDebug(DebugTraceFunctions, "pd_api_json_addArrayMember end\n");
}

void pd_api_json_endArray(struct json_encoder* encoder)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_endArray\n");
    encoder->depth--;
    if (encoder->pretty) enc_write(encoder, "\n");
    enc_write_indent(encoder);
    enc_write(encoder, "]");
    encoder->startedArray = 0;
    printfDebug(DebugTraceFunctions, "pd_api_json_endArray end\n");
}

// ---- table / object ----

void pd_api_json_startTable(struct json_encoder* encoder)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_startTable\n");
    enc_write(encoder, "{");
    if (encoder->pretty) enc_write(encoder, "\n");
    encoder->depth++;
    encoder->startedTable = 1;
    printfDebug(DebugTraceFunctions, "pd_api_json_startTable end\n");
}

void pd_api_json_addTableMember(struct json_encoder* encoder, const char* name, int len)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_addTableMember\n");
    if (!encoder->startedTable)
        enc_write_separator(encoder, true);
    encoder->startedTable = 0;
    enc_write_indent(encoder);

    // write quoted key
    enc_write(encoder, "\"");
    // write exactly 'len' bytes of name (len<=0 means use strlen)
    int keylen = (len > 0) ? len : (int)strlen(name);
    if (encoder->writeStringFunc)
        encoder->writeStringFunc(encoder->userdata, name, keylen);
    enc_write(encoder, "\":");
    if (encoder->pretty) enc_write(encoder, " ");
    printfDebug(DebugTraceFunctions, "pd_api_json_addTableMember end\n");
}

void pd_api_json_endTable(struct json_encoder* encoder)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_endTable\n");
    encoder->depth--;
    if (encoder->pretty) enc_write(encoder, "\n");
    enc_write_indent(encoder);
    enc_write(encoder, "}");
    encoder->startedTable = 0;
    printfDebug(DebugTraceFunctions, "pd_api_json_endTable end\n");
}

// ---- scalar writers ----

void pd_api_json_writeNull(struct json_encoder* encoder)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_writeNull\n");
    enc_write(encoder, "null");
    printfDebug(DebugTraceFunctions, "pd_api_json_writeNull end\n");
}

void pd_api_json_writeFalse(struct json_encoder* encoder)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_writeFalse\n");
    enc_write(encoder, "false");
    printfDebug(DebugTraceFunctions, "pd_api_json_writeFalse end\n");
}

void pd_api_json_writeTrue(struct json_encoder* encoder)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_writeTrue\n");
    enc_write(encoder, "true");
    printfDebug(DebugTraceFunctions, "pd_api_json_writeTrue end\n");
}

void pd_api_json_writeInt(struct json_encoder* encoder, int num)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_writeInt\n");
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", num);
    enc_write(encoder, buf);
    printfDebug(DebugTraceFunctions, "pd_api_json_writeInt end\n");
}

void pd_api_json_writeDouble(struct json_encoder* encoder, double num)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_writeDouble\n");
    // Match Playdate simulator behaviour: NaN and Inf are written as null
    if (isnan(num) || isinf(num))
    {
        enc_write(encoder, "null");
    }
    else
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%g", num);
        enc_write(encoder, buf);
    }
    printfDebug(DebugTraceFunctions, "pd_api_json_writeDouble end\n");
}

void pd_api_json_writeString(struct json_encoder* encoder, const char* str, int len)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_writeString\n");
    // Match simulator: null pointer writes "" (empty string)
    if (!str)
    {
        enc_write(encoder, "\"\"");
        printfDebug(DebugTraceFunctions, "pd_api_json_writeString end\n");
        return;
    }
    enc_write(encoder, "\"");
    // escape the string content
    // len=0 means zero-length string (NOT use strlen — that's len<0 or len>0 explicitly)
    int n = len;
    if (n < 0) n = (int)strlen(str);
    for (int i = 0; i < n; i++)
    {
        char c = str[i];
        switch (c)
        {
            case '"':  enc_write(encoder, "\\\""); break;
            case '\\': enc_write(encoder, "\\\\"); break;
            case '\n': enc_write(encoder, "\\n");  break;
            case '\r': enc_write(encoder, "\\r");  break;
            case '\t': enc_write(encoder, "\\t");  break;
            default:
                if (encoder->writeStringFunc)
                    encoder->writeStringFunc(encoder->userdata, &c, 1);
                break;
        }
    }
    enc_write(encoder, "\"");
    printfDebug(DebugTraceFunctions, "pd_api_json_writeString end\n");
}

// ============================================================
//  initEncoder
// ============================================================

void pd_api_json_initEncoder(json_encoder* encoder, json_writeFunc* write, void* userdata, int pretty)
{
    printfDebug(DebugTraceFunctions, "pd_api_json_initEncoder\n");
    encoder->writeStringFunc = write;
    encoder->userdata        = userdata;
    encoder->pretty          = pretty;
    encoder->startedTable    = 0;
    encoder->startedArray    = 0;
    encoder->depth           = 0;

    encoder->startArray     = pd_api_json_startArray;
    encoder->addArrayMember = pd_api_json_addArrayMember;
    encoder->endArray       = pd_api_json_endArray;
    encoder->startTable     = pd_api_json_startTable;
    encoder->addTableMember = pd_api_json_addTableMember;
    encoder->endTable       = pd_api_json_endTable;
    encoder->writeNull      = pd_api_json_writeNull;
    encoder->writeFalse     = pd_api_json_writeFalse;
    encoder->writeTrue      = pd_api_json_writeTrue;
    encoder->writeInt       = pd_api_json_writeInt;
    encoder->writeDouble    = pd_api_json_writeDouble;
    encoder->writeString    = pd_api_json_writeString;
    printfDebug(DebugTraceFunctions, "pd_api_json_initEncoder end\n");
}

// ============================================================
//  Factory
// ============================================================

playdate_json* pd_api_json_Create_playdate_json()
{
    playdate_json* Tmp = (playdate_json*) malloc(sizeof(*Tmp));
    Tmp->initEncoder  = pd_api_json_initEncoder;
    Tmp->decode       = pd_api_json_decode;
    Tmp->decodeString = pd_api_json_decodeString;
    return Tmp;
}
