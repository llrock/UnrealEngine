// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/Tasks/BTTask_RunBehavior.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeManager.h"
#if WITH_EDITOR
#include "Kismet2/KismetEditorUtilities.h"
#endif // WITH_EDITOR

DEFINE_STAT(STAT_AI_BehaviorTree_Tick);
DEFINE_STAT(STAT_AI_BehaviorTree_LoadTime);
DEFINE_STAT(STAT_AI_BehaviorTree_SearchTime);
DEFINE_STAT(STAT_AI_BehaviorTree_ExecutionTime);
DEFINE_STAT(STAT_AI_BehaviorTree_AuxUpdateTime);
DEFINE_STAT(STAT_AI_BehaviorTree_NumTemplates);
DEFINE_STAT(STAT_AI_BehaviorTree_NumInstances);
DEFINE_STAT(STAT_AI_BehaviorTree_InstanceMemory);

UBehaviorTreeManager::UBehaviorTreeManager(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	MaxDebuggerSteps = 100;
}

void UBehaviorTreeManager::FinishDestroy()
{
	SET_DWORD_STAT(STAT_AI_BehaviorTree_NumTemplates, 0);

	for (int32 Idx = 0; Idx < ActiveComponents.Num(); Idx++)
	{
		if (ActiveComponents[Idx] && !ActiveComponents[Idx]->HasAnyFlags(RF_BeginDestroyed))
		{
			ActiveComponents[Idx]->Cleanup();
		}
	}

	ActiveComponents.Reset();
	Super::FinishDestroy();
}

int32 UBehaviorTreeManager::GetAlignedDataSize(int32 Size)
{
	// round to 4 bytes
	return ((Size + 3) & ~3);
}

struct FNodeInitializationData
{
	UBTNode* Node;
	UBTCompositeNode* ParentNode;
	uint16 ExecutionIndex;
	uint16 DataSize;
	uint16 SpecialDataSize;
	uint8 TreeDepth;

	FNodeInitializationData() {}
	FNodeInitializationData(UBTNode* InNode, UBTCompositeNode* InParentNode,
		uint16 InExecutionIndex, uint8 InTreeDepth, uint16 NodeMemory, uint16 SpecialNodeMemory = 0)
		: Node(InNode), ParentNode(InParentNode), ExecutionIndex(InExecutionIndex), TreeDepth(InTreeDepth)
	{
		SpecialDataSize = UBehaviorTreeManager::GetAlignedDataSize(SpecialNodeMemory);

		const uint16 NodeMemorySize = NodeMemory + SpecialDataSize;
		DataSize = (NodeMemorySize <= 2) ? NodeMemorySize : UBehaviorTreeManager::GetAlignedDataSize(NodeMemorySize);
	}

	struct FMemorySort
	{
		FORCEINLINE bool operator()(const FNodeInitializationData& A, const FNodeInitializationData& B) const
		{
			return A.DataSize > B.DataSize;
		}
	};
};

static void InitializeNodeHelper(UBTCompositeNode* ParentNode, UBTNode* NodeOb,
	uint8 TreeDepth, uint16& ExecutionIndex, TArray<FNodeInitializationData>& InitList,
	class UBehaviorTree* TreeAsset, UObject* NodeOuter)
{
	// special case: subtrees
	UBTTask_RunBehavior* SubtreeTask = Cast<UBTTask_RunBehavior>(NodeOb);
	if (SubtreeTask)
	{
		ExecutionIndex += SubtreeTask->GetInjectedNodesCount();
	}

	InitList.Add(FNodeInitializationData(NodeOb, ParentNode, ExecutionIndex, TreeDepth, NodeOb->GetInstanceMemorySize(), NodeOb->GetSpecialMemorySize()));
	NodeOb->InitializeFromAsset(TreeAsset);
	ExecutionIndex++;

