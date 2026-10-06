#include "UI/NBBNotificationWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UNBBNotificationWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}

	// 클릭이 아래 채팅창으로 통과하도록
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (NotificationText)
	{
		NotificationText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNBBNotificationWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
	}

	Super::NativeDestruct();
}

void UNBBNotificationWidget::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	NotificationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NotificationText"));
	FSlateFontInfo Font = NotificationText->GetFont();
	Font.Size = FontSize;
	NotificationText->SetFont(Font);
	NotificationText->SetJustification(ETextJustify::Center);
	NotificationText->SetShadowOffset(FVector2D(2.f, 2.f));
	NotificationText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));

	UCanvasPanelSlot* TextSlot = Root->AddChildToCanvas(NotificationText);
	TextSlot->SetAnchors(FAnchors(0.5f, 0.2f));
	TextSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	TextSlot->SetAutoSize(true);
}

void UNBBNotificationWidget::ShowNotification(const FString& Message, float Duration)
{
	if (!NotificationText)
	{
		return;
	}

	NotificationText->SetText(FText::FromString(Message));
	NotificationText->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(HideTimerHandle, this, &UNBBNotificationWidget::HideNotification, Duration, false);
	}
}

void UNBBNotificationWidget::HideNotification()
{
	if (NotificationText)
	{
		NotificationText->SetVisibility(ESlateVisibility::Collapsed);
	}
}
