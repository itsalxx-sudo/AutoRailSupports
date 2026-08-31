#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ARSSettingsData.h"
#include "Types/SlateEnums.h"
#include "ARSPillarSlotWidget.generated.h"

class UARSBlueprintPickerWidget;
class UARSCheckRowWidget;
class UARSSliderWidget;
class UARSTripleSliderWidget;
class UButton;
class UARSSideModePickerWidget;
class UTextBlock;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARSSlotEdited, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARSSlotBlueprintPicked, int32, SlotIndex, const FString&, BlueprintName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARSSlotMoveRequested, int32, SlotIndex, int32, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARSSlotRemoveRequested, int32, SlotIndex);

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSPillarSlotWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	void SetupCard(int32 InSlotIndex, float TinyStep, float SmallStep, float LargeStep,
		float RotationTiny, float RotationSmall, float RotationLarge);

	void SetBlueprintOptions(const TArray<FString>& Options);

	void PushSlot(const FARSPillarSlotData& InSlot);
	const FARSPillarSlotData& GetSlot() const { return mSlot; }

	int32 GetSlotIndex() const { return mSlotIndex; }

	void SetListPosition(int32 InSlotIndex, int32 TotalSlots, bool bCanMoveUp, bool bCanMoveDown);

	void SetDoubleRailsControlsVisible(bool bVisible);

	void SetDimensions(const FText& Dimensions);

	UPROPERTY(BlueprintAssignable, Category = "ARS Pillar Slot") FARSSlotEdited OnSlotEdited;
	UPROPERTY(BlueprintAssignable, Category = "ARS Pillar Slot") FARSSlotBlueprintPicked OnSlotBlueprintPicked;
	UPROPERTY(BlueprintAssignable, Category = "ARS Pillar Slot") FARSSlotMoveRequested OnSlotMoveRequested;
	UPROPERTY(BlueprintAssignable, Category = "ARS Pillar Slot") FARSSlotRemoveRequested OnSlotRemoveRequested;

protected:
	//HEADER
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> BlueprintNameText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PriorityText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> HeaderRow;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> ExpandedChevron;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> CollapsedChevron;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> MoveUpButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> MoveDownButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> RemoveButton;

	//BODY
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> BodyContainer;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSBlueprintPickerWidget> BlueprintPicker;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> PlaceEverySlider;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> StartOffsetSlider;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSTripleSliderWidget> RotationGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSTripleSliderWidget> OffsetGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSideModePickerWidget> SideModeRow;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSCheckRowWidget> InclineCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSCheckRowWidget> AutoRollCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSCheckRowWidget> SkipCheck;

	UPROPERTY(EditAnywhere, Category = "ARS Pillar Slot|Text")
	FText EmptySlotText = NSLOCTEXT("AutoRailSupports", "SlotEmpty", "No blueprint");

	UPROPERTY(EditAnywhere, Category = "ARS Pillar Slot|Text")
	FText PriorityFormat = NSLOCTEXT("AutoRailSupports", "SlotPriority", "Priority {0} of {1}");

private:
	UFUNCTION() void HandleBlueprintPicked(const FString& BlueprintName);
	UFUNCTION() void HandlePlaceEveryCommitted(float Value);
	UFUNCTION() void HandleStartOffsetCommitted(float Value);
	UFUNCTION() void HandleRotationCommitted(int32 AxisIndex, float Value);
	UFUNCTION() void HandleOffsetCommitted(int32 AxisIndex, float Value);
	UFUNCTION() void HandleSideModeChanged(EARSSideMode SideMode);
	UFUNCTION() void HandleInclineChanged(bool bIsChecked);
	UFUNCTION() void HandleAutoRollChanged(bool bIsChecked);
	UFUNCTION() void HandleSkipChanged(bool bIsChecked);
	UFUNCTION() void HandleMoveUpClicked();
	UFUNCTION() void HandleMoveDownClicked();
	UFUNCTION() void HandleRemoveClicked();

	void ToggleExpanded();
	bool IsOverHeader(const FVector2D& ScreenPosition) const;

	void ApplyExpansion();
	void ApplyHeaderSummary();
	void ApplyAutoRollDependency();

	FARSPillarSlotData mSlot;
	int32 mSlotIndex = 0;
	bool bSuppressEvents = false;
};
