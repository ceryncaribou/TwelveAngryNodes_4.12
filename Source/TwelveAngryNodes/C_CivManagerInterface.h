// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "C_GameManager.h"
#include "Units/C_UnitGeneric.h"
#include "C_City.h"
#include "C_CivManagerInterface.generated.h"

/**
 * 
 */
UCLASS()
class TWELVEANGRYNODES_API AC_CivManagerInterface : public AActor
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
    
    //Possible values : 0, 1, 2, 3. At the end of GenerateTerrainType(), all non zero values get a -1, so that possible values are 0, 1->0, 2->1, 3->2.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> AltitudeMap;
    
    //Terrain map (as last seen)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<ETerrain> TerrainType;
    
    //Forests map : (as last seen)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> Forests;
    
    //Resource map : (as last seen)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<bool> revealedResources;
    //Short array of all undiscovered resource types; anything in this array not rendered on map
    TArray<EResource> UndiscoveredResourceTypes;
    
    //Improvement map : (as last seen)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<EImprovement> Improvements;
    
    //Cities map : (as last seen) ; -1 is no city, other values correspond to owning faction
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> CityDistricts;
    
    //Pathfinding
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Path")
    TArray<int32> currentpath;
    
    
    //Manager reference
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Manager Reference", Meta=(ExposeOnSpawn=true))
    AC_GameManager* GameManager;
    
    //Player Identifier
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turn Info", Meta=(ExposeOnSpawn=true))
    int32 playerID;
    
    //When true, ends players turn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turn Info")
    bool turnIsDone;
    
    //List of this civs owned units
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Unit List")
    TArray<AC_UnitGeneric*> CivUnitList;
    
    //List of this civs owned cities
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="City List")
    TArray<AC_City*> CivCityList;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Other Civs List")
    TArray<AC_CivManagerInterface*> CivRefList;
    
    
    //FUNCTIONS
    AC_CivManagerInterface(const FObjectInitializer& ObjectInitializer);
    
    UFUNCTION(BlueprintNativeEvent, BluePrintCallable, Category="Blueprint call Functions", meta=(DisplayName = "Begin Turn"))
    bool BeginTurn();
    virtual bool BeginTurn_Implementation();
    UFUNCTION(BlueprintNativeEvent, BluePrintCallable, Category="Blueprint call Functions", meta=(DisplayName = "End Turn Triggered"))
    void EndTurnTriggered();
    virtual void EndTurnTriggered_Implementation();
    
    //Pathfinding
    UFUNCTION(BluePrintCallable, Category="Pathfinding Functions")
    bool findBestPath(int32 from, int32 to, float moves_left, float max_moves);
    UFUNCTION(BluePrintCallable, Category="Pathfinding Functions")
    int32 moveAlongPath(int32 unitIndex);
    
    //Map modification functions
    UFUNCTION(BluePrintCallable, Category="Gameplay Functions")
    void PlaceNewCity(int32 position, int32 cityID);
    UFUNCTION(BluePrintCallable, Category="Gameplay Functions")
    void PlaceNewImprovement(int32 position, EImprovement inImprovement);
    UFUNCTION(BluePrintCallable, Category="Gameplay Functions")
    bool CheckIfLegitRazeSpot(int32 position);
    UFUNCTION(BluePrintCallable, Category="Gameplay Functions")
    bool CheckIfLegitQuarrySpot(int32 position);
    UFUNCTION(BluePrintCallable, Category="Gameplay Functions")
    bool CheckIfLegitMineSpot(int32 position);
    UFUNCTION(BluePrintCallable, Category="Gameplay Functions")
    bool CheckIfLegitFarmSpot(int32 position);
    
    //Initialization
    UFUNCTION(BluePrintCallable, Category="Initialization Functions")
    void InitializeCivManagerMapArrays();
    
    //Internal functions
    
    float getMovementCost(int32 source, int32 destination, int32 direction);
    float ManhattanDistanceHeuristic(int32 a, int32 b);
    void UpdateHexFeats(int32 position, int32 inForest, EImprovement inImprovement);
    
    int32 getX(int32 i);
    int32 getY(int32 i);
    int32 getNeighbor(int32 x, int32 y, int32 dir);
};


struct moveCosts {
    float priority;
    int32 value;
};

struct CompareMoveCosts {
    FORCEINLINE bool operator()(const moveCosts& A, const moveCosts& B) const
    {
        return A.priority < B.priority;
    }
};