	UBTCompositeNode* CompositeOb = Cast<UBTCompositeNode>(NodeOb);
	if (CompositeOb)
	{
		for (int32 ServiceIndex = 0; ServiceIndex < CompositeOb->Services.Num(); ServiceIndex++)
		{
			if (CompositeOb->Services[ServiceIndex] == NULL)
			{
				UE_LOG(LogBehaviorTree, Warning, TEXT("%s has missing service node! (parent: %s)"),
					*GetNameSafe(TreeAsset), *UBehaviorTreeTypes::DescribeNodeHelper(CompositeOb));

				CompositeOb->Services.RemoveAt(ServiceIndex, 1, false);
				ServiceIndex--;
				continue;
			}

			UBTService* Service = Cast<UBTService>(StaticDuplicateObject(CompositeOb->Services[ServiceIndex], NodeOuter, TEXT("None")));;
			CompositeOb->Services[ServiceIndex] = Service;

			InitList.Add(FNodeInitializationData(Service, CompositeOb, ExecutionIndex, TreeDepth,
				Service->GetInstanceMemorySize(), Service->GetSpecialMemorySize()));

			Service->InitializeFromAsset(TreeAsset);
			ExecutionIndex++;
		}

		for (int32 ChildIndex = 0; ChildIndex < CompositeOb->Children.Num(); ChildIndex++)
		{
			FBTCompositeChild& ChildInfo = CompositeOb->Children[ChildIndex];
			for (int32 DecoratorIndex = 0; DecoratorIndex < ChildInfo.Decorators.Num(); DecoratorIndex++)
			{
				if (ChildInfo.Decorators[DecoratorIndex] == NULL)
				{
					UE_LOG(LogBehaviorTree, Warning, TEXT("%s has missing decorator node! (parent: %s, branch: %d)"),
						*GetNameSafe(TreeAsset), *UBehaviorTreeTypes::DescribeNodeHelper(CompositeOb), ChildIndex);

					ChildInfo.Decorators.RemoveAt(DecoratorIndex, 1, false);
					DecoratorIndex--;
					continue;
				}

				UBTDecorator* Decorator = Cast<UBTDecorator>(StaticDuplicateObject(ChildInfo.Decorators[DecoratorIndex], NodeOuter, TEXT("None")));
				ChildInfo.Decorators[DecoratorIndex] = Decorator;

				InitList.Add(FNodeInitializationData(Decorator, CompositeOb, ExecutionIndex, TreeDepth,
					Decorator->GetInstanceMemorySize(), Decorator->GetSpecialMemorySize()));

				Decorator->InitializeFromAsset(TreeAsset);
				Decorator->InitializeDecorator(ChildIndex);
				ExecutionIndex++;
			}

			UBTNode* ChildNode = NULL;
			
			if (ChildInfo.ChildComposite)
			{
				ChildInfo.ChildComposite = Cast<UBTCompositeNode>(StaticDuplicateObject(ChildInfo.ChildComposite, NodeOuter, TEXT("None")));
				ChildNode = ChildInfo.ChildComposite;
			}
			else if (ChildInfo.ChildTask)
			{
				ChildInfo.ChildTask = Cast<UBTTaskNode>(StaticDuplicateObject(ChildInfo.ChildTask, NodeOuter, TEXT("None")));
				ChildNode = ChildInfo.ChildTask;
			}

			if (ChildNode)
			{
				InitializeNodeHelper(CompositeOb, ChildNode, TreeDepth + 1, ExecutionIndex, InitList, TreeAsset, NodeOuter);
			}
		}

		CompositeOb->InitializeComposite(InitList.Num() - 1);
	}
}

