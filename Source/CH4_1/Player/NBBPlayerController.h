#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NBBPlayerController.generated.h"

class UNBBChatWidget;
class UNBBNotificationWidget;

UCLASS()
class CH4_1_API ANBBPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANBBPlayerController();

	virtual void BeginPlay() override;

	// 위젯 -> 서버
	void SendChatMessage(const FString& Message);

	// 화면 출력
	void AddChatMessage(const FString& Message, const FLinearColor& Color);
	void ShowNotification(const FString& Message, float Duration);

	UFUNCTION(Server, Reliable)
	void ServerSendChatMessage(const FString& Message);

	// 로컬 UI가 준비됐다고 서버에 알림
	UFUNCTION(Server, Reliable)
	void ServerNotifyReady();

	UFUNCTION(Client, Reliable)
	void ClientReceiveChat(const FString& Message, const FLinearColor& Color);

	// 본인에게만 보내는 안내
	UFUNCTION(Client, Reliable)
	void ClientReceivePrivateMessage(const FString& Message);

protected:
	void RefreshStatus();

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNBBChatWidget> ChatWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNBBNotificationWidget> NotificationWidgetClass;

	UPROPERTY()
	TObjectPtr<UNBBChatWidget> ChatWidget;

	UPROPERTY()
	TObjectPtr<UNBBNotificationWidget> NotificationWidget;

	FTimerHandle StatusTimerHandle;
};
