// Fill out your copyright notice in the Description page of Project Settings.

#include "TwelveAngryNodes.h"
#include "UnrealMathUtility.h"
#include <time.h>
#include "C_MapGenerator.h"

//Empty constructor to be overriden in blueprint if necessary
AC_MapGenerator::AC_MapGenerator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    randomstream = FRandomStream(time(NULL));
}

/*
 HEX ARRAY FORMATTED AS A 1D ARRAY :
 
 (0,1,2,3,4,5,6,...)
 --->i
 
 BUT MODELISED AS 2D GRID :
 
 y      _ _ _ _ _ _
 ^    /_/_/_/_/_/_/
 |   /_/_/_/_/_/_/
 |  /6/_/_/_/_/_/
 | /0/1/2/3/4/5/
 --->x
 
 etc.
 */

//Returns x value from a 1D array
int32 AC_MapGenerator::getX(int32 i){
    return (i % mapsizex);
}

//Returns y value from a 1D array
int32 AC_MapGenerator::getY(int32 i){
    return (i / mapsizex);
}

int32 AC_MapGenerator::getNeighbor(int32 index, int32 dir)
{
    return getNeighbor(getX(index), getY(index), dir);
}


//From a position on the hex grid x and y, returns the index of the 'dir' neighbor (in 1D arrays) : 1=right, 2=topright, 3=topleft, 4=left, 5=botleft, 6=botright
//Assumes a cylinder wrap, and so uses the xy notation for simplicity of finding special cases
//Returns -1 if out of map
//If dir is bigger than 6, subtracts 6 from dir and calls itself with new dir; if smaller than 1, adds 6 to dir and calls itself with new dir
int32 AC_MapGenerator::getNeighbor(int32 x, int32 y, int32 dir)
{
    int32 ArrayPos=x+y*mapsizex;
    switch (dir) {
        case 1://going right
            if (x==(mapsizex-1)) {
                ArrayPos=ArrayPos-mapsizex+1;
            }
            else{
                ArrayPos++;
            }
            break;
        case 2://going topright
            if (y==(mapsizey-1)) {
                ArrayPos=-1; //going out top
            }
            else{
                ArrayPos=ArrayPos+mapsizex;
            }
            break;
        case 3://going topleft
            if (y==(mapsizey-1)){
                ArrayPos=-1; //going out top
            }
            else if (x==0) {
                ArrayPos=ArrayPos+2*mapsizex-1;
            }
            else{
                ArrayPos=ArrayPos+mapsizex-1;
            }
            break;
        case 4://going left
            if (x==0) {
                ArrayPos=ArrayPos+mapsizex-1;
            }
            else{
                ArrayPos--;
            }
            break;
        case 5://going botleft
            if (y==0) {
                ArrayPos=-1; //going out bot
            }
            else{
                ArrayPos=ArrayPos-mapsizex;
            }
            break;
        case 6://going botright
            if (y==0) {
                ArrayPos=-1; //going out bot
            }
            else if (x==(mapsizex-1)) {
                ArrayPos=ArrayPos-2*mapsizex+1;
            }
            else {
                ArrayPos=ArrayPos-mapsizex+1;
            }
            break;
        default:
            if (dir>6) {
                return getNeighbor(x, y, dir-6);
            }
            else if (dir<1) {
                return getNeighbor(x, y, dir+6);
            }
            else {
                ArrayPos=-1;
            }
            break;
    }
    return ArrayPos;
}


//Generates an altitude map for the current instance, in the form of a 1D Array
//Altitudes : 0=water, 1=lowlands, 2=midlands, 3=highlands
void AC_MapGenerator::GenerateAltitudeMap()
{
    int32 mapsize=mapsizex*mapsizey;
    AltitudeMap.SetNum(mapsize);
    
    for (int32 i = 0; i<(mapsize); i++) {
        AltitudeMap[i]=0;
    }
    
    
    //Chain mechanism : A certain number of chains of varying lengths will be placed on the map to elevate the altitude of the map; implementation of chains lower
    int32 const LongChains = 0/*1*/, LengthLong=1000;
    int32 const MediumChains = 1/*3*/, LengthMedium=400;
    int32 const ShortChains = 2, LengthShort=100;
    int32 const TinyChains = 10, LengthTiny=4;
    int32 const NumberOfChains = LongChains+MediumChains+ShortChains+TinyChains;
    int32 Chains[NumberOfChains];
    int32 ChainActual, gridx, gridy;
    int32 nextDir;
    //Chain implementation : each chain starts at a random location and elevates terrain there; the getNeighbor function is then used to move to an adjacent tile and elevate it; loops until the chain is finished, then moves on to the next chain
    for (int32 i=0; i<NumberOfChains; i++) {
        
        //Setting length of chains
        if (i<LongChains) {
            Chains[i]=randomstream.RandRange(LengthLong/2, LengthLong*3/2);
        }
        else if (i<(LongChains+MediumChains)) {
            Chains[i]=randomstream.RandRange(LengthMedium/2, LengthMedium*3/2);
        }
        else if (i<(LongChains+MediumChains+ShortChains)) {
            Chains[i]=randomstream.RandRange(LengthShort/2, LengthShort*3/2);
        }
        else {
            Chains[i]=randomstream.RandRange(LengthTiny/2, LengthTiny*3/2);
        }
        
        //Setting starting point of current chain
        ChainActual=randomstream.RandRange(0, mapsize-1);
        gridx=getX(ChainActual);
        gridy=getY(ChainActual);
        
        for (int32 j=0; j<Chains[i];j++) {
            if ((AltitudeMap[ChainActual])<3) { //Elevate if not too high
                AltitudeMap[ChainActual]++;
            }
            nextDir=randomstream.RandRange(1, 6);
            while (getNeighbor(gridx, gridy, nextDir)==-1) { //Handling out-of-map
                nextDir=randomstream.RandRange(1, 6);
            }
            ChainActual=getNeighbor(gridx, gridy, nextDir);
            gridx=getX(ChainActual);
            gridy=getY(ChainActual);
            
        }
    }
}

//Post-processes the previously generated altitude map so that there are less long land chains joining continents, creating more bays and islands
void AC_MapGenerator::PostProcessLongLandChains() {
    int32 mapsize=mapsizex*mapsizey;
    TArray<int32> LongChains;
    for (int32 i=0; i<mapsize; i++) {
        if (AltitudeMap[i]==1) {
            int32 NumberOfWaterNeighbors=0;
            bool isLongChain=true;
            for (int32 j=0; j<6; j++) {
                int32 Neighbor=getNeighbor(getX(i), getY(i), j+1);
                if (Neighbor != -1) {
                    if (AltitudeMap[Neighbor]==0) {
                        NumberOfWaterNeighbors++;
                    }
                    else if (AltitudeMap[Neighbor]!=1) {
                        isLongChain=false;
                        break;
                    }
                }
            }
            if ((NumberOfWaterNeighbors==4) && isLongChain) {
                LongChains.Push(i);
            }
        }
    }
    for (int32 i=0; i<LongChains.Num(); i++) {
        if (randomstream.RandRange(1, 10) < 8) {
            AltitudeMap[LongChains[i]]--;
        }
    }
}

//Post-processes lakes : removes some of them and elevates with land some others. Also fills up the Lakes array
void AC_MapGenerator::PostProcessLakes() {
    int32 mapsize=mapsizex*mapsizey;
    
    //Finds all lakes on the map
    for (int32 i=0; i<mapsize; i++) {
        if (AltitudeMap[i]==0) {
            if (!CheckIfOcean(i)) {
                bool isAlreadyLake=false;
                for (int32 k=0; k<Lakes.Num(); k++) {
                    if (Lakes[k]->tiles.Contains(i)) {
                        isAlreadyLake=true;
                        break;
                    }
                }
                if (!isAlreadyLake) {
                    WaterBody *potentialLake = new WaterBody();
                    if (CheckIfLake(i, potentialLake)) {
                        FString NewString = FString::FromInt(potentialLake->tiles.Num());
                        Lakes.Push(potentialLake);
                    }
                }
            }
        }
    }
    
    //Removes some lakes
    for (int32 i=Lakes.Num()-1; i>=0; i--) {
        if ((randomstream.RandRange(0, 8)) <= Lakes[i]->tiles.Num()) {
            for (int32 j=0; j<Lakes[i]->tiles.Num(); j++) {
                AltitudeMap[Lakes[i]->tiles[j]]++;
            }
            Lakes.RemoveAt(i);
        }
    }
    
    //Elevates some lakes
    for (int32 i=0; i<Lakes.Num(); i++) {
        TArray<int32> LakeCoast;
        for (int32 j=0; j<Lakes[i]->tiles.Num(); j++) {
            for (int32 k=0; k<6; k++) {
                int32 current = getNeighbor(getX(Lakes[i]->tiles[j]), getY(Lakes[i]->tiles[j]), k+1);
                if (current!=-1) { //Dealing with out-of-map cases
                    if (AltitudeMap[current]!=0) {
                        if (!(LakeCoast.Contains(current))) {
                            LakeCoast.Push(current);
                        }
                    }
                }
            }
        }
        int32 Higher=0;
        int32 ReallyHigher=0;
        for (int32 j=0; j<LakeCoast.Num(); j++) {
            if (AltitudeMap[LakeCoast[j]]==2) {
                Higher++;
            }
            if (AltitudeMap[LakeCoast[j]]==3) {
                ReallyHigher++;
            }
        }
        if ((Higher+ReallyHigher)>(2*(Lakes[i]->tiles.Num()))) {
            Lakes[i]->altitude++;//We store the eventual altitude of the lake here; will be actualized in AltitudeMap later, in GenerateTerrainType function
            for (int32 j=0; j<LakeCoast.Num(); j++) {
                if (AltitudeMap[LakeCoast[j]]<2) {
                    AltitudeMap[LakeCoast[j]]=2; //Set lake coasts to same level as lake
                }
            }
            if (ReallyHigher>(3*(Lakes[i]->tiles.Num()))) {
                Lakes[i]->altitude++;//We store the eventual altitude of the lake here; will be actualized in AltitudeMap later, in GenerateTerrainType function
                for (int32 j=0; j<LakeCoast.Num(); j++) {
                    if (AltitudeMap[LakeCoast[j]]<3) {
                        AltitudeMap[LakeCoast[j]]=3; //Set lake coasts to same level as lake
                    }
                }
            }
        }
    }
}

//Checks if a given waterbody is a lake; a lake has an area equal to or less than 8 tiles
bool AC_MapGenerator::CheckIfLake(int32 start, WaterBody *lake) {
    int32 mapsize=mapsizex*mapsizey;
    TArray<int32> frontier;
    TArray<int32> visited;
    
    frontier.Push(start);
    
    //Check if tile is part of a lake
    while (frontier.Num() != 0) {
        if (!(visited.Num()<8)) {
            return false;//water body is too large to be a lake
        }
        int32 current=frontier.Pop();
        visited.Push(current);
        for (int32 j=1; j<7; j++) {
            int32 actualNeighbor = getNeighbor(getX(current), getY(current), j);
            if (actualNeighbor!=-1) {
                if (AltitudeMap[actualNeighbor]==0) {
                    if ((!visited.Contains(actualNeighbor)) && !frontier.Contains(actualNeighbor)) {
                        frontier.Push(actualNeighbor);
                    }
                }
            }
        }
    }
    //If water body is a lake, fill up a WaterBody element
    for (int32 i=0; i<visited.Num(); i++) {
        lake->tiles.Push(visited[i]);
    }
    lake->altitude=0;
    return true;
}

