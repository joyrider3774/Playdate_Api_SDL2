#include <SDL.h>
#include <SDL_mixer.h>
#include <sys/stat.h>
#include "pd_api/pd_api_sound.h"
#include "gamestubcallbacks.h"
#include "gamestub.h"
#include "defines.h"

//AudioSample

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
    AudioSample *Tmp = (AudioSample*) malloc(sizeof(*Tmp));
    Tmp->sound = NULL;
    Tmp->channel = -1;
    Tmp->path = NULL;
    return Tmp;
}



AudioSample* pd_api_sound_loadSample(const char* path)
{    
    char ext[5];
    char* fullpath = malloc((strlen(path) + 7) * sizeof(char));
    bool needextension = true;
    if(strlen(path) > 4)
    {
        strcpy(ext, path + (strlen(path) - 4));
        needextension = (strcasecmp(ext, ".WAV") != 0) &&  (strcasecmp(ext, ".MP3") != 0);
    }

    if (needextension)
    {
        struct stat lstats;   
        sprintf(fullpath,"./%s.mp3", path);
        if(stat(fullpath, &lstats) != 0)
            sprintf(fullpath,"./%s.wav", path);
    }
    else
        sprintf(fullpath, "./%s", path);
    
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
            printf("Unable to load Audio file \"%s\": %s\n", path, Mix_GetError());
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
                SampleList = malloc(sizeof(*SampleList));
            }
            else
            {
                SampleList = realloc(SampleList, (SampleListCount + 1) * sizeof(*SampleList));
            }
            SampleListCount++;
            SampleList[index] = malloc(sizeof(SampleChannelEntry));
            if (SampleListCount != Mix_AllocateChannels(SampleListCount))
            {
                printf("Warning Mix_AllocateChannels(%d)\n failed!", SampleListCount);
            }; 
        }
        tmp->channel = index;
        SampleList[index]->Sample = tmp;
        SampleList[index]->loaded = true;
    }
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
    if(sample == NULL)
        return;

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
}

float pd_api_sound_getLengthSample(AudioSample* sample)
{
    if(sample == NULL)
        return 0.0f;
    if(sample->sound == NULL)
        return 0.0f;
    Uint32 points = 0;
    Uint32 frames = 0;
    int freq = 0;
    Uint16 fmt = 0;
    int chans = 0;
    if (!Mix_QuerySpec(&freq, &fmt, &chans))
        return 0.0f; 

    /* bytes / samplesize == sample points */
    points = (sample->sound->alen / ((fmt & 0xFF) / 8));

    /* sample points / channels == sample frames */
    frames = (points / chans);

    /* (sample frames * 1000) / frequency == play length in ms */
    return ((frames * 1000000.f) / freq);
}

playdate_sound_sample* pd_api_sound_Create_playdate_sound_soundsample(void)
{
    playdate_sound_sample *Tmp = (playdate_sound_sample*) malloc(sizeof(*Tmp));
    Tmp->newSampleBuffer = pd_api_sound_newSampleBuffer;
    Tmp->loadIntoSample = pd_api_sound_loadIntoSample;
    Tmp->load = pd_api_sound_loadSample;
    Tmp->newSampleFromData = pd_api_sound_newSampleFromData;
    Tmp->getData = pd_api_sound_getDataSample;
    Tmp->freeSample = pd_api_sound_freeSample;
    Tmp->getLength = pd_api_sound_getLengthSample;
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
    SamplePlayer *Tmp = (SamplePlayer*) malloc(sizeof(*Tmp));
    Tmp->sample = NULL;
    Tmp->VolumeL = 1.0f;
    Tmp->VolumeR = 1.0f;
    Tmp->Offset = 0.0f;
    Tmp->Rate = 0;
    Tmp->Start = 0;
    Tmp->End = 0;
    return Tmp;
}

void pd_api_sound_freeSamplePlayer(SamplePlayer* player)
{
    free(player);
}

void pd_api_sound_setSampleSamplePlayer(SamplePlayer* player, AudioSample* sample)
{
    player->sample = sample;
}

