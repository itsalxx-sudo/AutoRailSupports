#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ARSInfoRowWidget.h"
#include "ARSInfoPanelWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSInfoPanelWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetInfo(const FText& PartLabel, const FText& BlueprintName, const FText& Dimensions);

	void SetSettingsRows(const TArray<FARSInfoPanelRow>& Rows);

	void SetBackgroundOpacity(float Opacity);

protected:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PartLabelText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> BlueprintNameText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DimensionsText;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder> InfoPanelBorder;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSInfoRowWidget> InfoRow1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSInfoRowWidget> InfoRow2;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSInfoRowWidget> InfoRow3;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSInfoRowWidget> InfoRow4;
};
