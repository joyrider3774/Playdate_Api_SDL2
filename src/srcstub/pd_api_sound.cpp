#include <SDL.h>
#include <SDL_mixer.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "pd_api/pd_api_sound.h"
#include "gamestubcallbacks.h"
#include "gamestub.h"
#include "defines.h"
#include "debug.h"

//AudioSample
struct SoundSource{

};
typedef struct SoundSource SoundSource;

struct SoundChannel{

};
typedef struct SoundChannel SoundChannel;


struct SoundEffect{

};
typedef struct SoundEffect SoundEffect;


struct Overdrive{

};
typedef struct Overdrive Overdrive;


struct DelayLine{

};
typedef struct DelayLine DelayLine;


struct DelayLineTap{

};
typedef struct DelayLineTap DelayLineTap;


struct RingModulator{

};
typedef struct RingModulator RingModulator;


struct BitCrusher{

};
typedef struct BitCrusher BitCrusher;


struct OnePoleFilter{

};
typedef struct OnePoleFilter OnePoleFilter;


struct TwoPoleFilter{

};
typedef struct TwoPoleFilter TwoPoleFilter;


struct SoundSequence{

};
typedef struct SoundSequence SoundSequence;


struct SequenceTrack{

};
typedef struct SequenceTrack SequenceTrack;


struct PDSynthInstrument{

};
typedef struct PDSynthInstrument PDSynthInstrument;


struct ControlSignal{

};
typedef struct ControlSignal ControlSignal;


struct PDSynth{

};
typedef struct PDSynth PDSynth;


struct  PDSynthSignalValue{

};
typedef struct PDSynthSignalValue PDSynthSignalValue;


struct PDSynthEnvelope{

};
typedef struct PDSynthEnvelope PDSynthEnvelope;


struct PDSynthLFO{

};
typedef struct PDSynthLFO PDSynthLFO;


struct PDSynthSignal {

};
typedef struct PDSynthSignal PDSynthSignal;


struct SampleChannelEntry {
    AudioSample* Sample;
    bool loaded;
};
typedef struct SampleChannelEntry SampleChannelEntry;

SampleChannelEntry** SampleList = NULL;
int SampleListCount = 0;

struct AudioSample {
    Mix_Chunk *sound;
    int channel;
    char *path;
};

AudioSample* pd_api_sound_newSampleBuffer(int byteCount)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_newSampleBuffer\n");
    AudioSample *Tmp = (AudioSample*) malloc(sizeof(*Tmp));
    Tmp->sound = NULL;
    Tmp->channel = -1;
    Tmp->path = NULL;
    printfDebug(DebugTraceFunctions, "pd_api_sound_newSampleBuffer END\n");
    return Tmp;
}



AudioSample* pd_api_sound_loadSample(const char* path)
{    
    printfDebug(DebugTraceFunctions, "pd_api_sound_loadSample\n");
    char ext[5];
    char* fullpath = (char *) malloc((strlen(path) + 17) * sizeof(char));
    bool needextension = true;
    struct stat lstats;
	if(strlen(path) > 4)
    {
        strcpy(ext, path + (strlen(path) - 4));
        needextension = (strcasecmp(ext, ".WAV") != 0) &&  (strcasecmp(ext, ".MP3") != 0);
    }

    if (needextension)
    {
        sprintf(fullpath,"./%s/%s.ogg", _pd_api_get_current_source_dir(), path);
        if(stat(fullpath, &lstats) != 0)
		{
			sprintf(fullpath,"./%s/%s.mp3", _pd_api_get_current_source_dir(), path);
        	if(stat(fullpath, &lstats) != 0)
			{
            	sprintf(fullpath,"./%s/%s.wav", _pd_api_get_current_source_dir(), path);
				if(stat(fullpath, &lstats) != 0)
				{
					sprintf(fullpath,"./%s.ogg", path);
					if(stat(fullpath, &lstats) != 0)
					{
						sprintf(fullpath,"./%s.mp3", path);
						if(stat(fullpath, &lstats) != 0)
							sprintf(fullpath,"./%s.wav", path);
					}
				}
			}
		}
    }
    else
	{
        sprintf(fullpath, "./%s/%s", _pd_api_get_current_source_dir(), path);
		if(stat(fullpath, &lstats) != 0)
			sprintf(fullpath, "./%s", path);
	}
    
    AudioSample* tmp = NULL;
    
    for (int i= 0; i < SampleListCount; i++)
    {
        if(SampleList[i]->loaded && (SampleList[i]->Sample != NULL))
        {
            if (SampleList[i]->Sample->path != NULL)
            {
                if (strcasecmp(path, SampleList[i]->Sample->path) == 0)
                {
                    tmp = SampleList[i]->Sample;
                    break;
                }
            }
        }
    }

    if (tmp == NULL)
    {
        tmp = pd_api_sound_newSampleBuffer(0);
        tmp->path = fullpath;
        tmp->sound = Mix_LoadWAV(tmp->path); 
        if(tmp->sound == NULL) 
        { 
            printfDebug(DebugInfo, "Unable to load Audio file \"%s\": %s\n", path, Mix_GetError());
        }
        
        bool found = false;
        int index = SampleListCount;
        for (int i= 0; i < SampleListCount; i++)
        {
            if(!SampleList[i]->loaded)
            {
                index = i;
                found = true;
                break;
            }
        }

        if(!found)
        {            
            if(SampleListCount == 0)
            {
                SampleList = (SampleChannelEntry **) malloc(sizeof(*SampleList));
            }
            else
            {
                SampleList = (SampleChannelEntry **)  realloc(SampleList, (SampleListCount + 1) * sizeof(*SampleList));
            }
            SampleListCount++;
            SampleList[index] = (SampleChannelEntry *)  malloc(sizeof(SampleChannelEntry));
            if (SampleListCount != Mix_AllocateChannels(SampleListCount))
            {
                printfDebug(DebugInfo, "Warning Mix_AllocateChannels(%d)\n failed!", SampleListCount);
            }; 
        }
        tmp->channel = index;
        SampleList[index]->Sample = tmp;
        SampleList[index]->loaded = true;
    }
    printfDebug(DebugTraceFunctions, "pd_api_sound_loadSample end\n");
    return tmp;
}

int pd_api_sound_loadIntoSample(AudioSample* sample, const char* path)
{
    sample = pd_api_sound_loadSample(path);
    return (sample->sound != NULL);
}

AudioSample* pd_api_sound_newSampleFromData(uint8_t* data, SoundFormat format, uint32_t sampleRate, int byteCount)
{
    return pd_api_sound_newSampleBuffer(0);
}

void pd_api_sound_getDataSample(AudioSample* sample, uint8_t** data, SoundFormat* format, uint32_t* sampleRate, uint32_t* bytelength)
{
    *data = NULL;
    *format = kSoundADPCMStereo;
    *sampleRate = 44000;
    *bytelength = 0;
}

void pd_api_sound_freeSample(AudioSample* sample)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_freeSample\n");
    if(sample == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_freeSample end (sample= NULL)\n");
        return;
    }

    if(sample->sound)
        Mix_FreeChunk(sample->sound);

    if(sample->channel > -1)
    {
        if(SampleList[sample->channel]->loaded)
        {
            SampleList[sample->channel]->loaded = false;
            SampleList[sample->channel]->Sample = NULL;
        }
    }

    if(sample->path)
        free(sample->path);
    
    free(sample);
    printfDebug(DebugTraceFunctions, "pd_api_sound_freeSample end\n");
}

float pd_api_sound_getLengthSample(AudioSample* sample)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_getLengthSample\n");

    if(sample == NULL)
        return 0.0f;
    if(sample->sound == NULL)
        return 0.0f;
    Uint32 points = 0;
    Uint32 frames = 0;
    int freq = 0;
    Uint16 fmt = 0;
    int chans = 0;
    if (Mix_QuerySpec(&freq, &fmt, &chans) == 0)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_getLengthSample END Mix_QuerySpec failed\n");
        return 0.0f; 
    }

    /* bytes / samplesize == sample points */
    points = (sample->sound->alen / ((fmt & 0xFF) / 8));

    /* sample points / channels == sample frames */
    frames = (points / chans);

    /* (sample frames * 1000) / frequency == play length in ms */
    printfDebug(DebugTraceFunctions, "pd_api_sound_getLengthSample END\n");
    return ((frames * 1000000.f) / freq);
}

playdate_sound_sample* pd_api_sound_Create_playdate_sound_soundsample(void)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_soundsample\n");
    playdate_sound_sample *Tmp = (playdate_sound_sample*) malloc(sizeof(*Tmp));
    Tmp->newSampleBuffer = pd_api_sound_newSampleBuffer;
    Tmp->loadIntoSample = pd_api_sound_loadIntoSample;
    Tmp->load = pd_api_sound_loadSample;
    Tmp->newSampleFromData = pd_api_sound_newSampleFromData;
    Tmp->getData = pd_api_sound_getDataSample;
    Tmp->freeSample = pd_api_sound_freeSample;
    Tmp->getLength = pd_api_sound_getLengthSample;
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_soundsample end\n");
    return Tmp;
}

//Sample Player
struct SamplePlayer {
    AudioSample* sample;
    float VolumeL;
    float VolumeR;
    float Offset;
    int Rate;
    int Start;
    int End;
};

SamplePlayer* pd_api_sound_newSamplePlayer(void)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_newSamplePlayer\n");
    SamplePlayer *Tmp = (SamplePlayer*) malloc(sizeof(*Tmp));
    Tmp->sample = NULL;
    Tmp->VolumeL = 1.0f;
    Tmp->VolumeR = 1.0f;
    Tmp->Offset = 0.0f;
    Tmp->Rate = 0;
    Tmp->Start = 0;
    Tmp->End = 0;
    printfDebug(DebugTraceFunctions, "pd_api_sound_newSamplePlayer end\n");
    return Tmp;
}

void pd_api_sound_freeSamplePlayer(SamplePlayer* player)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_freeSamplePlayer\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_freeSamplePlayer END (player == NULL)\n");
        return;
    }
    free(player);
    printfDebug(DebugTraceFunctions, "pd_api_sound_freeSamplePlayer end\n");
}

