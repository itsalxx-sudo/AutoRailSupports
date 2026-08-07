#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ARSRowWidget.generated.h"

class UBorder;

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSRowWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "ARS Row")
	FText SettingName;

	UPROPERTY(EditAnywhere, Category = "ARS Row", meta = (MultiLine = true))
	FText SettingDescription;

protected:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder> RowBackground;

	virtual void NativeOnInitialized() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	void SetHighlighted(bool bHighlighted);
};
