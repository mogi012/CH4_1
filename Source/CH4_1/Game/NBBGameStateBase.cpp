#include "Game/NBBGameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

void ANBBGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANBBGameStateBase, TurnPlayerName);
	DOREPLIFETIME(ANBBGameStateBase, TurnEndServerTime);
	DOREPLIFETIME(ANBBGameStateBase, Notification);
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

void ANBBGameStateBase::SetNotification(const FString& Message, float Duration)
{
	if (!HasAuthority())
	{
		return;
	}

	Notification.Message = Message;
	Notification.Duration = FMath::Max(0.f, Duration);
	++Notification.Revision;
	GetWorldTimerManager().ClearTimer(NotificationTimerHandle);
	if (Notification.Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(NotificationTimerHandle, this, &ANBBGameStateBase::ExpireNotification, Notification.Duration, false);
	}
	ForceNetUpdate();
}

void ANBBGameStateBase::ExpireNotification()
{
	Notification.Duration = 0.f;
	++Notification.Revision;
	ForceNetUpdate();
}
