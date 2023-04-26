#include <string.h>
#include <dirent.h>
#include "pd_api/pd_api_scoreboards.h"
#include "pd_api.h"
#include "gamestubcallbacks.h"
#include "gamestub.h"
#include "defines.h"

int pd_api_scoreboards_addScore(const char *boardId, uint32_t value, AddScoreCallback callback)
{
    return -1;
}

int pd_api_scoreboards_getPersonalBest(const char *boardId, PersonalBestCallback callback)
{
    return -1;
}

void pd_api_scoreboards_freeScore(PDScore *score)
{

}

int pd_api_scoreboards_getScoreboards(BoardsListCallback callback)
{
    return -1;
}

void pd_api_scoreboards_freeBoardsList(PDBoardsList *boardsList)
{

}

int pd_api_scoreboards_getScores(const char *boardId, ScoresCallback callback)
{
    return -1;
}

void pd_api_scoreboards_freeScoresList(PDScoresList *scoresList)

{

}

playdate_scoreboards* pd_api_scoreboards_Create_playdate_scoreboards()
{
    playdate_scoreboards *Tmp = (playdate_scoreboards*) malloc(sizeof(*Tmp));
	Tmp->addScore = pd_api_scoreboards_addScore;
	Tmp->getPersonalBest = pd_api_scoreboards_getPersonalBest;
	Tmp->freeScore = pd_api_scoreboards_freeScore;

	Tmp->getScoreboards = pd_api_scoreboards_getScoreboards;
	Tmp->freeBoardsList = pd_api_scoreboards_freeBoardsList;

	Tmp->getScores = pd_api_scoreboards_getScores;
	Tmp->freeScoresList = pd_api_scoreboards_freeScoresList;
    return Tmp;
};