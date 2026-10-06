#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NBBNotificationWidget.generated.h"

class UTextBlock;

// 승리/무승부/새 게임 공지
UCLASS()
class CH4_1_API UNBBNotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowNotification(const FString& Message, float Duration);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	void BuildLayout();
	void HideNotification();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NotificationText;

	UPROPERTY(EditDefaultsOnly, Category = "Notification")
	int32 FontSize = 36;

	FTimerHandle HideTimerHandle;
};
