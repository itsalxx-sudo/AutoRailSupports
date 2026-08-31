#pragma once
#include "CoreMinimal.h"
#include "ARSRowWidget.h"
#include "ARSSegmentedSwitchWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARSSegmentSelected, int32, Index);

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSSegmentedSwitchWidget : public UARSRowWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;

	void SetSelectedIndex(int32 Index);
	int32 GetSelectedIndex() const { return mSelectedIndex; }

	UPROPERTY(BlueprintAssignable, Category = "ARS Segmented Switch")
	FARSSegmentSelected OnSegmentSelected;

protected:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Segment0Button;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Segment1Button;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> Segment0SelectedBackground;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> Segment1SelectedBackground;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> Segment0Check;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> Segment1Check;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Segment0Title;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Segment1Title;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Segment0Subtitle;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Segment1Subtitle;

	UPROPERTY(EditAnywhere, Category = "ARS Segmented Switch") FLinearColor SelectedTextColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, Category = "ARS Segmented Switch") FLinearColor UnselectedTextColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);

private:
	UFUNCTION() void OnSegment0Clicked();
	UFUNCTION() void OnSegment1Clicked();

	void ApplySelectionVisuals();

	int32 mSelectedIndex = 0;
};