bool UBehaviorTreeManager::LoadTree(class UBehaviorTree* Asset, UBTCompositeNode*& Root, uint16& InstanceMemorySize)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_BehaviorTree_LoadTime);

	for (int32 TemplateIndex = 0; TemplateIndex < LoadedTemplates.Num(); TemplateIndex++)
	{
		FBehaviorTreeTemplateInfo& TemplateInfo = LoadedTemplates[TemplateIndex];
		if (TemplateInfo.Asset == Asset)
		{
			Root = TemplateInfo.Template;
			InstanceMemorySize = TemplateInfo.InstanceMemorySize;
			return true;
		}
	}

	if (Asset->RootNode)
	{
		FBehaviorTreeTemplateInfo TemplateInfo;
		TemplateInfo.Asset = Asset;
		TemplateInfo.Template = Cast<UBTCompositeNode>(StaticDuplicateObject(Asset->RootNode, this, TEXT("None")));

		TArray<FNodeInitializationData> InitList;
		uint16 ExecutionIndex = 0;
		InitializeNodeHelper(NULL, TemplateInfo.Template, 0, ExecutionIndex, InitList, Asset, this);

#if USE_BEHAVIORTREE_DEBUGGER
		// fill in information about next nodes in execution index, before sorting memory offsets
		for (int32 Index = 0; Index < InitList.Num() - 1; Index++)
		{
			InitList[Index].Node->InitializeExecutionOrder(InitList[Index + 1].Node);
		}
#endif

		// sort nodes by memory size, so they can be packed better
		// it still won't protect against structures, that are internally misaligned (-> uint8, uint32)
		// but since all Engine level nodes are good... 
		InitList.Sort(FNodeInitializationData::FMemorySort());
		uint16 MemoryOffset = 0;
		for (int32 Index = 0; Index < InitList.Num(); Index++)
		{
			InitList[Index].Node->InitializeNode(InitList[Index].ParentNode, InitList[Index].ExecutionIndex, InitList[Index].SpecialDataSize + MemoryOffset, InitList[Index].TreeDepth);
			MemoryOffset += InitList[Index].DataSize;
		}
		
		TemplateInfo.InstanceMemorySize = MemoryOffset;

		INC_DWORD_STAT(STAT_AI_BehaviorTree_NumTemplates);
		LoadedTemplates.Add(TemplateInfo);
		Root = TemplateInfo.Template;
		InstanceMemorySize = TemplateInfo.InstanceMemorySize;
		return true;
	}

	return false;
}

void UBehaviorTreeManager::InitializeMemoryHelper(const TArray<UBTDecorator*>& Nodes, TArray<uint16>& MemoryOffsets, int32& MemorySize)
{
	TArray<FNodeInitializationData> InitList;
	for (int32 NodeIndex = 0; NodeIndex < Nodes.Num(); NodeIndex++)
	{
		InitList.Add(FNodeInitializationData(Nodes[NodeIndex], NULL, 0, 0, Nodes[NodeIndex]->GetInstanceMemorySize(), Nodes[NodeIndex]->GetSpecialMemorySize()));
	}

	InitList.Sort(FNodeInitializationData::FMemorySort());

	uint16 MemoryOffset = 0;
	MemoryOffsets.AddZeroed(Nodes.Num());

	for (int32 Index = 0; Index < InitList.Num(); Index++)
	{
		MemoryOffsets[Index] = InitList[Index].SpecialDataSize + MemoryOffset;
		MemoryOffset += InitList[Index].DataSize;
	}

	MemorySize = MemoryOffset;
}

//----------------------------------------------------------------------//
// stats dumping
//----------------------------------------------------------------------//
struct FNodeClassCounter
{
	TMap<UClass*, uint32> NodeClassUsage;

	FNodeClassCounter()
	{}

	void Declare(UClass* NodeClass)
	{
		NodeClassUsage.FindOrAdd(NodeClass);
	}

	void CountNode(UBTNode* Node)
	{
		uint32& Count = NodeClassUsage.FindOrAdd(Node->GetClass());
		++Count;
	}

	void Append(const FNodeClassCounter& Other)
	{
		for (auto Iterator : Other.NodeClassUsage)
		{
			uint32& Count = NodeClassUsage.FindOrAdd(Iterator.Key);
			Count += Iterator.Value;
		}
	}

