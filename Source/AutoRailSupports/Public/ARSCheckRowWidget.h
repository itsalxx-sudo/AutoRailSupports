#pragma once
#include "CoreMinimal.h"
#include "ARSRowWidget.h"
#include "ARSCheckRowWidget.generated.h"

class UCheckBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARSCheckRowChanged, bool, bIsChecked);

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSCheckRowWidget : public UARSRowWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;

	void SetChecked(bool bChecked);
	bool IsChecked() const;

	void SetDisabled(bool bDisabled);

	UPROPERTY(BlueprintAssignable, Category = "ARS Check Row")
	FARSCheckRowChanged OnCheckChanged;

protected:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Check;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS Check Row", meta = (MultiLine = true, ExposeOnSpawn))
	FText DisabledTooltip;

private:
	UFUNCTION() void HandleCheckStateChanged(bool bIsChecked);
};