//Checks if hex "i" is a lake tile, and returns its altitude; returns -1 if not a lake
int32 AC_MapGenerator::CheckIfLakeTile(int32 i) {
    for (int j=0; j<Lakes.Num(); j++) {
        if (Lakes[j]->tiles.Contains(i)) {
            return Lakes[j]->altitude;
        }
    }
    return -1;
}

//Checks whether a given water hex is a deepwater ocean tile
//Returns true if tile and all neighbors are water, false otherwise
bool AC_MapGenerator::CheckIfOcean(int32 i){
    bool isOcean=true; //Considered to be ocean until contrary proven
    if (AltitudeMap[i] != 0) {
        return false;//Tile is not water
    }
    int32 currentNeighbor;
    for (int32 j=1; j<7; j++) {
        currentNeighbor=getNeighbor(getX(i), getY(i), j);
        if (currentNeighbor != -1) {
            if (AltitudeMap[currentNeighbor] != 0) {
                isOcean=false;
                break;
            }
        }
    }
    return isOcean;
}



//Computes an "apparent latitude" for climate based on latitude and elevation
//SUPPOSES AN ODD NUMBER OF ROWS
int32 AC_MapGenerator::getApparentLatitude(int32 index){
    int32 y = getY(index);
    int32 equator = mapsizey/2;
    if (y<=equator) {
        return equator-y+(AltitudeMap[index]-1)*equator/6;//If adjusting elevation, dont forget to adjust also 'maxlatitude' in weight functions
    }
    else {
        return y-equator+(AltitudeMap[index]-1)*equator/6;
    }
}

//Computes real latitude for climate
//SUPPOSES AN ODD NUMBER OF ROWS
int32 AC_MapGenerator::getLatitude(int32 index){
    int32 y = getY(index);
    int32 equator = mapsizey/2;
    if (y<=equator) {
        return equator-y;
    }
    else {
        return y-equator;
    }
}

//Calculates a probability weight to get a snow tile
int32 AC_MapGenerator::getSnowWeight(int32 latitude){
    int32 maxlatitude=(mapsizey/2+1)*7/6;
    if (latitude>maxlatitude*3/4) {
        return 6*(latitude-maxlatitude*3/4);
    }
    else {
        return 0;
    }
}

//Calculates a probability weight to get a tundra tile
int32 AC_MapGenerator::getTundraWeight(int32 latitude){
    int32 maxlatitude=(mapsizey/2+1)*7/6;
    if (latitude>maxlatitude/2) {
        return latitude-maxlatitude/2;
    }
    else {
        return 0;
    }
}

//Calculates a probability weight to get a plain tile
int32 AC_MapGenerator::getPlainWeight(int32 latitude){
    int32 maxlatitude=(mapsizey/2+1)*7/6;
    if (latitude<=maxlatitude/2) {
        return latitude;
    }
    else {
        return maxlatitude-latitude;
    }
}

//Calculates a probability weight to get a grassland tile
int32 AC_MapGenerator::getGrassWeight(int32 latitude){
    int32 maxlatitude=(mapsizey/2+1)*7/6;
    if (latitude<=maxlatitude*2/3) {
        return 2*(maxlatitude*2/3-latitude);
    }
    else {
        return 0;
    }
}

//Generates terrain types : grassland, tundra, snow, etc. Also fills up the LandTiles and WaterTiles arrays. Does not place deserts; these will be placed later when rivers are placed.
void AC_MapGenerator::GenerateTerrainType()
{
    int32 mapsize = mapsizex*mapsizey;
    int32 latitude, applatitude, snowWeight, tundraWeight, plainWeight, grassWeight, totalWeight;
    int32 weightedRandomFactor;
    
    TerrainType.SetNum(mapsize);
    
    for (int32 i=0; i<mapsize; i++) {
        if (AltitudeMap[i]==0) {
            TerrainType[i]=ETerrain::VE_Coast;
            WaterTiles.Push(i);
            if (CheckIfOcean(i)) {
                TerrainType[i]=ETerrain::VE_Ocean;
            }
        }
        else {
            applatitude = getApparentLatitude(i);
            
            snowWeight = getSnowWeight(applatitude);
            tundraWeight = getTundraWeight(applatitude);
            plainWeight = getPlainWeight(applatitude);
            grassWeight = getGrassWeight(applatitude);
            totalWeight = snowWeight + tundraWeight + plainWeight + grassWeight;
            weightedRandomFactor= randomstream.RandRange(0, totalWeight-1);
            if (weightedRandomFactor<grassWeight) {
                TerrainType[i]=ETerrain::VE_Grassland;
            }
            else if (weightedRandomFactor<(grassWeight+plainWeight)){
                TerrainType[i]=ETerrain::VE_Plain;
            }
            else if (weightedRandomFactor<(grassWeight+plainWeight+tundraWeight)){
                TerrainType[i]=ETerrain::VE_Tundra;
            }
            else {
                TerrainType[i]=ETerrain::VE_Snow;
            }
            LandTiles.Push(i);
        }
    }
    
    for (int32 i=0; i<LandTiles.Num(); i++) {
        latitude=getLatitude(LandTiles[i]);
        if (latitude > getLatitude(0)*5/6) {
            TerrainType[LandTiles[i]]=ETerrain::VE_Snow;
        }
    }
    
    //Setting lakes to their correct altitude; also flags the tile as "lake"
    for (int32 i=0; i<Lakes.Num(); i++) {
        for (int32 j=0; j<Lakes[i]->tiles.Num(); j++) {
            AltitudeMap[Lakes[i]->tiles[j]]=Lakes[i]->altitude;
            TerrainType[Lakes[i]->tiles[j]]=ETerrain::VE_Lake;
        }
    }
}

//Utility functions to access river information in blueprint
int32 AC_MapGenerator::getTotalNumberOfRiverSegments() {return Rivers.Num();}
TArray<int32> AC_MapGenerator::getLeftBankArray(int32 i) {return Rivers[i]->LeftBank;}
TArray<int32> AC_MapGenerator::getRightBankArray(int32 i) {return Rivers[i]->RightBank;}
TArray<int32> AC_MapGenerator::getdirArray(int32 i) {return Rivers[i]->dir;}
int32 AC_MapGenerator::getSegAltitude(int32 i) {return Rivers[i]->segAltitude;}
int32 AC_MapGenerator::getSegStart(int32 i) {return Rivers[i]->segStart;}
int32 AC_MapGenerator::getSegEnd(int32 i) {return Rivers[i]->segEnd;}


//Places all rivers on the map
//Must be placed after GenerateTerrainType but before GenerateDeserts
void AC_MapGenerator::BuildRivers(int32 numberOfRivers)
{
    //First part of function checks eligible spots to start a river
    TArray<int32> PotentialStartLeftBank;
    TArray<int32> PotentialStartRightBank;
    TArray<int32> PotentialStartDir;
    for (int32 i=0; i<LandTiles.Num(); i++) {
        for (int32 j=0; j<7; j++) {
            if (CheckIfEligibleRiverStart(LandTiles[i], j)) {
                PotentialStartLeftBank.Push(LandTiles[i]);
                PotentialStartRightBank.Push(getNeighbor(getX(LandTiles[i]), getY(LandTiles[i]), j));
                PotentialStartDir.Push(j);
            }
        }
    }
    
    TArray<int32> StartLeftBank;
    TArray<int32> StartRightBank;
    TArray<int32> StartDir;
    
    //Selects some of the previous eligible spots to actually become river starts
    for (int32 i=0; i<numberOfRivers; i++) {
        if (PotentialStartLeftBank.Num() > 0) {
            int32 selected = randomstream.RandRange(1, PotentialStartLeftBank.Num()) - 1;
            StartLeftBank.Push(PotentialStartLeftBank[selected]);
            StartRightBank.Push(PotentialStartRightBank[selected]);
            StartDir.Push(PotentialStartDir[selected]);
            PotentialStartLeftBank.RemoveAt(selected);
            PotentialStartRightBank.RemoveAt(selected);
            PotentialStartDir.RemoveAt(selected);
        }
        else {
            if (GEngine) {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("no more river spots"));
            }
        }
    }
    
    //Create a river segment for each of the selected spots; each river segment may develop into more segments as the river gains in altitude or forks
    while (StartLeftBank.Num() != 0) {
        BuildRiverSegment(StartLeftBank.Pop(), StartRightBank.Pop(), StartDir.Pop(), 0, true, 0);
    }
    
    //Fills up RiverOn* arrays for pathfinding (and desert placement)
    //First filling with 0s
    int32 mapsize = mapsizex*mapsizey;
    RiverOn1.SetNum(mapsize);
    RiverOn2.SetNum(mapsize);
    RiverOn3.SetNum(mapsize);
    RiverOn4.SetNum(mapsize);
    RiverOn5.SetNum(mapsize);
    RiverOn6.SetNum(mapsize);
    for (int32 i=0; i<mapsize; i++) {
        RiverOn1[i]=0;
        RiverOn2[i]=0;
        RiverOn3[i]=0;
        RiverOn4[i]=0;
        RiverOn5[i]=0;
        RiverOn6[i]=0;
    }
    
    //Then actually filling with pertinent data
    for (int32 i=0; i<Rivers.Num(); i++) {
        for (int32 j=0; j<Rivers[i]->LeftBank.Num(); j++) {
            while (Rivers[i]->dir[j] > 6) {
                Rivers[i]->dir[j]= Rivers[i]->dir[j] - 6;
            }
            while (Rivers[i]->dir[j] < 1) {
                Rivers[i]->dir[j]= Rivers[i]->dir[j] + 6;
            }
            switch (Rivers[i]->dir[j]) {
                case 1:
                    RiverOn1[Rivers[i]->LeftBank[j]]=1;
                    RiverOn4[Rivers[i]->RightBank[j]]=1;
                    break;
                case 2:
                    RiverOn2[Rivers[i]->LeftBank[j]]=1;
                    RiverOn5[Rivers[i]->RightBank[j]]=1;
                    break;
                case 3:
                    RiverOn3[Rivers[i]->LeftBank[j]]=1;
                    RiverOn6[Rivers[i]->RightBank[j]]=1;
                    break;
                case 4:
                    RiverOn4[Rivers[i]->LeftBank[j]]=1;
                    RiverOn1[Rivers[i]->RightBank[j]]=1;
                    break;
                case 5:
                    RiverOn5[Rivers[i]->LeftBank[j]]=1;
                    RiverOn2[Rivers[i]->RightBank[j]]=1;
                    break;
                case 6:
                    RiverOn6[Rivers[i]->LeftBank[j]]=1;
                    RiverOn3[Rivers[i]->RightBank[j]]=1;
                    break;
                default:
                    if (GEngine) {
                        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("HELLO RIVERS"));
                    }
                    break;
            }
        }
    }
}

//Returns true if "i" is already on left bank of a river, false otherwise
bool AC_MapGenerator::CheckifAlreadyInLeftBank(int32 i){
    for (int32 j=0; j<Rivers.Num(); j++) {
        if (Rivers[j]->LeftBank.Contains(i)) {
            return true;
        }
    }
    return false;
}

//Returns true if "i" is already on left bank of a river, false otherwise
bool AC_MapGenerator::CheckifAlreadyInRightBank(int32 i){
    for (int32 j=0; j<Rivers.Num(); j++) {
        if (Rivers[j]->RightBank.Contains(i)) {
            return true;
        }
    }
    return false;
}