	void Print(const TCHAR* Separator=TEXT(" "))
	{
		for (auto Iterator : NodeClassUsage)
		{
			UE_LOG(LogBehaviorTree, Display, TEXT("%s%s%s(%s)%s%d")
				, Separator
				, *Iterator.Key->GetName()
				, Separator
				, Iterator.Key->HasAnyClassFlags(CLASS_CompiledFromBlueprint) ? TEXT("BP") : TEXT("C++")
				, Separator
				, Iterator.Value);
		}
	}
};

void StatNodeUsage(UBTNode* Node, FNodeClassCounter& NodeCounter)
{
	NodeCounter.CountNode(Node);

	UBTCompositeNode* CompositeOb = Cast<UBTCompositeNode>(Node);
	if (CompositeOb)
	{
		for (int32 ServiceIndex = 0; ServiceIndex < CompositeOb->Services.Num(); ServiceIndex++)
		{
			if (CompositeOb->Services[ServiceIndex] == NULL)
			{
				continue;
			}
			NodeCounter.CountNode(CompositeOb->Services[ServiceIndex]);
		}

		for (int32 ChildIndex = 0; ChildIndex < CompositeOb->Children.Num(); ChildIndex++)
		{
			FBTCompositeChild& ChildInfo = CompositeOb->Children[ChildIndex];
			for (int32 DecoratorIndex = 0; DecoratorIndex < ChildInfo.Decorators.Num(); DecoratorIndex++)
			{
				if (ChildInfo.Decorators[DecoratorIndex] == NULL)
				{
					continue;
				}

				NodeCounter.CountNode(ChildInfo.Decorators[DecoratorIndex]);
			}

			UBTNode* ChildNode = NULL;

			if (ChildInfo.ChildComposite)
			{
				StatNodeUsage(ChildInfo.ChildComposite, NodeCounter);
			}
			else if (ChildInfo.ChildTask)
			{
				NodeCounter.CountNode(ChildInfo.ChildTask);
			}
		}
	}
}

void UBehaviorTreeManager::DumpUsageStats() const
{
	FNodeClassCounter AllNodesCounter;
	
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(UBTNode::StaticClass()) && It->HasAnyClassFlags(CLASS_Abstract) == false
#if WITH_EDITOR
			&& !(FKismetEditorUtilities::IsClassABlueprintSkeleton(*It)
				|| It->HasAnyClassFlags(CLASS_NewerVersionExists))
#endif
			)

		{
			AllNodesCounter.Declare(*It);
		}
	}

	UE_LOG(LogBehaviorTree, Display, TEXT("----------------------UBehaviorTreeManager::DumpUsageStats----------------------\nBehavior Trees:"));

	// get all BTNode classes
	
	for (TObjectIterator<UBehaviorTree> It; It; ++It)
	{
		FNodeClassCounter TreeNodeCounter;
		UE_LOG(LogBehaviorTree, Display, TEXT("--- %s ---"), *(It->GetName()));
		StatNodeUsage(It->RootNode, TreeNodeCounter);
		TreeNodeCounter.Print();
		AllNodesCounter.Append(TreeNodeCounter);
	}
	
	UE_LOG(LogBehaviorTree, Display, TEXT("--- Total Nodes class usage:"));
	AllNodesCounter.Print(TEXT(","));
}

void UBehaviorTreeManager::AddActiveComponent(UBehaviorTreeComponent* Component)
{
	ActiveComponents.AddUnique(Component);
}

void UBehaviorTreeManager::RemoveActiveComponent(UBehaviorTreeComponent* Component)
{
	ActiveComponents.Remove(Component);
}

UBehaviorTreeManager* UBehaviorTreeManager::GetCurrent(UWorld* World)
{
	UAISystem* AISys = UAISystem::GetCurrent(World, false);
	return AISys ? AISys->GetBehaviorTreeManager() : NULL;
}

UBehaviorTreeManager* UBehaviorTreeManager::GetCurrent(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, false);
	UAISystem* AISys = UAISystem::GetCurrent(World, false);
	return AISys ? AISys->GetBehaviorTreeManager() : NULL;
}
