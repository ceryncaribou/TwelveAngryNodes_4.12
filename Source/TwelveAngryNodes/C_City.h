// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "C_GameManager.h"
#include "C_City.generated.h"



UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class ESpecialist : uint8
{
    VE_Artist       UMETA(DisplayName="Artist"),
    VE_Craftsman 	UMETA(DisplayName="Craftsman"),
    VE_Merchant     UMETA(DisplayName="Merchant"),
    VE_Sage         UMETA(DisplayName="Sage")
};

/**
 * 
 */
UCLASS()
class TWELVEANGRYNODES_API AC_City : public AActor
{
	GENERATED_BODY()
public:
	
//VARIABLES
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Position Variables", Meta=(ExposeOnSpawn=true))
    int32 position;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Citizen Allocation Variables")
    TArray<int32> workableTiles;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Citizen Allocation Variables")
    TArray<bool> workedTiles;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Citizen Allocation Variables")
    int32 maxArtists;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Citizen Allocation Variables")
    int32 maxCraftsmen;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Citizen Allocation Variables")
    int32 maxMerchants;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Citizen Allocation Variables")
    int32 maxSages;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Citizen Allocation Variables")
    int32 currentArtists;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Citizen Allocation Variables")
    int32 currentCraftsmen;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Citizen Allocation Variables")
    int32 currentMerchants;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Citizen Allocation Variables")
    int32 currentSages;
    
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="City Info Variables")
    int32 currentPopulation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="City Info Variables")
    int32 currentInfluence;
    
    //A 7 slot array going 0=center, 1=right and then up to six anticlockwise
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="City Info Variables")
    TArray<bool> districtList;
    
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Game Reference", Meta=(ExposeOnSpawn=true))
    int32 owningCiv;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Game Reference", Meta=(ExposeOnSpawn=true))
    int32 cityID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Game Reference", Meta=(ExposeOnSpawn=true))
    AC_GameManager* gameManager;
    
    
//FUNCTIONS
	AC_City(const FObjectInitializer& ObjectInitializer);
    
    //Initialize city
    UFUNCTION(BluePrintCallable, Category="City Initialization Functions")
    void initializeCity();
    
    //Allocates citizens
    UFUNCTION(BluePrintCallable, Category="City AI Functions")
    void allocateCitizens();
    
    //Specialist yield functions
	int32 getSpecFoodYield(ESpecialist specType);
    int32 getSpecProdYield(ESpecialist specType);
    int32 getSpecCoinYield(ESpecialist specType);
    int32 getSpecInfluenceYield(ESpecialist specType);
    int32 getSpecResearchYield(ESpecialist specType);
    int32 getSpecHappyYield(ESpecialist specType);
    int32 getSpecHealthYield(ESpecialist specType);
};
