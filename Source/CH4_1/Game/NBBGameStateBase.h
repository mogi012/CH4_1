#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NBBGameStateBase.generated.h"

class ANBBPlayerController;

UCLASS()
class CH4_1_API ANBBGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastShowNotification(const FString& Message, float Duration);

	// 서버에서만 호출
	void SetTurnInfo(const FString& InTurnPlayerName, double InTurnEndServerTime);
	void ClearTurnInfo();

	const FString& GetTurnPlayerName() const { return TurnPlayerName; }
	float GetTurnRemainingTime() const;

protected:
	// 현재 턴인 플레이어 이름. 비어 있으면 턴 없음
	UPROPERTY(Replicated)
	FString TurnPlayerName;

	// 턴 종료 시각 (서버 시간 기준)
	UPROPERTY(Replicated)
	double TurnEndServerTime = 0.0;

private:
	ANBBPlayerController* GetLocalPC() const;
};