void pd_api_sound_setSampleSamplePlayer(SamplePlayer* player, AudioSample* sample)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_setSampleSamplePlayer\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_setSampleSamplePlayer (player == NULL)\n");
        return;
    }
    player->sample = sample;
    printfDebug(DebugTraceFunctions, "pd_api_sound_setSampleSamplePlayer End\n");
}

int pd_api_sound_playSamplePlayer(SamplePlayer* player, int repeat, float rate)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_playSamplePlayer\n");
    if (player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_playSamplePlayer end (player == NULL)\n");
        return 0;
    }

    if(player->sample == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_playSamplePlayer end (player->sample == NULL)\n");
        return 0;
    }
    if(player->sample->sound == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_playSamplePlayer end (player->sample->sound == NULL)\n");
        return 0;
    }

    int channel = Mix_PlayChannel(player->sample->channel, player->sample->sound, repeat-1);
    
    printfDebug(DebugTraceFunctions, "pd_api_sound_playSamplePlayer end\n");
    if(channel > -1)
        return 0;
    else
        return -1;
}

int pd_api_sound_isPlayingSamplePlayer(SamplePlayer* player)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_isPlayingSamplePlayer\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_isPlayingSamplePlayer end (player == NULL)\n");
        return 0;
    }

    if(player->sample == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_isPlayingSamplePlayer end (player->sample == NULL)\n");
        return 0;
    }

    if(player->sample->channel > -1)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_isPlayingSamplePlayer end\n");
        if(Mix_Playing(player->sample->channel))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    printfDebug(DebugTraceFunctions, "pd_api_sound_isPlayingSamplePlayer end\n");
    return 0;
}

void pd_api_sound_stopSamplePlayer(SamplePlayer* player)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_stopSamplePlayer\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_stopSamplePlayer end player == NULL\n");
        return;
    }

    if(player->sample == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_stopSamplePlayer end player->sample == NULL\n");
        return;
    }

    if(player->sample->channel > -1)
    {
        Mix_HaltChannel(player->sample->channel);
    }
    printfDebug(DebugTraceFunctions, "pd_api_sound_stopSamplePlayer end\n");
}

void pd_api_sound_setVolumeSamplePlayer(SamplePlayer* player, float left, float right)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_setVolumeSamplePlayer\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_setVolumeSamplePlayer end player == NULL\n");
        return;
    }

    if(player->sample == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_setVolumeSamplePlayer end player->sample == NULL\n");
        return;
    }

    if(player->sample->sound == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_setVolumeSamplePlayer end player->sample->sound == NULL\n");
        return;
    }


    if(player->sample->channel > -1 )
        Mix_Volume(player->sample->channel, MIX_MAX_VOLUME * left);

    printfDebug(DebugTraceFunctions, "pd_api_sound_setVolumeSamplePlayer end\n");
}

void pd_api_sound_getVolumeSamplePlayer(SamplePlayer* player, float* left, float* right)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_getVolumeSamplePlayer\n");
    *left = 0.0f;
    *right = 0.0f;
    if(player)
    {
        *left = player->VolumeL;
        *right = player->VolumeR;
    }
    printfDebug(DebugTraceFunctions, "pd_api_sound_getVolumeSamplePlayer end\n");
}

float pd_api_sound_getLengthSamplePlayer(SamplePlayer* player)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_getLengthSamplePlayer\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_getLengthSamplePlayer end player == NULL\n");
        return 0.0f;
    }
    
    printfDebug(DebugTraceFunctions, "pd_api_sound_getLengthSamplePlayer end\n");
    return pd_api_sound_getLengthSample(player->sample);
}

void pd_api_sound_setOffsetSamplePlayer(SamplePlayer* player, float offset)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_setOffsetSamplePlayer\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions,"pd_api_sound_setOffsetSamplePlayer end player == NULL\n");
        return;
    }
    player->Offset = offset;
    printfDebug(DebugTraceFunctions, "pd_api_sound_setOffsetSamplePlayer end\n");
}

void pd_api_sound_setRateSamplePlayer(SamplePlayer* player, float rate)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_setRateSamplePlayer\n");
    if(player == NULL)
    {
       printfDebug(DebugTraceFunctions, "pd_api_sound_setRateSamplePlayer end player == NULL\n");
        return;
    }
    player->Rate = rate;
    printfDebug(DebugTraceFunctions, "pd_api_sound_setRateSamplePlayer end\n");
}

void pd_api_sound_setPlayRangeSamplePlayer(SamplePlayer* player, int start, int end)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_setPlayRangeSamplePlayer\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_setPlayRangeSamplePlayer end player == NULL\n");
        return;
    }
    player->Start = start;
    player->End = end;
    printfDebug(DebugTraceFunctions, "pd_api_sound_setPlayRangeSamplePlayer end\n");
}

void pd_api_sound_setFinishCallbackSamplePlayer(SamplePlayer* player, sndCallbackProc callback)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_setFinishCallbackSamplePlayer\n");
    printfDebug(DebugTraceFunctions, "pd_api_sound_setFinishCallbackSamplePlayer end\n");
}

void pd_api_sound_setLoopCallbackSamplePlayer(SamplePlayer* player, sndCallbackProc callback)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_setLoopCallbackSamplePlayer\n");
    printfDebug(DebugTraceFunctions, "pd_api_sound_setLoopCallbackSamplePlayer end\n");
}

float pd_api_sound_getOffsetSamplePlayer(SamplePlayer* player)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_getOffsetSamplePlayer\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_getOffsetSamplePlayer end player == NULL\n");
        return 0.0f;
    }
    printfDebug(DebugTraceFunctions, "pd_api_sound_getOffsetSamplePlayer end\n");
    return player->Offset;
}

float pd_api_sound_getRateSamplePlayer(SamplePlayer* player)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_getRateSamplePlayer\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_getRateSamplePlayer end player == NULL\n");
        return 0.0f;
    }
    printfDebug(DebugTraceFunctions, "pd_api_sound_getRateSamplePlayer end\n");
    return player->Rate;
}

void pd_api_sound_setPausedSamplePlayer(SamplePlayer* player, int flag)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_getRateSamplePlayer\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_setPausedSamplePlayer end player == NULL\n");
        return;
    }

    if (player->sample == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_setPausedSamplePlayer end player->sample == NULL\n");
        return;
    }

    if(player->sample->channel > -1)
    {
        if(flag == 1)
        {
            Mix_Pause(player->sample->channel);
        }
        else
        {
            Mix_Resume(player->sample->channel);
        }
    }
    printfDebug(DebugTraceFunctions, "pd_api_sound_setPausedSamplePlayer end\n");
}


playdate_sound_sampleplayer* pd_api_sound_Create_playdate_sound_sampleplayer(void)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_sampleplayer\n");
    playdate_sound_sampleplayer *Tmp = (playdate_sound_sampleplayer*) malloc(sizeof(*Tmp));
    Tmp->newPlayer = pd_api_sound_newSamplePlayer;
    Tmp->freePlayer = pd_api_sound_freeSamplePlayer;
    Tmp->setSample = pd_api_sound_setSampleSamplePlayer;
    Tmp->play = pd_api_sound_playSamplePlayer;
    Tmp->isPlaying = pd_api_sound_isPlayingSamplePlayer;
    Tmp->stop = pd_api_sound_stopSamplePlayer;
    Tmp->setVolume = pd_api_sound_setVolumeSamplePlayer;
    Tmp->getVolume = pd_api_sound_getVolumeSamplePlayer;
    Tmp->getLength = pd_api_sound_getLengthSamplePlayer;
    Tmp->setOffset = pd_api_sound_setOffsetSamplePlayer;
    Tmp->setRate = pd_api_sound_setRateSamplePlayer;
    Tmp->setPlayRange = pd_api_sound_setPlayRangeSamplePlayer;
    Tmp->setFinishCallback = pd_api_sound_setFinishCallbackSamplePlayer;
    Tmp->setLoopCallback = pd_api_sound_setLoopCallbackSamplePlayer;
    Tmp->getOffset = pd_api_sound_getOffsetSamplePlayer;
    Tmp->getRate = pd_api_sound_getRateSamplePlayer;
    Tmp->setPaused = pd_api_sound_setPausedSamplePlayer;
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_sampleplayer end\n");
    return Tmp;
}

//playdate_sound_fileplayer
struct FilePlayer {
    SamplePlayer* Player;
};

FilePlayer* pd_api_sound_newFilePlayer(void)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_newFilePlayer\n");
    FilePlayer *Tmp = (FilePlayer*) malloc(sizeof(*Tmp));
    Tmp->Player = pd_api_sound_newSamplePlayer();
    printfDebug(DebugTraceFunctions, "pd_api_sound_newFilePlayer end\n");
    return Tmp;
}

void pd_api_sound_freeFilePlayer(FilePlayer* player)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_freeFilePlayer\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_freeFilePlayer end player = NULL\n");
        return;
    }
    if(player->Player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_freeFilePlayer end player->Player = NULL\n");
        return;
    }
    pd_api_sound_freeSample(player->Player->sample);
    pd_api_sound_freeSamplePlayer(player->Player);
    free(player);
    player = NULL;
    printfDebug(DebugTraceFunctions, "pd_api_sound_freeFilePlayer end\n");
}


int pd_api_sound_loadIntoFilePlayer(FilePlayer* player, const char* path)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_loadIntoFilePlayer\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_loadIntoFilePlayer end player = NULL\n");
        return 0;
    }
    if(player->Player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_loadIntoFilePlayer end player->Player = NULL\n");
        return 0;
    }
    pd_api_sound_freeSample(player->Player->sample);
    AudioSample* sample = pd_api_sound_loadSample(path);
    if (sample)
    {
        pd_api_sound_setSampleSamplePlayer(player->Player, sample);
        printfDebug(DebugTraceFunctions, "pd_api_sound_loadIntoFilePlayer end\n");
        return 1;
    }
    return 0;
}

