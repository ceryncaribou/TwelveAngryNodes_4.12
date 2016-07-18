// Fill out your copyright notice in the Description page of Project Settings.

#include "TwelveAngryNodes.h"
#include "C_HexTile.h"

//Empty constructor to be overriden in blueprint if necessary
AC_HexTile::AC_HexTile(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    
}

//Called when a city district is built, destroyed or captured on this tile; updates relevant info and then calls CalcTileYields
void AC_HexTile::UpdateYieldsCity(int32 newDistrictFromCiv)
{
    hasCityDistrictFromCiv=newDistrictFromCiv;
    CalcTileYields();
}

//Checks terrain and features to calculate current tile yields
void AC_HexTile::CalcTileYields()
{
    switch (terrain) {
        case ETerrain::VE_Ocean:
            foodYield=0;
            prodYield=0;
            coinYield=0;
            break;
        case ETerrain::VE_Coast:
            foodYield=1;
            prodYield=0;
            coinYield=1;
            break;
        case ETerrain::VE_Lake:
            foodYield=2;
            prodYield=0;
            coinYield=1;
            break;
        case ETerrain::VE_Grassland:
            foodYield=2;
            prodYield=0;
            coinYield=0;
            break;
        case ETerrain::VE_Plain:
            foodYield=1;
            prodYield=1;
            coinYield=0;
            break;
        case ETerrain::VE_Tundra:
            foodYield=1;
            prodYield=0;
            coinYield=0;
            break;
        case ETerrain::VE_Snow:
            foodYield=0;
            prodYield=0;
            coinYield=0;
            break;
        case ETerrain::VE_Desert:
            foodYield=0;
            prodYield=0;
            coinYield=0;
            break;
        default://the rest I guess?
            foodYield=0;
            prodYield=0;
            coinYield=0;
            break;
    }
    if (hasForest==1) {
        prodYield++;//forest gives 1 prod
    }
    if (hasWater==2) {
        coinYield++;//river gives 1 coin
        if (terrain == ETerrain::VE_Desert) {//floodplain
            foodYield=3;
        }
    }
    
    //Improvement bonus yields (without resources)
    switch (improvementType) {
        case EImprovement::VE_Quarry :
            prodYield=prodYield+2;
            break;
        case EImprovement::VE_Farm :
            foodYield=foodYield+2;
            break;
        default:
            break;
    }
    
    //Unimproved/improved resource bonus yield
    switch (resourceType) {
        case EResource::VE_Granite:
            prodYield++;
            if (improvementType==EImprovement::VE_Quarry) {
                prodYield++;
            }
            break;
        case EResource::VE_Limestone:
            prodYield++;
            if (improvementType==EImprovement::VE_Quarry) {
                coinYield++;
            }
            break;
        case EResource::VE_Marble:
            prodYield++;
            coinYield++;
            if (improvementType==EImprovement::VE_Quarry) {
                prodYield--;
                coinYield=coinYield+2;
            }
            break;
        case EResource::VE_Salt:
            foodYield++;
            coinYield++;
            if (improvementType==EImprovement::VE_Quarry) {
                foodYield++;
                prodYield=prodYield-2;
                coinYield=coinYield+2;
            }
            break;
        case EResource::VE_Slate:
            coinYield++;
            if (improvementType==EImprovement::VE_Quarry) {
                prodYield=prodYield-2;
                coinYield=coinYield+3;
            }
            break;
        case EResource::VE_Copper:
            prodYield++;
            if (improvementType==EImprovement::VE_Mine) {
                prodYield=prodYield+2;
            }
            break;
        case EResource::VE_Iron:
            prodYield=prodYield+2;
            if (improvementType==EImprovement::VE_Mine) {
                prodYield=prodYield+3;
            }
            break;
        case EResource::VE_Mithril:
            prodYield=prodYield+3;
            if (improvementType==EImprovement::VE_Mine) {
                prodYield=prodYield+4;
            }
            break;
        case EResource::VE_Gems:
            coinYield=coinYield+2;
            if (improvementType==EImprovement::VE_Mine) {
                coinYield=coinYield+4;
            }
            break;
        case EResource::VE_Gold:
            coinYield++;
            if (improvementType==EImprovement::VE_Mine) {
                coinYield=coinYield+7;
            }
            break;
        case EResource::VE_Silver:
            coinYield++;
            if (improvementType==EImprovement::VE_Mine) {
                coinYield=coinYield+5;
                prodYield++;
            }
            break;
        default:
            break;
    }
    
    //Cities
    if (hasCityDistrictFromCiv!=-1) {
        prodYield++;
        coinYield++;
    }
}