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


//From a position on the hex sphere, returns the index of the 'dir' neighbor (in 1D arrays); it is not always clear which neighbor is which, but modifying dir by +-1 will return the next neighbor around the original tile
//Assumes a goldberg polyhedron; a positive index refers to a hex, a negative one to a pentagon, with an offset of -1 to prevent the double zero case (so pentagon indices go from -1 to -12 and should be mapped to 0 to 11)
//Returns -13 if out of map
//If dir is bigger than 6 (or 5 for a pentagon), subtracts 6 (or 5) from dir and calls itself with new dir; if smaller than 1, adds 6 to dir and calls itself with new dir
int32 AC_MapGenerator::getNeighbor(int32 index, int32 dir)
{
    if (index >= 0) {
        if (dir<1) {
            return getNeighbor(index, dir+6);
        }
        else if (dir>6) {
            return getNeighbor(index, dir-6);
        }
        else {
            return hex_links[index].links[dir-1];
        }
    }
    else if (index > -13) {
        if (dir<1) {
            return getNeighbor(index, dir+5);
        }
        else if (dir>5) {
            return getNeighbor(index, dir-5);
        }
        else {
            return pent_links[-index-1].links[dir-1];
        }
    }
    else return -13;
}


//Generates an altitude map for the current instance, in the form of a 1D Array
//Altitudes : 0=water, 1=lowlands, 2=midlands, 3=highlands
void AC_MapGenerator::GenerateAltitudeMap()
{
    int32 mapsize=hex_links.Num();
    AltitudeMap.SetNum(mapsize);
    
    for (int32 i = 0; i<mapsize; i++) {
        AltitudeMap[i]=0;
    }
    
    
    //Chain mechanism : A certain number of chains of varying lengths will be placed on the map to elevate the altitude of the map; implementation of chains lower
    int32 const LongChains = 1, LengthLong=1000;
    int32 const MediumChains = 3, LengthMedium=400;
    int32 const ShortChains = 4, LengthShort=100;
    int32 const TinyChains = 10, LengthTiny=4;
    int32 const NumberOfChains = LongChains+MediumChains+ShortChains+TinyChains;
    int32 Chains[NumberOfChains];
    int32 ChainActual;
    int32 nextDir;
    //Chain implementation : each chain starts next to a node and elevates terrain there; the getNeighbor function is then used to move to an adjacent tile and elevate it; loops until the chain is finished, then moves on to the next chain
    for (int32 i=0; i<NumberOfChains; i++) {
        
        //Setting length of current chain
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
        ChainActual=randomstream.RandRange(0, 11);
        ChainActual=getNeighbor(-ChainActual-1, randomstream.RandRange(1, 6));
        
        for (int32 j=0; j<Chains[i];j++) {
            if (ChainActual >= 0) {
                if ((AltitudeMap[ChainActual])<3) { //Elevate if not too high
                    AltitudeMap[ChainActual]++;
                }
            }
            nextDir=randomstream.RandRange(1, 6);
            ChainActual=getNeighbor(ChainActual, nextDir);
        }
    }
}