void pd_api_sound_FilePlayersetBufferLength(FilePlayer* player, float bufferLen)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetBufferLength\n");
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetBufferLength end\n");
}

int pd_api_sound_FilePlayerplay(FilePlayer* player, int repeat)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerplay\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerplay end player = NULL\n");
        return -1;
    }
    if(player->Player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerplay end player->Player = NULL\n");
        return -1;
    }
    int tmp = pd_api_sound_playSamplePlayer(player->Player, repeat, 1.0f);
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerplay end\n");
    return tmp;
}

int pd_api_sound_FilePlayerisPlaying(FilePlayer* player)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerisPlaying\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerisPlaying end player = NULL\n");
        return -1;
    }
    if(player->Player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerisPlaying end player->Player = NULL\n");
        return -1;
    }
    int tmp = pd_api_sound_isPlayingSamplePlayer(player->Player);
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerisPlaying end\n");
    return tmp;
}

void pd_api_sound_FilePlayerpause(FilePlayer* player)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerpause\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerpause end player = NULL\n");
        return;
    }
    if(player->Player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerpause end player->Player = NULL\n");
        return;
    }
    pd_api_sound_setPausedSamplePlayer(player->Player, 1);
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerpause end\n");
}

void pd_api_sound_FilePlayerstop(FilePlayer* player)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerstop\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerstop end player = NULL\n");
        return;
    }
    if(player->Player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerstop end player->Player = NULL\n");
        return;
    }
    pd_api_sound_stopSamplePlayer(player->Player);
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerstop end\n");
}

void pd_api_sound_FilePlayersetVolume(FilePlayer* player, float left, float right)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetVolume\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetVolume end player = NULL\n");
        return;
    }
    if(player->Player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetVolume end player->Player = NULL\n");
        return;
    }
    pd_api_sound_setVolumeSamplePlayer(player->Player, left, right);
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetVolume end\n");
}

void pd_api_sound_FilePlayergetVolume(FilePlayer* player, float* left, float* right)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetVolume\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetVolume end player = NULL\n");
        return;
    }
    if(player->Player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetVolume end player->Player = NULL\n");
        return;
    }
    pd_api_sound_getVolumeSamplePlayer(player->Player, left, right);
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetVolume end\n");
}

float pd_api_sound_FilePlayergetLength(FilePlayer* player)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetLength\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetLength end player = NULL\n");
        return 0.0f;
    }
    if(player->Player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetLength end player->Player = NULL\n");
        return 0.0f;
    }
    float tmp = pd_api_sound_getLengthSamplePlayer(player->Player);
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetLength end\n");
    return tmp;

}

void pd_api_sound_FilePlayersetOffset(FilePlayer* player, float offset)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetOffset\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetOffset end player = NULL\n");
        return;
    }
    if(player->Player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetOffset end player->Player = NULL\n");
        return;
    }
    pd_api_sound_setOffsetSamplePlayer(player->Player, offset);
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetOffset end\n");
}

void pd_api_sound_FilePlayersetRate(FilePlayer* player, float rate)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetRate\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetRate end player = NULL\n");
        return;
    }
    if(player->Player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetRate end player->Player = NULL\n");
        return;
    }
    pd_api_sound_setRateSamplePlayer(player->Player, rate);
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetRate end\n");
}

void pd_api_sound_FilePlayersetLoopRange(FilePlayer* player, float start, float end)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetLoopRange\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetLoopRange end player = NULL\n");
        return;
    }
    if(player->Player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetLoopRange end player->Player = NULL\n");
        return;
    }
    pd_api_sound_setPlayRangeSamplePlayer(player->Player, start, end);
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetLoopRange end\n");
}

int pd_api_sound_FilePlayerdidUnderrun(FilePlayer* player)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerdidUnderrun\n");
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerdidUnderrun end\n");
    return 0;
}

void pd_api_sound_FilePlayersetFinishCallback(FilePlayer* player, sndCallbackProc callback)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetFinishCallback\n");
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetFinishCallback end\n");
}

void pd_api_sound_FilePlayersetLoopCallback(FilePlayer* player, sndCallbackProc callback)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetLoopCallback\n");
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetLoopCallback end\n");
}

float pd_api_sound_FilePlayergetOffset(FilePlayer* player)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetOffset\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetOffset end player = NULL\n");
        return 0.0f;
    }
    if(player->Player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetOffset end player->Player = NULL\n");
        return 0.0f;
    }
    float tmp = pd_api_sound_getOffsetSamplePlayer(player->Player);
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetOffset end\n");
    return tmp;
}

float pd_api_sound_FilePlayergetRate(FilePlayer* player)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetRate\n");
    if(player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetRate end player = NULL\n");
        return 0.0f;
    }
    if(player->Player == NULL)
    {
        printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetRate end player->Player = NULL\n");
        return 0.0f;
    }
    float tmp = pd_api_sound_getRateSamplePlayer(player->Player);
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayergetRate end\n");
    return tmp;
}

void pd_api_sound_FilePlayersetStopOnUnderrun(FilePlayer* player, int flag)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetStopOnUnderrun\n");
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetStopOnUnderrun end\n");
}

void pd_api_sound_FilePlayerfadeVolume(FilePlayer* player, float left, float right, int32_t len, sndCallbackProc finishCallback)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerfadeVolume\n");
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayerfadeVolume end\n");
}

void pd_api_sound_FilePlayersetMP3StreamSource(FilePlayer* player, int (*dataSource)(uint8_t* data, int bytes, void* userdata), void* userdata, float bufferLen)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetMP3StreamSource\n");
    printfDebug(DebugTraceFunctions, "pd_api_sound_FilePlayersetMP3StreamSource end\n");
}

playdate_sound_fileplayer* pd_api_sound_Create_playdate_sound_fileplayer()
{
    printfDebug(DebugTraceFunctions,"pd_api_sound_Create_playdate_sound_fileplayer\n");
    playdate_sound_fileplayer *Tmp = (playdate_sound_fileplayer*) malloc(sizeof(*Tmp));
    Tmp->newPlayer = pd_api_sound_newFilePlayer;
	Tmp->freePlayer = pd_api_sound_freeFilePlayer;
	Tmp->loadIntoPlayer = pd_api_sound_loadIntoFilePlayer;
	Tmp->setBufferLength = pd_api_sound_FilePlayersetBufferLength;
	Tmp->play = pd_api_sound_FilePlayerplay;
	Tmp->isPlaying = pd_api_sound_FilePlayerisPlaying;
	Tmp->pause = pd_api_sound_FilePlayerpause;
	Tmp->stop = pd_api_sound_FilePlayerstop;
	Tmp->setVolume = pd_api_sound_FilePlayersetVolume;
	Tmp->getVolume = pd_api_sound_FilePlayergetVolume;
	Tmp->getLength = pd_api_sound_FilePlayergetLength;
	Tmp->setOffset = pd_api_sound_FilePlayersetOffset;
	Tmp->setRate = pd_api_sound_FilePlayersetRate;
	Tmp->setLoopRange = pd_api_sound_FilePlayersetLoopRange;
	Tmp->didUnderrun = pd_api_sound_FilePlayerdidUnderrun;
	Tmp->setFinishCallback = pd_api_sound_FilePlayersetFinishCallback;
	Tmp->setLoopCallback = pd_api_sound_FilePlayersetLoopCallback;
	Tmp->getOffset = pd_api_sound_FilePlayergetOffset;
	Tmp->getRate = pd_api_sound_FilePlayergetRate;
	Tmp->setStopOnUnderrun = pd_api_sound_FilePlayersetStopOnUnderrun;
	Tmp->fadeVolume = pd_api_sound_FilePlayerfadeVolume;
	Tmp->setMP3StreamSource = pd_api_sound_FilePlayersetMP3StreamSource;
    printfDebug(DebugTraceFunctions,"pd_api_sound_Create_playdate_sound_fileplayer end\n");
    return Tmp;
}


//Playdate Sound

uint32_t pd_api_sound_getCurrentTime(void)
{
    return 0;
}

SoundSource* pd_api_sound_addSource(AudioSourceFunction* callback, void* context, int stereo)
{
    SoundSource *Tmp = (SoundSource*) malloc(sizeof(*Tmp)); 
    return Tmp;  
}

SoundChannel* pd_api_sound_getDefaultChannel(void)
{
    return Api->sound->channel->newChannel();
}
	
int pd_api_sound_addChannel(SoundChannel* channel)
{
	return 0;
}

int pd_api_sound_removeChannel(SoundChannel* channel)
{
	return 0;
}
	
void pd_api_sound_setMicCallback(RecordCallback* callback, void* context, int forceInternal)
{

}

void pd_api_sound_getHeadphoneState(int* headphone, int* headsetmic, void (*changeCallback)(int headphone, int mic))
{

}

void pd_api_sound_setOutputsActive(int headphone, int speaker)
{

}
	
// 1.5
int pd_api_sound_removeSource(SoundSource* source)
{
	return 0; 
}
	
void _pd_api_sound_freeSampleList()
{
    for (int i = 0; i < SampleListCount; i++)
    {
        if(SampleList[i]->loaded)
        {
            pd_api_sound_freeSample(SampleList[i]->Sample);
            SampleList[i]->loaded = false;
        }
    }
}

void pd_api_sound_setVolumeSoundSource(SoundSource* c, float lvol, float rvol)
{

}

void pd_api_sound_getVolumeSoundSource(SoundSource* c, float* outl, float* outr)
{

}

int pd_api_sound_isPlayingSoundSource(SoundSource* c)
{
    return 0;
}

void pd_api_sound_setFinishCallbackSoundSource(SoundSource* c, sndCallbackProc callback)
{

}

playdate_sound_source* pd_api_sound_Create_playdate_sound_source()
{
    printfDebug(DebugTraceFunctions, "_pd_api_sound_Create_playdate_sound_source\n");
    playdate_sound_source *Tmp = (playdate_sound_source*) malloc(sizeof(*Tmp));

	Tmp->setVolume = pd_api_sound_setVolumeSoundSource;
	Tmp->getVolume = pd_api_sound_getVolumeSoundSource;
	Tmp->isPlaying = pd_api_sound_isPlayingSoundSource;
	Tmp->setFinishCallback = pd_api_sound_setFinishCallbackSoundSource;
    printfDebug(DebugTraceFunctions, "_pd_api_sound_Create_playdate_sound_source end\n");
    return Tmp;
};

