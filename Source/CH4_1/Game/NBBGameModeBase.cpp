#include "Game/NBBGameModeBase.h"
#include "Game/NBBGameStateBase.h"
#include "Player/NBBPlayerController.h"
#include "Player/NBBPlayerState.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogNumBaseball, Log, All);

namespace NBBColors
{
	const FLinearColor System(0.6f, 0.9f, 1.f);
	const FLinearColor Result(1.f, 0.85f, 0.2f);
	const FLinearColor NewGame(0.6f, 1.f, 0.6f);
	const FLinearColor Turn(0.8f, 0.8f, 1.f);
}

ANBBGameModeBase::ANBBGameModeBase()
{
	PlayerControllerClass = ANBBPlayerController::StaticClass();
	PlayerStateClass = ANBBPlayerState::StaticClass();
	GameStateClass = ANBBGameStateBase::StaticClass();
}

void ANBBGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	SecretNumber = GenerateRandomNumbers();
	UE_LOG(LogNumBaseball, Warning, TEXT("[Server] 정답: %s"), *SecretNumber);
}

void ANBBGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (ANBBPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ANBBPlayerState>() : nullptr)
	{
		++PlayerNumberCounter;
		PS->SetPlayerNameString(FString::Printf(TEXT("Player%d"), PlayerNumberCounter));
		PS->SetMaxGuessCount(MaxGuessCount);
		PS->ResetGuessCount();
	}
}

void ANBBGameModeBase::OnPlayerReady(ANBBPlayerController* PC)
{
	if (!IsValid(PC) || AllPlayerControllers.Contains(PC))
	{
		return;
	}

	ANBBPlayerState* PS = PC->GetPlayerState<ANBBPlayerState>();
	if (!PS)
	{
		return;
	}

	AllPlayerControllers.Add(PC);
	UE_LOG(LogNumBaseball, Log, TEXT("[Server] %s 입장 (총 %d명)"), *PS->GetPlayerNameString(), AllPlayerControllers.Num());
	BroadcastChat(FString::Printf(TEXT("%s 님이 입장했습니다."), *PS->GetPlayerNameString()), NBBColors::System);

	if (bGameStarted)
	{
		// 진행 중인 판에는 끼지 않고 다음 판부터
		PS->UseAllGuesses();
		PC->ClientReceivePrivateMessage(TEXT("진행 중인 판이 끝나면 다음 판부터 참여합니다."));
		return;
	}

	TryStartGame();
}

void ANBBGameModeBase::Logout(AController* Exiting)
{
	ANBBPlayerController* ExitingPC = Cast<ANBBPlayerController>(Exiting);
	if (!ExitingPC || !AllPlayerControllers.Contains(ExitingPC))
	{
		Super::Logout(Exiting);
		return;
	}

	const bool bWasTurnPlayer = ExitingPC == CurrentTurnPlayer;
	ANBBPlayerController* NextPlayer = bWasTurnPlayer ? FindNextTurnPlayer(ExitingPC) : nullptr;

	AllPlayerControllers.Remove(ExitingPC);

	if (const ANBBPlayerState* PS = ExitingPC->GetPlayerState<ANBBPlayerState>())
	{
		UE_LOG(LogNumBaseball, Log, TEXT("[Server] %s 퇴장"), *PS->GetPlayerNameString());
		BroadcastChat(FString::Printf(TEXT("%s 님이 나갔습니다."), *PS->GetPlayerNameString()), NBBColors::System);
	}
	if (RoundFirstPlayer == ExitingPC)
	{
		RoundFirstPlayer = nullptr;
	}
	if (bWasTurnPlayer)
	{
		CurrentTurnPlayer = nullptr;
	}

	Super::Logout(Exiting);

	if (AllPlayerControllers.Num() < MinPlayersToStart)
	{
		if (bGameStarted)
		{
			StopGame();
		}
		return;
	}

	if (!bGameStarted || bRoundOver)
	{
		return;
	}

	// 남은 사람이 전부 기회를 다 썼으면 무승부
	if (AreAllPlayersOutOfChances())
	{
		EndRound(TEXT("무승부! 아무도 정답을 맞히지 못했습니다."));
		return;
	}

	if (bUseTurnMode && bWasTurnPlayer)
	{
		StartTurn(NextPlayer);
	}
}

