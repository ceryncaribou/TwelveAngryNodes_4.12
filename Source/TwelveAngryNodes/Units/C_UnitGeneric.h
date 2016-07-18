// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "C_UnitGeneric.generated.h"

/**
 * 
 */
UCLASS(Meta=(ChildCanTick))
class TWELVEANGRYNODES_API AC_UnitGeneric : public AActor
{
	GENERATED_BODY()
	
public:
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Unit Base Properties")
    float maxMoves;
    
    
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Unit Status")
    float remainingMoves;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Unit Status", Meta=(ExposeOnSpawn=true))
    int32 position;
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Unit Status")
    TArray<int32> currentpath;
    
    //Array reference info
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Unit Reference Info", Meta=(ExposeOnSpawn=true))
    int32 owningPlayer;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Unit Reference Info", Meta=(ExposeOnSpawn=true))
    int32 unitIndex;
    
    //FUNCTIONS
    
    AC_UnitGeneric(const FObjectInitializer& ObjectInitializer);
    
    
    
};
