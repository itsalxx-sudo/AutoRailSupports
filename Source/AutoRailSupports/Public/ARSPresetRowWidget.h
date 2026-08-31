#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ARSPresetRowWidget.generated.h"

class UBorder;
class UButton;
class UEditableTextBox;
class UTextBlock;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARSPresetChosen, const FString&, PresetName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARSPresetRenamed, const FString&, OldName, const FString&, NewName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARSPresetDeleteRequested, const FString&, PresetName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARSPresetRenameStarted, const FString&, PresetName);

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSPresetRowWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UARSPresetRowWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	void SetPreset(const FString& InName, int32 ListIndex, bool bInSelected, bool bInDirty);
	const FString& GetPresetName() const { return mName; }

	void BeginRename();
	void EndRename();
	bool IsRenaming() const { return bRenaming; }

	UPROPERTY(BlueprintAssignable, Category = "ARS Preset Row") FARSPresetChosen OnPresetChosen;
	UPROPERTY(BlueprintAssignable, Category = "ARS Preset Row") FARSPresetRenamed OnPresetRenamed;
	UPROPERTY(BlueprintAssignable, Category = "ARS Preset Row") FARSPresetDeleteRequested OnPresetDeleteRequested;
	UPROPERTY(BlueprintAssignable, Category = "ARS Preset Row") FARSPresetRenameStarted OnPresetRenameStarted;

protected:
	UPROPERTY(EditAnywhere, Category = "ARS Preset Row|Colors")
	FLinearColor RowColorEven = FLinearColor(FColor(0x22, 0x22, 0x22));

	UPROPERTY(EditAnywhere, Category = "ARS Preset Row|Colors")
	FLinearColor RowColorOdd = FLinearColor(FColor(0x2A, 0x2A, 0x2A));

	UPROPERTY(EditAnywhere, Category = "ARS Preset Row|Colors")
	FLinearColor NameColor = FLinearColor(FColor(0xC8, 0xC8, 0xC8));

	UPROPERTY(EditAnywhere, Category = "ARS Preset Row|Colors")
	FLinearColor NameColorSelected = FLinearColor::White;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder> RowBackground;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> HoverHighlight;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> SelectedHighlight;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> SelectedMarker;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> DirtyIndicator;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> ActionRow;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> RenameButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> DeleteButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UEditableTextBox> RenameBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> RenameConfirmButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> RenameCancelButton;

private:
	UFUNCTION() void HandleRenameClicked();
	UFUNCTION() void HandleDeleteClicked();
	UFUNCTION() void HandleRenameConfirmClicked();
	UFUNCTION() void HandleRenameCancelClicked();
	UFUNCTION() void HandleRenameCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	void CommitRename();
	void ApplyVisuals();

	FString mName;
	int32 mListIndex = 0;
	bool bSelected = false;
	bool bDirty = false;
	bool bRenaming = false;
	bool bHovered = false;
};
