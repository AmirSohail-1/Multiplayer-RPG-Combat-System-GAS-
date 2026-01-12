#include "ArrowProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "MYY/AbilitySystem/AttributeSet/AttributeSetBase.h"
#include "MYY/AbilitySystem/Actor/Havankund/Havankund.h"

AArrowProjectile::AArrowProjectile()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicatingMovement(true);
        
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    RootComponent = CollisionSphere;
    CollisionSphere->SetSphereRadius(10.f);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    CollisionSphere->SetNotifyRigidBodyCollision(true);

    ArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowMesh"));
    ArrowMesh->SetupAttachment(CollisionSphere);
    ArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 3000.f;
    ProjectileMovement->MaxSpeed = 3000.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->ProjectileGravityScale = 0.5f;
}

void AArrowProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        CollisionSphere->OnComponentHit.AddDynamic(this, &AArrowProjectile::OnProjectileHit);
        
        // Auto-destroy after 10 seconds
        SetLifeSpan(15.0f);
    }
}


void AArrowProjectile::OnProjectileHit(
    UPrimitiveComponent* HitComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    FVector NormalImpulse,
    const FHitResult& Hit)
{

    /* =========================================================
       GHOST ACTORS (NO GAS DAMAGE)
       ========================================================= */
    AGhost* Ghost = Cast<AGhost>(OtherActor);
    if (Ghost && Ghost->IsActive())
    {
        UE_LOG(LogTemp, Warning, TEXT("Arrow hit Ghost!"));

        // Optional VFX/SFX
        if (ImpactVFX)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(),
                ImpactVFX,
                Hit.ImpactPoint,
                Hit.ImpactNormal.Rotation()
            );
        }

        if (ImpactSFX)
        {
            UGameplayStatics::PlaySoundAtLocation(
                GetWorld(),
                ImpactSFX,
                Hit.ImpactPoint
            );
        }

        // Ghost handles its own deactivation in its collision event
        Destroy();
        return; // 🚨 CRITICAL: stops GAS damage call
    }
    
    if (!HasAuthority()) return;
    if (!OtherActor || OtherActor == GetOwner()) return;

    

    /* =========================================================
       HAVANKUND / ENVIRONMENT OBJECTS (NO GAS)
       ========================================================= */
    if (OtherActor->IsA(AHavankund::StaticClass()))
    {
        // Optional VFX/SFX
        if (ImpactVFX)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(),
                ImpactVFX,
                Hit.ImpactPoint,
                Hit.ImpactNormal.Rotation()
            );
        }

        if (ImpactSFX)
        {
            UGameplayStatics::PlaySoundAtLocation(
                GetWorld(),
                ImpactSFX,
                Hit.ImpactPoint
            );
        }

        Destroy();
        return; // 🚨 CRITICAL: stops GAS damage call
    }

    /* =========================================================
       GAS DAMAGE (CHARACTERS / AI ONLY)
       ========================================================= */
    ApplyDamageToTarget(OtherActor, Hit);

    if (ImpactVFX)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            ImpactVFX,
            Hit.ImpactPoint,
            Hit.ImpactNormal.Rotation()
        );
    }

    if (ImpactSFX)
    {
        UGameplayStatics::PlaySoundAtLocation(
            GetWorld(),
            ImpactSFX,
            Hit.ImpactPoint
        );
    }

    Destroy();
}
 
 
void AArrowProjectile::ApplyDamageToTarget(AActor* Target, const FHitResult& HitResult)
{
    if (!Target) return;

    UAbilitySystemComponent* TargetASC = 
        UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
     

    if (!TargetASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ArrowProjectile] ⚠️ Target has no ASC"));
        return;
    }

    // ✅ FIX: Get instigator properly
    AActor* InstigatorActor = GetInstigator();
    if (!InstigatorActor)
    {
        InstigatorActor = GetOwner();
        UE_LOG(LogTemp, Warning, TEXT("[ArrowProjectile] Using Owner as Instigator: %s"), 
            *GetNameSafe(InstigatorActor));
    }
    
    if (!InstigatorActor)
    {
        UE_LOG(LogTemp, Error, TEXT("[ArrowProjectile] ❌ No Instigator or Owner set!"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[ArrowProjectile] Instigator: %s"), *InstigatorActor->GetName());

    UAbilitySystemComponent* InstigatorASC = 
       UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor);
    if (!InstigatorASC) return;

    // Create damage effect context
    FGameplayEffectContextHandle EffectContext = InstigatorASC->MakeEffectContext();
    EffectContext.AddSourceObject(this);
    EffectContext.AddHitResult(HitResult);
    
    AController* InstigatorController = InstigatorActor->GetInstigatorController();
    EffectContext.AddInstigator(InstigatorActor, InstigatorController);
    if (!InstigatorController && Cast<APawn>(InstigatorActor))
    {
        InstigatorController = Cast<APawn>(InstigatorActor)->GetController();
    }
    
    EffectContext.AddInstigator(InstigatorActor, InstigatorController);
    
    if (DamageEffect)
    {
        FGameplayEffectSpecHandle SpecHandle = 
            InstigatorASC->MakeOutgoingSpec(DamageEffect, 1.f, EffectContext);

        if (SpecHandle.IsValid())
        {
            FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag("Data.Damage");
            SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, BaseDamage);

            // Add ranged attack tag
            SpecHandle.Data->CapturedSourceTags.GetSpecTags().AddTag(
                FGameplayTag::RequestGameplayTag("Ability.Attack.Ranged"));

            FActiveGameplayEffectHandle GEHandle = 
                InstigatorASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

            if (GEHandle.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("✅[ArrowProjectile]  Arrow applied %.1f damage to %s"),
                    BaseDamage, *Target->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌[ArrowProjectile]  Failed to apply arrow damage effect"));
            }
        }
    }
    else
    {
        // Fallback damage application
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [ArrowProjectile]  No DamageEffect set, using fallback"));
        TargetASC->ApplyModToAttribute(
            UAttributeSetBase::GetDamageAttribute(),
            EGameplayModOp::Additive,
            BaseDamage
        );
    }
}
 