PDSynthSignal* pd_api_sound_newSignalPDSynthSignal(signalStepFunc step, signalNoteOnFunc noteOn, signalNoteOffFunc noteOff, signalDeallocFunc dealloc, void* userdata)
{
    PDSynthSignal *Tmp = (PDSynthSignal*) malloc(sizeof(*Tmp));
    return Tmp;
}

void pd_api_sound_freeSignalPDSynthSignal(PDSynthSignal* signal)
{
    if(signal)
        free(signal);
}

float pd_api_sound_getValuePDSynthSignal(PDSynthSignal* signal)
{
    return 0.0f;
}

void pd_api_sound_setValueScalePDSynthSignal(PDSynthSignal* signal, float scale)
{

}

void pd_api_sound_setValueOffsetPDSynthSignal(PDSynthSignal* signal, float offset)
{

}

playdate_sound_signal* pd_api_sound_Create_playdate_sound_signal()
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_signal\n");
    playdate_sound_signal *Tmp = (playdate_sound_signal*) malloc(sizeof(*Tmp));
	Tmp->newSignal = pd_api_sound_newSignalPDSynthSignal;
	Tmp->freeSignal = pd_api_sound_freeSignalPDSynthSignal;
	Tmp->getValue = pd_api_sound_getValuePDSynthSignal;
	Tmp->setValueScale = pd_api_sound_setValueScalePDSynthSignal;
	Tmp->setValueOffset = pd_api_sound_setValueOffsetPDSynthSignal;
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_signal end\n");
    return Tmp;
};

PDSynthLFO* pd_api_sound_newLFOPDSynthLFO(LFOType type)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound\n");
    PDSynthLFO *Tmp = (PDSynthLFO*) malloc(sizeof(*Tmp));
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound end\n");
    return Tmp;
}

void pd_api_sound_freeLFOPDSynthLFO(PDSynthLFO* lfo)
{
    if(lfo)
        free(lfo);
}

void pd_api_sound_setTypePDSynthLFO(PDSynthLFO* lfo, LFOType type)
{

}

void pd_api_sound_setRatePDSynthLFO(PDSynthLFO* lfo, float rate)
{

}

void pd_api_sound_setPhasePDSynthLFO(PDSynthLFO* lfo, float phase)
{

}

void pd_api_sound_setCenterPDSynthLFO(PDSynthLFO* lfo, float center)
{

}

void pd_api_sound_setDepthPDSynthLFO(PDSynthLFO* lfo, float depth)
{

}

void pd_api_sound_setArpeggiationPDSynthLFO(PDSynthLFO* lfo, int nSteps, float* steps)
{

}

void pd_api_sound_setFunctionPDSynthLFO(PDSynthLFO* lfo, float (*lfoFunc)(PDSynthLFO* lfo, void* userdata), void* userdata, int interpolate)
{

}

void pd_api_sound_setDelayPDSynthLFO(PDSynthLFO* lfo, float holdoff, float ramptime)
{

}

void pd_api_sound_setRetriggerPDSynthLFO(PDSynthLFO* lfo, int flag)
{

}

float pd_api_sound_getValuePDSynthLFO(PDSynthLFO* lfo)
{
    return 0.0f;
}

// 1.10
void pd_api_sound_setGlobalPDSynthLFO(PDSynthLFO* lfo, int global)
{

}


playdate_sound_lfo* pd_api_sound_Create_playdate_sound_lfo()
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_lfo\n");
    playdate_sound_lfo *Tmp = (playdate_sound_lfo*) malloc(sizeof(*Tmp));
	Tmp->newLFO = pd_api_sound_newLFOPDSynthLFO;
	Tmp->freeLFO = pd_api_sound_freeLFOPDSynthLFO;
	Tmp->setType = pd_api_sound_setTypePDSynthLFO;
	Tmp->setRate =pd_api_sound_setRatePDSynthLFO;
	Tmp->setPhase = pd_api_sound_setPhasePDSynthLFO;
	Tmp->setCenter = pd_api_sound_setCenterPDSynthLFO;
	Tmp->setDepth = pd_api_sound_setDepthPDSynthLFO;
	Tmp->setArpeggiation = pd_api_sound_setArpeggiationPDSynthLFO;
	Tmp->setFunction = pd_api_sound_setFunctionPDSynthLFO;
	Tmp->setDelay = pd_api_sound_setDelayPDSynthLFO;
	Tmp->setRetrigger = pd_api_sound_setRetriggerPDSynthLFO;
	Tmp->getValue = pd_api_sound_getValuePDSynthLFO;
	// 1.10
	Tmp->setGlobal = pd_api_sound_setGlobalPDSynthLFO;
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_lfo end\n");
    return Tmp;
};


PDSynthEnvelope* pd_api_sound_newEnvelopePDSynthEnvelope(float attack, float decay, float sustain, float release)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_newEnvelopePDSynthEnvelope\n");
    PDSynthEnvelope *Tmp = (PDSynthEnvelope*) malloc(sizeof(*Tmp));
    printfDebug(DebugTraceFunctions, "pd_api_sound_newEnvelopePDSynthEnvelope end\n");
    return Tmp;
}

void pd_api_sound_freeEnvelopePDSynthEnvelope(PDSynthEnvelope* env)
{
    if (env)
        free(env);
}

void pd_api_sound_setAttackPDSynthEnvelope(PDSynthEnvelope* env, float attack)
{

}

void pd_api_sound_setDecayPDSynthEnvelope(PDSynthEnvelope* env, float decay)
{

}

void pd_api_sound_setSustainPDSynthEnvelope(PDSynthEnvelope* env, float sustain)
{

}

void pd_api_sound_setReleasePDSynthEnvelope(PDSynthEnvelope* env, float release)
{

}


void pd_api_sound_setLegatoPDSynthEnvelope(PDSynthEnvelope* env, int flag)
{

}

void pd_api_sound_setRetriggerPDSynthEnvelope(PDSynthEnvelope* lfo, int flag)
{

}


float pd_api_sound_getValuePDSynthEnvelope(PDSynthEnvelope* env)
{
    return 0.0f;
}

playdate_sound_envelope* pd_api_sound_Create_playdate_sound_envelope()
{
    printfDebug(DebugTraceFunctions,"pd_api_sound_Create_playdate_sound_envelope\n");
    playdate_sound_envelope *Tmp = (playdate_sound_envelope*) malloc(sizeof(*Tmp));

	Tmp->newEnvelope = pd_api_sound_newEnvelopePDSynthEnvelope;
	Tmp->freeEnvelope = pd_api_sound_freeEnvelopePDSynthEnvelope;
	Tmp->setAttack = pd_api_sound_setAttackPDSynthEnvelope;
	Tmp->setDecay = pd_api_sound_setDecayPDSynthEnvelope;
	Tmp->setSustain = pd_api_sound_setSustainPDSynthEnvelope;
	Tmp->setRelease = pd_api_sound_setReleasePDSynthEnvelope;
	Tmp->setLegato = pd_api_sound_setLegatoPDSynthEnvelope;
	Tmp->setRetrigger = pd_api_sound_setRetriggerPDSynthEnvelope;
    Tmp->getValue = pd_api_sound_getValuePDSynthEnvelope;
    printfDebug(DebugTraceFunctions,"pd_api_sound_Create_playdate_sound_envelope end\n");
    return Tmp;
};


PDSynth* pd_api_sound_newSynthPDSynth(void)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_newSynthPDSynth\n");
    PDSynth *Tmp = (PDSynth*) malloc(sizeof(*Tmp));
    printfDebug(DebugTraceFunctions, "pd_api_sound_newSynthPDSynth end\n");
    return Tmp;
}

void pd_api_sound_freeSynthPDSynth(PDSynth* synth)
{
    if(synth)
        free(synth);
}

void pd_api_sound_setWaveformPDSynth(PDSynth* synth, SoundWaveform wave)
{

}

void pd_api_sound_setGeneratorPDSynth(PDSynth* synth, int stereo, synthRenderFunc render, synthNoteOnFunc noteOn, synthReleaseFunc release, synthSetParameterFunc setparam, synthDeallocFunc dealloc, void* userdata)
{

}

void pd_api_sound_setSamplePDSynth(PDSynth* synth, AudioSample* sample, uint32_t sustainStart, uint32_t sustainEnd)
{

}

void pd_api_sound_setAttackTimePDSynth(PDSynth* synth, float attack)

{

}

void pd_api_sound_setDecayTimePDSynth(PDSynth* synth, float decay)
{

}

void pd_api_sound_setSustainLevelPDSynth(PDSynth* synth, float sustain)
{

}

void pd_api_sound_setReleaseTimePDSynth(PDSynth* synth, float release)
{

}

void pd_api_sound_setTransposePDSynth(PDSynth* synth, float halfSteps)
{

}

void pd_api_sound_setFrequencyModulatorPDSynth(PDSynth* synth, PDSynthSignalValue* mod)
{

}

PDSynthSignalValue* pd_api_sound_getFrequencyModulatorPDSynth(PDSynth* synth)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}

void pd_api_sound_setAmplitudeModulatorPDSynth(PDSynth* synth, PDSynthSignalValue* mod)
{

}

PDSynthSignalValue* pd_api_sound_getAmplitudeModulatorPDSynth(PDSynth* synth)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}

int pd_api_sound_getParameterCountPDSynth(PDSynth* synth)
{
    return 0;
}

int pd_api_sound_setParameterPDSynth(PDSynth* synth, int parameter, float value)
{
    return 0;
}

void pd_api_sound_setParameterModulatorPDSynth(PDSynth* synth, int parameter, PDSynthSignalValue* mod)
{

}

PDSynthSignalValue* pd_api_sound_getParameterModulatorPDSynth(PDSynth* synth, int parameter)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}

