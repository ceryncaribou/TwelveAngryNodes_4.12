// Fill out your copyright notice in the Description page of Project Settings.

#include "TwelveAngryNodes.h"
#include "C_CivManagerInterface.h"

//Empty constructor to be overriden in blueprint if necessary
AC_CivManagerInterface::AC_CivManagerInterface(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    
}

//Necessary for virtual blueprint implementations
bool AC_CivManagerInterface::BeginTurn_Implementation(){return true;}
void AC_CivManagerInterface::EndTurnTriggered_Implementation(){}

//Returns x value from a 1D array
int32 AC_CivManagerInterface::getX(int32 i){
    return (i % mapsizex);
}

//Returns y value from a 1D array
int32 AC_CivManagerInterface::getY(int32 i){
    return (i / mapsizex);
}

//From a position on the hex grid x and y, returns the index of the 'dir' neighbor (in 1D arrays) : 1=right, 2=topright, 3=topleft, 4=left, 5=botleft, 6=botright
//Assumes a cylinder wrap, and so uses the xy notation for simplicity of finding special cases
//Returns -1 if out of map or if dir is 0 or less
//If dir is bigger than 6, subtracts 6 from dir and calls itself with new dir; if smaller than 1, adds 6 to dir and calls itself with new dir
int32 AC_CivManagerInterface::getNeighbor(int32 x, int32 y, int32 dir) {
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

//Moves along stocked path; if path is empty, stops moving. Updates position of unit in all useful arrays and then returns new position of unit.
int32 AC_CivManagerInterface::moveAlongPath(int32 unitIndex)
{
    while ((CivUnitList[unitIndex]->currentpath.Num() > 1) && (CivUnitList[unitIndex]->remainingMoves > 0)) {
        CivUnitList[unitIndex]->position=CivUnitList[unitIndex]->currentpath[1];
        int32 i=1;
        for (; i<7; i++) {
            if (CivUnitList[unitIndex]->currentpath[1] == getNeighbor(getX(CivUnitList[unitIndex]->currentpath[0]), getY(CivUnitList[unitIndex]->currentpath[0]), i)) {
                break;
            }
        }
        CivUnitList[unitIndex]->remainingMoves=CivUnitList[unitIndex]->remainingMoves-getMovementCost(CivUnitList[unitIndex]->currentpath[0], CivUnitList[unitIndex]->currentpath[1], i);
        
        CivUnitList[unitIndex]->currentpath.RemoveAt(0);
    }
    return CivUnitList[unitIndex]->position;
}


//Function calculating the movement cost to go from a hex tilewith index "source" to another one "destination" with given "direction". Returns movement cost, or -1 if movement is impossible. DOES NOT CHECK IF TILES ARE CONTIGUOUS FOR OPTIMIZATION PURPOSES.
float AC_CivManagerInterface::getMovementCost(int32 source, int32 destination, int32 direction) {
    float cost = 0;
    
    if (destination==-1) { //Dealing with out-of-map cases
        return -1;
    }
    else {
        if (TerrainType[source]==ETerrain::VE_Coast) { //Going from sea
            if (TerrainType[destination]==ETerrain::VE_Coast) { //to sea
                cost=1.01;
            }
            else { //to land
                return -1;
            }
        }
        else { //Going from land
            if (TerrainType[destination]==ETerrain::VE_Coast) { //to sea
                return -1;
            }
            else if (FMath::Max(AltitudeMap[source]-AltitudeMap[destination], AltitudeMap[destination]-AltitudeMap[source])<=1) { //to same altitude or through ramp
                cost=1.01;
            }
            else { //through cliff
                return -1;
            }
        }
        if (Forests[destination]!=0) {
            cost=cost+1.01;
        }
        switch (direction) {
            case 1:
                if (GameManager->RiverOn1[source]!=0) {
                    cost=50;
                }
                break;
            case 2:
                if (GameManager->RiverOn2[source]!=0) {
                    cost=50;
                }
                break;
            case 3:
                if (GameManager->RiverOn2[source]!=0) {
                    cost=50;
                }
                break;
            case 4:
                if (GameManager->RiverOn4[source]!=0) {
                    cost=50;
                }
                break;
            case 5:
                if (GameManager->RiverOn5[source]!=0) {
                    cost=50;
                }
                break;
            case 6:
                if (GameManager->RiverOn6[source]!=0) {
                    cost=50;
                }
                break;
            default:
                break;
        }
    }
    
    return cost;
}

//Generates the Manhattan distance between two tiles; takes into account possible wraparounds in both positive and negative x direction. Scales down by a factor of 250 to make the return value of the heuristic smaller than the real distance (in number of tiles)
float AC_CivManagerInterface::ManhattanDistanceHeuristic(int32 a, int32 b) {
    int32 ax=getX(a),bx=getX(b),ay=getY(a),by=getY(b);
    float axmain=173.2*ax + 86.6*ay;
    float axpos=axmain+173.2*mapsizex;
    float axneg=axmain-173.2*mapsizex;
    float bxmain=173.2*bx + 86.6*by;
    float deltax,deltay;
    deltax=FMath::Min3(FMath::Abs(bxmain-axmain), FMath::Abs(bxmain-axpos), FMath::Abs(bxmain-axneg));
    deltay=150.*FMath::Abs(ay-by);
    return (deltax+deltay)/4000.;
}

/*Finds best path between two tiles and stores it.
 PARAMETERS:
 from : index of the tile the path starts from
 to : index of the tile the path goes to
 moves_left : movement left to the given unit this turn when function is called
 max_moves : amount of moves the unit can make at the start a turn
 path : reference to an array to pass the path back to caller function
 */
bool AC_CivManagerInterface::findBestPath(int32 from, int32 to, float moves_left, float max_moves) {
    
    bool found=false;
    currentpath.Empty();
    
    if (from == to) {//Exception case where no path is needed since origin and destination are the same
        currentpath.Insert(to, 0);
        return true;
    }
    
    int32 mapsize = mapsizex*mapsizey;
    TArray<moveCosts*> frontier;
    moveCosts *root = new moveCosts();
    root->priority=0;
    root->value=from;
    
    frontier.HeapPush(root, CompareMoveCosts());
    
    TArray<int32> came_from;
    TArray<int32> turns_so_far;
    TArray<float> remaining_moves;
    
    came_from.SetNum(mapsize);
    turns_so_far.SetNum(mapsize);
    remaining_moves.SetNum(mapsize);
    
    for (int32 i=0; i<mapsize; i++) {
        turns_so_far[i]=10000; //arbitrarily large number
        remaining_moves[i]=-50;   //arbitrarily small number
    }
    
    came_from[from]=from;
    turns_so_far[from]=0;
    remaining_moves[from]=moves_left;
    
    while (frontier.Num() != 0) {
        moveCosts *current;
        frontier.HeapPop(current, CompareMoveCosts());
        if (current->value == to) {
            found=true;
            break;
        }
        for (int j=1; j<7; j++) {
            int32 actualNeighbor = getNeighbor(getX(current->value), getY(current->value), j);
            float new_cost = getMovementCost(current->value, actualNeighbor, j);
            if (new_cost > -0.5) {
                int32 new_turns;
                if (new_cost >= remaining_moves[current->value]) {
                    new_turns=turns_so_far[current->value]+1;
                }
                else {
                    new_turns=turns_so_far[current->value];
                }
                if ((new_turns < turns_so_far[actualNeighbor]) || ((new_turns == turns_so_far[actualNeighbor]) && ((remaining_moves[current->value]-new_cost) > remaining_moves[actualNeighbor]))) {
                    turns_so_far[actualNeighbor]=new_turns;
                    moveCosts *next = new moveCosts;
                    next->priority=(float)(new_turns)+ManhattanDistanceHeuristic(actualNeighbor,to);
                    next->value=actualNeighbor;
                    if (new_cost >= remaining_moves[current->value]) {
                        remaining_moves[actualNeighbor]=max_moves;
                    }
                    else {
                        remaining_moves[actualNeighbor]=remaining_moves[current->value]-new_cost;
                    }
                    frontier.HeapPush(next, CompareMoveCosts());
                    came_from[actualNeighbor]=current->value;
                }
            }
        }
    }
    while (found) {
        currentpath.Insert(to, 0);
        to=came_from[to];
        if (to == from) {
            break;
        }
    }
    currentpath.Insert(from, 0);
    bool youpi = true;
    return found;
}

//Updates features on a given hex in this instance of the civ manager, the game manager and in the appropriate C_HexTile; subsequently calculates new yields
void AC_CivManagerInterface::UpdateHexFeats(int32 position, int32 inForest, EImprovement inImprovement) {
    Forests[position]=inForest;
    Improvements[position]=inImprovement;
    
    GameManager->Forests[position]=inForest;
    GameManager->Improvements[position]=inImprovement;
    
    GameManager->PrimaryHexArray[position]->hasForest=inForest;
    GameManager->PrimaryHexArray[position]->improvementType=inImprovement;
    GameManager->PrimaryHexArray[position]->CalcTileYields();
    if (GameManager->hasPositiveTwin[position]) {
        GameManager->PositiveTwinHexArray[position]->hasForest=inForest;
        GameManager->PositiveTwinHexArray[position]->improvementType=inImprovement;
        GameManager->PositiveTwinHexArray[position]->CalcTileYields();
    }
    if (GameManager->hasNegativeTwin[position]) {
        GameManager->NegativeTwinHexArray[position]->hasForest=inForest;
        GameManager->NegativeTwinHexArray[position]->improvementType=inImprovement;
        GameManager->NegativeTwinHexArray[position]->CalcTileYields();
    }
}

//Sets hasCityFromCiv and cityIDfromCiv C_HexTile variables in appropriate hexes, then calls UpdateHexFeatures to remove forests or improvements on tile and recalculate yields
void AC_CivManagerInterface::PlaceNewCity(int32 position, int32 cityID) {
    CityDistricts[position]=playerID;
    GameManager->CityDistricts[position]=playerID;

    GameManager->PrimaryHexArray[position]->hasCityDistrictFromCiv=playerID;
    GameManager->PrimaryHexArray[position]->cityIDfromCiv=CivCityList.Num();
    if (GameManager->hasPositiveTwin[position]) {
        GameManager->PositiveTwinHexArray[position]->hasCityDistrictFromCiv=playerID;
        GameManager->PositiveTwinHexArray[position]->cityIDfromCiv=CivCityList.Num();
    }
    if (GameManager->hasNegativeTwin[position]) {
        GameManager->NegativeTwinHexArray[position]->hasCityDistrictFromCiv=playerID;
        GameManager->NegativeTwinHexArray[position]->cityIDfromCiv=CivCityList.Num();
    }
    UpdateHexFeats(position, 0, EImprovement::VE_None);
    CivCityList[cityID]->initializeCity();
    CivCityList[cityID]->allocateCitizens();
    
    FString NewString = FString::FromInt(GameManager->PrimaryHexArray[position]->cityIDfromCiv);
    if (GEngine) {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("new city " + NewString));
    }
}