int pd_api_sound_playSamplePlayer(SamplePlayer* player, int repeat, float rate)
{
    if(player->sample == NULL)
        return -1;
    
    if(player->sample->sound == NULL)
        return -1;

    int channel = Mix_PlayChannel(player->sample->channel, player->sample->sound, repeat-1);
    
    if(channel > -1)
        return 0;
    else
        return -1;
}

int pd_api_sound_isPlayingSamplePlayer(SamplePlayer* player)
{
    if(player->sample == NULL)
        return 0;

    if(player->sample->channel > -1)
    {
        if(Mix_Playing(player->sample->channel))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    return 0;
}

void pd_api_sound_stopSamplePlayer(SamplePlayer* player)
{
    if(player->sample == NULL)
        return;

    if(player->sample->channel > -1)
    {
        Mix_HaltChannel(player->sample->channel);
    }
}

void pd_api_sound_setVolumeSamplePlayer(SamplePlayer* player, float left, float right)
{
    if (player->sample != NULL)
    {
        if (player->sample->sound != NULL)
        {
            if(player->sample->channel > -1 )
                Mix_Volume(player->sample->channel, MIX_MAX_VOLUME * left);
        }
    }
}

void pd_api_sound_getVolumeSamplePlayer(SamplePlayer* player, float* left, float* right)
{
    *left = player->VolumeL;
    *right = player->VolumeR;
}

float pd_api_sound_getLengthSamplePlayer(SamplePlayer* player)
{
    return pd_api_sound_getLengthSample(player->sample);
}

void pd_api_sound_setOffsetSamplePlayer(SamplePlayer* player, float offset)
{
    player->Offset = offset;
}

void pd_api_sound_setRateSamplePlayer(SamplePlayer* player, float rate)
{
    player->Rate = rate;
}

void pd_api_sound_setPlayRangeSamplePlayer(SamplePlayer* player, int start, int end)
{
    player->Start = start;
    player->End = end;
}

void pd_api_sound_setFinishCallbackSamplePlayer(SamplePlayer* player, sndCallbackProc callback)
{

}

void pd_api_sound_setLoopCallbackSamplePlayer(SamplePlayer* player, sndCallbackProc callback)
{

}

float pd_api_sound_getOffsetSamplePlayer(SamplePlayer* player)
{
    return player->Offset;
}

float pd_api_sound_getRateSamplePlayer(SamplePlayer* player)
{
    return player->Rate;
}

void pd_api_sound_setPausedSamplePlayer(SamplePlayer* player, int flag)
{
    
    if (player->sample == NULL)
        return;

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
}


playdate_sound_sampleplayer* pd_api_sound_Create_playdate_sound_sampleplayer(void)
{
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
    return Tmp;
}

//playdate_sound_fileplayer
struct FilePlayer {
    SamplePlayer* Player;
};

FilePlayer* pd_api_sound_newFilePlayer(void)
{
    FilePlayer *Tmp = (FilePlayer*) malloc(sizeof(*Tmp));
    Tmp->Player = pd_api_sound_newSamplePlayer();
    return Tmp;
}

void pd_api_sound_freeFilePlayer(FilePlayer* player)
{
    if(player)
    {
        pd_api_sound_freeSample(player->Player->sample);
        pd_api_sound_freeSamplePlayer(player->Player);
    }
}


int pd_api_sound_loadIntoFilePlayer(FilePlayer* player, const char* path)
{
    pd_api_sound_freeSample(player->Player->sample);
    AudioSample* sample = pd_api_sound_loadSample(path);
    pd_api_sound_setSampleSamplePlayer(player->Player, sample);
}

void pd_api_sound_FilePlayersetBufferLength(FilePlayer* player, float bufferLen)
{

}

int pd_api_sound_FilePlayerplay(FilePlayer* player, int repeat)
{
    return pd_api_sound_playSamplePlayer(player->Player, repeat, 1.0f);
}

int pd_api_sound_FilePlayerisPlaying(FilePlayer* player)
{
    return pd_api_sound_isPlayingSamplePlayer(player->Player);
}

void pd_api_sound_FilePlayerpause(FilePlayer* player)
{
    pd_api_sound_setPausedSamplePlayer(player->Player, 1);
}

void pd_api_sound_FilePlayerstop(FilePlayer* player)
{
    pd_api_sound_stopSamplePlayer(player->Player);
}

void pd_api_sound_FilePlayersetVolume(FilePlayer* player, float left, float right)
{
    pd_api_sound_setVolumeSamplePlayer(player->Player, left, right);
}

void pd_api_sound_FilePlayergetVolume(FilePlayer* player, float* left, float* right)
{
    pd_api_sound_getVolumeSamplePlayer(player->Player, left, right);
}

float pd_api_sound_FilePlayergetLength(FilePlayer* player)
{
    return pd_api_sound_getLengthSamplePlayer(player->Player);
}

void pd_api_sound_FilePlayersetOffset(FilePlayer* player, float offset)
{
    pd_api_sound_setOffsetSamplePlayer(player->Player, offset);
}

void pd_api_sound_FilePlayersetRate(FilePlayer* player, float rate)
{
    pd_api_sound_setRateSamplePlayer(player->Player, rate);
}

void pd_api_sound_FilePlayersetLoopRange(FilePlayer* player, float start, float end)
{
    pd_api_sound_setPlayRangeSamplePlayer(player->Player, start, end);
}

int pd_api_sound_FilePlayerdidUnderrun(FilePlayer* player)
{
    return 0;
}

void pd_api_sound_FilePlayersetFinishCallback(FilePlayer* player, sndCallbackProc callback)
{

}

void pd_api_sound_FilePlayersetLoopCallback(FilePlayer* player, sndCallbackProc callback)
{

}

float pd_api_sound_FilePlayergetOffset(FilePlayer* player)
{
    return pd_api_sound_getOffsetSamplePlayer(player->Player);
}

float pd_api_sound_FilePlayergetRate(FilePlayer* player)
{
    return pd_api_sound_getRateSamplePlayer(player->Player);
}

void pd_api_sound_FilePlayersetStopOnUnderrun(FilePlayer* player, int flag)
{
    
}

void pd_api_sound_FilePlayerfadeVolume(FilePlayer* player, float left, float right, int32_t len, sndCallbackProc finishCallback)
{

}

void pd_api_sound_FilePlayersetMP3StreamSource(FilePlayer* player, int (*dataSource)(uint8_t* data, int bytes, void* userdata), void* userdata, float bufferLen)
{

}

playdate_sound_fileplayer* pd_api_sound_Create_playdate_sound_fileplayer()
{
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

    return Tmp;
}


//Playdate Sound

uint32_t pd_api_sound_getCurrentTime(void)
{

}

SoundSource* pd_api_sound_addSource(AudioSourceFunction* callback, void* context, int stereo)
{

}

SoundChannel* pd_api_sound_getDefaultChannel(void)
{

}
	
void pd_api_sound_addChannel(SoundChannel* channel)
{

}

void pd_api_sound_removeChannel(SoundChannel* channel)
{

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
void pd_api_sound_removeSource(SoundSource* source)
{
    
}
	

playdate_sound* pd_api_sound_Create_playdate_sound()
{
    playdate_sound *Tmp = (playdate_sound*) malloc(sizeof(*Tmp));
    
    Tmp->fileplayer =  pd_api_sound_Create_playdate_sound_fileplayer();
    Tmp->sample = pd_api_sound_Create_playdate_sound_soundsample();
    Tmp->sampleplayer = pd_api_sound_Create_playdate_sound_sampleplayer();
    
    Tmp->removeSource = pd_api_sound_removeSource;
	Tmp->getCurrentTime = pd_api_sound_getCurrentTime;
    Tmp->addSource = pd_api_sound_addSource;
    Tmp->getDefaultChannel = pd_api_sound_getDefaultChannel;
    Tmp->addChannel = pd_api_sound_addChannel;
    Tmp->removeChannel = pd_api_sound_removeChannel;
    Tmp->setMicCallback = pd_api_sound_setMicCallback;
    Tmp->getHeadphoneState = pd_api_sound_getHeadphoneState;
    Tmp->setOutputsActive = pd_api_sound_setOutputsActive;
    // 1.5
    Tmp->removeSource = pd_api_sound_removeSource;	

    return Tmp;
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