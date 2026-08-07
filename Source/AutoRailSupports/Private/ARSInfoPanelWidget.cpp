#include "ARSInfoPanelWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"

void UARSInfoPanelWidget::SetInfo(const FText& PartLabel, const FText& BlueprintName, const FText& Dimensions)
{
	if (PartLabelText) { PartLabelText->SetText(PartLabel); }
	if (BlueprintNameText) { BlueprintNameText->SetText(BlueprintName); }
	if (DimensionsText) { DimensionsText->SetText(Dimensions); }
}

void UARSInfoPanelWidget::SetBackgroundOpacity(float Opacity)
{
	if (InfoPanelBorder) { InfoPanelBorder->SetRenderOpacity(Opacity); }
}

void UARSInfoPanelWidget::SetSettingsRows(const TArray<FARSInfoPanelRow>& Rows)
{
	UARSInfoRowWidget* RowWidgets[] = { InfoRow1, InfoRow2, InfoRow3, InfoRow4 };

	for (int32 i = 0; i < UE_ARRAY_COUNT(RowWidgets); i++)
	{
		if (!RowWidgets[i])
		{
			continue;
		}
		const bool bUsed = Rows.IsValidIndex(i);
		RowWidgets[i]->SetVisibility(bUsed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bUsed)
		{
			RowWidgets[i]->SetRow(Rows[i]);
		}
	}
}