void ANBBGameModeBase::TryStartGame()
{
	if (bGameStarted)
	{
		return;
	}

	if (AllPlayerControllers.Num() < MinPlayersToStart)
	{
		BroadcastChat(FString::Printf(TEXT("다른 플레이어를 기다리는 중입니다. (%d/%d)"), AllPlayerControllers.Num(), MinPlayersToStart), NBBColors::System);
		return;
	}

	bGameStarted = true;
	bRoundOver = false;

	for (const ANBBPlayerController* PC : AllPlayerControllers)
	{
		if (ANBBPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<ANBBPlayerState>() : nullptr)
		{
			PS->ResetGuessCount();
		}
	}

	UE_LOG(LogNumBaseball, Log, TEXT("[Server] 게임 시작"));
	BroadcastChat(TEXT("----- 게임 시작! 1~9 중 서로 다른 숫자 3개를 맞혀 보세요 -----"), NBBColors::NewGame);
	BroadcastNotification(TEXT("게임 시작!"), NoticeDuration);

	if (bUseTurnMode)
	{
		RoundFirstPlayer = FindNextTurnPlayer(nullptr);
		StartTurn(RoundFirstPlayer);
	}
}

void ANBBGameModeBase::StopGame()
{
	ClearTurn();
	GetWorldTimerManager().ClearTimer(ResetTimerHandle);

	bGameStarted = false;
	bRoundOver = false;
	RoundFirstPlayer = nullptr;

	SecretNumber = GenerateRandomNumbers();
	UE_LOG(LogNumBaseball, Warning, TEXT("[Server] 인원 부족으로 중단. 다음 정답: %s"), *SecretNumber);

	for (const ANBBPlayerController* PC : AllPlayerControllers)
	{
		if (ANBBPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<ANBBPlayerState>() : nullptr)
		{
			PS->ResetGuessCount();
		}
	}

	BroadcastChat(FString::Printf(TEXT("인원이 부족해 게임을 멈춥니다. 다른 플레이어를 기다리는 중입니다. (%d/%d)"), AllPlayerControllers.Num(), MinPlayersToStart), NBBColors::System);
}

void ANBBGameModeBase::ProcessChatMessage(ANBBPlayerController* Sender, const FString& InMessage)
{
	if (!IsValid(Sender))
	{
		return;
	}

	ANBBPlayerState* PS = Sender->GetPlayerState<ANBBPlayerState>();
	if (!IsValid(PS))
	{
		return;
	}

	// 너무 긴 입력은 자름
	const FString Message = InMessage.TrimStartAndEnd().Left(100);
	if (Message.IsEmpty())
	{
		return;
	}

	const FString Name = PS->GetPlayerNameString();

	// /로 시작하면 일반 채팅
	if (Message.StartsWith(TEXT("/")))
	{
		const FString ChatBody = Message.RightChop(1).TrimStartAndEnd();
		if (!ChatBody.IsEmpty())
		{
			BroadcastChat(FString::Printf(TEXT("%s: %s"), *Name, *ChatBody));
		}
		return;
	}

	if (!bGameStarted)
	{
		Sender->ClientReceivePrivateMessage(TEXT("다른 플레이어를 기다리는 중입니다."));
		return;
	}

	if (bRoundOver)
	{
		Sender->ClientReceivePrivateMessage(TEXT("결과 발표 중입니다. 잠시 후 새 게임이 시작됩니다."));
		return;
	}

	if (!PS->HasGuessLeft())
	{
		Sender->ClientReceivePrivateMessage(FString::Printf(TEXT("기회를 모두 사용했습니다 %s. 다른 플레이어를 기다려 주세요."), *PS->GetGuessCountString()));
		return;
	}

	if (bUseTurnMode && Sender != CurrentTurnPlayer)
	{
		const ANBBPlayerState* TurnPS = CurrentTurnPlayer ? CurrentTurnPlayer->GetPlayerState<ANBBPlayerState>() : nullptr;
		Sender->ClientReceivePrivateMessage(FString::Printf(TEXT("지금은 %s 님 차례입니다."), TurnPS ? *TurnPS->GetPlayerNameString() : TEXT("다른 플레이어")));
		return;
	}

	// 잘못된 입력은 기회 차감 없이 다시 입력
	const FString Error = ValidateGuess(Message);
	if (!Error.IsEmpty())
	{
		UE_LOG(LogNumBaseball, Log, TEXT("[Server] %s 잘못된 입력 '%s': %s"), *Name, *Message, *Error);
		Sender->ClientReceivePrivateMessage(FString::Printf(TEXT("[%s] %s 다시 입력하세요."), *Message, *Error));
		return;
	}

	PS->IncreaseGuessCount();
	const FString Result = CheckAnswer(Message);

	UE_LOG(LogNumBaseball, Log, TEXT("[Server] %s %s 입력 %s -> %s"), *Name, *PS->GetGuessCountString(), *Message, *Result);
	BroadcastChat(FString::Printf(TEXT("%s %s : %s -> %s"), *Name, *PS->GetGuessCountString(), *Message, *Result));

	if (!JudgeGame(Sender, Result) && bUseTurnMode)
	{
		AdvanceTurn();
	}
}

