#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NBBPlayerState.generated.h"

UCLASS()
class CH4_1_API ANBBPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	const FString& GetPlayerNameString() const { return PlayerNameString; }
	int32 GetCurrentGuessCount() const { return CurrentGuessCount; }
	int32 GetMaxGuessCount() const { return MaxGuessCount; }
	bool HasGuessLeft() const { return CurrentGuessCount < MaxGuessCount; }

	// "[1/3]"
	FString GetGuessCountString() const;
	// "Player1 [1/3]"
	FString GetPlayerInfoString() const;

	// 서버에서만 호출
	void SetPlayerNameString(const FString& InName);
	void SetMaxGuessCount(int32 InMax);
	void IncreaseGuessCount();
	void ResetGuessCount();
	void UseAllGuesses();

protected:
	UPROPERTY(Replicated)
	FString PlayerNameString;

	UPROPERTY(Replicated)
	int32 CurrentGuessCount = 0;

	UPROPERTY(Replicated)
	int32 MaxGuessCount = 3;
};