void pd_api_sound_playNotePDSynth(PDSynth* synth, float freq, float vel, float len, uint32_t when) // len == -1 for indefinite
{

}

void pd_api_sound_playMIDINotePDSynth(PDSynth* synth, MIDINote note, float vel, float len, uint32_t when) // len == -1 for indefinite
{

}

void pd_api_sound_noteOffPDSynth(PDSynth* synth, uint32_t when) // move to release part of envelope
{

}

void pd_api_sound_stopPDSynth(PDSynth* synth) // stop immediately
{

}

void pd_api_sound_setVolumePDSynth(PDSynth* synth, float left, float right)
{

}

void pd_api_sound_getVolumePDSynth(PDSynth* synth, float* left, float* right)
{

}

int pd_api_sound_isPlayingPDSynth(PDSynth* synth)
{
    return 0;
}

PDSynthEnvelope* pd_api_sound_getEnvelopePDSynth(PDSynth* synth) // synth keeps ownership--don't free this!
{
	PDSynthEnvelope* Tmp = (PDSynthEnvelope*) malloc(sizeof(*Tmp));
	return Tmp;
}

// PDSynth extends SoundSource
playdate_sound_synth* pd_api_sound_Create_playdate_sound_synth()
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_synth\n");
    playdate_sound_synth *Tmp = (playdate_sound_synth*) malloc(sizeof(*Tmp));	
    Tmp->newSynth = pd_api_sound_newSynthPDSynth;
	Tmp->freeSynth = pd_api_sound_freeSynthPDSynth;
	Tmp->setWaveform = pd_api_sound_setWaveformPDSynth;
	Tmp->setGenerator = pd_api_sound_setGeneratorPDSynth;
	Tmp->setSample = pd_api_sound_setSamplePDSynth;
    Tmp->setAttackTime = pd_api_sound_setAttackTimePDSynth;
	Tmp->setDecayTime = pd_api_sound_setDecayTimePDSynth;
	Tmp->setSustainLevel = pd_api_sound_setSustainLevelPDSynth;
	Tmp->setReleaseTime = pd_api_sound_setReleaseTimePDSynth;
	Tmp->setTranspose = pd_api_sound_setTransposePDSynth;
	Tmp->setFrequencyModulator = pd_api_sound_setFrequencyModulatorPDSynth;
	Tmp->getFrequencyModulator = pd_api_sound_getFrequencyModulatorPDSynth;
	Tmp->setAmplitudeModulator = pd_api_sound_setAmplitudeModulatorPDSynth;
	Tmp->getAmplitudeModulator = pd_api_sound_getAmplitudeModulatorPDSynth;
	Tmp->getParameterCount = pd_api_sound_getParameterCountPDSynth;
	Tmp->setParameter = pd_api_sound_setParameterPDSynth;
	Tmp->setParameterModulator = pd_api_sound_setParameterModulatorPDSynth;
	Tmp->getParameterModulator = pd_api_sound_getParameterModulatorPDSynth;
	Tmp->playNote = pd_api_sound_playNotePDSynth;
	Tmp->playMIDINote = pd_api_sound_playMIDINotePDSynth;
	Tmp->noteOff = pd_api_sound_noteOffPDSynth;
	Tmp->stop = pd_api_sound_stopPDSynth;
	Tmp->setVolume = pd_api_sound_setVolumePDSynth;
	Tmp->getVolume = pd_api_sound_getVolumePDSynth;
	Tmp->isPlaying = pd_api_sound_isPlayingPDSynth;
	// 1.13
	Tmp->getEnvelope = pd_api_sound_getEnvelopePDSynth;
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_synth end\n");
    return Tmp;
};

ControlSignal* pd_api_sound_newSignalControlSignal(void)
{
    ControlSignal *Tmp = (ControlSignal*) malloc(sizeof(*Tmp));
    return Tmp;
}

void pd_api_sound_freeSignalControlSignal(ControlSignal* signal)
{
    if(signal)
        free(signal);
}

void pd_api_sound_clearEventsControlSignal(ControlSignal* control)
{

}

void pd_api_sound_addEventControlSignal(ControlSignal* control, int step, float value, int interpolate)
{

}

void pd_api_sound_removeEventControlSignal(ControlSignal* control, int step)
{

}

int pd_api_sound_getMIDIControllerNumberControlSignal(ControlSignal* control)
{
    return 0;
}


playdate_control_signal* pd_api_sound_Create_playdate_control_signal()
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_control_signal\n");
    playdate_control_signal *Tmp = (playdate_control_signal*) malloc(sizeof(*Tmp));
	Tmp->newSignal = pd_api_sound_newSignalControlSignal;
	Tmp->freeSignal = pd_api_sound_freeSignalControlSignal;
	Tmp->clearEvents = pd_api_sound_clearEventsControlSignal;
	Tmp->addEvent = pd_api_sound_addEventControlSignal;
	Tmp->removeEvent = pd_api_sound_removeEventControlSignal;
	Tmp->getMIDIControllerNumber = pd_api_sound_getMIDIControllerNumberControlSignal;
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_control_signal end\n");
    return Tmp;
};

PDSynthInstrument* pd_api_sound_newInstrumentPDSynthInstrument(void)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_newInstrumentPDSynthInstrument\n");
    PDSynthInstrument *Tmp = (PDSynthInstrument*) malloc(sizeof(*Tmp));
    printfDebug(DebugTraceFunctions, "pd_api_sound_newInstrumentPDSynthInstrument end\n");
    return Tmp;
}

void pd_api_sound_freeInstrumentPDSynthInstrument(PDSynthInstrument* inst)
{
    if(inst)
        free(inst);
}

int pd_api_sound_addVoicePDSynthInstrument(PDSynthInstrument* inst, PDSynth* synth, MIDINote rangeStart, MIDINote rangeEnd, float transpose)
{
    return 0;
}

PDSynth* pd_api_sound_playNotePDSynthInstrument(PDSynthInstrument* inst, float frequency, float vel, float len, uint32_t when)
{
    return Api->sound->synth->newSynth();
}

PDSynth* pd_api_sound_playMIDINotePDSynthInstrument(PDSynthInstrument* inst, MIDINote note, float vel, float len, uint32_t when)
{
    return Api->sound->synth->newSynth();
}

void pd_api_sound_setPitchBendPDSynthInstrument(PDSynthInstrument* inst, float bend)
{

}

void pd_api_sound_setPitchBendRangePDSynthInstrument(PDSynthInstrument* inst, float halfSteps)
{

}

void pd_api_sound_setTransposePDSynthInstrument(PDSynthInstrument* inst, float halfSteps)
{

}

void pd_api_sound_noteOffPDSynthInstrument(PDSynthInstrument* inst, MIDINote note, uint32_t when)
{

}

void pd_api_sound_allNotesOffPDSynthInstrument(PDSynthInstrument* inst, uint32_t when)
{

}

void pd_api_sound_setVolumePDSynthInstrument(PDSynthInstrument* inst, float left, float right)
{

}

void pd_api_sound_getVolumePDSynthInstrument(PDSynthInstrument* inst, float* left, float* right)
{

}

int pd_api_sound_activeVoiceCountPDSynthInstrument(PDSynthInstrument* inst)
{
    return 0;
}


playdate_sound_instrument* pd_api_sound_Create_playdate_sound_instrument()
{
    printfDebug(DebugTraceFunctions,"pd_api_sound_Create_playdate_sound_instrument\n");
    playdate_sound_instrument *Tmp = (playdate_sound_instrument*) malloc(sizeof(*Tmp));

	Tmp->newInstrument = pd_api_sound_newInstrumentPDSynthInstrument;
	Tmp->freeInstrument = pd_api_sound_freeInstrumentPDSynthInstrument;
	Tmp->addVoice = pd_api_sound_addVoicePDSynthInstrument;
	Tmp->playNote = pd_api_sound_playNotePDSynthInstrument;
	Tmp->playMIDINote = pd_api_sound_playMIDINotePDSynthInstrument;
	Tmp->setPitchBend = pd_api_sound_setPitchBendPDSynthInstrument;
	Tmp->setPitchBendRange = pd_api_sound_setPitchBendRangePDSynthInstrument;
	Tmp->setTranspose = pd_api_sound_setTransposePDSynthInstrument;
	Tmp->noteOff = pd_api_sound_noteOffPDSynthInstrument;
	Tmp->allNotesOff = pd_api_sound_allNotesOffPDSynthInstrument;
	Tmp->setVolume = pd_api_sound_setVolumePDSynthInstrument;
	Tmp->getVolume = pd_api_sound_getVolumePDSynthInstrument;
	Tmp->activeVoiceCount = pd_api_sound_activeVoiceCountPDSynthInstrument;
    printfDebug(DebugTraceFunctions,"pd_api_sound_Create_playdate_sound_instrument end\n");
    return Tmp;
};

SequenceTrack* pd_api_sound_newTrackSequenceTrack(void)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_newTrackSequenceTrack\n");
    SequenceTrack *Tmp = (SequenceTrack*) malloc(sizeof(*Tmp));
    printfDebug(DebugTraceFunctions, "pd_api_sound_newTrackSequenceTrack end\n");
    return Tmp;
}

void pd_api_sound_freeTrackSequenceTrack(SequenceTrack* track)
{
    if(track)
        free(track);
}

void pd_api_sound_setInstrumentSequenceTrack(SequenceTrack* track, PDSynthInstrument* inst)
{

}

PDSynthInstrument* pd_api_sound_getInstrumentSequenceTrack(SequenceTrack* track)
{
    return Api->sound->instrument->newInstrument();
}

void pd_api_sound_addNoteEventSequenceTrack(SequenceTrack* track, uint32_t step, uint32_t len, MIDINote note, float velocity)
{

}

void pd_api_sound_removeNoteEventSequenceTrack(SequenceTrack* track, uint32_t step, MIDINote note)
{

}

void pd_api_sound_clearNotesSequenceTrack(SequenceTrack* track)
{

}

int pd_api_sound_getControlSignalCountSequenceTrack(SequenceTrack* track)
{
    return 0;
}