FString ANBBGameModeBase::GenerateRandomNumbers() const
{
	TArray<int32> Digits = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

	// 섞은 뒤 앞에서 3개
	for (int32 i = Digits.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Digits.Swap(i, j);
	}

	FString Result;
	for (int32 i = 0; i < 3; ++i)
	{
		Result.AppendInt(Digits[i]);
	}
	return Result;
}

FString ANBBGameModeBase::ValidateGuess(const FString& Input) const
{
	for (const TCHAR Ch : Input)
	{
		if (Ch < TEXT('1') || Ch > TEXT('9'))
		{
			return TEXT("1~9 사이의 숫자만 입력해주세요.");
		}
	}

	if (Input.Len() != 3)
	{
		return TEXT("3자리 숫자를 입력해주세요.");
	}

	TArray<TCHAR> Used;
	for (const TCHAR Ch : Input)
	{
		if (Used.Contains(Ch))
		{
			return TEXT("중복되지 않은 숫자를 입력해주세요.");
		}
		Used.Add(Ch);
	}

	return FString();
}

FString ANBBGameModeBase::CheckAnswer(const FString& Input) const
{
	int32 Strike = 0;
	int32 Ball = 0;

	for (int32 i = 0; i < 3; ++i)
	{
		if (Input[i] == SecretNumber[i])
		{
			++Strike;
		}
		else
		{
			int32 FoundIndex = INDEX_NONE;
			if (SecretNumber.FindChar(Input[i], FoundIndex))
			{
				++Ball;
			}
		}
	}

	if (Strike == 0 && Ball == 0)
	{
		return TEXT("OUT");
	}
	return FString::Printf(TEXT("%dS%dB"), Strike, Ball);
}

bool ANBBGameModeBase::JudgeGame(ANBBPlayerController* Sender, const FString& Result)
{
	// 3S면 승리
	if (Result == TEXT("3S0B"))
	{
		const ANBBPlayerState* PS = Sender ? Sender->GetPlayerState<ANBBPlayerState>() : nullptr;
		EndRound(FString::Printf(TEXT("%s 승리!"), PS ? *PS->GetPlayerNameString() : TEXT("Unknown")));
		return true;
	}

	// 다 썼으면 무승부
	if (AreAllPlayersOutOfChances())
	{
		EndRound(TEXT("무승부! 아무도 정답을 맞히지 못했습니다."));
		return true;
	}

	return false;
}

bool ANBBGameModeBase::AreAllPlayersOutOfChances() const
{
	for (const ANBBPlayerController* PC : AllPlayerControllers)
	{
		const ANBBPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<ANBBPlayerState>() : nullptr;
		if (PS && PS->HasGuessLeft())
		{
			return false;
		}
	}
	return true;
}

void ANBBGameModeBase::EndRound(const FString& ResultMessage)
{
	bRoundOver = true;
	ClearTurn();

	const FString FullMessage = FString::Printf(TEXT("%s  (정답: %s)"), *ResultMessage, *SecretNumber);
	UE_LOG(LogNumBaseball, Log, TEXT("[Server] %s"), *FullMessage);

	BroadcastChat(FullMessage, NBBColors::Result);
	BroadcastNotification(FullMessage, ResetDelay);

	GetWorldTimerManager().SetTimer(ResetTimerHandle, this, &ANBBGameModeBase::ResetGame, ResetDelay, false);
}

void ANBBGameModeBase::ResetGame()
{
	SecretNumber = GenerateRandomNumbers();
	UE_LOG(LogNumBaseball, Warning, TEXT("[Server] 새 게임. 정답: %s"), *SecretNumber);

	for (const ANBBPlayerController* PC : AllPlayerControllers)
	{
		if (ANBBPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<ANBBPlayerState>() : nullptr)
		{
			PS->ResetGuessCount();
		}
	}

	bRoundOver = false;

	BroadcastChat(TEXT("----- 새 게임 시작 -----"), NBBColors::NewGame);
	BroadcastNotification(TEXT("새 게임 시작!"), NoticeDuration);

	if (bGameStarted && bUseTurnMode)
	{
		// 선공은 판마다 다음 사람
		RoundFirstPlayer = FindNextTurnPlayer(RoundFirstPlayer);
		StartTurn(RoundFirstPlayer);
	}
}

