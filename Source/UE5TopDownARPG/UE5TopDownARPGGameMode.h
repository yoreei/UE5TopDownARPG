// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CrowdPF/Public/CrowdPF.h"
#include "UE5TopDownARPGGameMode.generated.h"

UCLASS(minimalapi)
class AUE5TopDownARPGGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AUE5TopDownARPGGameMode();

	void StartPlay() override;
	void EndGame(bool IsWin);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
	bool UseCrowdPf;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
	bool DrawDebugPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
	FCrowdPFOptions Options;
};