ControlSignal* pd_api_sound_getControlSignalSequenceTrack(SequenceTrack* track, int idx)
{
    return Api->sound->controlsignal->newSignal();
}

void pd_api_sound_clearControlEventsSequenceTrack(SequenceTrack* track)
{

}

int pd_api_sound_getPolyphonySequenceTrack(SequenceTrack* track)
{
    return 0;
}

int pd_api_sound_activeVoiceCountSequenceTrack(SequenceTrack* track)
{
    return 0;
}

void pd_api_sound_setMutedSequenceTrack(SequenceTrack* track, int mute)
{

}

// 1.1
uint32_t pd_api_sound_getLengthSequenceTrack(SequenceTrack* track) // in steps, includes full last note
{
    return 0;
}

int pd_api_sound_getIndexForStepSequenceTrack(SequenceTrack* track, uint32_t step)
{
    return 0;
}

int pd_api_sound_getNoteAtIndexSequenceTrack(SequenceTrack* track, int index, uint32_t* outStep, uint32_t* outLen, MIDINote* outNote, float* outVelocity)
{
    return 0;
}

// 1.10
ControlSignal* pd_api_sound_getSignalForControllerSequenceTrack(SequenceTrack* track, int controller, int create)
{
    return Api->sound->controlsignal->newSignal();
}

playdate_sound_track* pd_api_sound_Create_playdate_sound_track()
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_track\n");
    playdate_sound_track *Tmp = (playdate_sound_track*) malloc(sizeof(*Tmp));
	Tmp->newTrack = pd_api_sound_newTrackSequenceTrack;
	Tmp->freeTrack = pd_api_sound_freeTrackSequenceTrack;
	Tmp->setInstrument = pd_api_sound_setInstrumentSequenceTrack;
	Tmp->getInstrument = pd_api_sound_getInstrumentSequenceTrack;
	Tmp->addNoteEvent = pd_api_sound_addNoteEventSequenceTrack;
	Tmp->removeNoteEvent = pd_api_sound_removeNoteEventSequenceTrack;
	Tmp->clearNotes = pd_api_sound_clearNotesSequenceTrack;
	Tmp->getControlSignalCount = pd_api_sound_getControlSignalCountSequenceTrack;
	Tmp->getControlSignal = pd_api_sound_getControlSignalSequenceTrack;
	Tmp->clearControlEvents = pd_api_sound_clearControlEventsSequenceTrack;
	Tmp->getPolyphony = pd_api_sound_getPolyphonySequenceTrack;
	Tmp->activeVoiceCount = pd_api_sound_activeVoiceCountSequenceTrack;
	Tmp->setMuted = pd_api_sound_setMutedSequenceTrack;
	// 1.1
	Tmp->getLength = pd_api_sound_getLengthSequenceTrack;
	Tmp->getIndexForStep = pd_api_sound_getIndexForStepSequenceTrack;
	Tmp->getNoteAtIndex = pd_api_sound_getNoteAtIndexSequenceTrack;
	// 1.10
	Tmp->getSignalForController = pd_api_sound_getSignalForControllerSequenceTrack;
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_track end\n");
    return Tmp;
};

SoundSequence* pd_api_sound_newSequenceSoundSequence(void)
{
    printfDebug(DebugTraceFunctions,"pd_api_sound_newSequenceSoundSequence\n");
    SoundSequence *Tmp = (SoundSequence*) malloc(sizeof(*Tmp));
    printfDebug(DebugTraceFunctions,"pd_api_sound_newSequenceSoundSequence end\n");
    return Tmp;
}

void pd_api_sound_freeSequenceSoundSequence(SoundSequence* sequence)
{
    if(sequence)
        free(sequence);
}

int pd_api_sound_loadMidiFileSoundSequence(SoundSequence* seq, const char* path)
{
    return 0;
}

uint32_t pd_api_sound_getTimeSoundSequence(SoundSequence* seq)
{
    return 0;
}

void pd_api_sound_setTimeSoundSequence(SoundSequence* seq, uint32_t time)
{

}

void pd_api_sound_setLoopsSoundSequence(SoundSequence* seq, int loopstart, int loopend, int loops)
{

}

int pd_api_sound_getTempoSoundSequence(SoundSequence* seq)
{
    return 0;
}

void pd_api_sound_setTempoSoundSequence(SoundSequence* seq, int stepsPerSecond)
{

}

int pd_api_sound_getTrackCountSoundSequence(SoundSequence* seq)
{
    return 0;
}

SequenceTrack* pd_api_sound_addTrackSoundSequence(SoundSequence* seq)
{
    return Api->sound->track->newTrack();
}

SequenceTrack* pd_api_sound_getTrackAtIndexSoundSequence(SoundSequence* seq, unsigned int track)
{
    return Api->sound->track->newTrack();
}

void pd_api_sound_setTrackAtIndexSoundSequence(SoundSequence* seq, SequenceTrack* track, unsigned int idx)
{

}

void pd_api_sound_allNotesOffSoundSequence(SoundSequence* seq)
{

}

// 1.1
int pd_api_sound_isPlayingSoundSequence(SoundSequence* seq)
{
    return 0;
}

uint32_t pd_api_sound_getLengthSoundSequence(SoundSequence* seq) // in steps, includes full last note
{
    return 0;
}

void pd_api_sound_playSoundSequence(SoundSequence* seq, SequenceFinishedCallback finishCallback, void* userdata)
{

}

void pd_api_sound_stopSoundSequence(SoundSequence* seq)
{

}

int pd_api_sound_getCurrentStepSoundSequence(SoundSequence* seq, int* timeOffset)
{
    return 0;
}

void pd_api_sound_setCurrentStepSoundSequence(SoundSequence* seq, int step, int timeOffset, int playNotes)
{

}

playdate_sound_sequence* pd_api_sound_Create_playdate_sound_sequence()
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_sequence\n");
    playdate_sound_sequence *Tmp = (playdate_sound_sequence*) malloc(sizeof(*Tmp)); 

	Tmp->newSequence = pd_api_sound_newSequenceSoundSequence;
	Tmp->freeSequence = pd_api_sound_freeSequenceSoundSequence;
	Tmp->loadMidiFile = pd_api_sound_loadMidiFileSoundSequence;
	Tmp->getTime = pd_api_sound_getTimeSoundSequence;
	Tmp->setTime = pd_api_sound_setTimeSoundSequence;
	Tmp->setLoops = pd_api_sound_setLoopsSoundSequence;
	Tmp->getTempo = pd_api_sound_getTempoSoundSequence;
	Tmp->setTempo = pd_api_sound_setTempoSoundSequence;
	Tmp->getTrackCount = pd_api_sound_getTrackCountSoundSequence;
	Tmp->addTrack = pd_api_sound_addTrackSoundSequence;
	Tmp->getTrackAtIndex = pd_api_sound_getTrackAtIndexSoundSequence;
	Tmp->setTrackAtIndex = pd_api_sound_setTrackAtIndexSoundSequence;
	Tmp->allNotesOff = pd_api_sound_allNotesOffSoundSequence;
	// 1.1
	Tmp->isPlaying = pd_api_sound_isPlayingSoundSequence;
	Tmp->getLength = pd_api_sound_getLengthSoundSequence;
	Tmp->play = pd_api_sound_playSoundSequence;
	Tmp->stop = pd_api_sound_stopSoundSequence;
	Tmp->getCurrentStep = pd_api_sound_getCurrentStepSoundSequence;
	Tmp->setCurrentStep = pd_api_sound_setCurrentStepSoundSequence;
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_sequence end\n");
    return Tmp;
};

TwoPoleFilter* pd_api_sound_newFilterTwoPoleFilter(void)
{
     printfDebug(DebugTraceFunctions, "pd_api_sound_newFilterTwoPoleFilter\n");
     TwoPoleFilter *Tmp = (TwoPoleFilter*) malloc(sizeof(*Tmp));
     printfDebug(DebugTraceFunctions, "pd_api_sound_newFilterTwoPoleFilter end\n");
     return Tmp;
}

void pd_api_sound_freeFilterTwoPoleFilter(TwoPoleFilter* filter)
{
    if(filter)
        free(filter);
}

void pd_api_sound_setTypeTwoPoleFilter(TwoPoleFilter* filter, TwoPoleFilterType type)
{

}

void pd_api_sound_setFrequencyTwoPoleFilter(TwoPoleFilter* filter, float frequency)
{

}

void pd_api_sound_setFrequencyModulatorTwoPoleFilter(TwoPoleFilter* filter, PDSynthSignalValue* signal)
{

}

PDSynthSignalValue* pd_api_sound_getFrequencyModulatorTwoPoleFilter(TwoPoleFilter* filter)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}

void pd_api_sound_setGainTwoPoleFilter(TwoPoleFilter* filter, float gain)
{

}

void pd_api_sound_setResonanceTwoPoleFilter(TwoPoleFilter* filter, float resonance)
{

}

void pd_api_sound_setResonanceModulatorTwoPoleFilter(TwoPoleFilter* filter, PDSynthSignalValue* signal)
{

}

PDSynthSignalValue* pd_api_sound_getResonanceModulatorTwoPoleFilter(TwoPoleFilter* filter)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}

playdate_sound_effect_twopolefilter* pd_api_sound_Create_playdate_sound_effect_twopolefilter()
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_effect_twopolefilter\n");
    playdate_sound_effect_twopolefilter *Tmp = (playdate_sound_effect_twopolefilter*) malloc(sizeof(*Tmp));
	Tmp->newFilter = pd_api_sound_newFilterTwoPoleFilter;
	Tmp->freeFilter = pd_api_sound_freeFilterTwoPoleFilter;
	Tmp->setType = pd_api_sound_setTypeTwoPoleFilter;
	Tmp->setFrequency = pd_api_sound_setFrequencyTwoPoleFilter;
	Tmp->setFrequencyModulator = pd_api_sound_setFrequencyModulatorTwoPoleFilter;
	Tmp->getFrequencyModulator = pd_api_sound_getFrequencyModulatorTwoPoleFilter;
	Tmp->setGain = pd_api_sound_setGainTwoPoleFilter;
	Tmp->setResonance = pd_api_sound_setResonanceTwoPoleFilter;
	Tmp->setResonanceModulator = pd_api_sound_setResonanceModulatorTwoPoleFilter;
	Tmp->getResonanceModulator = pd_api_sound_getResonanceModulatorTwoPoleFilter;
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_effect_twopolefilter end\n");
    return Tmp;
};

