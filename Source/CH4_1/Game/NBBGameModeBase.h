#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NBBGameModeBase.generated.h"

class ANBBPlayerController;

UCLASS()
class CH4_1_API ANBBGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANBBGameModeBase();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	// 클라 UI 준비 완료 (PlayerController BeginPlay에서 알려줌)
	void OnPlayerReady(ANBBPlayerController* PC);

	// 채팅/숫자 입력 처리
	void ProcessChatMessage(ANBBPlayerController* Sender, const FString& InMessage);

	// 1~9 중 중복 없는 3자리
	FString GenerateRandomNumbers() const;

	// 규칙에 맞으면 빈 문자열, 아니면 안내 문구
	FString ValidateGuess(const FString& Input) const;

	// "1S1B", "3S0B", "OUT"
	FString CheckAnswer(const FString& Input) const;

	void ResetGame();

protected:
	// 승리/무승부 판정. 라운드가 끝났으면 true
	bool JudgeGame(ANBBPlayerController* Sender, const FString& Result);
	bool AreAllPlayersOutOfChances() const;
	void EndRound(const FString& ResultMessage);

	void TryStartGame();
	void StopGame();

	// 턴제
	void StartTurn(ANBBPlayerController* TurnPlayer);
	void AdvanceTurn();
	void OnTurnTimeout();
	void ClearTurn();
	ANBBPlayerController* FindNextTurnPlayer(const ANBBPlayerController* From) const;

	void BroadcastChat(const FString& Message, const FLinearColor& Color = FLinearColor::White);
	void BroadcastNotification(const FString& Message, float Duration);

	UPROPERTY(EditDefaultsOnly, Category = "Baseball")
	int32 MaxGuessCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Baseball")
	int32 MinPlayersToStart = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Baseball")
	float ResetDelay = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Baseball")
	float NoticeDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Baseball|Turn")
	bool bUseTurnMode = true;

	UPROPERTY(EditDefaultsOnly, Category = "Baseball|Turn")
	float TurnTimeLimit = 15.f;

private:
	FString SecretNumber;
	bool bGameStarted = false;
	bool bRoundOver = false;
	int32 PlayerNumberCounter = 0;

	UPROPERTY()
	TArray<TObjectPtr<ANBBPlayerController>> AllPlayerControllers;

	UPROPERTY()
	TObjectPtr<ANBBPlayerController> CurrentTurnPlayer;

	// 라운드 첫 턴을 돌아가면서 잡으려고 저장
	UPROPERTY()
	TObjectPtr<ANBBPlayerController> RoundFirstPlayer;

	FTimerHandle ResetTimerHandle;
	FTimerHandle TurnTimerHandle;
};
