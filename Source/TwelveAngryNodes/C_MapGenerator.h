// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "C_GameManager.h"
#include "C_MapGenerator.generated.h"

/**
 * 
 */


struct WaterBody {
    TArray<int32> tiles;
    int32 altitude;
};
/*
 struct ConnectedLandBody {
 TArray<int32> tiles;
 };
 */


struct RiverSegment {
    TArray<int32> LeftBank;
    TArray<int32> RightBank;
    TArray<int32> dir;//to go from left to right
    int32 length;
    int32 segAltitude;
    int32 segStart;
    int32 segEnd;
    
    RiverSegment(int32 left, int32 right, int32 direction, int32 start){
        LeftBank.Push(left);
        RightBank.Push(right);
        dir.Push(direction);
        segStart=start;
        length=1;
    }
};

struct GoldbergFace {
    float c1x, c1y, c1z, c2x, c2y, c2z, c3x, c3y, c3z;//corners of a triangular face
    int c1,c2,c3;//ref index of each corner
    
    GoldbergFace(float ic1x, float ic1y, float ic1z, float ic2x, float ic2y, float ic2z, float ic3x, float ic3y, float ic3z, int ic1, int ic2, int ic3)
        :   c1x(ic1x)
        ,   c1y(ic1y)
        ,   c1z(ic1z)
        ,   c2x(ic2x)
        ,   c2y(ic2y)
        ,   c2z(ic2z)
        ,   c3x(ic3x)
        ,   c3y(ic3y)
        ,   c3z(ic3z)
        ,   c1(ic1)
        ,   c2(ic2)
        ,   c3(ic3)
    {   }
};

USTRUCT()
struct FGoldbergLink {
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> links;
    
    FGoldbergLink() {    }
};

UCLASS()
class TWELVEANGRYNODES_API AC_MapGenerator : public AActor
{
    GENERATED_BODY()
    
public:
    
    
    //VARIABLES
    
    //Map size in x
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info", Meta=(ExposeOnSpawn=true))
    int32 mapsizex;
    
    //Map size in y
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info", Meta=(ExposeOnSpawn=true))
    int32 mapsizey;
    
    //Position of goldberg polyhedron pents and hexes; size of pent arrays will always be 12, size of hex arrays will depend on the size of the map, and go as 10(x^2-1) where x is the number of hexes between pents + 1
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Goldberg Info")
    TArray<float> xpent;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Goldberg Info")
    TArray<float> ypent;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Goldberg Info")
    TArray<float> zpent;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Goldberg Info")
    TArray<float> xhex;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Goldberg Info")
    TArray<float> yhex;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Goldberg Info")
    TArray<float> zhex;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Goldberg Info")
    TArray<float> scalefactor;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex References")
    TArray<FGoldbergLink > pent_links;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex References")
    TArray<FGoldbergLink > hex_links;
    
    //Possible values : 0, 1, 2, 3. At the end of GenerateTerrainType(), all non zero values get a -1, so that possible values are 0, 1->0, 2->1, 3->2.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> AltitudeMap;
    
    //Terrain type map
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<ETerrain> TerrainType;
    
    //Generic terrain info; contain indexes of all tiles which fit the profile
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn Info")
    TArray<int32> LandTiles;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn Info")
    TArray<int32> WaterTiles;
    
    
    //Stocking river info for pathfinding
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
    
    //Map features
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> Forests;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> freshWater;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<EResource> resources;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn Info")
    TArray<int32> resourceRotations;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<EImprovement> improvements;

    
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
    
    //Manager reference
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Manager Reference", Meta=(ExposeOnSpawn=true))
    AC_GameManager* manager;
    
    //Internal variables
    
    FRandomStream randomstream;
    TArray<WaterBody*> Lakes;
    TArray<RiverSegment*> Rivers;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info")
    TArray<int32> initialCityDistricts;
    
    //FUNCTIONS
    
    AC_MapGenerator(const FObjectInitializer& ObjectInitializer);
    
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    void GenerateAltitudeMap();
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    void PostProcessLongLandChains();
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    void PostProcessLakes();
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    void BuildRivers(int32 numberOfRivers);
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    void CheckFreshWater();
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    void GenerateTerrainType();
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    void GenerateDeserts();
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    void ReduceLandAltitude();
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    void PlaceForests();
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    void PlaceResources();
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    void PlaceImprovements();
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    void GetStartingSpots();
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    void InitializeGameManager();
    
    //Goldberg polyhedron related functions; numDivs is the number of hexes between pents, radiusScaling is a float used to scaled the radius of the sphere (usually slightly larger than 1). Method also calculates and stores arrays of information on tile neighbors
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    float GenerateGoldbergPolyhedron(int numDivs, float radiusScaling);
    UFUNCTION(BluePrintCallable, Category="Map Generation Functions")
    void OrderGoldbergLinking();
        
    //River utility functions to access what is inside the "Rivers" array
    UFUNCTION(BluePrintCallable, Category="River Utility Functions")
    int32 getTotalNumberOfRiverSegments();
    UFUNCTION(BluePrintCallable, Category="River Utility Functions")
    TArray<int32> getLeftBankArray(int32 i);
    UFUNCTION(BluePrintCallable, Category="River Utility Functions")
    TArray<int32> getRightBankArray(int32 i);
    UFUNCTION(BluePrintCallable, Category="River Utility Functions")
    TArray<int32> getdirArray(int32 i);
    UFUNCTION(BluePrintCallable, Category="River Utility Functions")
    int32 getSegAltitude(int32 i);
    UFUNCTION(BluePrintCallable, Category="River Utility Functions")
    int32 getSegStart(int32 i);
    UFUNCTION(BluePrintCallable, Category="River Utility Functions")
    int32 getSegEnd(int32 i);
    
    
    //Internal and access functions
        
    UFUNCTION(BluePrintCallable, Category="Access Functions")
    int32 getNeighbor(int32 index, int32 dir);
        
    bool CheckIfLake(int32 start, WaterBody *lake);
    int32 CheckIfLakeTile(int32 i);
    bool CheckIfOcean(int32 i);
    
    bool BuildRiverSegment(int32 startLeft, int32 startRight, int32 startDir, int initstate, bool waterStart, int32 waterStartAltitude);
    bool CheckifAlreadyInLeftBank(int32 i);
    bool CheckifAlreadyInRightBank(int32 i);
    bool CheckIfEligibleRiverStart(int32 i, int32 dir);
    bool CheckIfEligibleRiverLakeStart(int32 i, int32 dir, int32 lake);
    bool CheckIfTileIsNextToWater(int32 index);
    float getApparentLatitude(int32 index);
    float getLatitude(int32 index);
    float getSnowWeight(float latitude);
    float getTundraWeight(float latitude);
    float getPlainWeight(float latitude);
    float getGrassWeight(float latitude);
};