OnePoleFilter* pd_api_sound_newFilterOnePoleFilter(void)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_newFilterOnePoleFilter\n");
    OnePoleFilter *Tmp = (OnePoleFilter*) malloc(sizeof(*Tmp)); 
    printfDebug(DebugTraceFunctions, "pd_api_sound_newFilterOnePoleFilter end\n");
    return Tmp;   
}

void pd_api_sound_freeFilterOnePoleFilter(OnePoleFilter* filter)
{
    if(filter)
        free(filter);
}

void pd_api_sound_setParameterOnePoleFilter(OnePoleFilter* filter, float parameter)
{

}

void pd_api_sound_setParameterModulatorOnePoleFilter(OnePoleFilter* filter, PDSynthSignalValue* signal)
{

}

PDSynthSignalValue* pd_api_sound_getParameterModulatorOnePoleFilter(OnePoleFilter* filter)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}

playdate_sound_effect_onepolefilter* pd_api_sound_Create_playdate_sound_effect_onepolefilter()
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_effect_onepolefilter\n");
    playdate_sound_effect_onepolefilter *Tmp = (playdate_sound_effect_onepolefilter*) malloc(sizeof(*Tmp)); 
    Tmp->newFilter = pd_api_sound_newFilterOnePoleFilter;
	Tmp->freeFilter = pd_api_sound_freeFilterOnePoleFilter;
	Tmp->setParameter = pd_api_sound_setParameterOnePoleFilter;
	Tmp->setParameterModulator = pd_api_sound_setParameterModulatorOnePoleFilter;
	Tmp->getParameterModulator = pd_api_sound_getParameterModulatorOnePoleFilter;
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_effect_onepolefilter end\n");
    return Tmp; 
};


BitCrusher* pd_api_sound_newBitCrusherBitCrusher(void)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_newBitCrusherBitCrusher\n");
    BitCrusher *Tmp = (BitCrusher*) malloc(sizeof(*Tmp));
    printfDebug(DebugTraceFunctions, "pd_api_sound_newBitCrusherBitCrusher end\n");
    return Tmp;
}

void pd_api_sound_freeBitCrusherBitCrusher(BitCrusher* filter)
{
    if(filter)
        free(filter);
}

void pd_api_sound_setAmountBitCrusher(BitCrusher* filter, float amount)
{

}

void pd_api_sound_setAmountModulatorBitCrusher(BitCrusher* filter, PDSynthSignalValue* signal)
{

}

PDSynthSignalValue* pd_api_sound_getAmountModulatorBitCrusher(BitCrusher* filter)
{    
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}

void pd_api_sound_setUndersamplingBitCrusher(BitCrusher* filter, float undersampling)
{

}

void pd_api_sound_setUndersampleModulatorBitCrusher(BitCrusher* filter, PDSynthSignalValue* signal)
{

}

PDSynthSignalValue* pd_api_sound_getUndersampleModulatorBitCrusher(BitCrusher* filter)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}

playdate_sound_effect_bitcrusher* pd_api_sound_Create_playdate_sound_effect_bitcrusher()
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_effect_bitcrusher\n");
    playdate_sound_effect_bitcrusher *Tmp = (playdate_sound_effect_bitcrusher*) malloc(sizeof(*Tmp));
    Tmp->newBitCrusher = pd_api_sound_newBitCrusherBitCrusher;
	Tmp->freeBitCrusher = pd_api_sound_freeBitCrusherBitCrusher;
	Tmp->setAmount = pd_api_sound_setAmountBitCrusher;
	Tmp->setAmountModulator = pd_api_sound_setAmountModulatorBitCrusher;
	Tmp->getAmountModulator = pd_api_sound_getAmountModulatorBitCrusher;
	Tmp->setUndersampling = pd_api_sound_setUndersamplingBitCrusher;
	Tmp->setUndersampleModulator = pd_api_sound_setUndersampleModulatorBitCrusher;
	Tmp->getUndersampleModulator = pd_api_sound_getUndersampleModulatorBitCrusher;
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_effect_bitcrusher end\n");
    return Tmp;
};

RingModulator* pd_api_sound_newRingmodRingModulator(void)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_newRingmodRingModulator\n");
    RingModulator *Tmp = (RingModulator*) malloc(sizeof(*Tmp));
    printfDebug(DebugTraceFunctions, "pd_api_sound_newRingmodRingModulator end\n");
    return Tmp;
}

void pd_api_sound_freeRingmodRingModulator(RingModulator* filter)
{
    if(filter)
        free(filter);
}

void pd_api_sound_setFrequencyRingModulator(RingModulator* filter, float frequency)
{

}

void pd_api_sound_setFrequencyModulatorRingModulator(RingModulator* filter, PDSynthSignalValue* signal)
{

}

PDSynthSignalValue* pd_api_sound_getFrequencyModulatorRingModulator(RingModulator* filter)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}


playdate_sound_effect_ringmodulator* pd_api_sound_Create_playdate_sound_effect_ringmodulator()
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_effect_ringmodulator\n");
    playdate_sound_effect_ringmodulator *Tmp = (playdate_sound_effect_ringmodulator*) malloc(sizeof(*Tmp));
	Tmp->newRingmod = pd_api_sound_newRingmodRingModulator;
	Tmp->freeRingmod = pd_api_sound_freeRingmodRingModulator;
	Tmp->setFrequency = pd_api_sound_setFrequencyRingModulator;
	Tmp->setFrequencyModulator = pd_api_sound_setFrequencyModulatorRingModulator;
	Tmp->getFrequencyModulator = pd_api_sound_getFrequencyModulatorRingModulator;
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_effect_ringmodulator end\n");
    return Tmp;
};

DelayLine* pd_api_sound_newDelayLineDelayLine(int length, int stereo)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_newDelayLineDelayLine\n");
    DelayLine *Tmp = (DelayLine*) malloc(sizeof(*Tmp));
    printfDebug(DebugTraceFunctions, "pd_api_sound_newDelayLineDelayLine end\n");
    return Tmp;
}

void pd_api_sound_freeDelayLineDelayLine(DelayLine* filter)
{
    if (filter)
        free(filter);
}

void pd_api_sound_setLengthDelayLine(DelayLine* d, int frames)
{

}

void pd_api_sound_setFeedbackDelayLine(DelayLine* d, float fb)
{

}

DelayLineTap* pd_api_sound_addTapDelayLine(DelayLine* d, int delay)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_addTapDelayLine\n");
    DelayLineTap *Tmp = (DelayLineTap*) malloc(sizeof(*Tmp));
    printfDebug(DebugTraceFunctions, "pd_api_sound_addTapDelayLine end\n");
    return Tmp;
}

	
// note that DelayLineTap is a SoundSource, not a SoundEffect
void pd_api_sound_freeTapDelayLineTap(DelayLineTap* tap)
{
    if(tap)
        free(tap);
}

void pd_api_sound_setTapDelayDelayLineTap(DelayLineTap* t, int frames)
{

}

void pd_api_sound_setTapDelayModulatorDelayLineTap(DelayLineTap* t, PDSynthSignalValue* mod)
{

}

PDSynthSignalValue* pd_api_sound_getTapDelayModulatorDelayLineTap(DelayLineTap* t)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}

void pd_api_sound_setTapChannelsFlippedDelayLineTap(DelayLineTap* t, int flip)
{

}

playdate_sound_effect_delayline*  pd_api_sound_Create_playdate_sound_effect_delayline()
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_effect_delayline\n");
    playdate_sound_effect_delayline *Tmp = (playdate_sound_effect_delayline*) malloc(sizeof(*Tmp));
	Tmp->newDelayLine = pd_api_sound_newDelayLineDelayLine;
	Tmp->freeDelayLine = pd_api_sound_freeDelayLineDelayLine;
	Tmp->setLength = pd_api_sound_setLengthDelayLine;
	Tmp->setFeedback = pd_api_sound_setFeedbackDelayLine;
	Tmp->addTap = pd_api_sound_addTapDelayLine;
	
	// note that DelayLineTap is a SoundSource, not a SoundEffect
	Tmp->freeTap = pd_api_sound_freeTapDelayLineTap;
	Tmp->setTapDelay = pd_api_sound_setTapDelayDelayLineTap;
	Tmp->setTapDelayModulator = pd_api_sound_setTapDelayModulatorDelayLineTap;
	Tmp->getTapDelayModulator = pd_api_sound_getTapDelayModulatorDelayLineTap;
	Tmp->setTapChannelsFlipped = pd_api_sound_setTapChannelsFlippedDelayLineTap;
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_effect_delayline end\n");
    return Tmp;
}

Overdrive* pd_api_sound_newOverdriveOverdrive(void)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_newOverdriveOverdrive\n");
    Overdrive *Tmp = (Overdrive*) malloc(sizeof(*Tmp));
    printfDebug(DebugTraceFunctions, "pd_api_sound_newOverdriveOverdrive end\n");
    return Tmp;
}

void pd_api_sound_freeOverdriveOverdrive(Overdrive* filter)
{
    if(filter)
        free(filter);
}

void pd_api_sound_setGainOverdrive(Overdrive* o, float gain)
{

}

void pd_api_sound_setLimitOverdrive(Overdrive* o, float limit)
{

}

void pd_api_sound_setLimitModulatorOverdrive(Overdrive* o, PDSynthSignalValue* mod)
{

}

PDSynthSignalValue* pd_api_sound_getLimitModulatorOverdrive(Overdrive* o)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}

void pd_api_sound_setOffsetOverdrive(Overdrive* o, float offset)
{

}

void pd_api_sound_setOffsetModulatorOverdrive(Overdrive* o, PDSynthSignalValue* mod)
{

}

