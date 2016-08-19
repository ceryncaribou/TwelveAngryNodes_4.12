// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Units/C_UnitGeneric.h"
#include "C_HexTile.generated.h"

/**
 * 
 */


UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class EResource : uint8
{
    VE_None         UMETA(DisplayName="None"),
    //Mines
    VE_Copper       UMETA(DisplayName="Copper"),
    VE_Iron         UMETA(DisplayName="Iron"),
    VE_Mithril      UMETA(DisplayName="Mithril"),
    VE_Gems         UMETA(DisplayName="Gems"),
    VE_Gold         UMETA(DisplayName="Gold"),
    VE_Silver       UMETA(DisplayName="Silver"),
    //Quarries
    VE_Granite      UMETA(DisplayName="Granite"),
    VE_Limestone	UMETA(DisplayName="Limestone"),
    VE_Marble       UMETA(DisplayName="Marble"),
    VE_Salt         UMETA(DisplayName="Salt"),
    VE_Slate        UMETA(DisplayName="Slate"),
    //Fishing boats
    //Camps
    //Pastures
    //Farms
    VE_Barley       UMETA(DisplayName="Barley"),
    VE_Beans        UMETA(DisplayName="Beans"),
    VE_Corn         UMETA(DisplayName="Corn"),
    VE_Rice         UMETA(DisplayName="Rice"),
    VE_Tomato       UMETA(DisplayName="Tomato"),
    VE_Wheat        UMETA(DisplayName="Wheat"),
    //Plantations
};

UENUM(BlueprintType)
enum class ETerrain : uint8
{
    VE_Ocean        UMETA(DisplayName="Ocean"),
    VE_Coast        UMETA(DisplayName="Coast"),
    VE_Lake         UMETA(DisplayName="Lake"),
    VE_Marsh        UMETA(DisplayName="Marsh"),
    VE_Grassland    UMETA(DisplayName="Grassland"),
    VE_Plain        UMETA(DisplayName="Plain"),
    VE_Tundra       UMETA(DisplayName="Tundra"),
    VE_Snow         UMETA(DisplayName="Snow"),
    VE_Desert       UMETA(DisplayName="Desert"),
    VE_Peak         UMETA(DisplayName="Peak"),
    VE_Void         UMETA(DisplayName="Void"),
    VE_Hell         UMETA(DisplayName="Hell")
};

UENUM(BlueprintType)
enum class EImprovement : uint8
{
    VE_None         UMETA(DisplayName="None"),
    VE_Mine         UMETA(DisplayName="Mine"),
    VE_Quarry       UMETA(DisplayName="Quarry"),
    VE_FishingBoat  UMETA(DisplayName="Fishing Boat"),
    VE_Camp         UMETA(DisplayName="Camp"),
    VE_Pasture      UMETA(DisplayName="Pasture"),
    VE_Farm         UMETA(DisplayName="Farm"),
    VE_Plantation   UMETA(DisplayName="Plantation"),
    VE_Lumbermill   UMETA(DisplayName="Lumbermill")
};


UCLASS()
class TWELVEANGRYNODES_API AC_HexTile : public AActor
{
	GENERATED_BODY()
public:
    
//VARIABLES
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Position Variables", Meta=(ExposeOnSpawn=true))
    int32 xgrid;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Position Variables", Meta=(ExposeOnSpawn=true))
    int32 ygrid;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info Variables", Meta=(ExposeOnSpawn=true))
    int32 index;
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info Variables", Meta=(ExposeOnSpawn=true))
    int32 altitude;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info Variables", Meta=(ExposeOnSpawn=true))
    ETerrain terrain;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info Variables", Meta=(ExposeOnSpawn=true))
    int32 hasForest;
    
    //0=no water, 1=water from lake/irrigation, 2=water from river
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info Variables", Meta=(ExposeOnSpawn=true))
    int32 hasWater;
    
    //-1 is no city; higher values correspond to owning civ ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info Variables")
    int32 hasCityDistrictFromCiv;
    //-1 is no city; higher values correspond to owning city index in the civ's city array
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info Variables")
    int32 cityIDfromCiv;
    
    //Resource type : banana, marble, horses, etc.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info Variables", Meta=(ExposeOnSpawn=true))
    EResource resourceType;
    //Goes from 0 to 5; mainly used for quarry resources
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info Variables", Meta=(ExposeOnSpawn=true))
    int32 resourceRotation;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Info Variables", Meta=(ExposeOnSpawn=true))
    EImprovement improvementType;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Units")
    TArray<AC_UnitGeneric*> UnitListOnHex;
    
    //Neighbor link for transitions between tiles. WARNING : NEVER _EVER_ MODIFY NEIGHBORS DIRECTLY FROM A HEX
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Neighbor Links")
    AC_HexTile* neighbor1;
    //Neighbor link for transitions between tiles. WARNING : NEVER _EVER_ MODIFY NEIGHBORS DIRECTLY FROM A HEX
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Neighbor Links")
    AC_HexTile* neighbor2;
    //Neighbor link for transitions between tiles. WARNING : NEVER _EVER_ MODIFY NEIGHBORS DIRECTLY FROM A HEX
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Neighbor Links")
    AC_HexTile* neighbor3;
    //Neighbor link for transitions between tiles. WARNING : NEVER _EVER_ MODIFY NEIGHBORS DIRECTLY FROM A HEX
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Neighbor Links")
    AC_HexTile* neighbor4;
    //Neighbor link for transitions between tiles. WARNING : NEVER _EVER_ MODIFY NEIGHBORS DIRECTLY FROM A HEX
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Neighbor Links")
    AC_HexTile* neighbor5;
    //Neighbor link for transitions between tiles. WARNING : NEVER _EVER_ MODIFY NEIGHBORS DIRECTLY FROM A HEX
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Neighbor Links")
    AC_HexTile* neighbor6;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Game Info Variables")
    int32 foodYield;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Game Info Variables")
    int32 prodYield;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Game Info Variables")
    int32 coinYield;
    
    
//FUNCTIONS
    
    AC_HexTile(const FObjectInitializer& ObjectInitializer);
    
    UFUNCTION(BluePrintCallable, Category="Yield Functions")
    void CalcTileYields();
    UFUNCTION(BluePrintCallable, Category="Yield Functions")
    void UpdateYieldsCity(int32 newDistrictFromCiv);
    //UFUNCTION(BluePrintCallable, Category="Yield Functions")
    //void UpdateYieldsImprovement(bool hasDistrict);
    //UFUNCTION(BluePrintCallable, Category="Yield Functions")
    //void UpdateYieldsTerrainFeatures(bool hasDistrict);
    //UFUNCTION(BluePrintCallable, Category="Yield Functions")
    //void UpdateYieldsResource(EResourceTypes resource);
    
    
    
    
    
	
};
