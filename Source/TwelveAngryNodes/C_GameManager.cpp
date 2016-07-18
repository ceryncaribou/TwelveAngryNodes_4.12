// Fill out your copyright notice in the Description page of Project Settings.

#include "TwelveAngryNodes.h"
#include "C_GameManager.h"

//Empty constructor to be overriden in blueprint if necessary
AC_GameManager::AC_GameManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    
}

//Returns x value from a 1D array
int32 AC_GameManager::getX(int32 i){
    return (i % mapsizex);
}

//Returns y value from a 1D array
int32 AC_GameManager::getY(int32 i){
    return (i / mapsizex);
}

int32 AC_GameManager::getNeighbor(int32 index, int32 dir)
{
    return getNeighbor(getX(index), getY(index), dir);
}

//From a position on the hex grid x and y, returns the index of the 'dir' neighbor (in 1D arrays) : 1=right, 2=topright, 3=topleft, 4=left, 5=botleft, 6=botright
//Assumes a cylinder wrap, and so uses the xy notation for simplicity of finding special cases
//Returns -1 if out of map or if dir is 0 or less
//If dir is bigger than 6, subtracts 6 from dir and calls itself with new dir; if smaller than 1, adds 6 to dir and calls itself with new dir
int32 AC_GameManager::getNeighbor(int32 x, int32 y, int32 dir) {
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

//Checks if given hex has fresh water; 0 is no fresh water, 1 is next to lake, 2 is next to river; river overrides lake.
int32 AC_GameManager::CheckFreshWaterOnHex(int32 position)
{
    int32 freshWaterType=0;
    if ((TerrainType[position]!=ETerrain::VE_Coast) || (TerrainType[position]!=ETerrain::VE_Lake) || (TerrainType[position]!=ETerrain::VE_Ocean)) {//if tile is not water, then
        for (int32 j=1; j<7; j++) {
            int32 current = getNeighbor(getX(position), getY(position), j);
            if (current != -1) {
                if (TerrainType[current] == ETerrain::VE_Lake) {//check if it's next to a lake
                    freshWaterType=1;
                }
            }
        }
        if ((RiverOn1[position]==1) || (RiverOn2[position]==1) || (RiverOn3[position]==1) || (RiverOn4[position]==1) || (RiverOn5[position]==1) || (RiverOn6[position]==1)) {//or to a river
            freshWaterType=2;
        }
    }
    return freshWaterType;
}