//Builds a river segment. A river segment is here understood to be any segment of continuous river between possible ending points : change of altitude, beginning and end of river, fork. Builds an element to put int the Rivers array, then checks if it should put further segments if it ended with a change of altitude (up) or a fork.
//startLeft is the first left bank of the river
//startRight is the first right bank of the river
//startDir is the first direction to go from left to right bank
//initstate is the starting state of the segment : 0=ocean, 1=fork, 2=cascade (altitude change of 1), 3=waterfall (altitude change of 2), 4=lake
//Everything is the same for endings, except 0=source
bool AC_MapGenerator::BuildRiverSegment(int32 startLeft, int32 startRight, int32 startDir, int32 initstate, bool waterStart, int32 waterStartAltitude)
{
    RiverSegment *actualSeg = new RiverSegment(startLeft, startRight, startDir, initstate);
    if (AltitudeMap[startLeft]<=AltitudeMap[startRight]) {
        actualSeg->segAltitude=AltitudeMap[startLeft];
    }
    else {
        actualSeg->segAltitude=AltitudeMap[startRight];
    }
    if (waterStart && (AltitudeMap[startLeft] > (waterStartAltitude+1)) && (AltitudeMap[startRight] > (waterStartAltitude+1))) {//Case where river drops into ocean/lake as a waterfall
        actualSeg->segStart=3;
    }
    Rivers.Push(actualSeg);

    
    bool anotherLoop=true;
    while (anotherLoop) {
        int32 goingTowards=getNeighbor(getX(actualSeg->LeftBank.Last()), getY(actualSeg->LeftBank.Last()), actualSeg->dir.Last()+1);
        if (goingTowards!=-1) {//REDUNDANCY CHECKS :D
            
            if ((CheckifAlreadyInLeftBank(goingTowards) && CheckifAlreadyInRightBank(startLeft)) ||
                (CheckifAlreadyInRightBank(goingTowards) && CheckifAlreadyInLeftBank(startRight))) {
                Rivers.RemoveAt(Rivers.Num()-1);
                return false; //Case where a river can't start where it was supposed to because of another river having already taken its place
            }
            
            int32 goingToLeft=getNeighbor(getX(actualSeg->LeftBank.Last()), getY(actualSeg->LeftBank.Last()), actualSeg->dir.Last()+2);
            int32 goingToRight=getNeighbor(getX(actualSeg->RightBank.Last()), getY(actualSeg->RightBank.Last()), actualSeg->dir.Last()+1);
            int32 nextTurn=0;//1=right, 2=left, 3=random, 4=end, 5=hit a lake
            if (CheckIfLakeTile(goingTowards) != -1) {
                nextTurn=5;
            }
            else if (goingToLeft == -1) {
                if ((((AltitudeMap[goingToRight]>=AltitudeMap[actualSeg->RightBank.Last()]) ||
                    (AltitudeMap[goingToRight]>=AltitudeMap[goingTowards])) &&
                    (!CheckifAlreadyInRightBank(goingToRight)) && (!CheckifAlreadyInRightBank(goingTowards))) ||
                    (CheckIfLakeTile(goingToRight)>=(AltitudeMap[goingTowards]-1)) ||
                    (CheckIfLakeTile(goingToRight)>=(AltitudeMap[actualSeg->RightBank.Last()]-1))) {
                    nextTurn=1;
                }
                else {
                    nextTurn=4;
                }
            }
            else if (goingToRight == -1) {
                if ((((AltitudeMap[goingToLeft]>=AltitudeMap[actualSeg->LeftBank.Last()]) ||
                    (AltitudeMap[goingToLeft]>=AltitudeMap[goingTowards])) &&
                    (!CheckifAlreadyInLeftBank(goingToLeft)) && (!CheckifAlreadyInLeftBank(goingTowards))) ||
                    (CheckIfLakeTile(goingToLeft)>=(AltitudeMap[goingTowards]-1)) ||
                    (CheckIfLakeTile(goingToLeft)>=(AltitudeMap[actualSeg->LeftBank.Last()]-1))) {
                    nextTurn=2;
                }
                else {
                    nextTurn=4;
                }
            }
            else {
                if ((((AltitudeMap[goingToRight]>=AltitudeMap[actualSeg->RightBank.Last()]) ||
                    (AltitudeMap[goingToRight]>=AltitudeMap[goingTowards])) &&
                    (!CheckifAlreadyInRightBank(goingToRight)) && (!CheckifAlreadyInRightBank(goingTowards))) ||
                    (CheckIfLakeTile(goingToRight)>=(AltitudeMap[goingTowards]-1)) ||
                    (CheckIfLakeTile(goingToRight)>=(AltitudeMap[actualSeg->RightBank.Last()]-1))) {
                    if ((((AltitudeMap[goingToLeft]>=AltitudeMap[actualSeg->LeftBank.Last()]) ||
                        (AltitudeMap[goingToLeft]>=AltitudeMap[goingTowards])) &&
                        (!CheckifAlreadyInLeftBank(goingToLeft)) && (!CheckifAlreadyInLeftBank(goingTowards))) ||
                        (CheckIfLakeTile(goingToLeft)>=(AltitudeMap[goingTowards]-1)) ||
                        (CheckIfLakeTile(goingToLeft)>=(AltitudeMap[actualSeg->LeftBank.Last()]-1))) {
                        nextTurn=3;
                    }
                    else {
                        nextTurn=1;
                    }
                }
                else if ((((AltitudeMap[goingToLeft]>=AltitudeMap[actualSeg->LeftBank.Last()]) ||
                         (AltitudeMap[goingToLeft]>=AltitudeMap[goingTowards])) &&
                         (!CheckifAlreadyInLeftBank(goingToLeft)) && (!CheckifAlreadyInLeftBank(goingTowards))) ||
                         (CheckIfLakeTile(goingToLeft)>=(AltitudeMap[goingTowards]-1)) ||
                         (CheckIfLakeTile(goingToLeft)>=(AltitudeMap[actualSeg->LeftBank.Last()]-1))) {
                    nextTurn=2;
                }
                else {
                    nextTurn=4;
                }
            }
            switch (nextTurn) {
                case 1://Turning right
                    if ((AltitudeMap[actualSeg->RightBank.Last()] > actualSeg->segAltitude) &&
                        (AltitudeMap[goingTowards] > actualSeg->segAltitude)) {
                        if (((AltitudeMap[actualSeg->RightBank.Last()]-actualSeg->segAltitude)==2) &&
                            ((AltitudeMap[goingTowards]-actualSeg->segAltitude)==2)) {
                            actualSeg->segEnd=3;
                            BuildRiverSegment(goingTowards, actualSeg->RightBank.Last(), actualSeg->dir.Last()-1, 3, false, 0);
                        }
                        else {
                            actualSeg->segEnd=2;
                            BuildRiverSegment(goingTowards, actualSeg->RightBank.Last(), actualSeg->dir.Last()-1, 2, false, 0);
                        }
                        anotherLoop=false;
                    }
                    else {
                        int32 temp;
                        actualSeg->LeftBank.Push(goingTowards);
                        temp=actualSeg->RightBank.Last();
                        actualSeg->RightBank.Push(temp);
                        temp=actualSeg->dir.Last()-1;
                        actualSeg->dir.Push(temp);
                    }
                    break;
                case 2://Turning left
                    if ((AltitudeMap[actualSeg->LeftBank.Last()] > actualSeg->segAltitude) &&
                        (AltitudeMap[goingTowards] > actualSeg->segAltitude)) {
                        if (((AltitudeMap[actualSeg->LeftBank.Last()]-actualSeg->segAltitude)==2) &&
                            ((AltitudeMap[goingTowards]-actualSeg->segAltitude)==2)) {
                            actualSeg->segEnd=3;
                            BuildRiverSegment(actualSeg->LeftBank.Last(), goingTowards, actualSeg->dir.Last()+1, 3, false, 0);
                        }
                        else {
                            actualSeg->segEnd=2;
                            BuildRiverSegment(actualSeg->LeftBank.Last(), goingTowards, actualSeg->dir.Last()+1, 2, false, 0);
                        }
                        anotherLoop=false;
                    }
                    else {
                        int32 temp;
                        actualSeg->RightBank.Push(goingTowards);
                        temp=actualSeg->LeftBank.Last();
                        actualSeg->LeftBank.Push(temp);
                        temp=actualSeg->dir.Last()+1;
                        actualSeg->dir.Push(temp);
                    }
                    break;
                case 3:
                    if ((AltitudeMap[actualSeg->RightBank.Last()] == AltitudeMap[goingTowards]) &&
                        (AltitudeMap[actualSeg->LeftBank.Last()] == AltitudeMap[goingTowards]) &&
                        (randomstream.RandRange(0, 9) == 0)) {//Forking only allowed when all 3 main tiles at same height, with given probability
                        actualSeg->segEnd=1;
                        bool leftok, rightok;
                        rightok=BuildRiverSegment(goingTowards, actualSeg->RightBank.Last(), actualSeg->dir.Last()-1, 1, false, 0);//Building right fork seg
                        leftok=BuildRiverSegment(actualSeg->LeftBank.Last(), goingTowards, actualSeg->dir.Last()+1, 1, false, 0);//Building left fork seg
                        if (leftok && rightok) {
                            anotherLoop=false;
                        }
                        else if (leftok || rightok) {
                            Rivers.RemoveAt(Rivers.Num()-1);//Failed fork (often because right branch took too much space)
                        }
                    }
                    else if (randomstream.RandRange(0, 1) == 1) {//Turning right
                        if ((AltitudeMap[actualSeg->RightBank.Last()] > actualSeg->segAltitude) &&
                            (AltitudeMap[goingTowards] > actualSeg->segAltitude)) {
                            if (((AltitudeMap[actualSeg->RightBank.Last()]-actualSeg->segAltitude)==2) &&
                                ((AltitudeMap[goingTowards]-actualSeg->segAltitude)==2)) {
                                actualSeg->segEnd=3;
                                BuildRiverSegment(goingTowards, actualSeg->RightBank.Last(), actualSeg->dir.Last()-1, 3, false, 0);
                            }
                            else {
                                actualSeg->segEnd=2;
                                BuildRiverSegment(goingTowards, actualSeg->RightBank.Last(), actualSeg->dir.Last()-1, 2, false, 0);
                            }
                            anotherLoop=false;
                        }
                        else {
                            int32 temp;
                            actualSeg->LeftBank.Push(goingTowards);
                            temp=actualSeg->RightBank.Last();
                            actualSeg->RightBank.Push(temp);
                            temp=actualSeg->dir.Last()-1;
                            actualSeg->dir.Push(temp);
                        }
                    }
                    else {//Turning left
                        if ((AltitudeMap[actualSeg->LeftBank.Last()] > actualSeg->segAltitude) &&
                            (AltitudeMap[goingTowards] > actualSeg->segAltitude)) {
                            if (((AltitudeMap[actualSeg->LeftBank.Last()]-actualSeg->segAltitude)==2) &&
                                ((AltitudeMap[goingTowards]-actualSeg->segAltitude)==2)) {
                                actualSeg->segEnd=3;
                                BuildRiverSegment(actualSeg->LeftBank.Last(), goingTowards, actualSeg->dir.Last()+1, 3, false, 0);
                            }
                            else {
                                actualSeg->segEnd=2;
                                BuildRiverSegment(actualSeg->LeftBank.Last(), goingTowards, actualSeg->dir.Last()+1, 2, false, 0);
                            }
                            anotherLoop=false;
                        }
                        else {
                            int32 temp;
                            actualSeg->RightBank.Push(goingTowards);
                            temp=actualSeg->LeftBank.Last();
                            actualSeg->LeftBank.Push(temp);
                            temp=actualSeg->dir.Last()+1;
                            actualSeg->dir.Push(temp);
                        }
                    }
                    break;
                case 4:
                    anotherLoop=false;
                    actualSeg->segEnd=0;
                    break;
                case 5:
                    anotherLoop=false;
                    actualSeg->segEnd=4;/*
                    for (int32 i=0; i<Lakes.Num(); i++) {
                        if (Lakes[i]->tiles.Contains(goingTowards)) {//Finding which Lake we connected
                            TArray<int32> EligibleLakeLeftStart;
                            TArray<int32> EligibleLakeRightStart;
                            TArray<int32> EligibleLakeDir;
                            for (int32 j=0; j<Lakes[i]->tiles.Num(); j++) {
                                for (int32 k=1; k<7; k++) {
                                    int32 current=getNeighbor(getX(Lakes[i]->tiles[j]), getY(Lakes[i]->tiles[j]), k);
                                    if (current!=-1) {
                                        if (!EligibleLakeLeftStart.Contains(current)) {
                                            for (int32 l=1; l<7; l++) {
                                                if (CheckIfEligibleRiverLakeStart(current, l, i)) {
                                                    EligibleLakeLeftStart.Push(current);
                                                    EligibleLakeRightStart.Push(getNeighbor(getX(current), getY(current), l));
                                                    EligibleLakeDir.Push(l);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            FString NewString = FString::FromInt(EligibleLakeLeftStart.Num());
                            
                            for (int32 j=0; j<2; j++) {//3 attempts to place a river; might result in 0-3 new segments
                                if ((EligibleLakeLeftStart.Num() > 0) && (randomstream.RandRange(0,EligibleLakeLeftStart.Num())) > 0) {
                                    int32 selected = randomstream.RandRange(1, EligibleLakeLeftStart.Num()) - 1;
                                    BuildRiverSegment(EligibleLakeLeftStart[selected], EligibleLakeRightStart[selected], EligibleLakeDir[selected], 0, true, actualSeg->segAltitude);
                                    EligibleLakeLeftStart.RemoveAt(selected);
                                    EligibleLakeRightStart.RemoveAt(selected);
                                    EligibleLakeDir.RemoveAt(selected);
                                }
                            }
                        }
                    }*/
                    break;
                  
                default:
                    anotherLoop=false;
                    actualSeg->segEnd=0;
                    break;
            }
            if (anotherLoop) {
                if (randomstream.RandRange(1, 100) > 87) {
                    anotherLoop=false;
                    actualSeg->segEnd=0;
                }
            }
        }
    }
    return true;
}

