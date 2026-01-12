// ArrowCountWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArrowCountWidget.generated.h"

class UTextBlock;
class AMYYCharacterBase;
class ABaseWeapon;

UCLASS()
class MYY_API UArrowCountWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ArrowCountText;

	UPROPERTY()
	AMYYCharacterBase* OwningCharacter;

	UFUNCTION()
	void OnWeaponChanged(ABaseWeapon* NewWeapon, ABaseWeapon* OldWeapon);

	UFUNCTION()
	void UpdateArrowCount();

	void BindToWeapon(ABaseWeapon* Weapon);
	void UnbindFromWeapon(ABaseWeapon* Weapon);

	FTimerHandle UpdateTimerHandle;
};