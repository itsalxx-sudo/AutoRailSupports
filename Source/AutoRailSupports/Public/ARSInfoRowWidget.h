#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ARSInfoRowWidget.generated.h"

class UTextBlock;

struct FARSInfoPanelCell
{
	FText Type;
	FText Value;
	bool bActive = true;
};

struct FARSInfoPanelRow
{
	FText Label;
	TArray<FARSInfoPanelCell> Cells;
	bool bActive = false;
};

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSInfoRowWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetRow(const FARSInfoPanelRow& Row);

protected:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> Cell0;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Cell0Type;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Cell0Value;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> Cell1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Cell1Type;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Cell1Value;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> Cell2;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Cell2Type;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Cell2Value;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> SelectedMarker;

private:
	static constexpr float InactiveOpacity = 0.5f;
};