ANBBPlayerController* ANBBGameModeBase::FindNextTurnPlayer(const ANBBPlayerController* From) const
{
	const int32 Num = AllPlayerControllers.Num();
	if (Num == 0)
	{
		return nullptr;
	}

	// From 다음 사람부터 한 바퀴 돌면서 기회가 남은 사람 찾기
	const int32 StartIndex = From ? AllPlayerControllers.IndexOfByKey(From) : INDEX_NONE;
	for (int32 Step = 1; Step <= Num; ++Step)
	{
		const int32 Index = (StartIndex + Step + Num) % Num;
		ANBBPlayerController* Candidate = AllPlayerControllers[Index];
		const ANBBPlayerState* PS = IsValid(Candidate) ? Candidate->GetPlayerState<ANBBPlayerState>() : nullptr;
		if (PS && PS->HasGuessLeft())
		{
			return Candidate;
		}
	}
	return nullptr;
}

void ANBBGameModeBase::StartTurn(ANBBPlayerController* TurnPlayer)
{
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	CurrentTurnPlayer = TurnPlayer;

	ANBBGameStateBase* GS = GetGameState<ANBBGameStateBase>();
	const ANBBPlayerState* PS = IsValid(TurnPlayer) ? TurnPlayer->GetPlayerState<ANBBPlayerState>() : nullptr;
	if (!GS || !PS)
	{
		ClearTurn();
		return;
	}

	GS->SetTurnInfo(PS->GetPlayerNameString(), GS->GetServerWorldTimeSeconds() + TurnTimeLimit);
	GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &ANBBGameModeBase::OnTurnTimeout, TurnTimeLimit, false);

	UE_LOG(LogNumBaseball, Log, TEXT("[Server] %s 턴 시작"), *PS->GetPlayerNameString());
	BroadcastChat(FString::Printf(TEXT(">> %s 님 차례입니다. (%.0f초)"), *PS->GetPlayerNameString(), TurnTimeLimit), NBBColors::Turn);
}

void ANBBGameModeBase::AdvanceTurn()
{
	if (ANBBPlayerController* Next = FindNextTurnPlayer(CurrentTurnPlayer))
	{
		StartTurn(Next);
		return;
	}

	EndRound(TEXT("무승부! 아무도 정답을 맞히지 못했습니다."));
}

void ANBBGameModeBase::OnTurnTimeout()
{
	if (bRoundOver || !IsValid(CurrentTurnPlayer))
	{
		return;
	}

	if (ANBBPlayerState* PS = CurrentTurnPlayer->GetPlayerState<ANBBPlayerState>())
	{
		PS->IncreaseGuessCount();
		UE_LOG(LogNumBaseball, Log, TEXT("[Server] %s 시간 초과 %s"), *PS->GetPlayerNameString(), *PS->GetGuessCountString());
		BroadcastChat(FString::Printf(TEXT("%s %s : 시간 초과! 기회 1회 소진"), *PS->GetPlayerNameString(), *PS->GetGuessCountString()), NBBColors::Result);
	}

	if (!JudgeGame(CurrentTurnPlayer, FString()))
	{
		AdvanceTurn();
	}
}

void ANBBGameModeBase::ClearTurn()
{
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	CurrentTurnPlayer = nullptr;

	if (ANBBGameStateBase* GS = GetGameState<ANBBGameStateBase>())
	{
		GS->ClearTurnInfo();
	}
}

void ANBBGameModeBase::BroadcastChat(const FString& Message, const FLinearColor& Color)
{
	// 채팅은 각 PC에 Client RPC로
	for (ANBBPlayerController* PC : AllPlayerControllers)
	{
		if (IsValid(PC))
		{
			PC->ClientReceiveChat(Message, Color);
		}
	}
}

void ANBBGameModeBase::BroadcastNotification(const FString& Message, float Duration)
{
	if (ANBBGameStateBase* GS = GetGameState<ANBBGameStateBase>())
	{
		GS->MulticastShowNotification(Message, Duration);
	}
}