PDSynthSignalValue* pd_api_sound_getOffsetModulatorOverdrive(Overdrive* o)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}


playdate_sound_effect_overdrive* pd_api_sound_Create_playdate_sound_effect_overdrive()
{
    printfDebug(DebugTraceFunctions,"pd_api_sound_Create_playdate_sound_effect_overdrive\n");
    playdate_sound_effect_overdrive *Tmp = (playdate_sound_effect_overdrive*) malloc(sizeof(*Tmp));
	Tmp->newOverdrive = pd_api_sound_newOverdriveOverdrive; 
	Tmp->freeOverdrive = pd_api_sound_freeOverdriveOverdrive;
	Tmp->setGain = pd_api_sound_setGainOverdrive;
	Tmp->setLimit = pd_api_sound_setLimitOverdrive;
	Tmp->setLimitModulator = pd_api_sound_setLimitModulatorOverdrive;
	Tmp->getLimitModulator = pd_api_sound_getLimitModulatorOverdrive;
	Tmp->setOffset = pd_api_sound_setOffsetOverdrive;
	Tmp->setOffsetModulator = pd_api_sound_setOffsetModulatorOverdrive;
	Tmp->getOffsetModulator = pd_api_sound_getOffsetModulatorOverdrive;
    printfDebug(DebugTraceFunctions,"pd_api_sound_Create_playdate_sound_effect_overdrive end\n");
    return Tmp;
};

SoundEffect* pd_api_sound_newEffectSoundEffect(effectProc* proc, void* userdata)
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_newEffectSoundEffect\n");
    SoundEffect *Tmp = (SoundEffect*) malloc(sizeof(*Tmp));
    printfDebug(DebugTraceFunctions, "pd_api_sound_newEffectSoundEffect end\n");
    return Tmp;
}

void pd_api_sound_freeEffectSoundEffect(SoundEffect* effect)
{
    if(effect)
        free(effect);
}

void pd_api_sound_setMixSoundEffect(SoundEffect* effect, float level)
{

}

void pd_api_sound_setMixModulatorSoundEffect(SoundEffect* effect, PDSynthSignalValue* signal)
{

}

PDSynthSignalValue* pd_api_sound_getMixModulatorSoundEffect(SoundEffect* effect)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}

void pd_api_sound_setUserdataSoundEffect(SoundEffect* effect, void* userdata)
{

}

void* pd_api_sound_getUserdataSoundEffect(SoundEffect* effect)
{
    return NULL;
}

playdate_sound_effect* pd_api_sound_Create_playdate_sound_effect()
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_effect\n");
    playdate_sound_effect *Tmp = (playdate_sound_effect*) malloc(sizeof(*Tmp));
	Tmp->newEffect = pd_api_sound_newEffectSoundEffect;
	Tmp->freeEffect = pd_api_sound_freeEffectSoundEffect;
	Tmp->setMix = pd_api_sound_setMixSoundEffect;
	Tmp->setMixModulator = pd_api_sound_setMixModulatorSoundEffect;
	Tmp->getMixModulator = pd_api_sound_getMixModulatorSoundEffect;
	Tmp->setUserdata = pd_api_sound_setUserdataSoundEffect;
	Tmp->getUserdata = pd_api_sound_getUserdataSoundEffect;

	Tmp->bitcrusher = pd_api_sound_Create_playdate_sound_effect_bitcrusher();
    Tmp->delayline = pd_api_sound_Create_playdate_sound_effect_delayline();
    Tmp->onepolefilter = pd_api_sound_Create_playdate_sound_effect_onepolefilter();
    Tmp->twopolefilter = pd_api_sound_Create_playdate_sound_effect_twopolefilter();
    Tmp->ringmodulator = pd_api_sound_Create_playdate_sound_effect_ringmodulator();
    Tmp->overdrive = pd_api_sound_Create_playdate_sound_effect_overdrive();
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_effect end\n");
    return Tmp;
};

SoundChannel*  pd_api_sound_newChannelSoundChannel(void)
{
    printfDebug(DebugTraceFunctions,"pd_api_sound_newChannelSoundChannel\n");
    SoundChannel *Tmp = (SoundChannel*) malloc(sizeof(*Tmp));
    printfDebug(DebugTraceFunctions,"pd_api_sound_newChannelSoundChannel end\n");
    return Tmp;

}

void pd_api_sound_freeChannelSoundChannel(SoundChannel* channel)
{
    if(channel)
        free(channel);
}

int pd_api_sound_addSourceSoundChannel(SoundChannel* channel, SoundSource* source)
{
    return 0;
}

int pd_api_sound_removeSourceSoundChannel(SoundChannel* channel, SoundSource* source)
{
    return 0;
}

SoundSource* pd_api_sound_addCallbackSourceSoundChannel(SoundChannel* channel, AudioSourceFunction* callback, void* context, int stereo)
{
    SoundSource *Tmp = (SoundSource*) malloc(sizeof(*Tmp));
    return Tmp;
}

void pd_api_sound_addEffectSoundChannel(SoundChannel* channel, SoundEffect* effect)
{

}

void pd_api_sound_removeEffectSoundChannel(SoundChannel* channel, SoundEffect* effect)
{

}

void pd_api_sound_setVolumeSoundChannel(SoundChannel* channel, float volume)
{

}

float pd_api_sound_getVolumeSoundChannel(SoundChannel* channel)
{
    return 0.0f;
}

void pd_api_sound_setVolumeModulatorSoundChannel(SoundChannel* channel, PDSynthSignalValue* mod)
{

}

PDSynthSignalValue* pd_api_sound_getVolumeModulatorSoundChannel(SoundChannel* channel)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}

void pd_api_sound_setPanSoundChannel(SoundChannel* channel, float pan)
{

}

void pd_api_sound_setPanModulatorSoundChannel(SoundChannel* channel, PDSynthSignalValue* mod)
{

}

PDSynthSignalValue* pd_api_sound_getPanModulatorSoundChannel(SoundChannel* channel)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}

PDSynthSignalValue* pd_api_sound_getDryLevelSignalSoundChannel(SoundChannel* channel)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}

PDSynthSignalValue* pd_api_sound_getWetLevelSignalSoundChannel(SoundChannel* channel)
{
    PDSynthSignalValue *Tmp = (PDSynthSignalValue*) malloc(sizeof(*Tmp));
    return Tmp;
}


playdate_sound_channel*  pd_api_sound_Create_playdate_sound_channel()
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_channel\n");
    playdate_sound_channel *Tmp = (playdate_sound_channel*) malloc(sizeof(*Tmp));
	Tmp->newChannel = pd_api_sound_newChannelSoundChannel;
	Tmp->freeChannel = pd_api_sound_freeChannelSoundChannel;
	Tmp->addSource = pd_api_sound_addSourceSoundChannel;
	Tmp->removeSource = pd_api_sound_removeSourceSoundChannel;
	Tmp->addCallbackSource = pd_api_sound_addCallbackSourceSoundChannel;
	Tmp->addEffect = pd_api_sound_addEffectSoundChannel;
	Tmp->removeEffect = pd_api_sound_removeEffectSoundChannel;
	Tmp->setVolume = pd_api_sound_setVolumeSoundChannel;
	Tmp->getVolume = pd_api_sound_getVolumeSoundChannel;
	Tmp->setVolumeModulator = pd_api_sound_setVolumeModulatorSoundChannel;
	Tmp->getVolumeModulator = pd_api_sound_getVolumeModulatorSoundChannel;
	Tmp->setPan = pd_api_sound_setPanSoundChannel;
	Tmp->setPanModulator = pd_api_sound_setPanModulatorSoundChannel;
	Tmp->getPanModulator = pd_api_sound_getPanModulatorSoundChannel;
	Tmp->getDryLevelSignal = pd_api_sound_getDryLevelSignalSoundChannel;
	Tmp->getWetLevelSignal = pd_api_sound_getWetLevelSignalSoundChannel;
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound_channel end\n");
    return Tmp;
};


playdate_sound* pd_api_sound_Create_playdate_sound()
{
    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound\n");
    playdate_sound *Tmp = (playdate_sound*) malloc(sizeof(*Tmp));
    //sub classes
    Tmp->source = pd_api_sound_Create_playdate_sound_source();
    Tmp->fileplayer =  pd_api_sound_Create_playdate_sound_fileplayer();
    Tmp->sample = pd_api_sound_Create_playdate_sound_soundsample();
    Tmp->sampleplayer = pd_api_sound_Create_playdate_sound_sampleplayer();
    Tmp->signal = pd_api_sound_Create_playdate_sound_signal();
    Tmp->lfo = pd_api_sound_Create_playdate_sound_lfo();
    Tmp->envelope = pd_api_sound_Create_playdate_sound_envelope();
    Tmp->synth = pd_api_sound_Create_playdate_sound_synth();
    Tmp->controlsignal = pd_api_sound_Create_playdate_control_signal();
    Tmp->instrument = pd_api_sound_Create_playdate_sound_instrument();
    Tmp->track = pd_api_sound_Create_playdate_sound_track();
    Tmp->sequence = pd_api_sound_Create_playdate_sound_sequence();
    Tmp->effect = pd_api_sound_Create_playdate_sound_effect();
    Tmp->channel = pd_api_sound_Create_playdate_sound_channel();

    Tmp->removeSource = pd_api_sound_removeSource;
	Tmp->getCurrentTime = pd_api_sound_getCurrentTime;
    Tmp->addSource = pd_api_sound_addSource;
    Tmp->getDefaultChannel = pd_api_sound_getDefaultChannel;
    Tmp->addChannel = pd_api_sound_addChannel;
    Tmp->removeChannel = pd_api_sound_removeChannel;
    Tmp->setMicCallback = pd_api_sound_setMicCallback;
    Tmp->getHeadphoneState = pd_api_sound_getHeadphoneState;
    Tmp->setOutputsActive = pd_api_sound_setOutputsActive;

    printfDebug(DebugTraceFunctions, "pd_api_sound_Create_playdate_sound end\n");
    return Tmp;
}
