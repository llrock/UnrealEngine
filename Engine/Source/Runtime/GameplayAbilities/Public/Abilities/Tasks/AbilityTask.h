// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameplayAbility.h"
#include "AbilityTask.generated.h"


/**
 *	AbilityTasks are small, self contained operations that can be performed while executing an ability.
 *	They are latent/asynchronous is nature. They will generally follow the pattern of 'start something and wait until it is finished or interrupted'
 *	
 *	We have code in K2Node_LatentAbilityCall to make using these in blueprints streamlined. The best way to become familiar with AbilityTasks is to 
 *	look at existing tasks like UAbilityTask_WaitOverlap (very simple) and UAbilityTask_WaitTargetData (much more complex).
 *	
 *	These are the basic requirements for using an ability task:
 *	
 *	1) Define dynamic multicast, BlueprintAssignable delegates in your AbilityTask. These are the OUTPUTs of your task. When these delegates fire,
 *	execution resumes in the calling blueprints.
 *	
 *	2) Your inputs are defined by a static factory function which will instantiate an instance of your task. The parameters of this function define
 *	the INPUTs into your task. All the factory function should do is instantiate your task and possibly set starting parameters. It should NOT invoke
 *	any of the callback delegates!
 *	
 *	3) Implement a Activate() function (defined here in base class). This function should actually start/execute your task logic. It is safe to invoke
 *	callback delegates here.
 *	
 *	
 *	This is all you need for basic AbilityTasks. 
 *	
 *	
 *	CheckList:
 *		-Override ::OnDestroy() and unregister any callbacks that the task registered. Call Super::EndTask too!
 *		-Implemented an Activate function which truly 'starts' the task. Do not 'start' the task in your static factory function!
 *	
 *	
 *	--------------------------------------
 *	
 *	We have additional support for AbilityTasks that want to spawn actors. Though this could be accomplished in an Activate() function, it would not be
 *	possible to pass in dynamic "ExposeOnSpawn" actor properties. This is a powerful feature of blueprints, in order to support this, you need to implement 
 *	a different step 3:
 *	
 *	Instead of an Activate() function, you should implement a BeginSpawningActor() and FinishSpawningActor() function.
 *	
 *	BeginSpawningActor() must take in a TSubclassOf<YourActorClassToSpawn> parameters named 'Class'. It must also have a out reference parameters of type 
 *	YourActorClassToSpawn*& named SpawnedActor. This function is allowed to decide whether it wants to spawn the actor or not (useful if wishing to
 *	predicate actor spawning on network authority).
 *	
 *	BeginSpawningActor() can instantiate an actor with SpawnActorDefferred. This is important, otherwise the UCS will run before spawn parameters are set.
 *	BeginSpawningActor() should also set the SpawnedActor parameter to the actor it spawned.
 *	
 *	[Next, the generated byte code will set the expose on spawn parameters to whatever the user has set]
 *	
 *	If you spawned something, FinishSpawningActor() will be called and pass in the same actor that was just spawned. You MUST call ExecuteConstruction + PostActorConstruction
 *	on this actor!
 *	
 *	This is a lot of steps but in general, AbilityTask_SpawnActor() gives a clear, minimal example.
 *	
 *	
 */
UCLASS()
class GAMEPLAYABILITIES_API UAbilityTask : public UObject
{
	GENERATED_UCLASS_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGenericAbilityTaskDelegate);

	// Called to trigger the actual task once the delegates have been set up
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "Abilities")
	virtual void Activate();

	/** Initailizes the task with the owning GameplayAbility but does not actviate until Activate() is called */
	virtual void InitTask(UGameplayAbility* InAbility);

	/** Tick function for this task, if bTickingTask == true */
	virtual void TickTask(float DeltaTime) {}

	/** Called when the ability is asked to confirm from an outside node. What this means depends on the individual task. By default, this does nothing other than ending if bEndTask is true. */
	virtual void ExternalConfirm(bool bEndTask);

	/** Called when the ability is asked to cancel from an outside node. What this means depends on the individual task. By default, this does nothing other than ending the task. */
	virtual void ExternalCancel();

	/** Return debug string describing task */
	virtual FString GetDebugString() const;
	
	/** GameplayAbility that created us */
	TWeakObjectPtr<UGameplayAbility> Ability;

	TWeakObjectPtr<UAbilitySystemComponent>	AbilitySystemComponent;

	/** Helper function for getting UWorld off a task */
	UWorld* GetWorld() const;

	/** Proper way to get the owning actor of the ability that owns this task (usually a pawn, tower, etc) */
	AActor* GetActor() const;

	/** Helper function for instantiating and initializing a new task */
	template <class T>
	static T*	NewTask(UObject* WorldContextObject, FName InstanceName = FName())
	{
		check(WorldContextObject);

		T* MyObj = NewObject<T>();
		UGameplayAbility* ThisAbility = CastChecked<UGameplayAbility>(WorldContextObject);
		MyObj->InitTask(ThisAbility);
		MyObj->InstanceName = InstanceName;
		return MyObj;
	}

	/** Called when owning ability is ended (before the task ends) kills the task. Calls OnDestroy. */
	void AbilityEnded();

	/** Called explicitly to end the task (usually by the task itself). Calls OnDestroy. */
	void EndTask();

	/** This name allows us to find the task later so that we can end it. */
	UPROPERTY()
	FName InstanceName;

protected:	

	/** End and CleanUp the task - may be called by the task itself or by the owning ability if the ability is ending. Do NOT call directly! Call EndTask() or AbilityEnded() */
	virtual void OnDestroy(bool AbilityIsEnding);

	/** If true, this task will receive TickTask calls from AbilitySystemComponent */
	bool bTickingTask;
};

//For searching through lists of ability instances
struct FAbilityInstanceNamePredicate
{
	FAbilityInstanceNamePredicate(FName DesiredInstanceName)
	{
		InstanceName = DesiredInstanceName;
	}

	bool operator()(const TWeakObjectPtr<UAbilityTask> A) const
	{
		return (A.IsValid() && !A.Get()->InstanceName.IsNone() && A.Get()->InstanceName.IsValid() && (A.Get()->InstanceName == InstanceName));
	}

	FName InstanceName;
};
