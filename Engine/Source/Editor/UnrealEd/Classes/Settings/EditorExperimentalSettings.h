// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EditorExperimentalSettings.h: Declares the UEditorExperimentalSettings class.
=============================================================================*/

#pragma once


#include "EditorExperimentalSettings.generated.h"


/**
 * Implements Editor settings for experimental features.
 */
UCLASS(config=EditorUserSettings)
class UNREALED_API UEditorExperimentalSettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** Allows usage of the Translation Editor */
	UPROPERTY(EditAnywhere, config, Category = Tools, meta = (DisplayName = "Translation Editor"))
	bool bEnableTranslationEditor;

	/** The Blutility shelf holds editor utility Blueprints. Summon from the Workspace menu. */
	UPROPERTY(EditAnywhere, config, Category=Tools, meta=(DisplayName="Editor Utility Blueprints (Blutility)"))
	bool bEnableEditorUtilityBlueprints;

	/** The Game Launcher provides advanced workflows for packaging, deploying and launching your games. */
	UPROPERTY(EditAnywhere, config, Category=Tools, meta=(DisplayName="Game Launcher"))
	bool bGameLauncher;

	/** The Messaging Debugger provides a visual utility for debugging the messaging system. */
	UPROPERTY(EditAnywhere, config, Category=Tools, meta=(DisplayName="Messaging Debugger"))
	bool bMessagingDebugger;

	/** Allows to use actor merging utilities (Simplygon Proxy LOD, Grouping by Materials)*/
	UPROPERTY(EditAnywhere, config, Category=Tools, meta=(DisplayName="Actor Merging"))
	bool bActorMerging;

	UPROPERTY(EditAnywhere, config, Category=UserInterface, meta=(DisplayName="Console for Gamepad Labels"))
	TEnumAsByte<EConsoleForGamepadLabels::Type> ConsoleForGamepadLabels;

	/** Allows for customization of toolbars and menus throughout the editor */
	UPROPERTY(config)
	bool bToolbarCustomization;

	/** Break on Exceptions allows you to trap Access Nones and other exceptional events in Blueprints. */
	UPROPERTY(EditAnywhere, config, Category=Blueprints, meta=(DisplayName="Blueprint Break on Exceptions"))
	bool bBreakOnExceptions;

	/** Should arrows indicating data/execution flow be drawn halfway along wires? */
	UPROPERTY(/*EditAnywhere - deprecated (moved into UBlueprintEditorSettings), */config/*, Category=Blueprints, meta=(DisplayName="Draw midpoint arrows in Blueprints")*/)
	bool bDrawMidpointArrowsInBlueprints;

	/** Determines if the Blueprint editor should use the new (experimental) menu system, or the old (legacy) system */
	UPROPERTY(EditAnywhere, config, Category=Blueprints, meta=(DisplayName="Use New Blueprint Menuing System"))
	bool bUseRefactoredBlueprintMenuingSystem;

	/** Whether to show Audio Streaming options for SoundWaves (disabling will not stop all audio streaming) */
	UPROPERTY(EditAnywhere, config, Category=Audio)
	bool bShowAudioStreamingOptions;

	/** Allows ChunkIDs to be assigned to assets to via the content browser context menu. */
	UPROPERTY(EditAnywhere,config,Category=UserInterface,meta=(DisplayName="Allow ChunkID Assignments"))
	bool bContextMenuChunkAssignments;

	/** Enables the dynamic feature level switching functionality */
	UPROPERTY(EditAnywhere, config, Category = Rendering, meta = (DisplayName = "Feature Level Preview"))
	bool bFeatureLevelPreview;

	/**
	 * Returns an event delegate that is executed when a setting has changed.
	 *
	 * @return The delegate.
	 */
	DECLARE_EVENT_OneParam(UEditorExperimentalSettings, FSettingChangedEvent, FName /*PropertyName*/);
	FSettingChangedEvent& OnSettingChanged( ) { return SettingChangedEvent; }

protected:

	// UObject overrides

	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) override;

private:

	// Holds an event delegate that is executed when a setting has changed.
	FSettingChangedEvent SettingChangedEvent;
};
