#include "ProfilingAutomation.h"
#include "UE5TopDownARPGAutomationTestBase.h"
#include "../UE5TopDownARPG.h"
#include "../UE5TopDownARPGCharacter.h"

#include "GameFramework/PlayerStart.h"

bool FLatent_ActivateAbility::Update()
{
	//if (IsValid(ActiveCharacter) == false)
	//{
	//	UE_LOG(LogUE5TopDownARPG, Error, TEXT("FLatent_ActivateAbility::Update IsValid(ActiveCharacter) == false"));
	//	return false;
	//}

	//if (IsValid(TargetCharacter) == false)
	//{
	//	UE_LOG(LogUE5TopDownARPG, Error, TEXT("FLatent_ActivateAbility::Update IsValid(TargetCharacter) == false"));
	//	return false;
	//}

	//if (ActiveCharacter->ActivateAbility(TargetCharacter->GetActorLocation()) == false)
	//{
	//	UE_LOG(LogUE5TopDownARPG, Error, TEXT("FLatent_ActivateAbility::Update ActiveCharacter->ActivateAbility(TargetCharacter->GetActorLocation()) == false"));
	//	return false;
	//}

	return true;
}

//bool FLatent_CheckBoltAbilityTargetHealth::Update()
//{
//	UE_LOG(LogUE5TopDownARPG, Display, TEXT("Bolt skill - the target is expected to be at %f health"), ExpectedHealth);
//	UE_LOG(LogUE5TopDownARPG, Display, TEXT("Target current health: %f"), Target->GetHealth());
//
//	if (Target->GetHealth() != ExpectedHealth)
//	{
//		UE_LOG(LogUE5TopDownARPG, Error, TEXT("FTBoltAbilityTest::FLatent_CheckBoltTargetHealth() Target->GetHealth() != ExpectedHealth"));
//	}
//	else
//	{
//		UE_LOG(LogUE5TopDownARPG, Display, TEXT("Test Success ----- FTBoltAbilityTest::FLatent_CheckBoltTargetHealth() Target->GetHealth() == ExpectedHealth -----"));
//	}
//	
//	return true;
//}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FT1UnitMazeTest, FUE5TopDownARPGAutomationTestBase, "CrowdPF.FT1UnitMaze",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)

	bool FT1UnitMazeTest::RunTest(const FString& params)
{	
	AutomationOpenMap(TEXT("/Game/Content/TopDown/Maps/1UnitMaze"));

	UWorld* World = GetAutomationTestWorld();

	if (IsValid(World) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("FT1UnitMazeTest::RunTest() IsValid(World) == false"));
		return false;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand);

	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FT50UnitMaze, FUE5TopDownARPGAutomationTestBase, "CrowdPF.FT50UnitMaze",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)

	bool FT50UnitMaze::RunTest(const FString& params)
{
	AutomationOpenMap(TEXT("/Game/Content/TopDown/Maps/50UnitMaze"));

	UWorld* World = GetAutomationTestWorld();

	if (IsValid(World) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("FT50UnitMaze::RunTest() IsValid(World) == false"));
		return false;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand);

	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FT200UnitMaze, FUE5TopDownARPGAutomationTestBase, "CrowdPF.FT200UnitMaze",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)

	bool FT200UnitMaze::RunTest(const FString& params)
{
	AutomationOpenMap(TEXT("/Game/Content/TopDown/Maps/200UnitMaze"));

	UWorld* World = GetAutomationTestWorld();

	if (IsValid(World) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("ProfilingAutomation::RunTest() IsValid(World) == false"));
		return false;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand);

	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FT1UnitSimple, FUE5TopDownARPGAutomationTestBase, "CrowdPF.FT1UnitSimple",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)

	bool FT1UnitSimple::RunTest(const FString& params)
{
	AutomationOpenMap(TEXT("/Game/Content/TopDown/Maps/1UnitSimple"));

	UWorld* World = GetAutomationTestWorld();

	if (IsValid(World) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("FT1UnitSimple::RunTest() IsValid(World) == false"));
		return false;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand);

	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FT50UnitSimple, FUE5TopDownARPGAutomationTestBase, "CrowdPF.FT50UnitSimple",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)

	bool FT50UnitSimple::RunTest(const FString& params)
{
	AutomationOpenMap(TEXT("/Game/Content/TopDown/Maps/50UnitSimple"));

	UWorld* World = GetAutomationTestWorld();

	if (IsValid(World) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("ProfilingAutomation::RunTest() IsValid(World) == false"));
		return false;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand);

	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FT200UnitSimple, FUE5TopDownARPGAutomationTestBase, "CrowdPF.FT200UnitSimple",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)

	bool FT200UnitSimple::RunTest(const FString& params)
{
	AutomationOpenMap(TEXT("/Game/Content/TopDown/Maps/200UnitSimple"));

	UWorld* World = GetAutomationTestWorld();

	if (IsValid(World) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("ProfilingAutomation::RunTest() IsValid(World) == false"));
		return false;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand);

	return true;
}