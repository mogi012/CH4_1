#include "Player/NBBPlayerState.h"
#include "Net/UnrealNetwork.h"

void ANBBPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANBBPlayerState, PlayerNameString);
	DOREPLIFETIME(ANBBPlayerState, CurrentGuessCount);
	DOREPLIFETIME(ANBBPlayerState, MaxGuessCount);
}

FString ANBBPlayerState::GetGuessCountString() const
{
	return FString::Printf(TEXT("[%d/%d]"), GetCurrentGuessCount(), GetMaxGuessCount());
}

FString ANBBPlayerState::GetPlayerInfoString() const
{
	return FString::Printf(TEXT("%s %s"), *PlayerNameString, *GetGuessCountString());
}

void ANBBPlayerState::SetPlayerNameString(const FString& InName)
{
	if (HasAuthority())
	{
		PlayerNameString = InName;
		ForceNetUpdate();
	}
}

void ANBBPlayerState::SetMaxGuessCount(int32 InMax)
{
	if (HasAuthority())
	{
		MaxGuessCount = InMax;
		ForceNetUpdate();
	}
}

void ANBBPlayerState::IncreaseGuessCount()
{
	if (HasAuthority() && CurrentGuessCount < MaxGuessCount)
	{
		++CurrentGuessCount;
		ForceNetUpdate();
	}
}

void ANBBPlayerState::ResetGuessCount()
{
	if (HasAuthority())
	{
		CurrentGuessCount = 0;
		ForceNetUpdate();
	}
}

void ANBBPlayerState::UseAllGuesses()
{
	if (HasAuthority())
	{
		CurrentGuessCount = MaxGuessCount;
		ForceNetUpdate();
	}
}
