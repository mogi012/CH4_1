#include "UI/NBBChatWidget.h"
#include "Player/NBBPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UNBBChatWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}

	if (InputBox)
	{
		InputBox->OnTextCommitted.AddDynamic(this, &UNBBChatWidget::HandleTextCommitted);
	}
}

void UNBBChatWidget::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.5f));
	Background->SetPadding(FMargin(10.f));

	UCanvasPanelSlot* BackgroundSlot = Root->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(0.f, 0.35f, 0.6f, 1.f));
	BackgroundSlot->SetOffsets(FMargin(20.f, 0.f, 0.f, 20.f));

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
	Background->SetContent(VBox);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetText(FText::FromString(TEXT("접속 중...")));
	VBox->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));

	TurnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TurnText"));
	TurnText->SetText(FText::FromString(TEXT("대기 중")));
	VBox->AddChildToVerticalBox(TurnText)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	ChatScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ChatScrollBox"));
	UVerticalBoxSlot* ScrollSlot = VBox->AddChildToVerticalBox(ChatScrollBox);
	ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ScrollSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	InputBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("InputBox"));
	InputBox->SetHintText(FText::FromString(TEXT("숫자 3개 입력 후 Enter (/로 시작하면 채팅)")));
	VBox->AddChildToVerticalBox(InputBox);
}

void UNBBChatWidget::HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter)
	{
		return;
	}

	const FString Message = Text.ToString().TrimStartAndEnd();
	if (!Message.IsEmpty())
	{
		if (ANBBPlayerController* PC = GetOwningPlayer<ANBBPlayerController>())
		{
			PC->SendChatMessage(Message);
		}
	}

	InputBox->SetText(FText::GetEmpty());

	// Enter 치면 포커스가 풀려서 다음 틱에 다시 잡음
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (InputBox)
			{
				InputBox->SetKeyboardFocus();
			}
		}));
	}
}

void UNBBChatWidget::AddChatLine(const FString& Message, const FLinearColor& Color)
{
	if (!ChatScrollBox)
	{
		return;
	}

	UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Line->SetText(FText::FromString(Message));
	Line->SetColorAndOpacity(FSlateColor(Color));
	Line->SetAutoWrapText(true);
	ChatScrollBox->AddChild(Line);

	while (ChatScrollBox->GetChildrenCount() > MaxChatLines)
	{
		ChatScrollBox->RemoveChildAt(0);
	}

	ChatScrollBox->ScrollToEnd();
}

void UNBBChatWidget::SetStatusText(const FString& InText)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(InText));
	}
}

void UNBBChatWidget::SetTurnText(const FString& InText, const FLinearColor& Color)
{
	if (TurnText)
	{
		TurnText->SetText(FText::FromString(InText));
		TurnText->SetColorAndOpacity(FSlateColor(Color));
	}
}
