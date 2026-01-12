#include "BTDecorator_CheckStamina.h"
#include "AIController.h"
#include "MYY/AbilitySystem/MYYCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "MYY/AbilitySystem/AttributeSet/AttributeSetBase.h"

UBTDecorator_CheckStamina::UBTDecorator_CheckStamina()
{
	NodeName = "Check Stamina";
}

bool UBTDecorator_CheckStamina::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return false;
	}

	AMYYCharacterBase* AICharacter = Cast<AMYYCharacterBase>(AIController->GetPawn());
	if (!AICharacter || !AICharacter->AbilitySystemComponent)
	{
		return false;
	}

	UAttributeSetBase* AttributeSet = const_cast<UAttributeSetBase*>( Cast<UAttributeSetBase>( AICharacter->AbilitySystemComponent->GetAttributeSet( UAttributeSetBase::StaticClass() ) ) );

	if (!AttributeSet)
	{
		return false;
	}

	float CurrentStamina = AttributeSet->GetStamina();
	return CurrentStamina >= MinimumStamina;
}
