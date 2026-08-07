#include "ARSInfoRowWidget.h"

#include "Components/TextBlock.h"

void UARSInfoRowWidget::SetRow(const FARSInfoPanelRow& Row)
{
	if (LabelText) { LabelText->SetText(Row.Label); }

	SetRenderOpacity(Row.bActive ? 1.0f : InactiveOpacity);

	if (SelectedMarker)
	{
		SelectedMarker->SetVisibility(Row.bActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	UWidget* Boxes[3] = { Cell0, Cell1, Cell2 };
	UTextBlock* Types[3] = { Cell0Type, Cell1Type, Cell2Type };
	UTextBlock* Values[3] = { Cell0Value, Cell1Value, Cell2Value };

	for (int32 i = 0; i < 3; i++)
	{
		const bool bUsed = Row.Cells.IsValidIndex(i);
		if (Boxes[i])
		{
			Boxes[i]->SetVisibility(bUsed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (!bUsed)
		{
			continue;
		}
		const FARSInfoPanelCell& Cell = Row.Cells[i];
		if (Types[i])
		{
			Types[i]->SetText(Cell.Type);
			Types[i]->SetVisibility(Cell.Type.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
		if (Values[i]) { Values[i]->SetText(Cell.Value); }
		if (Boxes[i]) { Boxes[i]->SetRenderOpacity(Cell.bActive ? 1.0f : InactiveOpacity); }
	}
}
