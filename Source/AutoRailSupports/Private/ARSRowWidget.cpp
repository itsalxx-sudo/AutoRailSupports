#include "ARSRowWidget.h"

#include "ARSConfigWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"

void UARSRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (RowBackground)
	{
		FSlateBrush Brush = RowBackground->Background;
		Brush.TintColor = FSlateColor(FLinearColor::White);
		RowBackground->SetBrush(Brush);
	}
	SetHighlighted(false);
	ApplyLabel();
}

void UARSRowWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyLabel();
}

void UARSRowWidget::SetSettingText(const FText& Name, const FText& Description)
{
	SettingName = Name;
	SettingDescription = Description;
	ApplyLabel();
}

void UARSRowWidget::ApplyLabel()
{
	if (LabelText && !SettingName.IsEmpty())
	{
		LabelText->SetText(SettingName);
	}
}

void UARSRowWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	SetHighlighted(true);
	if (UARSConfigWidget* ConfigWidget = GetTypedOuter<UARSConfigWidget>())
	{
		ConfigWidget->SetHoverInfo(SettingName, SettingDescription, this);
	}
}

void UARSRowWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	SetHighlighted(false);
	if (UARSConfigWidget* ConfigWidget = GetTypedOuter<UARSConfigWidget>())
	{
		ConfigWidget->ClearHoverInfo(this);
	}
}

void UARSRowWidget::SetHighlighted(bool bHighlighted)
{
	if (RowBackground)
	{
		RowBackground->SetBrushColor(bHighlighted ? HoverColor : RestColor);
	}
}
