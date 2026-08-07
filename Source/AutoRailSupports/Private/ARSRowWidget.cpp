#include "ARSRowWidget.h"

#include "ARSConfigWidget.h"
#include "Components/Border.h"

namespace
{
	const FLinearColor ARSRowHover = FLinearColor(FColor(0xE5, 0x93, 0x45));
	const FLinearColor ARSRowNormal = FLinearColor(1.0f, 1.0f, 1.0f, 0.0f);
}

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
}

void UARSRowWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	SetHighlighted(true);
	if (UARSConfigWidget* ConfigWidget = GetTypedOuter<UARSConfigWidget>())
	{
		ConfigWidget->SetHoverInfo(SettingName, SettingDescription);
	}
}

void UARSRowWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	SetHighlighted(false);
	if (UARSConfigWidget* ConfigWidget = GetTypedOuter<UARSConfigWidget>())
	{
		ConfigWidget->ClearHoverInfo();
	}
}

void UARSRowWidget::SetHighlighted(bool bHighlighted)
{
	if (RowBackground)
	{
		RowBackground->SetBrushColor(bHighlighted ? ARSRowHover : ARSRowNormal);
	}
}