//Checks if, given a starting hex "i" and a direction "dir", this side of a coastal hex could start a river; requires both starting hexes to be same height. Will also return false if hex is not a coastal hex or if hex is water. Note that this only checks if the tile is on the left bank, so there will be only 1 pairing left/right bank.
bool AC_MapGenerator::CheckIfEligibleRiverStart(int32 i, int32 dir)
{
    if ((TerrainType[i]==ETerrain::VE_Coast) || (TerrainType[i]==ETerrain::VE_Ocean) || (TerrainType[i]==ETerrain::VE_Lake)) {
        return false;//hex is water
    }
    int32 topdir=dir+1;
    int32 botdir=dir-1;
    
    int32 current=getNeighbor(getX(i), getY(i), dir);
    int32 topcurrent=getNeighbor(getX(i), getY(i), topdir);
    int32 botcurrent=getNeighbor(getX(i), getY(i), botdir);
    
    if ((current==-1) || (topcurrent==-1) || (botcurrent==-1)) {//This is probably redundant, but better be safe
        return false;//side of map -> nope
    }
    
    if (TerrainType[botcurrent]==ETerrain::VE_Coast) {
        if (TerrainType[current]!=ETerrain::VE_Coast) {
            if (AltitudeMap[current] == AltitudeMap[i]) {
                if (TerrainType[topcurrent]!=ETerrain::VE_Coast) {
                    if (TerrainType[topcurrent]!=ETerrain::VE_Lake) {
                        if (AltitudeMap[topcurrent]>=AltitudeMap[current]) {
                            return true;//sea land land (higher or same height)
                        }
                        else {
                            return false;//sea land land (lower) -> nope
                        }
                    }
                    else {
                        if ((AltitudeMap[topcurrent]-AltitudeMap[current]) == -1) {
                            return true;//sea land lake (same height)
                        }
                        else {
                            if (AltitudeMap[topcurrent]-AltitudeMap[i] == -1) {
                                return true;//sea land lake (same height as "i" hex)
                            }
                            else {
                                return false;//sea land lake (lower) -> nope
                            }
                        }
                    }
                }
                else {
                    return false;//sea land sea -> nope
                }
            }
            else {
                return false;//starting hexes not at same height
            }
        }
        else {
            return false;//sea sea -> nope
        }
    }
    return false;//bot is not water
}

//Checks if, given a starting hex "i" and a direction "dir", this side of a lakeside hex next to a specific lake "lake" could start a river; requires both starting hexes to be same height. Will also return false if hex is not a lakeside hex or if hex is water. Note that this only checks if the tile is on the left bank, so there will be only 1 pairing left/right bank.
bool AC_MapGenerator::CheckIfEligibleRiverLakeStart(int32 i, int32 dir, int32 lake)
{
    if ((TerrainType[i]==ETerrain::VE_Coast) || (TerrainType[i]==ETerrain::VE_Ocean) || (TerrainType[i]==ETerrain::VE_Lake)) {
        return false;//hex is water
    }
    int32 topdir=dir+1;
    int32 botdir=dir-1;
    
    int32 current=getNeighbor(getX(i), getY(i), dir);
    int32 topcurrent=getNeighbor(getX(i), getY(i), topdir);
    int32 botcurrent=getNeighbor(getX(i), getY(i), botdir);
    
    if ((current==-1) || (topcurrent==-1) || (botcurrent==-1)) {//This is probably redundant, but better be safe
        return false;//side of map -> nope
    }

    if (TerrainType[botcurrent]==ETerrain::VE_Lake) {
        bool isGoodLake=false;
        if (Lakes[lake]->tiles.Contains(botcurrent)) {
            isGoodLake=true;
        }
        if (isGoodLake) {
            if (TerrainType[current]!=ETerrain::VE_Lake) {
                if (AltitudeMap[current] == AltitudeMap[i]) {
                    if (TerrainType[topcurrent]!=ETerrain::VE_Coast) {
                        if (TerrainType[topcurrent]!=ETerrain::VE_Lake) {
                            if (AltitudeMap[topcurrent]>=AltitudeMap[current]) {
                                return true;//lake land land (higher or same height)
                            }
                            else {
                                return false;//lake land land (lower) -> nope
                            }
                        }
                        else {
                            if ((AltitudeMap[topcurrent]-AltitudeMap[current]) == -1) {
                                return true;//lake land lake (same height)
                            }
                            else {
                                if (AltitudeMap[topcurrent]-AltitudeMap[i] == -1) {
                                    return true;//lake land lake (same height as "i" hex)
                                }
                                else {
                                    return false;//lake land lake (lower) -> nope
                                }
                            }
                        }
                    }
                    else {
                        return false;//lake land sea -> nope
                    }
                }
                else {
                    return false;//starting hexes not same height
                }
            }
            else {
                return false;//lake lake -> nope
            }
        }
    }
    return false;//bot is not lake
}

//Fills up the freshWaterTiles array; 0 is no fresh water, 1 is next to lake, 2 is next to river; river overrides lake. Needs rivers and lakes to be placed so need to be placed after BuildRivers and before GenerateDeserts
void AC_MapGenerator::CheckFreshWater()
{
    freshWater.SetNum(mapsizex*mapsizey);
    for (int32 i=0; i<(mapsizex*mapsizey); i++) {
        int32 freshWaterType=0;
        if ((TerrainType[i]!=ETerrain::VE_Coast) || (TerrainType[i]!=ETerrain::VE_Lake) || (TerrainType[i]!=ETerrain::VE_Ocean)) {//if tile is not water, then
            for (int32 j=1; j<7; j++) {
                int32 current = getNeighbor(getX(i), getY(i), j);
                if (current != -1) {
                    if (TerrainType[current] == ETerrain::VE_Lake) {//check if it's next to a lake
                        freshWaterType=1;
                    }
                }
            }
            if ((RiverOn1[i]==1) || (RiverOn2[i]==1) || (RiverOn3[i]==1) || (RiverOn4[i]==1) || (RiverOn5[i]==1) || (RiverOn6[i]==1)) {//or to a river
                freshWaterType=2;
            }
        }
        freshWater[i]=freshWaterType;
    }
}

//Internal function; returns false if tile is not next to a water tile or a river
bool AC_MapGenerator::CheckIfTileIsNextToWater(int32 index)
{
    for (int32 i=1; i<7; i++) {
        int32 current = getNeighbor(getX(index), getY(index), i);
        if (current != -1) {
            if ((TerrainType[i]==ETerrain::VE_Coast) || (TerrainType[i]==ETerrain::VE_Lake) || (TerrainType[i]==ETerrain::VE_Ocean)) {
                return true;
            }
        }
    }
    if ((RiverOn1[index]==1) || (RiverOn2[index]==1) || (RiverOn3[index]==1) || (RiverOn4[index]==1) || (RiverOn5[index]==1) || (RiverOn6[index]==1)) {
        return true;
    }
    return false;
}

//Generates deserts on the map
//Uses TerrainType info and river positioning to place desert "seeds" far from water and not on snow, so needs to be placed after BuildRivers bet before SetTransitions
void AC_MapGenerator::GenerateDeserts()
{
    TArray<int32> potentialSeeds;
    for (int32 i=0; i<LandTiles.Num(); i++) {
        if ((CheckIfTileIsNextToWater(LandTiles[i])) && (TerrainType[LandTiles[i]] != ETerrain::VE_Snow)) {//Tile is not next to water and is not snow
            bool neighborsHaveNoWater=true;
            for (int32 j=1; j<7; j++) {
                int32 current= getNeighbor(getX(LandTiles[i]), getY(LandTiles[i]), j);
                if (current != -1) {
                    if (freshWater[current] != 0) {//Tile's neighbors are not next to water either
                        neighborsHaveNoWater=false;
                        break;
                    }
                }
            }
            if (neighborsHaveNoWater) {
                potentialSeeds.Push(LandTiles[i]);
            }
        }
    }
    
    for (int32 i=potentialSeeds.Num()-1; i>=0; i--) {//Removing approx 3/4 of desert seeds
        if (randomstream.RandRange(0, 3) != 0) {
            potentialSeeds.RemoveAt(i);
        }
    }
    
    int32 ChainLength = 6;
    TArray<int32> Chains;
    Chains.SetNum(potentialSeeds.Num());
    int32 ChainActual, gridx, gridy;
    int32 nextDir;

    //Markov chains similar to the ones used to generate altitude map; starts somewhere and then moves randomly around for a randomized length, transforming every land tile to desert (except snow)
    for (int32 i=0; i<potentialSeeds.Num(); i++) {
        Chains[i]=randomstream.RandRange(1, ChainLength*2);
        
        ChainActual=potentialSeeds[i];
        gridx=getX(ChainActual);
        gridy=getY(ChainActual);
        
        for (int32 j=0; j<Chains[i]; j++) {
            if ((TerrainType[ChainActual] != ETerrain::VE_Coast) && (TerrainType[ChainActual] != ETerrain::VE_Snow)) { //If terrain is not water or snow, set to desert
                TerrainType[ChainActual]=ETerrain::VE_Desert;
            }
            nextDir=randomstream.RandRange(1, 6);
            while (getNeighbor(gridx, gridy, nextDir) == -1) {//Handling out-of-map cases
                nextDir=randomstream.RandRange(1, 6);
            }
            ChainActual=getNeighbor(gridx, gridy, nextDir);
            gridx=getX(ChainActual);
            gridy=getY(ChainActual);
        }
    }
}