//Post-processes the previously generated altitude map so that there are less long land chains joining continents, creating more bays and islands
void AC_MapGenerator::PostProcessLongLandChains() {
    int32 mapsize=hex_links.Num();
    TArray<int32> LongChains;
    for (int32 i=0; i<mapsize; i++) {
        if (AltitudeMap[i]==1) {
            int32 NumberOfWaterNeighbors=0;
            bool isLongChain=true;
            for (int32 j=0; j<6; j++) {
                int32 Neighbor=getNeighbor(i, j+1);
                if (Neighbor >= 0) {
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
    int32 mapsize=hex_links.Num();
    
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
                int32 current = getNeighbor(Lakes[i]->tiles[j], k+1);
                if (current>-1) { //Dealing with out-of-map cases
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

//Checks if a given waterbody is a lake; a lake has an area equal to or less than 8 tiles. Fills up input waterbody if actually a lake
bool AC_MapGenerator::CheckIfLake(int32 start, WaterBody *lake) {
    int32 mapsize=hex_links.Num();
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
            int32 actualNeighbor = getNeighbor(current, j);
            if (actualNeighbor>-1) {
                if (AltitudeMap[actualNeighbor]==0) {
                    if ((!visited.Contains(actualNeighbor)) && !frontier.Contains(actualNeighbor)) {
                        frontier.Push(actualNeighbor);
                    }
                }
            }
        }
    }
    //If water body is a lake, fill up a WaterBody element
    lake->tiles.Append(visited);
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
        currentNeighbor=getNeighbor(i, j);
        if (currentNeighbor > -1) {
            if (AltitudeMap[currentNeighbor] != 0) {
                isOcean=false;
                break;
            }
        }
    }
    return isOcean;
}



//Computes an "apparent latitude" for climate based on latitude and elevation; max value of 4/3
float AC_MapGenerator::getApparentLatitude(int32 index){
    return fabsf(getLatitude(index)+(AltitudeMap[index]-1)/6);//If changing /6 factor, also need to change maxlatitude in weight functions just below
}

//Computes real latitude for climate: ratio of z position to radius
float AC_MapGenerator::getLatitude(int32 index){
    return fabsf(zhex[index]/zpent[0]);
}

//Calculates a probability weight to get a snow tile
float AC_MapGenerator::getSnowWeight(float latitude){
    float maxlatitude=(float) 4/ (float) 3;
    if (latitude>maxlatitude*3/4) {
        return 6*(latitude-maxlatitude*3/4);
    }
    else {
        return 0;
    }
}

//Calculates a probability weight to get a tundra tile
float AC_MapGenerator::getTundraWeight(float latitude){
    float maxlatitude=(float) 4/ (float) 3;
    if (latitude>maxlatitude/2) {
        return latitude-maxlatitude/2;
    }
    else {
        return 0;
    }
}

//Calculates a probability weight to get a plain tile
float AC_MapGenerator::getPlainWeight(float latitude){
    float maxlatitude=(float) 4/ (float) 3;
    if (latitude<=maxlatitude/2) {
        return latitude;
    }
    else {
        return maxlatitude-latitude;
    }
}

//Calculates a probability weight to get a grassland tile
float AC_MapGenerator::getGrassWeight(float latitude){
    float maxlatitude=(float) 4/ (float) 3;
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
    int32 mapsize = hex_links.Num();
    float latitude, applatitude, snowWeight, tundraWeight, plainWeight, grassWeight, totalWeight;
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
            //weightedRandomFactor= randomstream.RandRange(0, totalWeight-1);
            weightedRandomFactor = randomstream.FRandRange(0, totalWeight);
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
        if (latitude > ((float) 5/(float) 6)) {
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
                PotentialStartRightBank.Push(getNeighbor(LandTiles[i], j));
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
    int32 mapsize = hex_links.Num();
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
        int32 goingTowards=getNeighbor(actualSeg->LeftBank.Last(), actualSeg->dir.Last()+1);
        if (goingTowards!=-1) {//REDUNDANCY CHECKS :D
            
            if ((CheckifAlreadyInLeftBank(goingTowards) && CheckifAlreadyInRightBank(startLeft)) ||
                (CheckifAlreadyInRightBank(goingTowards) && CheckifAlreadyInLeftBank(startRight))) {
                Rivers.RemoveAt(Rivers.Num()-1);
                return false; //Case where a river can't start where it was supposed to because of another river having already taken its place
            }
            
            int32 goingToLeft=getNeighbor(actualSeg->LeftBank.Last(), actualSeg->dir.Last()+2);
            int32 goingToRight=getNeighbor(actualSeg->RightBank.Last(), actualSeg->dir.Last()+1);
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
    
    int32 current=getNeighbor(i, dir);
    int32 topcurrent=getNeighbor(i, topdir);
    int32 botcurrent=getNeighbor(i, botdir);
    
    if ((current>-1) || (topcurrent>-1) || (botcurrent>-1)) {//This is probably redundant, but better be safe
        return false;//near a node -> nope
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
    
    int32 current=getNeighbor(i, dir);
    int32 topcurrent=getNeighbor(i, topdir);
    int32 botcurrent=getNeighbor(i, botdir);
    
    if ((current>-1) || (topcurrent>-1) || (botcurrent>-1)) {//This is probably redundant, but better be safe
        return false;//near a node -> nope
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
    freshWater.SetNum(hex_links.Num());
    for (int32 i=0; i<hex_links.Num(); i++) {
        int32 freshWaterType=0;
        if ((TerrainType[i]!=ETerrain::VE_Coast) || (TerrainType[i]!=ETerrain::VE_Lake) || (TerrainType[i]!=ETerrain::VE_Ocean)) {//if tile is not water, then
            for (int32 j=1; j<7; j++) {
                int32 current = getNeighbor(i, j);
                if (current > -1) {
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
        int32 current = getNeighbor(index, i);
        if (current > -1) {
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
                int32 current= getNeighbor(LandTiles[i], j);
                if (current > -1) {
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
    int32 ChainActual;
    int32 nextDir;

    //Markov chains similar to the ones used to generate altitude map; starts somewhere and then moves randomly around for a randomized length, transforming every land tile to desert (except snow)
    for (int32 i=0; i<potentialSeeds.Num(); i++) {
        Chains[i]=randomstream.RandRange(1, ChainLength*2);
        
        ChainActual=potentialSeeds[i];
        
        for (int32 j=0; j<Chains[i]; j++) {
            if (ChainActual >= 0) {
                if ((TerrainType[ChainActual] != ETerrain::VE_Coast) && (TerrainType[ChainActual] != ETerrain::VE_Snow)) { //If terrain is not water or snow, set to desert
                    TerrainType[ChainActual]=ETerrain::VE_Desert;
                }
            }
            nextDir=randomstream.RandRange(1, 6);
            ChainActual=getNeighbor(ChainActual, nextDir);
        }
    }
}

//Reduces altitude of all non-water terrain by 1 so that the lowest land level is at the same level as water; also reduces altitude of all rivers
void AC_MapGenerator::ReduceLandAltitude()
{
    int32 mapsize = hex_links.Num();
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
    int32 mapsize = hex_links.Num();
    int32 landsize = LandTiles.Num();
    Forests.SetNumZeroed(hex_links.Num());
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
    resources.SetNum(hex_links.Num());
    resourceRotations.SetNum(hex_links.Num());
    
    //Initialize resources to none
    for (int32 i=0; i<(hex_links.Num()); i++) {
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
        int32 randres;
        
        //QUARRIES
        if (AltitudeMap[LandResourceSpots[i]] == 0) {//Checking for quarry resource spots (bottom of a cliff)
            bool legitQuarrySpot=false;
            for (int32 j=1; j<7; j++) {
                int32 current = getNeighbor(LandResourceSpots[i], j);
                if (current > -1) {
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
                    int32 current = getNeighbor(LandResourceSpots[i], randres);
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
                int32 current = getNeighbor(LandResourceSpots[i], j);
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
    improvements.SetNum(hex_links.Num());
    for (int32 i=0; i<hex_links.Num(); i++) {
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
    
    /*
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
    
    int32 mapsize = hex_links.Num();
    initialCityDistricts.SetNum(mapsize);
    for (int32 i=0; i<mapsize; i++) {
        initialCityDistricts[i]=-1;
    }
    manager->CityDistricts=TArray<int32>(initialCityDistricts);
    
    manager->StartingSpots=TArray<int32>(StartingSpots);
    */
    manager->PrimaryHexArray=TArray<AC_HexTile*>(PrimaryHexArray);
    //manager->PositiveTwinHexArray=TArray<AC_HexTile*>(PositiveTwinHexArray);
    //manager->NegativeTwinHexArray=TArray<AC_HexTile*>(NegativeTwinHexArray);
    //manager->hasPositiveTwin=TArray<bool>(hasPositiveTwin);
    //manager->hasNegativeTwin=TArray<bool>(hasNegativeTwin);
}

float AC_MapGenerator::GenerateGoldbergPolyhedron(int numDivs, float radiusScaling) {
    //initializing arrays of 3D positions for pents
    const float tao = 1618.03398875;//golden mean * 1000
    
    float xinit[] = {0., 0., -tao, -1000., 1000., tao, -1000., -tao, 0., tao, 1000., 0.};
    float yinit[] = {1000., -1000., 0., tao, tao, 0., -tao, 0., 1000., 0., -tao, -1000.};
    float zinit[] = {tao, tao, 1000., 0., 0., 1000., 0., -1000., -tao, -1000., 0., -tao};
    
    xpent.Append(xinit,12);
    ypent.Append(yinit,12);
    zpent.Append(zinit,12);
    
    //ROTATION SO THAT POLES ARE NODES 0 AND 11
    const float theta = 0.55357436;//required rotation around x
    for(int i=0; i<12; i++) {
        float ytemp=ypent[i];//Necessary since ypent get modified before zpent
        ypent[i]=ypent[i]*cosf(theta)-zpent[i]*sinf(theta);
        zpent[i]=ytemp*sinf(theta)+zpent[i]*cosf(theta);
    }
    
    //Initializing faces of polyhedron with correct corners (the corners corresponding to the positions of the pentagons)
    TArray<GoldbergFace> faces;
    faces.Push(GoldbergFace(xpent[0], ypent[0], zpent[0], xpent[1], ypent[1], zpent[1], xpent[2], ypent[2], zpent[2], 0, 1, 2));
    faces.Push(GoldbergFace(xpent[0], ypent[0], zpent[0], xpent[2], ypent[2], zpent[2], xpent[3], ypent[3], zpent[3], 0, 2, 3));
    faces.Push(GoldbergFace(xpent[0], ypent[0], zpent[0], xpent[3], ypent[3], zpent[3], xpent[4], ypent[4], zpent[4], 0, 3, 4));
    faces.Push(GoldbergFace(xpent[0], ypent[0], zpent[0], xpent[4], ypent[4], zpent[4], xpent[5], ypent[5], zpent[5], 0, 4, 5));
    faces.Push(GoldbergFace(xpent[0], ypent[0], zpent[0], xpent[5], ypent[5], zpent[5], xpent[1], ypent[1], zpent[1], 0, 5, 1));
    faces.Push(GoldbergFace(xpent[1], ypent[1], zpent[1], xpent[6], ypent[6], zpent[6], xpent[2], ypent[2], zpent[2], 1, 6, 2));
    faces.Push(GoldbergFace(xpent[2], ypent[2], zpent[2], xpent[7], ypent[7], zpent[7], xpent[3], ypent[3], zpent[3], 2, 7, 3));
    faces.Push(GoldbergFace(xpent[3], ypent[3], zpent[3], xpent[8], ypent[8], zpent[8], xpent[4], ypent[4], zpent[4], 3, 8, 4));
    faces.Push(GoldbergFace(xpent[4], ypent[4], zpent[4], xpent[9], ypent[9], zpent[9], xpent[5], ypent[5], zpent[5], 4, 9, 5));
    faces.Push(GoldbergFace(xpent[5], ypent[5], zpent[5], xpent[10], ypent[10], zpent[10], xpent[1], ypent[1], zpent[1], 5, 10, 1));
    faces.Push(GoldbergFace(xpent[6], ypent[6], zpent[6], xpent[2], ypent[2], zpent[2], xpent[7], ypent[7], zpent[7], 6, 2, 7));
    faces.Push(GoldbergFace(xpent[7], ypent[7], zpent[7], xpent[3], ypent[3], zpent[3], xpent[8], ypent[8], zpent[8], 7, 3, 8));
    faces.Push(GoldbergFace(xpent[8], ypent[8], zpent[8], xpent[4], ypent[4], zpent[4], xpent[9], ypent[9], zpent[9], 8, 4, 9));
    faces.Push(GoldbergFace(xpent[9], ypent[9], zpent[9], xpent[5], ypent[5], zpent[5], xpent[10], ypent[10], zpent[10], 9, 5, 10));
    faces.Push(GoldbergFace(xpent[10], ypent[10], zpent[10], xpent[1], ypent[1], zpent[1], xpent[6], ypent[6], zpent[6], 10, 1, 6));
    faces.Push(GoldbergFace(xpent[11], ypent[11], zpent[11], xpent[6], ypent[6], zpent[6], xpent[7], ypent[7], zpent[7], 11, 6, 7));
    faces.Push(GoldbergFace(xpent[11], ypent[11], zpent[11], xpent[7], ypent[7], zpent[7], xpent[8], ypent[8], zpent[8], 11, 7, 8));
    faces.Push(GoldbergFace(xpent[11], ypent[11], zpent[11], xpent[8], ypent[8], zpent[8], xpent[9], ypent[9], zpent[9], 11, 8, 9));
    faces.Push(GoldbergFace(xpent[11], ypent[11], zpent[11], xpent[9], ypent[9], zpent[9], xpent[10], ypent[10], zpent[10], 11, 9, 10));
    faces.Push(GoldbergFace(xpent[11], ypent[11], zpent[11], xpent[10], ypent[10], zpent[10], xpent[6], ypent[6], zpent[6], 11, 10, 6));
    
    //initialize links for pents
    for (int i=0; i<12; i++) {
        pent_links.Push(FGoldbergLink());
    }
    
    //Interpolate all points within a face
    for(int i = 0; i<faces.Num(); i++) {
        if (GEngine)
        {
            //GEngine->AddOnScreenDebugMessage(-1, 150.f, FColor::Yellow, FString::Printf(TEXT("Face %d with corners %f %f %f ; %f %f %f ; %f %f %f"), i, faces[i].c1x, faces[i].c1y, faces[i].c1z, faces[i].c2x, faces[i].c2y, faces[i].c2z, faces[i].c3x, faces[i].c3y, faces[i].c3z));
        }
        
        //Subdividing straight segments between pents
        TArray<float> leftx, lefty, leftz, rightx, righty, rightz, botx, boty, botz, facex, facey, facez;
        TArray<int> memleft, memright, membot;//arrays used to keep in memory proper indices of hexes in segments for linking
        bool didLeft=false, didRight=false, didBot=false;
        const float max_err = .1;
        
        for(int j = 1; j<numDivs+1; j++) {
            float curr=(float)j/ (float)(numDivs+1);
            
            //Left segment; between face corner 1 and 2
            leftx.Push(faces[i].c1x * (1.-curr) + faces[i].c2x * curr);
            lefty.Push(faces[i].c1y * (1.-curr) + faces[i].c2y * curr);
            leftz.Push(faces[i].c1z * (1.-curr) + faces[i].c2z * curr);
            for (int k = 0; k<xhex.Num(); k++) {//check if this segment was already in x-y-zhex; if so, won't add again
                if ((fabsf(xhex[k]-leftx.Last())<max_err) && (fabsf(yhex[k]-lefty.Last())<max_err) && (fabsf(zhex[k]-leftz.Last())<max_err)) {
                    if (GEngine)
                    {
                        //GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("found a duplicate segment left %d %d %d %f %f %f %f %f %f"), i, j, k, xhex[k], leftx.Last(), yhex[k], lefty.Last(), zhex[k], leftz.Last()));
                    }
                    memleft.Push(k);
                    didLeft=true;
                }
            }
            //Right segment; between face corner 1 and 3
            rightx.Push(faces[i].c1x * (1.-curr) + faces[i].c3x * curr);
            righty.Push(faces[i].c1y * (1.-curr) + faces[i].c3y * curr);
            rightz.Push(faces[i].c1z * (1.-curr) + faces[i].c3z * curr);
            for (int k = 0; k<xhex.Num(); k++) {//check if this segment was already in x-y-zhex; if so, won't add again
                if ((fabsf(xhex[k]-rightx.Last())<max_err) && (fabsf(yhex[k]-righty.Last())<max_err) && (fabsf(zhex[k]-rightz.Last())<max_err)) {
                    if (GEngine)
                    {
                        //GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("found a duplicate segment right %d %d %d %f %f"), i, j, k, xhex[k], rightx.Last()));
                    }
                    memright.Push(k);
                    didRight=true;
                }
            }
            
            //Bot segment; between face corner 2 and 3
            botx.Push(faces[i].c2x * (1.-curr) + faces[i].c3x * curr);
            boty.Push(faces[i].c2y * (1.-curr) + faces[i].c3y * curr);
            botz.Push(faces[i].c2z * (1.-curr) + faces[i].c3z * curr);
            for (int k = 0; k<xhex.Num(); k++) {//check if this segment was already in x-y-zhex; if so, won't add again
                if ((fabsf(xhex[k]-botx.Last())<max_err) && (fabsf(yhex[k]-boty.Last())<max_err) && (fabsf(zhex[k]-botz.Last())<max_err)) {
                    if (GEngine)
                    {
                        //GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("found a duplicate segment bot %d %d %d %f %f"), i, j, k, xhex[k], botx.Last()));
                    }
                    membot.Push(k);
                    didBot=true;
                }
            }
        }
        
        if (GEngine)
        {
            //GEngine->AddOnScreenDebugMessage(-1, 45.f, FColor::Yellow, FString::Printf(TEXT("face %d left : %f %f %f"), i, leftx[0], lefty[0], leftz[0]));
        }
        
        if (GEngine)
        {
            //GEngine->AddOnScreenDebugMessage(-1, 45.f, FColor::Yellow, FString::Printf(TEXT("face %d left : %f %f %f"), i, leftx[12], lefty[12], leftz[12]));
        }
        
        /********************************
         * Starting linking; a link is a list of index references that a each tile has to know its neighbors if index is negative,
         * neighbor is a pentagon of index 1 smaller than listed (so as not to have a double 0 case)
         ********************************/
        //push local segments to points creating hexasphere if not already in, and link up the hexes/pents in segments
        if (!didLeft) {
            xhex.Append(leftx);
            yhex.Append(lefty);
            zhex.Append(leftz);
            
            //Linking local yet undone left segment
            for (int j = 0; j<leftx.Num(); j++) {
                hex_links.Push(FGoldbergLink());
                memleft.Push(hex_links.Num()-1);//For future reference when linking interior of faces
                if (j==0) {
                    hex_links.Last().links.Push(-(1+faces[i].c1));//Linking first of segment with pentagon
                    pent_links[faces[i].c1].links.Push(hex_links.Num()-1);//Linking back
                    if (didRight) {
                        hex_links.Last().links.Push(memright[0]);//Linking corner 1 if right on another face
                        hex_links[memright[0]].links.Push(hex_links.Num()-1);//Linking back
                    }
                }
                else {
                    hex_links.Last().links.Push(hex_links.Num()-2);//Linking with previous element of segment
                    hex_links.Last(1).links.Push(hex_links.Num()-1);//Linking back
                    if (j==(leftx.Num()-1)) {
                        hex_links.Last().links.Push(-(1+faces[i].c2));//Linking last segment with pentagon at corner 2
                        pent_links[faces[i].c2].links.Push(hex_links.Num()-1);//Linking back
                        if (didBot) {
                            hex_links.Last().links.Push(membot[0]);//Linking corner 2 if bot on another face
                            hex_links[membot[0]].links.Push(hex_links.Num()-1);//Linking back
                        }
                    }
                }
            }
        }
        else if (didRight && didBot) {
            hex_links[memleft[0]].links.Push(memright[0]);//Linking corner 1 if both segments on other faces
            hex_links[memright[0]].links.Push(memleft[0]);//Linking back
            hex_links[memleft.Last()].links.Push(membot[0]);//Linking corner 2 if both segments on other faces
            hex_links[membot[0]].links.Push(memleft.Last());//Linking back
        }
        else if (didRight) {
            hex_links[memleft[0]].links.Push(memright[0]);//Linking corner 1 if both segments on other faces
            hex_links[memright[0]].links.Push(memleft[0]);//Linking back
        }
        else if (didBot) {
            hex_links[memleft.Last()].links.Push(membot[0]);//Linking corner 2 if both segments on other faces
            hex_links[membot[0]].links.Push(memleft.Last());//Linking back
        }
        
        if (!didRight) {
            xhex.Append(rightx);
            yhex.Append(righty);
            zhex.Append(rightz);
            
            //Linking local yet undone right segment (also with left)
            for (int j = 0; j<rightx.Num(); j++) {
                hex_links.Push(FGoldbergLink());
                memright.Push(hex_links.Num()-1);//For future reference when linking interior of faces
                if (j==0) {
                    hex_links.Last().links.Push(-(1+faces[i].c1));//Linking first of segment with pentagon
                    pent_links[faces[i].c1].links.Push(hex_links.Num()-1);//Linking back
                    if (!didLeft) {
                        hex_links.Last().links.Push(hex_links.Num()-leftx.Num()-1);//Linking corner 1 if both on this face
                        hex_links[hex_links.Num()-leftx.Num()-1].links.Push(hex_links.Num()-1);//Linking back
                    }
                    else {
                        hex_links.Last().links.Push(memleft[0]);//Linking corner 1 if left on another face
                        hex_links[memleft[0]].links.Push(hex_links.Num()-1);//Linking back
                    }
                    
                }
                else {
                    hex_links.Last().links.Push(hex_links.Num()-2);//Linking with previous element of segment
                    hex_links.Last(1).links.Push(hex_links.Num()-1);//Linking back
                    if (j==(rightx.Num()-1)) {
                        hex_links.Last().links.Push(-(1+faces[i].c3));//Linking last segment with pentagon at corner 3
                        pent_links[faces[i].c3].links.Push(hex_links.Num()-1);//Linking back
                        if (didBot) {
                            hex_links.Last().links.Push(membot.Last());//Linking corner 3 if bot on another face
                            hex_links[membot.Last()].links.Push(hex_links.Num()-1);//Linking back
                        }
                    }
                }
            }
        }
        else if (didBot) {
            hex_links[memright.Last()].links.Push(membot.Last());//Linking corner 3 if both segments on other faces
            hex_links[membot.Last()].links.Push(memright.Last());//Linking back
        }
        
        if (!didBot) {
            xhex.Append(botx);
            yhex.Append(boty);
            zhex.Append(botz);
            
            //Linking local yet undone bot segment (also with left and right)
            for (int j = 0; j<botx.Num(); j++) {
                hex_links.Push(FGoldbergLink());
                membot.Push(hex_links.Num()-1);//For future reference when linking interior of faces
                if (j==0) {
                    hex_links.Last().links.Push(-(1+faces[i].c2));//Linking first of segment with corner 2
                    pent_links[faces[i].c2].links.Push(hex_links.Num()-1);//Linking back
                    if (!didLeft) {
                        if (!didRight) {
                            hex_links.Last().links.Push(hex_links.Num()-rightx.Num()-2);//Linking corner 2 if both on this face with right
                            hex_links[hex_links.Num()-rightx.Num()-2].links.Push(hex_links.Num()-1);//Linking back
                        }
                        else {
                            hex_links.Last().links.Push(hex_links.Num()-2);//Linking corner 2 if both on this face without right
                            hex_links.Last(1).links.Push(hex_links.Num()-1);//Linking back
                        }
                    }
                    else {
                        hex_links.Last().links.Push(memleft.Last());//Linking corner 2 if left on another face
                        hex_links[memleft.Last()].links.Push(hex_links.Num()-1);//Linking back
                    }
                }
                else {
                    hex_links.Last().links.Push(hex_links.Num()-2);//Linking with previous element of segment
                    hex_links.Last(1).links.Push(hex_links.Num()-1);//Linking back
                    if (j==(botx.Num()-1)) {
                        hex_links.Last().links.Push(-(1+faces[i].c3));//Linking last segment with pentagon at corner 3
                        pent_links[faces[i].c3].links.Push(hex_links.Num()-1);//Linking back
                        if (!didRight) {
                            hex_links.Last().links.Push(hex_links.Num()-botx.Num()-1);//Linking corner 3 if both on this face
                            hex_links[hex_links.Num()-botx.Num()-1].links.Push(hex_links.Num()-1);//Linking back
                        }
                        else {
                            hex_links.Last().links.Push(memright.Last());//Linking corner 3 if right on another face
                            hex_links[memright.Last()].links.Push(hex_links.Num()-1);//Linking back
                        }
                    }
                }
            }
        }
        
        //Checking if correct number of tiles in clone segments before proceeding
        if ((memleft.Num()!=leftx.Num()) || (memright.Num()!=rightx.Num()) || membot.Num()!=botx.Num()) {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 45.f, FColor::Yellow, FString::Printf(TEXT("Failed to properly link sphere; inconsistent number of tiles in segments on face %d : %d %d %d %d %d %d"), i, memleft.Num(), leftx.Num(), memright.Num(), rightx.Num(), membot.Num(), botx.Num()));
            }
        }
        
        //Local segments are now known, we now subdivide space between segment elements and thus cover the surface of the face
        for (int j=1; j<numDivs+1; j++) {
            for (int k=1; k<j; k++) {
                float curr=(float)k / (float)(j);
                facex.Push(leftx[j-1] * (1-curr) + rightx[j-1] * curr);
                facey.Push(lefty[j-1] * (1-curr) + righty[j-1] * curr);
                facez.Push(leftz[j-1] * (1-curr) + rightz[j-1] * curr);
                
                //Linking interior of face
                hex_links.Push(FGoldbergLink());
                if (k==1) {
                    hex_links.Last().links.Push(memleft[j-1]);//Linking with left on segment
                    hex_links[memleft[j-1]].links.Push(hex_links.Num()-1);//Linking back
                    hex_links.Last().links.Push(memleft[j-2]);//linking with topleft on segment
                    hex_links[memleft[j-2]].links.Push(hex_links.Num()-1);//Linking back
                }
                else {
                    hex_links.Last().links.Push(hex_links.Num()-2);//Linking with left within face
                    hex_links.Last(1).links.Push(hex_links.Num()-1);//Linking back
                    hex_links.Last().links.Push(hex_links.Num()-j);//Linking with topleft within face
                    hex_links.Last(j-1).links.Push(hex_links.Num()-1);//Linking back
                }
                if (k==(j-1)) {
                    hex_links.Last().links.Push(memright[j-2]);//Linking with topright on segment
                    hex_links[memright[j-2]].links.Push(hex_links.Num()-1);//Linking back
                    hex_links.Last().links.Push(memright[j-1]);//Linking with right on segment
                    hex_links[memright[j-1]].links.Push(hex_links.Num()-1);//Linking back
                }
                else {
                    hex_links.Last().links.Push(hex_links.Num()-(j-1));//Linking with topright within face
                    hex_links.Last(j-2).links.Push(hex_links.Num()-1);//Linking back
                }
                if (j==numDivs) {
                    hex_links.Last().links.Push(membot[k-1]);//Linking with botleft on segment
                    hex_links[membot[k-1]].links.Push(hex_links.Num()-1);//Linking back
                    hex_links.Last().links.Push(membot[k]);//Linking with botright on segment
                    hex_links[membot[k]].links.Push(hex_links.Num()-1);//Linking back
                }
            }
        }
        
        if (i==4) {
            for(int j = 0; j<hex_links.Num(); j++) {
                int temp2 = -13, temp3 =-13, temp4 = -13, temp5 = -13;
                if (hex_links[j].links.Num()==3) {
                    temp2 = hex_links[j].links[2];
                }
                else if (hex_links[j].links.Num()==4) {
                    temp2 = hex_links[j].links[2];
                    temp3 = hex_links[j].links[3];
                }
                else if (hex_links[j].links.Num()==5) {
                    temp2 = hex_links[j].links[2];
                    temp3 = hex_links[j].links[3];
                    temp4 = hex_links[j].links[4];
                }
                else if (hex_links[j].links.Num()==6) {
                    temp2 = hex_links[j].links[2];
                    temp3 = hex_links[j].links[3];
                    temp4 = hex_links[j].links[4];
                    temp5 = hex_links[j].links[5];
                }
                if (GEngine)
                {
                    //GEngine->AddOnScreenDebugMessage(-1, 145.f, FColor::Yellow, FString::Printf(TEXT("Links on face %d: %d %d %d %d %d %d"), i, hex_links[j].links[0], hex_links[j].links[1], temp2, temp3, temp4, temp5));
                }
            }
        }
        
        //push face surface to points creating hexasphere
        xhex.Append(facex);
        yhex.Append(facey);
        zhex.Append(facez);
        
        //finished with this face, moving on to next face
    }
    
    //Checking total number of links of hexagons
    int nLinks=0;
    for (int i=0; i<hex_links.Num(); i++) {
        nLinks+=hex_links[i].links.Num();
    }
    if (GEngine) {
            GEngine->AddOnScreenDebugMessage(-1, 155.f, FColor::Yellow, FString::Printf(TEXT("Number of hex links : %d"), nLinks));
    }
    
    //Checking if all hexagons have 6 neighbors
    for (int i=0; i<hex_links.Num(); i++) {
        if (hex_links[i].links.Num() != 6) {
            if (GEngine) {
                GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("Failed to properly link sphere; hexagon does not have 6 neighbors : %d; nNeighbors: %d"), i, hex_links[i].links.Num()));
                
            }
        }
    }
    
    //Checking if all pentagons have 5 neighbors
    for (int i=0; i<pent_links.Num(); i++) {
        if (pent_links[i].links.Num() != 5) {
            if (GEngine) {
                    GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("Failed to properly link sphere; pentagon does not have 5 neighbors : %d"), i));
            
            }
        }
    }
    
    //all points of all faces are now into the x-y-zhex arrays; must now rescale to correct radius for hexes and pents
    //calculate correct radius for numDivs
    const float angle_pents = 1.10714871779406;
    const float height_hex = 173.205080756888;
    float radius = radiusScaling * height_hex * numDivs / angle_pents;
    
    //rescale hexes to correct radius, and calculate scaling factor
    float minirad=sqrt(xpent[0]*xpent[0] + ypent[0]*ypent[0] + zpent[0]*zpent[0]);
    for (int i=0; i<xhex.Num(); i++) {
        float mag = sqrtf(xhex[i]*xhex[i] + yhex[i]*yhex[i] + zhex[i]*zhex[i]);
        float scale = radius / mag;
        xhex[i]*=scale;
        yhex[i]*=scale;
        zhex[i]*=scale;
        scalefactor.Push(minirad/mag);
    }
    
    //rescale pents to correct radius
    for (int i=0; i<12; i++) {
        float scale = radius/minirad;
        xpent[i]*=scale;
        ypent[i]*=scale;
        zpent[i]*=scale;
    }
    
    return radius;
}

void AC_MapGenerator::OrderGoldbergLinking() {
    //For all hexes, we order the neighbors; i.e. the neighbors next to each other in the array are next to each other on the sphere (first and last element are also neighbors)
    for (int i=0; i<hex_links.Num(); i++) {
        //Ordering each hex individually:
        for (int j=0; j<(hex_links[i].links.Num()-2); j++) {
            int current = hex_links[i].links[j];
            //For each neighbor,
            for (int k=j+1; k<hex_links[i].links.Num(); k++) {
                int potNeighbor = hex_links[i].links[k];
                bool foundNext=false;
                //check if that neighbor is also a neighbor of current
                if (current >= 0) {
                    for (int l=0; l<hex_links[current].links.Num(); l++) {
                    //if so, swap positions of neighbors in array of original hex
                        if (hex_links[current].links[l] == potNeighbor) {
                            hex_links[i].links[k]=hex_links[i].links[j+1];
                            hex_links[i].links[j+1]=potNeighbor;
                            foundNext=true;
                            break;
                        }
                    }
                }
                else {//if neighbor is a pentagon
                    current=-current-1;
                    for (int l=0; l<pent_links[current].links.Num(); l++) {
                    //if so, swap positions of neighbors in array of original hex
                        if (pent_links[current].links[l] == potNeighbor) {
                            hex_links[i].links[k]=hex_links[i].links[j+1];
                            hex_links[i].links[j+1]=potNeighbor;
                            foundNext=true;
                            break;
                        }
                    }
                }
                if (foundNext) break;
            }
        }
    }
    
    //For all pents, we order the neighbors; i.e. the neighbors next to each other in the array are next to each other on the sphere (first and last element are also neighbors)
    for (int i=0; i<pent_links.Num(); i++) {
        //Ordering each pent individually:
        for (int j=0; j<pent_links[i].links.Num(); j++) {
            int current = pent_links[i].links[j];
            //For each neighbor,
            for (int k=j+1; k<pent_links[i].links.Num(); k++) {
                int potNeighbor = pent_links[i].links[k];
                bool foundNext=false;
                //check if that neighbor is also a neighbor of current
                for (int l=0; l<hex_links[current].links.Num(); l++) {
                    //if so, swap positions of neighbors in array of original hex
                    if (hex_links[current].links[l] == potNeighbor) {
                        pent_links[i].links[k]=pent_links[i].links[j+1];
                        pent_links[i].links[j+1]=potNeighbor;
                        foundNext=true;
                        break;
                    }
                }
                if (foundNext) break;
            }
        }
    }
    
    
    for (int i=0; i<33; i++) {
        if (GEngine) {
            //GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("Check on link ordering on hex %d : %d %d %d %d %d %d"), i, hex_links[i].links[0], hex_links[i].links[1], hex_links[i].links[2], hex_links[i].links[3], hex_links[i].links[4], hex_links[i].links[5]));
            
        }
    }
    
    for (int i=0; i<12; i++) {
        if (GEngine) {
            //GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("Check on link ordering on pent %d : %d %d %d %d %d"), i, pent_links[i].links[0], pent_links[i].links[1], pent_links[i].links[2], pent_links[i].links[3], pent_links[i].links[4]));
            
        }
    }
}









