// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	WindowsPlatformStackWalk.h: Windows platform stack walk functions
==============================================================================================*/

#pragma once

#undef PLATFORM_SUPPORTS_STACK_SYMBOLS
#define PLATFORM_SUPPORTS_STACK_SYMBOLS 1


/**
 * Windows implementation of the stack walking.
 **/
struct CORE_API FWindowsPlatformStackWalk : public FGenericPlatformStackWalk
{
	static bool InitStackWalking();
	
	static void ProgramCounterToSymbolInfo( uint64 ProgramCounter, FProgramCounterSymbolInfo& out_SymbolInfo );
	static void CaptureStackBackTrace( uint64* BackTrace, uint32 MaxDepth, void* Context = NULL );
	static void StackWalkAndDump( ANSICHAR* HumanReadableString, SIZE_T HumanReadableStringSize, int32 IgnoreCount, void* Context = NULL );

	static int32 GetProcessModuleCount();
	static int32 GetProcessModuleSignatures(FStackWalkModuleInfo *ModuleSignatures, const int32 ModuleSignaturesSize);

	static void RegisterOnModulesChanged();
};

typedef FWindowsPlatformStackWalk FPlatformStackWalk;

