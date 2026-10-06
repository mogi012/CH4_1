#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "NBBChatWidget.generated.h"

class UScrollBox;
class UEditableTextBox;
class UTextBlock;

UCLASS()
class CH4_1_API UNBBChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void AddChatLine(const FString& Message, const FLinearColor& Color);
	void SetStatusText(const FString& InText);
	void SetTurnText(const FString& InText, const FLinearColor& Color);
	UEditableTextBox* GetInputBox() const { return InputBox; }

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	void HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	void BuildLayout();

	// WBP에 있으면 그거 쓰고 없으면 C++로 만듦
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ChatScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> InputBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TurnText;

	UPROPERTY(EditDefaultsOnly, Category = "Chat")
	int32 MaxChatLines = 50;
};