//Takes in a altitude map and determines where ramps should be placed on every hex in the map, in the form of 2 1D arrays. See .h for hex ramp types, rotations are multiples of +60 degrees (0,1,2,3,4,5)
//Uses unreduced AltitudeMap and TerrainType arrays (to check if terrain is water), so has to be placed between GenerateTerrainType and ReduceLandAltitude
void AC_MapGenerator::GetHexTypesAndRotations()
{
    int32 mapsize = mapsizex*mapsizey;
    int32 x, y;
    
    int32 currentRotation, additionalCoastRotation;
    int32 arrayPosNeighbor;
    
    int32 temp;
    int32 Ramps[6];
    int32 CliffCoast[6];
    bool Ocean[6];
    
    RampType.SetNum(mapsize);
    CoastType.SetNum(mapsize);
    OceanCoastType.SetNum(mapsize);
    RampRotation.SetNum(mapsize);
    CoastRotation.SetNum(mapsize);
    
    //Determining the type and rotation of each tile from the Ramps array
    for (int32 i=0; i<mapsize; i++) {
 
        //Determining configuration of neighbors for position of ramps/coasts
        x=getX(i);
        y=getY(i);
        for (int32 j=0; j<6; j++) {
            CliffCoast[j]=0;
            Ocean[j]=false;
            arrayPosNeighbor = getNeighbor(x, y, j+1);
            if (arrayPosNeighbor == -1) {//Dealing with out-of-map cases
                Ramps[j]=0;
            }
            else if ((AltitudeMap[arrayPosNeighbor]-AltitudeMap[i]) == 1) {
                Ramps[j]=1;
            }
            else if ((TerrainType[i] == ETerrain::VE_Coast) || (TerrainType[i] == ETerrain::VE_Lake)) {//For cases of coasts next to cliffs
                if ((AltitudeMap[arrayPosNeighbor]-AltitudeMap[i]) > 1) {
                    Ramps[j]=1;
                    CliffCoast[j]=1;
                }
                else {
                    Ramps[j]=0;
                    Ocean[j]=CheckIfOcean(arrayPosNeighbor);//Checks if neighbor is an ocean hex
                }
            }
            else {
                Ramps[j]=0;
            }
        }
 
        currentRotation=0;
        additionalCoastRotation=0;
        OceanCoastType[i]=0;//By default, all ocean coast types are 0
        
        //Checking 000000 case
        if (Ramps[0]==0 && Ramps[1]==0 && Ramps[2]==0 && Ramps[3]==0 && Ramps[4]==0 && Ramps[5]==0) {
            RampType[i]=0;
            CoastType[i]=0;
            currentRotation=randomstream.RandRange(0, 5);
        }

        //Checking 111111 case
        else if (Ramps[0]==1 && Ramps[1]==1 && Ramps[2]==1 && Ramps[3]==1 && Ramps[4]==1 && Ramps[5]==1) {
            RampType[i]=13;
            CoastType[i]=13;
            
            //Checking cliff profile; no possible ocean profile
            if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3]+CliffCoast[4]+CliffCoast[5])==0) {
                currentRotation=randomstream.RandRange(0, 5);
            }
            else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3]+CliffCoast[4]+CliffCoast[5])==1) {
                CoastType[i]=12;
                if (CliffCoast[0]==1) {
                    additionalCoastRotation=1;
                }
                else if (CliffCoast[1]==1) {
                    additionalCoastRotation=2;
                }
                else if (CliffCoast[2]==1) {
                    additionalCoastRotation=3;
                }
                else if (CliffCoast[3]==1) {
                    additionalCoastRotation=4;
                }
                else if (CliffCoast[4]==1) {
                    additionalCoastRotation=5;
                }
            }
            else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3]+CliffCoast[4]+CliffCoast[5])==2) {
                if (CliffCoast[0]==1) {
                    if (CliffCoast[1]==1) {
                        CoastType[i]=9;
                        additionalCoastRotation=2;
                    }
                    else if (CliffCoast[2]==1) {
                        CoastType[i]=10;
                        additionalCoastRotation=3;
                    }
                    else if (CliffCoast[3]==1) {
                        CoastType[i]=11;
                        additionalCoastRotation=1;
                    }
                    else if (CliffCoast[4]==1) {
                        CoastType[i]=10;
                        additionalCoastRotation=1;
                    }
                    else {
                        CoastType[i]=9;
                        additionalCoastRotation=1;
                    }
                }
                else if (CliffCoast[1]==1) {
                    if (CliffCoast[2]==1) {
                        CoastType[i]=9;
                        additionalCoastRotation=3;
                    }
                    else if (CliffCoast[3]==1) {
                        CoastType[i]=10;
                        additionalCoastRotation=4;
                    }
                    else if (CliffCoast[4]==1) {
                        CoastType[i]=11;
                        additionalCoastRotation=2;
                    }
                    else {
                        CoastType[i]=10;
                        additionalCoastRotation=2;
                    }
                }
                else if (CliffCoast[2]==1) {
                    if (CliffCoast[3]==1) {
                        CoastType[i]=9;
                        additionalCoastRotation=4;
                    }
                    else if (CliffCoast[4]==1) {
                        CoastType[i]=10;
                        additionalCoastRotation=5;
                    }
                    else {
                        CoastType[i]=11;
                    }
                }
                else if (CliffCoast[3]==1) {
                    if (CliffCoast[4]==1) {
                        CoastType[i]=9;
                        additionalCoastRotation=5;
                    }
                    else {
                        CoastType[i]=10;
                    }
                }
                else {
                    CoastType[i]=9;
                }
            }
            else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3]+CliffCoast[4]+CliffCoast[5])==3) {
                if (CliffCoast[0]==0) {
                    if (CliffCoast[1]==0) {
                        if (CliffCoast[2]==0) {
                            CoastType[i]=5;
                        }
                        else if (CliffCoast[3]==0) {
                            CoastType[i]=6;
                        }
                        else if (CliffCoast[4]==0) {
                            CoastType[i]=7;
                        }
                        else {
                            CoastType[i]=5;
                            additionalCoastRotation=5;
                        }
                    }
                    else if (CliffCoast[2]==0) {
                        if (CliffCoast[3]==0) {
                            CoastType[i]=7;
                            additionalCoastRotation=2;
                        }
                        else if (CliffCoast[4]==0) {
                            CoastType[i]=8;
                        }
                        else {
                            CoastType[i]=6;
                            additionalCoastRotation=5;
                        }
                    }
                    else if (CliffCoast[3]==0) {
                        if (CliffCoast[4]==0) {
                            CoastType[i]=6;
                            additionalCoastRotation=3;
                        }
                        else {
                            CoastType[i]=7;
                            additionalCoastRotation=5;
                        }
                    }
                    else {
                        CoastType[i]=5;
                        additionalCoastRotation=4;
                    }
                }
                else if (CliffCoast[1]==0) {
                    if (CliffCoast[2]==0) {
                        additionalCoastRotation=1;
                        if (CliffCoast[3]==0) {
                            CoastType[i]=5;
                        }
                        else if (CliffCoast[4]==0) {
                            CoastType[i]=6;
                        }
                        else {
                            CoastType[i]=7;
                        }
                    }
                    else if (CliffCoast[3]==0) {
                        if (CliffCoast[4]==0) {
                            CoastType[i]=7;
                            additionalCoastRotation=3;
                        }
                        else {
                            CoastType[i]=8;
                            additionalCoastRotation=1;
                        }
                    }
                    else {
                        CoastType[i]=6;
                        additionalCoastRotation=4;
                    }
                }
                else if (CliffCoast[2]==0) {
                    if (CliffCoast[3]==0) {
                        additionalCoastRotation=2;
                        if (CliffCoast[4]==0) {
                            CoastType[i]=5;
                        }
                        else {
                            CoastType[i]=6;
                        }
                    }
                    else {
                        CoastType[i]=7;
                        additionalCoastRotation=4;
                    }
                }
                else {
                    CoastType[i]=5;
                    additionalCoastRotation=3;
                }
            }
            else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3]+CliffCoast[4]+CliffCoast[5])==4) {
                if (CliffCoast[0]==0) {
                    if (CliffCoast[1]==0) {
                        CoastType[i]=2;
                    }
                    else if (CliffCoast[2]==0) {
                        CoastType[i]=3;
                    }
                    else if (CliffCoast[3]==0) {
                        CoastType[i]=4;
                    }
                    else if (CliffCoast[4]==0) {
                        CoastType[i]=3;
                        additionalCoastRotation=4;
                    }
                    else {
                        CoastType[i]=2;
                        additionalCoastRotation=5;
                    }
                }
                else if (CliffCoast[1]==0) {
                    if (CliffCoast[2]==0) {
                        CoastType[i]=2;
                        additionalCoastRotation=1;
                    }
                    else if (CliffCoast[3]==0) {
                        CoastType[i]=3;
                        additionalCoastRotation=1;
                    }
                    else if (CliffCoast[4]==0) {
                        CoastType[i]=4;
                        additionalCoastRotation=1;
                    }
                    else {
                        CoastType[i]=3;
                        additionalCoastRotation=5;
                    }
                }
                else if (CliffCoast[2]==0) {
                    additionalCoastRotation=2;
                    if (CliffCoast[3]==0) {
                        CoastType[i]=2;
                    }
                    else if (CliffCoast[4]==0) {
                        CoastType[i]=3;
                    }
                    else {
                        CoastType[i]=4;
                    }
                }
                else if (CliffCoast[3]==0) {
                    additionalCoastRotation=3;
                    if (CliffCoast[4]==0) {
                        CoastType[i]=2;
                    }
                    else {
                        CoastType[i]=3;
                    }
                }
                else {
                    CoastType[i]=2;
                    additionalCoastRotation=4;
                }
            }
            else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3]+CliffCoast[4]+CliffCoast[5])==5) {
                CoastType[i]=1;
                if (CliffCoast[1]==0) {
                    additionalCoastRotation=1;
                }
                else if (CliffCoast[2]==0) {
                    additionalCoastRotation=2;
                }
                else if (CliffCoast[3]==0) {
                    additionalCoastRotation=3;
                }
                else if (CliffCoast[4]==0) {
                    additionalCoastRotation=4;
                }
                else if (CliffCoast[5]==0) {
                    additionalCoastRotation=5;
                }
            }
            else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3]+CliffCoast[4]+CliffCoast[5])==6) {
                CoastType[i]=0;
            }
        }
        
        //Must account for possible rotations in all other cases
        else {
            for (int32 j=0; j<6; j++) {
                
                //Checking 100000 case
                if (Ramps[0]==1 && Ramps[1]==0 && Ramps[2]==0 && Ramps[3]==0 && Ramps[4]==0 && Ramps[5]==0) {
                    RampType[i]=1;
                    CoastType[i]=1;
                    
                    //Checking cliff profile
                    if (CliffCoast[0]==1) {
                        CoastType[i]=0;
                    }
                    
                    //Checking ocean profile; can be done separately only in this specific case
                    if ((Ocean[2]==true) && (Ocean[3]==false) && (Ocean[4]==false)) {
                        OceanCoastType[i]=1;
                    }
                    if ((Ocean[2]==false) && (Ocean[3]==true) && (Ocean[4]==false)) {
                        OceanCoastType[i]=2;
                    }
                    if ((Ocean[2]==false) && (Ocean[3]==false) && (Ocean[4]==true)) {
                        OceanCoastType[i]=3;
                    }
                    if ((Ocean[2]==true) && (Ocean[3]==true) && (Ocean[4]==false)) {
                        OceanCoastType[i]=4;
                    }
                    if ((Ocean[2]==true) && (Ocean[3]==false) && (Ocean[4]==true)) {
                        OceanCoastType[i]=5;
                    }
                    if ((Ocean[2]==false) && (Ocean[3]==true) && (Ocean[4]==true)) {
                        OceanCoastType[i]=6;
                    }
                    if ((Ocean[2]==true) && (Ocean[3]==true) && (Ocean[4]==true)) {
                        OceanCoastType[i]=7;
                    }
                    break;
                }
                
                //Checking 110000 case
                else if (Ramps[0]==1 && Ramps[1]==1 && Ramps[2]==0 && Ramps[3]==0 && Ramps[4]==0 && Ramps[5]==0) {
                    RampType[i]=2;
                    CoastType[i]=2;
                    
                    //Checking cliff profile
                    if ((CliffCoast[0]+CliffCoast[1])==1) {
                        CoastType[i]=1;
                        if (CliffCoast[1]==0) {
                            additionalCoastRotation=1;
                            //Checking ocean profile
                            if ((Ocean[3]==true) && (Ocean[4]==false)) {
                                OceanCoastType[i]=1;
                            }
                            if ((Ocean[3]==false) && (Ocean[4]==true)) {
                                OceanCoastType[i]=2;
                            }
                            if ((Ocean[3]==true) && (Ocean[4]==true)) {
                                OceanCoastType[i]=4;
                            }
                        }
                        else {
                            //Checking ocean profile
                            if ((Ocean[3]==true) && (Ocean[4]==false)) {
                                OceanCoastType[i]=2;
                            }
                            if ((Ocean[3]==false) && (Ocean[4]==true)) {
                                OceanCoastType[i]=3;
                            }
                            if ((Ocean[3]==true) && (Ocean[4]==true)) {
                                OceanCoastType[i]=6;
                            }
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1])==2) {
                        CoastType[i]=0;
                        //Checking ocean profile
                        if ((Ocean[3]==true) && (Ocean[4]==false)) {
                            OceanCoastType[i]=2;
                        }
                        if ((Ocean[3]==false) && (Ocean[4]==true)) {
                            OceanCoastType[i]=3;
                        }
                        if ((Ocean[3]==true) && (Ocean[4]==true)) {
                            OceanCoastType[i]=6;
                        }
                    }
                    else {
                        //Checking ocean profile
                        if ((Ocean[3]==true) && (Ocean[4]==false)) {
                            OceanCoastType[i]=2;
                        }
                        if ((Ocean[3]==false) && (Ocean[4]==true)) {
                            OceanCoastType[i]=3;
                        }
                        if ((Ocean[3]==true) && (Ocean[4]==true)) {
                            OceanCoastType[i]=6;
                        }
                    }
                    break;
                }
                
                //Checking 101000 case
                else if (Ramps[0]==1 && Ramps[1]==0 && Ramps[2]==1 && Ramps[3]==0 && Ramps[4]==0 && Ramps[5]==0) {
                    RampType[i]=3;
                    CoastType[i]=3;
                    
                    //Checking cliff profile
                    if ((CliffCoast[0]+CliffCoast[2])==1) {
                        CoastType[i]=1;
                        if (CliffCoast[2]==0) {
                            additionalCoastRotation=2;
                            //Checking ocean profile
                            if (Ocean[4]==true) {
                                OceanCoastType[i]=1;
                            }
                        }
                        else {
                            //Checking ocean profile
                            if (Ocean[4]==true) {
                                OceanCoastType[i]=3;
                            }
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[2])==2) {
                        CoastType[i]=0;
                        //Checking ocean profile
                        if (Ocean[4]==true) {
                            OceanCoastType[i]=3;
                        }
                    }
                    else {
                        //Checking ocean profile
                        if (Ocean[4]==true) {
                            OceanCoastType[i]=3;
                        }
                    }
                    break;
                }
                
                //Checking 100100 case
                else if (Ramps[0]==1 && Ramps[1]==0 && Ramps[2]==0 && Ramps[3]==1 && Ramps[4]==0 && Ramps[5]==0) {
                    RampType[i]=4;
                    CoastType[i]=4;
                    
                    //Checking cliff profile; no possible ocean profile
                    if ((CliffCoast[0]+CliffCoast[3])==1) {
                        CoastType[i]=1;
                        if (CliffCoast[3]==0) {
                            additionalCoastRotation=3;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[3])==2) {
                        CoastType[i]=0;
                    }
                    break;
                }
                
                //Checking 111000 case
                else if (Ramps[0]==1 && Ramps[1]==1 && Ramps[2]==1 && Ramps[3]==0 && Ramps[4]==0 && Ramps[5]==0) {
                    RampType[i]=5;
                    CoastType[i]=5;
                    
                    //Checking cliff profile
                    if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2])==1) {
                        if (CliffCoast[0]==1) {
                            CoastType[i]=2;
                            additionalCoastRotation=1;
                            //Checking ocean profile
                            if (Ocean[4]==true) {
                                OceanCoastType[i]=2;
                            }
                        }
                        else if (CliffCoast[1]==1) {
                            CoastType[i]=3;
                            //Checking ocean profile
                            if (Ocean[4]==true) {
                                OceanCoastType[i]=3;
                            }
                        }
                        else {
                            CoastType[i]=2;
                            //Checking ocean profile
                            if (Ocean[4]==true) {
                                OceanCoastType[i]=3;
                            }
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2])==2) {
                        CoastType[i]=1;
                        if (CliffCoast[1]==0) {
                            additionalCoastRotation=1;
                            //Checking ocean profile
                            if (Ocean[4]==true) {
                                OceanCoastType[i]=2;
                            }
                        }
                        else if (CliffCoast[2]==0) {
                            additionalCoastRotation=2;
                            //Checking ocean profile
                            if (Ocean[4]==true) {
                                OceanCoastType[i]=1;
                            }
                        }
                        else {
                            //Checking ocean profile
                            if (Ocean[4]==true) {
                                OceanCoastType[i]=3;
                            }
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2])==3) {
                        CoastType[i]=0;
                        //Checking ocean profile
                        if (Ocean[4]==true) {
                            OceanCoastType[i]=3;
                        }
                    }
                    else {
                        //Checking ocean profile
                        if (Ocean[4]==true) {
                            OceanCoastType[i]=3;
                        }
                    }
                    break;
                }
                
                //Checking 110100 case
                else if (Ramps[0]==1 && Ramps[1]==1 && Ramps[2]==0 && Ramps[3]==1 && Ramps[4]==0 && Ramps[5]==0) {
                    RampType[i]=6;
                    CoastType[i]=6;
                    
                    //Checking cliff profile; no possible ocean profile
                    if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[3])==1) {
                        if (CliffCoast[0]==1) {
                            CoastType[i]=3;
                            additionalCoastRotation=1;
                        }
                        else if (CliffCoast[1]==1) {
                            CoastType[i]=4;
                        }
                        else {
                            CoastType[i]=2;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[3])==2) {
                        CoastType[i]=1;
                        if (CliffCoast[1]==0) {
                            additionalCoastRotation=1;
                        }
                        else if (CliffCoast[3]==0) {
                            additionalCoastRotation=3;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[3])==3) {
                        CoastType[i]=0;
                    }
                    break;
                }
                
                //Checking 110010 case
                else if (Ramps[0]==1 && Ramps[1]==1 && Ramps[2]==0 && Ramps[3]==0 && Ramps[4]==1 && Ramps[5]==0) {
                    RampType[i]=7;
                    CoastType[i]=7;
                    
                    //Checking cliff profile; no possible ocean profile
                    if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[4])==1) {
                        if (CliffCoast[0]==1) {
                            CoastType[i]=4;
                            additionalCoastRotation=1;
                        }
                        else if (CliffCoast[1]==1) {
                            CoastType[i]=3;
                            additionalCoastRotation=4;
                        }
                        else {
                            CoastType[i]=2;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[4])==2) {
                        CoastType[i]=1;
                        if (CliffCoast[1]==0) {
                            additionalCoastRotation=1;
                        }
                        else if (CliffCoast[4]==0) {
                            additionalCoastRotation=4;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[4])==3) {
                        CoastType[i]=0;
                    }
                    break;
                }
                
                //Checking 101010 case
                else if (Ramps[0]==1 && Ramps[1]==0 && Ramps[2]==1 && Ramps[3]==0 && Ramps[4]==1 && Ramps[5]==0) {
                    RampType[i]=8;
                    CoastType[i]=8;
                    
                    //Checking cliff profile; no possible ocean profile
                    if ((CliffCoast[0]+CliffCoast[2]+CliffCoast[4])==1) {
                        CoastType[i]=3;
                        if (CliffCoast[0]==1) {
                            additionalCoastRotation=2;
                        }
                        else if (CliffCoast[2]==1) {
                            additionalCoastRotation=4;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[2]+CliffCoast[4])==2) {
                        CoastType[i]=1;
                        if (CliffCoast[2]==0) {
                            additionalCoastRotation=2;
                        }
                        else if (CliffCoast[4]==0) {
                            additionalCoastRotation=4;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[2]+CliffCoast[4])==3) {
                        CoastType[i]=0;
                    }
                    break;
                }
                
                //Checking 111100 case
                else if (Ramps[0]==1 && Ramps[1]==1 && Ramps[2]==1 && Ramps[3]==1 && Ramps[4]==0 && Ramps[5]==0) {
                    RampType[i]=9;
                    CoastType[i]=9;
                    
                    //Checking cliff profile; no possible ocean profile
                    if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3])==1) {
                        if (CliffCoast[0]==1) {
                            CoastType[i]=5;
                            additionalCoastRotation=1;
                        }
                        else if (CliffCoast[1]==1) {
                            CoastType[i]=7;
                            additionalCoastRotation=2;
                        }
                        else if (CliffCoast[2]==1) {
                            CoastType[i]=6;
                        }
                        else {
                            CoastType[i]=5;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3])==2) {
                        if (CliffCoast[0]==0) {
                            if (CliffCoast[1]==0) {
                                CoastType[i]=2;
                            }
                            else if (CliffCoast[2]==0) {
                                CoastType[i]=3;
                            }
                            else {
                                CoastType[i]=4;
                            }
                        }
                        else if (CliffCoast[1]==0) {
                            additionalCoastRotation=1;
                            if (CliffCoast[2]==0) {
                                CoastType[i]=2;
                            }
                            else {
                                CoastType[i]=3;
                            }
                        }
                        else {
                            CoastType[i]=2;
                            additionalCoastRotation=2;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3])==3) {
                        CoastType[i]=1;
                        if (CliffCoast[1]==0) {
                            additionalCoastRotation=1;
                        }
                        else if (CliffCoast[2]==0) {
                            additionalCoastRotation=2;
                        }
                        else if (CliffCoast[3]==0) {
                            additionalCoastRotation=3;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3])==4) {
                        CoastType[i]=0;
                    }
                    break;
                }
                
                //Checking 111010 case
                else if (Ramps[0]==1 && Ramps[1]==1 && Ramps[2]==1 && Ramps[3]==0 && Ramps[4]==1 && Ramps[5]==0) {
                    RampType[i]=10;
                    CoastType[i]=10;
                    
                    //Checking cliff profile; no possible ocean profile
                    if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[4])==1) {
                        if (CliffCoast[0]==1) {
                            CoastType[i]=6;
                            additionalCoastRotation=1;
                        }
                        else if (CliffCoast[1]==1) {
                            CoastType[i]=8;
                        }
                        else if (CliffCoast[2]==1) {
                            CoastType[i]=7;
                        }
                        else {
                            CoastType[i]=5;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[4])==2) {
                        if (CliffCoast[0]==0) {
                            if (CliffCoast[1]==0) {
                                CoastType[i]=2;
                            }
                            else if (CliffCoast[2]==0) {
                                CoastType[i]=3;
                            }
                            else {
                                CoastType[i]=3;
                                additionalCoastRotation=4;
                            }
                        }
                        else if (CliffCoast[1]==0) {
                            additionalCoastRotation=1;
                            if (CliffCoast[2]==0) {
                                CoastType[i]=2;
                            }
                            else {
                                CoastType[i]=4;
                            }
                        }
                        else {
                            CoastType[i]=3;
                            additionalCoastRotation=2;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[4])==3) {
                        CoastType[i]=1;
                        if (CliffCoast[1]==0) {
                            additionalCoastRotation=1;
                        }
                        else if (CliffCoast[2]==0) {
                            additionalCoastRotation=2;
                        }
                        else if (CliffCoast[4]==0) {
                            additionalCoastRotation=4;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[4])==4) {
                        CoastType[i]=0;
                    }
                    break;
                }
                
                //Checking 110110 case
                else if (Ramps[0]==1 && Ramps[1]==1 && Ramps[2]==0 && Ramps[3]==1 && Ramps[4]==1 && Ramps[5]==0) {
                    RampType[i]=11;
                    CoastType[i]=11;
                    
                    //Checking cliff profile; no possible ocean profile
                    if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[3]+CliffCoast[4])==1) {
                        if (CliffCoast[0]==1) {
                            CoastType[i]=7;
                            additionalCoastRotation=3;
                        }
                        else if (CliffCoast[1]==1) {
                            CoastType[i]=6;
                            additionalCoastRotation=3;
                        }
                        else if (CliffCoast[3]==1) {
                            CoastType[i]=7;
                        }
                        else {
                            CoastType[i]=6;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[3]+CliffCoast[4])==2) {
                        if (CliffCoast[0]==0) {
                            if (CliffCoast[1]==0) {
                                CoastType[i]=2;
                            }
                            else if (CliffCoast[3]==0) {
                                CoastType[i]=4;
                            }
                            else {
                                CoastType[i]=3;
                                additionalCoastRotation=4;
                            }
                        }
                        else if (CliffCoast[1]==0) {
                            additionalCoastRotation=1;
                            if (CliffCoast[3]==0) {
                                CoastType[i]=3;
                            }
                            else {
                                CoastType[i]=4;
                            }
                        }
                        else {
                            CoastType[i]=2;
                            additionalCoastRotation=3;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[3]+CliffCoast[4])==3) {
                        CoastType[i]=1;
                        if (CliffCoast[1]==0) {
                            additionalCoastRotation=1;
                        }
                        else if (CliffCoast[3]==0) {
                            additionalCoastRotation=3;
                        }
                        else if (CliffCoast[4]==0) {
                            additionalCoastRotation=4;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[3]+CliffCoast[4])==4) {
                        CoastType[i]=0;
                    }
                    break;
                }
                
                //Checking 111110 case
                else if (Ramps[0]==1 && Ramps[1]==1 && Ramps[2]==1 && Ramps[3]==1 && Ramps[4]==1 && Ramps[5]==0) {
                    RampType[i]=12;
                    CoastType[i]=12;
                    
                    //Checking cliff profile; no possible ocean profile
                    if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3]+CliffCoast[4])==1) {
                        if (CliffCoast[0]==1) {
                            CoastType[i]=9;
                            additionalCoastRotation=1;
                        }
                        else if (CliffCoast[1]==1) {
                            CoastType[i]=10;
                            additionalCoastRotation=2;
                        }
                        else if (CliffCoast[2]==1) {
                            CoastType[i]=11;
                        }
                        else if (CliffCoast[3]==1) {
                            CoastType[i]=10;
                        }
                        else {
                            CoastType[i]=9;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3]+CliffCoast[4])==2) {
                        if (CliffCoast[0]==1) {
                            if (CliffCoast[1]==1) {
                                CoastType[i]=5;
                                additionalCoastRotation=2;
                            }
                            else if (CliffCoast[2]==1) {
                                CoastType[i]=7;
                                additionalCoastRotation=3;
                            }
                            else if (CliffCoast[3]==1) {
                                CoastType[i]=6;
                                additionalCoastRotation=1;
                            }
                            else {
                                CoastType[i]=5;
                                additionalCoastRotation=1;
                            }
                        }
                        else if (CliffCoast[1]==1) {
                            if (CliffCoast[2]==1) {
                                CoastType[i]=6;
                                additionalCoastRotation=3;
                            }
                            else if (CliffCoast[3]==1) {
                                CoastType[i]=8;
                            }
                            else {
                                CoastType[i]=7;
                                additionalCoastRotation=2;
                            }
                        }
                        else if (CliffCoast[2]==1) {
                            if (CliffCoast[3]==1) {
                                CoastType[i]=7;
                            }
                            else {
                                CoastType[i]=6;
                            }
                        }
                        else {
                            CoastType[i]=5;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3]+CliffCoast[4])==3) {
                        if (CliffCoast[0]==0) {
                            if (CliffCoast[1]==0) {
                                CoastType[i]=2;
                            }
                            else if (CliffCoast[2]==0) {
                                CoastType[i]=3;
                            }
                            else if (CliffCoast[3]==0) {
                                CoastType[i]=4;
                            }
                            else {
                                CoastType[i]=3;
                                additionalCoastRotation=4;
                            }
                        }
                        else if (CliffCoast[1]==0) {
                            additionalCoastRotation=1;
                            if (CliffCoast[2]==0) {
                                CoastType[i]=2;
                            }
                            else if (CliffCoast[3]==0) {
                                CoastType[i]=3;
                            }
                            else {
                                CoastType[i]=4;
                            }
                        }
                        else if (CliffCoast[2]==0) {
                            additionalCoastRotation=2;
                            if (CliffCoast[3]==0) {
                                CoastType[i]=2;
                            }
                            else {
                                CoastType[i]=3;
                            }
                        }
                        else {
                            CoastType[i]=2;
                            additionalCoastRotation=3;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3]+CliffCoast[4])==4) {
                        CoastType[i]=1;
                        if (CliffCoast[1]==0) {
                            additionalCoastRotation=1;
                        }
                        else if (CliffCoast[2]==0) {
                            additionalCoastRotation=2;
                        }
                        else if (CliffCoast[3]==0) {
                            additionalCoastRotation=3;
                        }
                        else if (CliffCoast[4]==0) {
                            additionalCoastRotation=4;
                        }
                    }
                    else if ((CliffCoast[0]+CliffCoast[1]+CliffCoast[2]+CliffCoast[3]+CliffCoast[4])==5) {
                        CoastType[i]=0;
                    }
                    break;
                }
                
                //No match with previous cases, so we rotate by 60 degrees and increase currentRotation by 1
                else {
                    temp = Ramps[0];
                    Ramps[0] = Ramps[1];
                    Ramps[1] = Ramps[2];
                    Ramps[2] = Ramps[3];
                    Ramps[3] = Ramps[4];
                    Ramps[4] = Ramps[5];
                    Ramps[5] = temp;
                    temp = CliffCoast[0];
                    CliffCoast[0] = CliffCoast[1];
                    CliffCoast[1] = CliffCoast[2];
                    CliffCoast[2] = CliffCoast[3];
                    CliffCoast[3] = CliffCoast[4];
                    CliffCoast[4] = CliffCoast[5];
                    CliffCoast[5] = temp;
                    bool tempb;
                    tempb = Ocean[0];
                    Ocean[0] = Ocean[1];
                    Ocean[1] = Ocean[2];
                    Ocean[2] = Ocean[3];
                    Ocean[3] = Ocean[4];
                    Ocean[4] = Ocean[5];
                    Ocean[5] = tempb;
                    currentRotation++;
                }
            }
        }
        RampRotation[i]=currentRotation;
        CoastRotation[i]=currentRotation+additionalCoastRotation;
    }
}

