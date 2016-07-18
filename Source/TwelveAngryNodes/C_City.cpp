// Fill out your copyright notice in the Description page of Project Settings.

#include "TwelveAngryNodes.h"
#include "C_City.h"

//Empty constructor to be overriden in blueprint if necessary
AC_City::AC_City(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    
}

//Sets initial values to all variables that need initializing
void AC_City::initializeCity()
{
    districtList.Push(true);
    for (int32 i=1; i<7; i++) {
        int32 current=gameManager->getNeighbor(gameManager->getX(position), gameManager->getY(position), i);
        if (current != -1) {
            workableTiles.Push(current);
            workedTiles.Push(false);
        }
        districtList.Push(false);
    }
    
    maxArtists=0;
    maxCraftsmen=0;
    maxMerchants=0;
    maxSages=0;
    currentArtists=0;
    currentCraftsmen=0;
    currentMerchants=0;
    currentSages=0;
    
    currentPopulation=1;
    currentInfluence=0;
}

//Allocates citizens according to some predefined rules
void AC_City::allocateCitizens()
{
    TArray<int32> totalTileYields;
    for (int32 i=0; i<workedTiles.Num(); i++) {
        workedTiles[i]=false;//Reinitializing
    }
    
    for (int32 i=0; i<currentPopulation; i++) {
        int32 highestYield=0;
        int32 highestIndex=-1;
        for (int32 j=0; j<workableTiles.Num(); j++) {
            if (!workedTiles[j]) {
                int32 tileYield=gameManager->PrimaryHexArray[workableTiles[j]]->foodYield + gameManager->PrimaryHexArray[workableTiles[j]]->prodYield + gameManager->PrimaryHexArray[workableTiles[j]]->coinYield;
                if (tileYield >= highestYield) {
                    highestYield=tileYield;
                    highestIndex=j;
                }
            }
        }
        workedTiles[highestIndex]=true;
    }
}




//Utility functions to get the current yields of a specialist in this city
int32 AC_City::getSpecFoodYield(ESpecialist specType)
{
    return 0;
}

int32 AC_City::getSpecProdYield(ESpecialist specType)
{
    return 0;
}

int32 AC_City::getSpecCoinYield(ESpecialist specType)

{
    return 0;
}

int32 AC_City::getSpecInfluenceYield(ESpecialist specType)
{
    return 0;
}

int32 AC_City::getSpecResearchYield(ESpecialist specType)
{
    return 0;
}

int32 AC_City::getSpecHappyYield(ESpecialist specType)
{
    return 0;
}

int32 AC_City::getSpecHealthYield(ESpecialist specType)
{
    return 0;
}
