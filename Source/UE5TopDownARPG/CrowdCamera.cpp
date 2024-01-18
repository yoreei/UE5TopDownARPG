// Fill out your copyright notice in the Description page of Project Settings.


#include "CrowdCamera.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

// Sets default values
ACrowdCamera::ACrowdCamera()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	//CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	
	//CameraBoom->SocketOffset = FVector(99999.0f, 999999.0f, 99999.0f);

	//CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	//TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
}

// Called when the game starts or when spawned
void ACrowdCamera::BeginPlay()
{
	Super::BeginPlay();
	CameraBoom->TargetArmLength = 0.f;
	CameraBoom->SetRelativeRotation(FRotator(-80.f, 0.f, 0.f));
	TopDownCameraComponent->FieldOfView = 75.f;
	CameraBoom->SocketOffset = FVector(-3500.0f, 0.0f, 0.0f);
}

// Called every frame
void ACrowdCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ACrowdCamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