//Reduces altitude of all non-water terrain by 1 so that the lowest land level is at the same level as water; also reduces altitude of all rivers
void AC_MapGenerator::ReduceLandAltitude()
{
    int32 mapsize = mapsizex*mapsizey;
    for (int32 i=0; i<mapsize; i++) {
        if ((TerrainType[i]!=ETerrain::VE_Coast) && (TerrainType[i]!=ETerrain::VE_Ocean) && (TerrainType[i]!=ETerrain::VE_Lake)) {
            AltitudeMap[i]--;
        }
    }
    for (int32 i=0; i<Rivers.Num(); i++) {
        Rivers[i]->segAltitude--;
    }
}

//Places forests on land tiles
void AC_MapGenerator::PlaceForests() {
    int32 mapsize = mapsizex*mapsizey;
    int32 landsize = LandTiles.Num();
    Forests.SetNumZeroed(mapsizex*mapsizey);
    for (int32 i=0; i<landsize; i++) {
        int32 latitude = getApparentLatitude(LandTiles[i]);
        if ((randomstream.RandRange(0, latitude*latitude)) < (latitude + 4)) {
            if ((TerrainType[LandTiles[i]] != ETerrain::VE_Snow) && (TerrainType[LandTiles[i]] != ETerrain::VE_Desert)) {//No forests on snow and deserts
                Forests[LandTiles[i]]=1;
            }
        }
    }
}

