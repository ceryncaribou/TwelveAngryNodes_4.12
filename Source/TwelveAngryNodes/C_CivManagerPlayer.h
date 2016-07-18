// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "C_CivManagerInterface.h"
#include "C_CivManagerPlayer.generated.h"

/**
 * 
 */
UCLASS(Meta=(ChildCanTick))
class TWELVEANGRYNODES_API AC_CivManagerPlayer : public AC_CivManagerInterface
{
	GENERATED_BODY()
	
public:
    
    AC_CivManagerPlayer(const FObjectInitializer& ObjectInitializer);
    
//    virtual bool playTurn() OVERRIDE;
	
};
