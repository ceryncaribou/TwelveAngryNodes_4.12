// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "C_HexTile.h"
#include "C_GameManager.generated.h"

/**
 * 
 */


UCLASS(Meta=(ChildCanTick))
class TWELVEANGRYNODES_API AC_GameManager : public AActor
{
	GENERATED_BODY()
	
public:
    
    //VARIABLES
    
    //Map size in x
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    int32 mapsizex;
    
    //Map size in y
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    int32 mapsizey;
    
    //Possible values : 0, 1, 2, 3. At the end of GenerateTerrainType(), all non zero values get a -1, so that possible values are 0, 1, 2.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> AltitudeMap;
    
    //Terrain types : 0=water, 1=grassland, 2=plain, 3=tundra, 4=snow, 5=desert
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<ETerrain> TerrainType;
    
    //Forest types :
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> Forests;
    
    //Resource map :
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<EResource> Resources;
    
    //Improvement map :
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<EImprovement> Improvements;
    
    //Cities map : (as last seen) ; -1 is no city, other values correspond to owning faction
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> CityDistricts;
    
    //Rivers
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<bool> hasRiver;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> RiverOn1;//right
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> RiverOn2;//topright
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> RiverOn3;//topleft
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> RiverOn4;//left
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> RiverOn5;//botleft
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> RiverOn6;//botright
    
    
    //Array of decent starting spots for civs; may contain more spots than there are civs, but not the contrary
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> StartingSpots;
    
    //Arrays used to keep track of references to hexes, and which hexes have twins.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex References")
    TArray<AC_HexTile*> PrimaryHexArray;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex References")
    TArray<AC_HexTile*> PositiveTwinHexArray;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex References")
    TArray<AC_HexTile*> NegativeTwinHexArray;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex References")
    TArray<bool> hasPositiveTwin;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex References")
    TArray<bool> hasNegativeTwin;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Path")
    TArray<int32> currentpath;
    
    
    //Player Identifier
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turn Info")
    int32 currentPlayer;
    
    //When true, ends players turn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turn Info")
    int32 currentTurn;
	
    
    
    
    //FUNCTIONS
    
    AC_GameManager(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BluePrintCallable, Category="Access Functions")
    int32 getNeighbor(int32 index, int32 dir);
    
    int32 CheckFreshWaterOnHex(int32 position);
    
    int32 getX(int32 i);
    int32 getY(int32 i);
    int32 getNeighbor(int32 x, int32 y, int32 dir);
    

    
};
