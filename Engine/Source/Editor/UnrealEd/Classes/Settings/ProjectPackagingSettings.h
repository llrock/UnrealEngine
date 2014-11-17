// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ProjectPackagingSettings.generated.h"


/**
 * Enumerates the available build configurations for project packaging.
 */
UENUM()
enum EProjectPackagingBuildConfigurations
{
	/** Debug configuration. */
	PPBC_DebugGame UMETA(DisplayName="DebugGame"),

	/** Development configuration. */
	PPBC_Development UMETA(DisplayName="Development"),

	/** Shipping configuration. */
	PPBC_Shipping UMETA(DisplayName="Shipping")
};


/**
 * Implements the Editor's user settings.
 */
UCLASS(config=Game, defaultconfig)
class UNREALED_API UProjectPackagingSettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** The build configuration for which the project is packaged. */
	UPROPERTY(config, EditAnywhere, Category=Project)
	TEnumAsByte<EProjectPackagingBuildConfigurations> BuildConfiguration;

	/** The directory to which the packaged project will be copied. */
	UPROPERTY(config, EditAnywhere, Category=Project)
	FDirectoryPath StagingDirectory;

	/**
	 * If enabled, a full rebuild will be enforced each time the project is being packaged.
	 * If disabled, only modified files will be built, which can improve iteration time.
	 * Unless you iterate on packaging, we recommend full rebuilds when packaging.
	 */
	UPROPERTY(config, EditAnywhere, Category=Project, AdvancedDisplay)
	bool FullRebuild;

	/**
	 * If enabled, a distribution build will be created and the shipping configuration will be used
	 * If disabled, an development build will be created
	 * Distribution builds are for publishing to the App Store
	 */
	UPROPERTY(config, EditAnywhere, Category=Project, AdvancedDisplay)
	bool ForDistribution;

	/** If enabled, all content will be put into a single .pak file instead of many individual files (default = enabled). */
	UPROPERTY(config, EditAnywhere, Category=Packaging)
	bool UsePakFile;

	/** If enabled, on Android platforms, .pak files are placed inside the APK. */
	UPROPERTY(config, EditAnywhere, Category = Packaging)
	bool UseOBB_InAPK;

	/**
	 * ISO codes of cultures whose data should be cooked, staged, and packaged.
	 * Note: These paths are relative to your project Content directory
	 */
	UPROPERTY(config, EditAnywhere, Category=Packaging, AdvancedDisplay, meta=(DisplayName="Cultures to Package"))
	TArray<FString> CulturesToStage;

	/**
	 * Directories containing .uasset files that should always be cooked regardless of whether they're referenced by anything in your project
	 * Note: These paths are relative to your project Content directory
	 */
	UPROPERTY(config, EditAnywhere, Category=Packaging, AdvancedDisplay, meta=(DisplayName="Additional Asset Directories to Cook", RelativeToGameContentDir))
	TArray<FDirectoryPath> DirectoriesToAlwaysCook;

	/**
	 * Directories containing files that should always be added to the .pak file (if using a .pak file; otherwise they're copied as individual files)
	 * This is used to stage additional files that you manually load via the UFS (Unreal File System) file IO API
	 * Note: These paths are relative to your project Content directory
	 */
	UPROPERTY(config, EditAnywhere, Category=Packaging, AdvancedDisplay, meta=(DisplayName="Additional Non-Asset Directories to Package", RelativeToGameContentDir))
	TArray<FDirectoryPath> DirectoriesToAlwaysStageAsUFS;

	/**
	 * Directories containing files that should always be copied when packaging your project, but are not supposed to be part of the .pak file
	 * This is used to stage additional files that you manually load without using the UFS (Unreal File System) file IO API, eg, third-party libraries that perform their own internal file IO
	 * Note: These paths are relative to your project Content directory
	 */
	UPROPERTY(config, EditAnywhere, Category=Packaging, AdvancedDisplay, meta=(DisplayName="Additional Non-Asset Directories To Copy", RelativeToGameContentDir))
	TArray<FDirectoryPath> DirectoriesToAlwaysStageAsNonUFS;

public:

	// UObject Interface

	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) override;
	virtual bool CanEditChange( const UProperty* InProperty ) const;
};
