// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "BlueprintGraphDefinitions.h"
#include "GraphEditorSettings.h"

UNiagaraNodeOutputUpdate::UNiagaraNodeOutputUpdate(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}


void UNiagaraNodeOutputUpdate::AllocateDefaultPins()
{
	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();

	UNiagaraScriptSource* Source = GetSource();

	TArray<FName> OutputNames;
	Source->GetParticleAttributes(OutputNames);

	UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Vector"));
	for (int32 i = 0; i < OutputNames.Num(); i++)
	{
		UEdGraphPin* Pin = CreatePin(EGPD_Input, Schema->PC_Struct, TEXT(""), VectorStruct, false, false, OutputNames[i].ToString());
		Pin->bDefaultValueIsIgnored = true;
	}
}


FText UNiagaraNodeOutputUpdate::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("NiagaraNode", "Output", "Output");
}

FLinearColor UNiagaraNodeOutputUpdate::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->FunctionTerminatorNodeTitleColor;
}
