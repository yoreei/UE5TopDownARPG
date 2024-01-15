// Fill out your copyright notice in the Description page of Project Settings.


#include "CrowdCharacter.h"

// Sets default values
ACrowdCharacter::ACrowdCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACrowdCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACrowdCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ACrowdCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

