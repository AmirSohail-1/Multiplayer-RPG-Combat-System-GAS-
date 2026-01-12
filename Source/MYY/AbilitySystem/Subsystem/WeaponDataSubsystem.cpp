#include "WeaponDataSubsystem.h"
#include "Engine/AssetManager.h"
#include "AssetRegistry/AssetRegistryModule.h"

void UWeaponDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	WeaponCache.Empty();

	// FIX: Check if AssetManager is ready
	if (!UAssetManager::IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("AssetManager not ready, delaying weapon cache load"));
		return;
	}

	UAssetManager& AssetManager = UAssetManager::Get();

	// FIXED PATH FOR ALL WEAPONS → best mobile optimization
	const FName WeaponPath = TEXT("/Game/GAS/DataTable");

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(UWeaponDataAsset::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(WeaponPath);

	TArray<FAssetData> FoundAssets;

	// FIX: Use AssetRegistryModule directly instead of AssetManager
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& Registry = AssetRegistryModule.Get();
	
	// Check if registry is loading
	if (Registry.IsLoadingAssets())
	{
		UE_LOG(LogTemp, Warning, TEXT("AssetRegistry still loading, delaying..."));
		return;
	}

	Registry.GetAssets(Filter, FoundAssets);

	for (const FAssetData& Asset : FoundAssets)
	{
		UWeaponDataAsset* Data = Cast<UWeaponDataAsset>(Asset.GetAsset());
		if (Data && Data->WeaponID_ReplicateWeapon_DA > 0)
		{
			WeaponCache.Add(Data->WeaponID_ReplicateWeapon_DA, Data);
			UE_LOG(LogTemp, Log, TEXT("Cached weapon: %s (ID: %d)"), *Data->GetName(), Data->WeaponID_ReplicateWeapon_DA);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[WeaponDataSubsystem] Loaded %d weapons."), WeaponCache.Num());
}

UWeaponDataAsset* UWeaponDataSubsystem::GetByID(int32 ID) const
{
	// FIX: Safe access to map
	const UWeaponDataAsset* const* FoundData = WeaponCache.Find(ID);
	if (FoundData && *FoundData)
	{
		return const_cast<UWeaponDataAsset*>(*FoundData);
	}
	
	UE_LOG(LogTemp, Error, TEXT("WeaponDataSubsystem: No weapon found for ID: %d"), ID);
	return nullptr;
}