//Places a new improvement on given tile position
void AC_CivManagerInterface::PlaceNewImprovement(int32 position, EImprovement inImprovement) {
    if ((inImprovement == EImprovement::VE_Lumbermill) || (inImprovement == EImprovement::VE_Camp) || (inImprovement == EImprovement::VE_None)) {
        UpdateHexFeats(position, Forests[position], inImprovement);
    }
    else {
        UpdateHexFeats(position, 0, inImprovement);
    }
    if (GEngine) {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("new improvement "));
    }
}

//Checks if the given position is a legit razing spot, i.e. if it has an improvement.
bool AC_CivManagerInterface::CheckIfLegitRazeSpot(int32 position) {
    if (GameManager->Improvements[position] != EImprovement::VE_None) {
        return true;
    }
    return false;
}

//Checks if the given position is a legit quarry spot, i.e. if it has a quarry resource of if it's near to a cliff. Also sets a correct rotation to the corresponding hexes.
bool AC_CivManagerInterface::CheckIfLegitQuarrySpot(int32 position) {
    if ((GameManager->Resources[position] == EResource::VE_Granite) || (GameManager->Resources[position] == EResource::VE_Limestone) || (GameManager->Resources[position] == EResource::VE_Marble) || (GameManager->Resources[position] == EResource::VE_Salt) || (GameManager->Resources[position] == EResource::VE_Slate)) {
        return true;
    }
    if (GameManager->AltitudeMap[position] == 0) {
        bool legitQuarrySpot=false;
        for (int32 j=1; j<7; j++) {
            int32 current = getNeighbor(getX(position), getY(position), j);
            if (current != -1) {
                if (AltitudeMap[current] == 2) {
                    switch (j) {
                        case 1:
                            if (GameManager->RiverOn1[position] == 0) {legitQuarrySpot=true;}
                            break;
                        case 2:
                            if (GameManager->RiverOn2[position] == 0) {legitQuarrySpot=true;}
                            break;
                        case 3:
                            if (GameManager->RiverOn3[position] == 0) {legitQuarrySpot=true;}
                            break;
                        case 4:
                            if (GameManager->RiverOn4[position] == 0) {legitQuarrySpot=true;}
                            break;
                        case 5:
                            if (GameManager->RiverOn5[position] == 0) {legitQuarrySpot=true;}
                            break;
                        case 6:
                            if (GameManager->RiverOn6[position] == 0) {legitQuarrySpot=true;}
                            break;
                        default:
                            break;
                    }
                }
            }
        }
        if (legitQuarrySpot) {
            bool rotationFound = false;
            while (!rotationFound) {
                int32 randres=FMath::RandRange(1, 6);
                int32 current = getNeighbor(getX(position), getY(position), randres);
                if (current != -1) {
                    if (AltitudeMap[current] == 2) {
                        switch (randres) {
                            case 1:
                                if (GameManager->RiverOn1[position] == 0) {
                                    GameManager->PrimaryHexArray[position]->resourceRotation=0;
                                    if (GameManager->hasNegativeTwin[position] == true) {
                                        GameManager->NegativeTwinHexArray[position]->resourceRotation=0;
                                    }
                                    if (GameManager->hasPositiveTwin[position] == true) {
                                        GameManager->PositiveTwinHexArray[position]->resourceRotation=0;
                                    }
                                    rotationFound=true;
                                }
                                break;
                            case 2:
                                if (GameManager->RiverOn2[position] == 0) {
                                    GameManager->PrimaryHexArray[position]->resourceRotation=1;
                                    if (GameManager->hasNegativeTwin[position] == true) {
                                        GameManager->NegativeTwinHexArray[position]->resourceRotation=1;
                                    }
                                    if (GameManager->hasPositiveTwin[position] == true) {
                                        GameManager->PositiveTwinHexArray[position]->resourceRotation=1;
                                    }
                                    rotationFound=true;
                                }
                                break;
                            case 3:
                                if (GameManager->RiverOn3[position] == 0) {
                                    GameManager->PrimaryHexArray[position]->resourceRotation=2;
                                    if (GameManager->hasNegativeTwin[position] == true) {
                                        GameManager->NegativeTwinHexArray[position]->resourceRotation=2;
                                    }
                                    if (GameManager->hasPositiveTwin[position] == true) {
                                        GameManager->PositiveTwinHexArray[position]->resourceRotation=2;
                                    }
                                    rotationFound=true;
                                }
                                break;
                            case 4:
                                if (GameManager->RiverOn4[position] == 0) {
                                    GameManager->PrimaryHexArray[position]->resourceRotation=3;
                                    if (GameManager->hasNegativeTwin[position] == true) {
                                        GameManager->NegativeTwinHexArray[position]->resourceRotation=3;
                                    }
                                    if (GameManager->hasPositiveTwin[position] == true) {
                                        GameManager->PositiveTwinHexArray[position]->resourceRotation=3;
                                    }
                                    rotationFound=true;
                                }
                                break;
                            case 5:
                                if (GameManager->RiverOn5[position] == 0) {
                                    GameManager->PrimaryHexArray[position]->resourceRotation=4;
                                    if (GameManager->hasNegativeTwin[position] == true) {
                                        GameManager->NegativeTwinHexArray[position]->resourceRotation=4;
                                    }
                                    if (GameManager->hasPositiveTwin[position] == true) {
                                        GameManager->PositiveTwinHexArray[position]->resourceRotation=4;
                                    }
                                    rotationFound=true;
                                }
                                break;
                            case 6:
                                if (GameManager->RiverOn6[position] == 0) {
                                    GameManager->PrimaryHexArray[position]->resourceRotation=5;
                                    if (GameManager->hasNegativeTwin[position] == true) {
                                        GameManager->NegativeTwinHexArray[position]->resourceRotation=5;
                                    }
                                    if (GameManager->hasPositiveTwin[position] == true) {
                                        GameManager->PositiveTwinHexArray[position]->resourceRotation=5;
                                    }
                                    rotationFound=true;
                                }
                                break;
                            default:
                                break;
                        }
                        if (GEngine) {
                            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, TEXT("checking if legit cliff quarry spot"));
                        }
                    }
                }
            }
            return true;
        }
    }
    return false;
}

