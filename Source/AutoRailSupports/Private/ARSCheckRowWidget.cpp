#include "ARSCheckRowWidget.h"

#include "ARSClientManager.h"
#include "Components/CheckBox.h"

void UARSCheckRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Check)
	{
		Check->OnCheckStateChanged.AddDynamic(this, &UARSCheckRowWidget::HandleCheckStateChanged);
	}
	SetDisabled(false);
}

void UARSCheckRowWidget::SetChecked(bool bChecked)
{
	if (Check)
	{
		Check->SetIsChecked(bChecked);
	}
}

bool UARSCheckRowWidget::IsChecked() const
{
	return Check && Check->IsChecked();
}

void UARSCheckRowWidget::SetDisabled(bool bDisabled)
{
	if (Check)
	{
		Check->SetIsEnabled(!bDisabled);
	}
	SetRenderOpacity(bDisabled ? 0.5f : 1.0f);
	SetToolTipText(bDisabled ? DisabledTooltip : FText::GetEmpty());
}

void UARSCheckRowWidget::HandleCheckStateChanged(bool bIsChecked)
{
	AARSClientManager::PlayMenuSound(this, bIsChecked ? EARSMenuSound::Ticked : EARSMenuSound::UnTicked);
	OnCheckChanged.Broadcast(bIsChecked);
}
