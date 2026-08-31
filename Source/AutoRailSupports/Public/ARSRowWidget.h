#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ARSRowWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSRowWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS Row", meta = (ExposeOnSpawn))
	FText SettingName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS Row", meta = (MultiLine = true, ExposeOnSpawn))
	FText SettingDescription;

	void SetSettingText(const FText& Name, const FText& Description);

protected:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder> RowBackground;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(EditAnywhere, Category = "ARS Row")
	FLinearColor HoverColor = FLinearColor(FColor(0xE5, 0x93, 0x45));

	UPROPERTY(EditAnywhere, Category = "ARS Row")
	FLinearColor RestColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.0f);

	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	void ApplyLabel();

private:
	void SetHighlighted(bool bHighlighted);
};
