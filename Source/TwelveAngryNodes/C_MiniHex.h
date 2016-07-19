// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "C_MiniHex.generated.h"

UCLASS()
class TWELVEANGRYNODES_API AC_MiniHex : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AC_MiniHex();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	//Identification number inside of a given HexTile; follows :
    //     00/01/02/03
    //    04/05/06/07/08
    //  09/10/11/12/13/14
    //15/16/17/18/19/20/21
    //  22/23/24/25/26/27
    //    28/29/30/31/32
    //     33/34/35/36
    //with the 0-1-2-3 side corresponding to the side 1 (going to the right) on a Hextile
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Position Variables", Meta=(ExposeOnSpawn=true))
    int32 idNum;
    
    //altitude of the minihex
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Position Variables", Meta=(ExposeOnSpawn=true))
    float altitude;
    
};
