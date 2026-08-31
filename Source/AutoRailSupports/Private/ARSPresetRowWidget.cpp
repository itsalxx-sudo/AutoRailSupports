#include "ARSPresetRowWidget.h"

#include "ARSClientManager.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

UARSPresetRowWidget::UARSPresetRowWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibilityInternal(ESlateVisibility::Visible);
}

void UARSPresetRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (RenameButton) { RenameButton->OnClicked.AddDynamic(this, &UARSPresetRowWidget::HandleRenameClicked); }
	if (DeleteButton) { DeleteButton->OnClicked.AddDynamic(this, &UARSPresetRowWidget::HandleDeleteClicked); }
	if (RenameConfirmButton) { RenameConfirmButton->OnClicked.AddDynamic(this, &UARSPresetRowWidget::HandleRenameConfirmClicked); }
	if (RenameCancelButton) { RenameCancelButton->OnClicked.AddDynamic(this, &UARSPresetRowWidget::HandleRenameCancelClicked); }
	if (RenameBox) { RenameBox->OnTextCommitted.AddDynamic(this, &UARSPresetRowWidget::HandleRenameCommitted); }
	for (UButton* Button : { RenameButton.Get(), DeleteButton.Get(), RenameConfirmButton.Get(), RenameCancelButton.Get() })
	{
		AARSClientManager::BindButtonSounds(this, Button);
	}
	ApplyVisuals();
}

void UARSPresetRowWidget::SetPreset(const FString& InName, int32 ListIndex, bool bInSelected, bool bInDirty)
{
	mName = InName;
	mListIndex = ListIndex;
	bSelected = bInSelected;
	bDirty = bInDirty;
	if (NameText)
	{
		NameText->SetText(FText::FromString(mName));
	}
	if (RowBackground)
	{
		RowBackground->SetBrushColor(mListIndex % 2 == 0 ? RowColorEven : RowColorOdd);
	}
	ApplyVisuals();
}

void UARSPresetRowWidget::BeginRename()
{
	bRenaming = true;
	if (RenameBox)
	{
		RenameBox->SetText(FText::FromString(mName));
		RenameBox->SetKeyboardFocus();
	}
	ApplyVisuals();
	OnPresetRenameStarted.Broadcast(mName);
}

void UARSPresetRowWidget::EndRename()
{
	bRenaming = false;
	ApplyVisuals();
}

void UARSPresetRowWidget::ApplyVisuals()
{
	if (SelectedHighlight) { SelectedHighlight->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	if (HoverHighlight) { HoverHighlight->SetVisibility(bHovered && !bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	if (SelectedMarker) { SelectedMarker->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden); }
	if (DirtyIndicator) { DirtyIndicator->SetVisibility(bSelected && bDirty ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }

	if (NameText)
	{
		NameText->SetVisibility(bRenaming ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		NameText->SetColorAndOpacity(FSlateColor(bSelected ? NameColorSelected : NameColor));
	}
	if (RenameBox) { RenameBox->SetVisibility(bRenaming ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
	if (RenameConfirmButton) { RenameConfirmButton->SetVisibility(bRenaming ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
	if (RenameCancelButton) { RenameCancelButton->SetVisibility(bRenaming ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
	if (ActionRow)
	{
		ActionRow->SetVisibility(!bRenaming && (bHovered || bSelected)
			? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

FReply UARSPresetRowWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bRenaming && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		AARSClientManager::PlayMenuSound(this, EARSMenuSound::Select);
		OnPresetChosen.Broadcast(mName);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UARSPresetRowWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	AARSClientManager::PlayMenuSound(this, EARSMenuSound::Hover);
	bHovered = true;
	ApplyVisuals();
}

void UARSPresetRowWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bHovered = false;
	ApplyVisuals();
}

void UARSPresetRowWidget::HandleRenameClicked()
{
	BeginRename();
}

void UARSPresetRowWidget::HandleDeleteClicked()
{
	OnPresetDeleteRequested.Broadcast(mName);
}

void UARSPresetRowWidget::HandleRenameConfirmClicked()
{
	CommitRename();
}

void UARSPresetRowWidget::HandleRenameCancelClicked()
{
	EndRename();
}

void UARSPresetRowWidget::HandleRenameCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		CommitRename();
	}
	else if (CommitMethod == ETextCommit::OnCleared)
	{
		EndRename();
	}
}

void UARSPresetRowWidget::CommitRename()
{
	const FString NewName = RenameBox ? RenameBox->GetText().ToString().TrimStartAndEnd() : FString();
	EndRename();
	if (!NewName.IsEmpty() && NewName != mName)
	{
		OnPresetRenamed.Broadcast(mName, NewName);
	}
}
