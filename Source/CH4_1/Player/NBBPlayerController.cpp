#include "Player/NBBPlayerController.h"
#include "Player/NBBPlayerState.h"
#include "Game/NBBGameModeBase.h"
#include "Game/NBBGameStateBase.h"
#include "UI/NBBChatWidget.h"
#include "UI/NBBNotificationWidget.h"
#include "Components/EditableTextBox.h"
#include "Engine/World.h"
#include "TimerManager.h"

ANBBPlayerController::ANBBPlayerController()
{
	ChatWidgetClass = UNBBChatWidget::StaticClass();
	NotificationWidgetClass = UNBBNotificationWidget::StaticClass();
}

void ANBBPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// UI는 로컬 PC에서만
	if (!IsLocalController())
	{
		return;
	}

	if (ChatWidgetClass)
	{
		ChatWidget = CreateWidget<UNBBChatWidget>(this, ChatWidgetClass);
		if (ChatWidget)
		{
			ChatWidget->AddToViewport(0);
		}
	}

	if (NotificationWidgetClass)
	{
		NotificationWidget = CreateWidget<UNBBNotificationWidget>(this, NotificationWidgetClass);
		if (NotificationWidget)
		{
			NotificationWidget->AddToViewport(10);
		}
	}

	FInputModeUIOnly InputMode;
	if (ChatWidget && ChatWidget->GetInputBox())
	{
		InputMode.SetWidgetToFocus(ChatWidget->GetInputBox()->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	ServerNotifyReady();

	// 내 정보, 턴 표시 갱신
	GetWorldTimerManager().SetTimer(StatusTimerHandle, this, &ANBBPlayerController::RefreshStatus, 0.2f, true, 0.f);
}

void ANBBPlayerController::SendChatMessage(const FString& Message)
{
	if (IsLocalController())
	{
		ServerSendChatMessage(Message);
	}
}

void ANBBPlayerController::ServerSendChatMessage_Implementation(const FString& Message)
{
	if (ANBBGameModeBase* GM = GetWorld()->GetAuthGameMode<ANBBGameModeBase>())
	{
		GM->ProcessChatMessage(this, Message);
	}
}

void ANBBPlayerController::ServerNotifyReady_Implementation()
{
	if (ANBBGameModeBase* GM = GetWorld()->GetAuthGameMode<ANBBGameModeBase>())
	{
		GM->OnPlayerReady(this);
	}
}

void ANBBPlayerController::ClientReceiveChat_Implementation(const FString& Message, const FLinearColor& Color)
{
	AddChatMessage(Message, Color);
}

void ANBBPlayerController::ClientReceivePrivateMessage_Implementation(const FString& Message)
{
	AddChatMessage(FString::Printf(TEXT("[안내] %s"), *Message), FLinearColor(1.f, 0.5f, 0.4f));
}

void ANBBPlayerController::AddChatMessage(const FString& Message, const FLinearColor& Color)
{
	if (ChatWidget)
	{
		ChatWidget->AddChatLine(Message, Color);
	}
}

void ANBBPlayerController::ShowNotification(const FString& Message, float Duration)
{
	if (NotificationWidget)
	{
		NotificationWidget->ShowNotification(Message, Duration);
	}
}

void ANBBPlayerController::RefreshStatus()
{
	const ANBBGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState<ANBBGameStateBase>() : nullptr;
	if (NotificationWidget && GS && LastNotificationRevision != GS->GetNotification().Revision)
	{
		LastNotificationRevision = GS->GetNotification().Revision;
		ShowNotification(GS->GetNotification().Message, GS->GetNotification().Duration);
	}

	if (!ChatWidget)
	{
		return;
	}

	const ANBBPlayerState* PS = GetPlayerState<ANBBPlayerState>();
	if (PS && !PS->GetPlayerNameString().IsEmpty())
	{
		ChatWidget->SetStatusText(FString::Printf(TEXT("나: %s"), *PS->GetPlayerInfoString()));
	}

	if (!GS)
	{
		return;
	}

	const FString& TurnName = GS->GetTurnPlayerName();
	if (TurnName.IsEmpty())
	{
		ChatWidget->SetTurnText(TEXT("대기 중"), FLinearColor(0.7f, 0.7f, 0.7f));
		return;
	}

	const int32 Remaining = FMath::CeilToInt(GS->GetTurnRemainingTime());
	if (PS && PS->GetPlayerNameString() == TurnName)
	{
		ChatWidget->SetTurnText(FString::Printf(TEXT("내 차례입니다! 남은 시간 %d초"), Remaining), FLinearColor(1.f, 0.85f, 0.2f));
	}
	else
	{
		ChatWidget->SetTurnText(FString::Printf(TEXT("현재 차례: %s (%d초)"), *TurnName, Remaining), FLinearColor::White);
	}
}