//Places resources on tiles; needs to be placed after ReduceLandAltitude.
void AC_MapGenerator::PlaceResources() {
    TArray<int32> LandResourceSpots, WaterResourceSpots;
    resources.SetNum(mapsizex*mapsizey);
    resourceRotations.SetNum(mapsizex*mapsizey);
    
    //Initialize resources to none
    for (int32 i=0; i<(mapsizex*mapsizey); i++) {
        resources[i]=EResource::VE_None;
    }
    
    //Sample land tiles
    for (int32 i=0; i<LandTiles.Num(); i++) {
        resourceRotations[LandTiles[i]] = randomstream.RandRange(0, 5);
        if (randomstream.RandRange(1, 10) == 10) {
            LandResourceSpots.Push(LandTiles[i]);
        }
    }
    
    //Sample water tiles
    for (int32 i=0; i<WaterTiles.Num(); i++) {
        if (randomstream.RandRange(1, 10) == 10) {
            WaterResourceSpots.Push(WaterTiles[i]);
        }
    }
    
    //Place land resources according to the characteristics or the tile
    for (int32 i=0; i<LandResourceSpots.Num(); i++) {
        int32 x = getX(LandResourceSpots[i]);
        int32 y = getY(LandResourceSpots[i]);
        int32 randres;
        
        //QUARRIES
        if (AltitudeMap[LandResourceSpots[i]] == 0) {//Checking for quarry resource spots (bottom of a cliff)
            bool legitQuarrySpot=false;
            for (int32 j=1; j<7; j++) {
                int32 current = getNeighbor(x, y, j);
                if (current != -1) {
                    if (AltitudeMap[current] == 2) {
                        switch (j) {
                            case 1:
                                if (RiverOn1[LandResourceSpots[i]] == 0) legitQuarrySpot=true;
                                break;
                            case 2:
                                if (RiverOn2[LandResourceSpots[i]] == 0) legitQuarrySpot=true;
                                break;
                            case 3:
                                if (RiverOn3[LandResourceSpots[i]] == 0) legitQuarrySpot=true;
                                break;
                            case 4:
                                if (RiverOn4[LandResourceSpots[i]] == 0) legitQuarrySpot=true;
                                break;
                            case 5:
                                if (RiverOn5[LandResourceSpots[i]] == 0) legitQuarrySpot=true;
                                break;
                            case 6:
                                if (RiverOn6[LandResourceSpots[i]] == 0) legitQuarrySpot=true;
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
            if (legitQuarrySpot) {
                switch (TerrainType[LandResourceSpots[i]]) {
                    case ETerrain::VE_Grassland:
                        randres=randomstream.RandRange(1, 5);
                        switch (randres) {
                            case 1:
                                resources[LandResourceSpots[i]]=EResource::VE_Limestone;
                                break;
                            case 2:
                                resources[LandResourceSpots[i]]=EResource::VE_Marble;
                                break;
                            case 3:
                                resources[LandResourceSpots[i]]=EResource::VE_Salt;
                                break;
                            default:
                                resources[LandResourceSpots[i]]=EResource::VE_Slate;
                                break;
                        }
                        break;
                    case ETerrain::VE_Plain:
                        randres=randomstream.RandRange(1, 4);
                        switch (randres) {
                            case 1:
                                resources[LandResourceSpots[i]]=EResource::VE_Limestone;
                                break;
                            case 2:
                                resources[LandResourceSpots[i]]=EResource::VE_Marble;
                                break;
                            default:
                                resources[LandResourceSpots[i]]=EResource::VE_Granite;
                                break;
                        }
                        break;
                    case ETerrain::VE_Tundra:
                        randres=randomstream.RandRange(1, 2);
                        switch (randres) {
                            case 1:
                                resources[LandResourceSpots[i]]=EResource::VE_Limestone;
                                break;
                            case 2:
                                resources[LandResourceSpots[i]]=EResource::VE_Granite;
                                break;
                            default:
                                break;
                        }
                        break;
                    case ETerrain::VE_Snow:
                        resources[LandResourceSpots[i]]=EResource::VE_Limestone;
                        break;
                    case ETerrain::VE_Desert:
                        randres=randomstream.RandRange(1, 2);
                        switch (randres) {
                            case 1:
                                resources[LandResourceSpots[i]]=EResource::VE_Limestone;
                                break;
                            case 2:
                                resources[LandResourceSpots[i]]=EResource::VE_Salt;
                                break;
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
                bool rotationFound = false;
                while (!rotationFound) {
                    randres=randomstream.RandRange(1, 6);
                    int32 current = getNeighbor(x, y, randres);
                    if (current != -1) {
                        if (AltitudeMap[current] == 2) {
                            switch (randres) {
                                case 1:
                                    if (RiverOn1[LandResourceSpots[i]] == 0) {
                                        resourceRotations[LandResourceSpots[i]]=0;
                                        rotationFound=true;
                                    }
                                    break;
                                case 2:
                                    if (RiverOn2[LandResourceSpots[i]] == 0) {
                                        resourceRotations[LandResourceSpots[i]]=1;
                                        rotationFound=true;
                                    }
                                    break;
                                case 3:
                                    if (RiverOn3[LandResourceSpots[i]] == 0) {
                                        resourceRotations[LandResourceSpots[i]]=2;
                                        rotationFound=true;
                                    }
                                    break;
                                case 4:
                                    if (RiverOn4[LandResourceSpots[i]] == 0) {
                                        resourceRotations[LandResourceSpots[i]]=3;
                                        rotationFound=true;
                                    }
                                    break;
                                case 5:
                                    if (RiverOn5[LandResourceSpots[i]] == 0) {
                                        resourceRotations[LandResourceSpots[i]]=4;
                                        rotationFound=true;
                                    }
                                    break;
                                case 6:
                                    if (RiverOn6[LandResourceSpots[i]] == 0) {
                                        resourceRotations[LandResourceSpots[i]]=5;
                                        rotationFound=true;
                                    }
                                    break;
                                default:
                                    break;
                            }
                            {
                                GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, TEXT("placing a cliff quarry resource"));
                            }
                        }
                    }
                }
                continue;//skip the other resource placements and go to next resources
            }
        }
        else if (AltitudeMap[LandResourceSpots[i]] == 2){ //Flat quarry spots on plateaus
            bool legitFlatQuarrySpot=true;
            for (int32 j=1; j<7; j++) {
                int32 current = getNeighbor(x, y, j);
                if (current != -1) {
                    if (AltitudeMap[current] != AltitudeMap[LandResourceSpots[i]]) {
                        legitFlatQuarrySpot=false;
                    }
                }
            }
            if ((RiverOn1[LandResourceSpots[i]] == 1) || (RiverOn2[LandResourceSpots[i]] == 1) || (RiverOn3[LandResourceSpots[i]] == 1) || (RiverOn4[LandResourceSpots[i]] == 1) || (RiverOn5[LandResourceSpots[i]] == 1) || (RiverOn6[LandResourceSpots[i]] == 1)) {
                legitFlatQuarrySpot=false;
            }
            if (randomstream.RandRange(1, 4) != 1) {
                legitFlatQuarrySpot=false;
            }
            if (legitFlatQuarrySpot) {
                switch (TerrainType[LandResourceSpots[i]]) {
                    case ETerrain::VE_Grassland:
                        randres=randomstream.RandRange(1, 5);
                        switch (randres) {
                            case 1:
                                resources[LandResourceSpots[i]]=EResource::VE_Limestone;
                                break;
                            case 2:
                                resources[LandResourceSpots[i]]=EResource::VE_Marble;
                                break;
                            case 3:
                                resources[LandResourceSpots[i]]=EResource::VE_Salt;
                                break;
                            default:
                                resources[LandResourceSpots[i]]=EResource::VE_Slate;
                                break;
                        }
                        break;
                    case ETerrain::VE_Plain:
                        randres=randomstream.RandRange(1, 4);
                        switch (randres) {
                            case 1:
                                resources[LandResourceSpots[i]]=EResource::VE_Limestone;
                                break;
                            case 2:
                                resources[LandResourceSpots[i]]=EResource::VE_Marble;
                                break;
                            default:
                                resources[LandResourceSpots[i]]=EResource::VE_Granite;
                                break;
                        }
                        break;
                    case ETerrain::VE_Tundra:
                        randres=randomstream.RandRange(1, 2);
                        switch (randres) {
                            case 1:
                                resources[LandResourceSpots[i]]=EResource::VE_Limestone;
                                break;
                            case 2:
                                resources[LandResourceSpots[i]]=EResource::VE_Granite;
                                break;
                            default:
                                break;
                        }
                        break;
                    case ETerrain::VE_Snow:
                        resources[LandResourceSpots[i]]=EResource::VE_Limestone;
                        break;
                    case ETerrain::VE_Desert:
                        randres=randomstream.RandRange(1, 2);
                        switch (randres) {
                            case 1:
                                resources[LandResourceSpots[i]]=EResource::VE_Limestone;
                                break;
                            case 2:
                                resources[LandResourceSpots[i]]=EResource::VE_Salt;
                                break;
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, TEXT("placing a quarry resource"));
                }
                continue;//skip the other resource placements and go to next resources
            }
        }
        
        
        resourceRotations[LandResourceSpots[i]] = randomstream.RandRange(0, 5);
        int typerand=randomstream.RandRange(0, 15);
        //MINE RESOURCES
        if (typerand < 3) {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, TEXT("placing a mine resource"));
            }
            
            randres=randomstream.RandRange(0,18);
            if (randres < 4) {
                resources[LandResourceSpots[i]]=EResource::VE_Copper;
            }
            else if (randres < 6) {
                resources[LandResourceSpots[i]]=EResource::VE_Iron;
            }
            else if (randres < 7) {
                resources[LandResourceSpots[i]]=EResource::VE_Mithril;
            }
            else if (randres < 11) {
                resources[LandResourceSpots[i]]=EResource::VE_Gems;
            }
            else if (randres < 15) {
                resources[LandResourceSpots[i]]=EResource::VE_Gold;
            }
            else {
                resources[LandResourceSpots[i]]=EResource::VE_Silver;
            }
        }
        //PLANTATION RESOURCES
        else if (typerand < 6) {
            
        }
        //PASTURE RESOURCES
        else if (typerand < 9) {
            
        }
        //CAMP RESOURCES
        else if (typerand < 11) {
            
        }
        //FARM RESOURCES
        else if (typerand < 15) {
            
        }
        //LUMBERMILL RESOURCES
        else {
            
        }
        
        
    }
}

void AC_MapGenerator::PlaceImprovements() {
    improvements.SetNum(mapsizex*mapsizey);
    for (int32 i=0; i<(mapsizex*mapsizey); i++) {
        improvements[i]=EImprovement::VE_None;
    }
}

//Fills up the StartingSpots array with decent starting spots AT THE MOMENT ITS JUST A RANDOM LAND TILE
void AC_MapGenerator::GetStartingSpots() {
    //StartingSpots.Push(214);
    for (int32 i=0; i<10; i++) {
        StartingSpots.Push(LandTiles[randomstream.RandRange(0, LandTiles.Num()-1)]);
    }
}


void AC_MapGenerator::InitializeGameManager() {
    
    manager->mapsizex=mapsizex;
    manager->mapsizey=mapsizey;
    
    manager->AltitudeMap=TArray<int32>(AltitudeMap);
    manager->TerrainType=TArray<ETerrain>(TerrainType);

    manager->Forests=TArray<int32>(Forests);
    manager->RiverOn1=TArray<int32>(RiverOn1);
    manager->RiverOn2=TArray<int32>(RiverOn2);
    manager->RiverOn3=TArray<int32>(RiverOn3);
    manager->RiverOn4=TArray<int32>(RiverOn4);
    manager->RiverOn5=TArray<int32>(RiverOn5);
    manager->RiverOn6=TArray<int32>(RiverOn6);

    manager->Resources=TArray<EResource>(resources);
    manager->Improvements=TArray<EImprovement>(improvements);
    
    int32 mapsize = mapsizex*mapsizey;
    initialCityDistricts.SetNum(mapsize);
    for (int32 i=0; i<mapsize; i++) {
        initialCityDistricts[i]=-1;
    }
    manager->CityDistricts=TArray<int32>(initialCityDistricts);
    
    manager->StartingSpots=TArray<int32>(StartingSpots);
    
    manager->PrimaryHexArray=TArray<AC_HexTile*>(PrimaryHexArray);
    manager->PositiveTwinHexArray=TArray<AC_HexTile*>(PositiveTwinHexArray);
    manager->NegativeTwinHexArray=TArray<AC_HexTile*>(NegativeTwinHexArray);
    manager->hasPositiveTwin=TArray<bool>(hasPositiveTwin);
    manager->hasNegativeTwin=TArray<bool>(hasNegativeTwin);
}









