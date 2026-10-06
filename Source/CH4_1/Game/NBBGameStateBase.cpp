#include "Game/NBBGameStateBase.h"
#include "Player/NBBPlayerController.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

void ANBBGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANBBGameStateBase, TurnPlayerName);
	DOREPLIFETIME(ANBBGameStateBase, TurnEndServerTime);
}

void ANBBGameStateBase::SetTurnInfo(const FString& InTurnPlayerName, double InTurnEndServerTime)
{
	if (HasAuthority())
	{
		TurnPlayerName = InTurnPlayerName;
		TurnEndServerTime = InTurnEndServerTime;
		ForceNetUpdate();
	}
}

void ANBBGameStateBase::ClearTurnInfo()
{
	SetTurnInfo(FString(), 0.0);
}

float ANBBGameStateBase::GetTurnRemainingTime() const
{
	if (TurnPlayerName.IsEmpty())
	{
		return 0.f;
	}
	return static_cast<float>(FMath::Max(0.0, TurnEndServerTime - GetServerWorldTimeSeconds()));
}

ANBBPlayerController* ANBBGameStateBase::GetLocalPC() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// 리슨 서버는 다른 클라 PC도 들고 있어서 로컬 PC만 찾음
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ANBBPlayerController* PC = Cast<ANBBPlayerController>(It->Get());
		if (PC && PC->IsLocalController())
		{
			return PC;
		}
	}
	return nullptr;
}

void ANBBGameStateBase::MulticastShowNotification_Implementation(const FString& Message, float Duration)
{
	if (ANBBPlayerController* PC = GetLocalPC())
	{
		PC->ShowNotification(Message, Duration);
	}
}