//Checks if the given position is a legit mine spot, i.e. if it has a mine resource.
bool AC_CivManagerInterface::CheckIfLegitMineSpot(int32 position) {
    if ((GameManager->Resources[position] == EResource::VE_Copper) || (GameManager->Resources[position] == EResource::VE_Iron) || (GameManager->Resources[position] == EResource::VE_Mithril) || (GameManager->Resources[position] == EResource::VE_Gems) || (GameManager->Resources[position] == EResource::VE_Gold) || (GameManager->Resources[position] == EResource::VE_Silver)) {
        return true;
    }
    return false;
}

bool AC_CivManagerInterface::CheckIfLegitFarmSpot(int32 position) {
    if ((GameManager->Resources[position] == EResource::VE_Barley) || (GameManager->Resources[position] == EResource::VE_Beans) || (GameManager->Resources[position] == EResource::VE_Corn) || (GameManager->Resources[position] == EResource::VE_Rice) || (GameManager->Resources[position] == EResource::VE_Tomato) || (GameManager->Resources[position] == EResource::VE_Wheat)) {
        return true;
    }
    if ((GameManager->CheckFreshWaterOnHex(position) != 0) && (TerrainType[position]!=ETerrain::VE_Snow) && (TerrainType[position]!=ETerrain::VE_Hell) && (TerrainType[position]!=ETerrain::VE_Void)) {
        return true;
    }
    return false;
}


void AC_CivManagerInterface::InitializeCivManagerMapArrays() {
    
    mapsizex=GameManager->mapsizex;
    mapsizey=GameManager->mapsizey;
    
    AltitudeMap=TArray<int32>(GameManager->AltitudeMap);
    TerrainType=TArray<ETerrain>(GameManager->TerrainType);
    Forests=TArray<int32>(GameManager->Forests);
    Improvements=TArray<EImprovement>(GameManager->Improvements);
    CityDistricts=TArray<int32>(GameManager->CityDistricts);
    
    revealedResources=TArray<bool>();
    UndiscoveredResourceTypes=TArray<EResource>();
    revealedResources.SetNum(mapsizex*mapsizey);
    for (int32 i=0; i<(mapsizex*mapsizey); i++) {
        if (GameManager->Resources[i] == EResource::VE_None) {
            revealedResources[i]=false;
        }
        else {
            if (UndiscoveredResourceTypes.Contains(GameManager->Resources[i])) {
                revealedResources[i]=false;
            }
            else {
                revealedResources[i]=true;
            }
        }
    }
}