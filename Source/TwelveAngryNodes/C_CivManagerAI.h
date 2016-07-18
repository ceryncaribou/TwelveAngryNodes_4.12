// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "C_CivManagerInterface.h"
#include "C_CivManagerAI.generated.h"

/**
 * 
 */
UCLASS()
class TWELVEANGRYNODES_API AC_CivManagerAI : public AC_CivManagerInterface
{
	GENERATED_BODY()
	
public:
    
    AC_CivManagerAI(const FObjectInitializer& ObjectInitializer);
    
//    virtual bool playTurn() OVERRIDE;
	
	
};
