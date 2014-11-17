// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "ModuleInterface.h"

struct FTemplateCategory;

/**
 * Game Project Generation module
 */
class FGameProjectGenerationModule : public IModuleInterface
{

public:
	typedef TMap<FName, TSharedPtr<FTemplateCategory>> FTemplateCategoryMap;

	/**
	 * Called right after the plugin DLL has been loaded and the plugin object has been created
	 */
	virtual void StartupModule();

	/**
	 * Called before the plugin is unloaded, right before the plugin object is destroyed.
	 */
	virtual void ShutdownModule();

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FGameProjectGenerationModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FGameProjectGenerationModule >( "GameProjectGeneration" );
	}

	/** Creates the game project dialog */
	virtual TSharedRef<class SWidget> CreateGameProjectDialog(bool bAllowProjectOpening, bool bAllowProjectCreate);

	/** Creates a new class dialog for creating classes based on the passed-in class. */
	virtual TSharedRef<class SWidget> CreateNewClassDialog(class UClass* InClass);

	/** Opens a dialog to adds code files to the current project. */
	virtual void OpenAddCodeToProjectDialog();

	/** Delegate for when the AddCodeToProject dialog is opened */
	DECLARE_EVENT(FGameProjectGenerationModule, FAddCodeToProjectDialogOpenedEvent);
	FAddCodeToProjectDialogOpenedEvent& OnAddCodeToProjectDialogOpened() { return AddCodeToProjectDialogOpenedEvent; }

	/** Tries to make the project file writable. Prompts to check out as necessary. */
	virtual void TryMakeProjectFileWriteable(const FString& ProjectFile);

	/** Prompts the user to update his project file, if necessary. */
	virtual void CheckForOutOfDateGameProjectFile();

	/** Updates the currently loaded project. Returns true if the project was updated successfully or if no update was needed */
	virtual bool UpdateGameProject(const FString& ProjectFile, const FString& EngineIdentifier, FText& OutFailReason);

	/** Updates the current code project */
	virtual bool UpdateCodeProject(FText& OutFailReason);

	/** Gets the current projects source file count */
	virtual int32 GetProjectCodeFileCount();

	/** Gets file and size info about the source directory */
	virtual void GetProjectSourceDirectoryInfo(int32& OutNumFiles, int64& OutDirectorySize);

	/** Update code resource files */
	virtual bool UpdateCodeResourceFiles(TArray<FString>& OutCreatedFiles, FText& OutFailReason);

	/** Warn the user if the project filename is invalid in case they renamed it outside the editor */
	virtual void CheckAndWarnProjectFilenameValid();

	/**
	 * Update the list of supported target platforms based upon the parameters provided
	 * This will take care of checking out and saving the updated .uproject file automatically
	 * 
	 * @param	InPlatformName		Name of the platform to target (eg, WindowsNoEditor)
	 * @param	bIsSupported		true if the platform should be supported by this project, false if it should not
	 */
	virtual void UpdateSupportedTargetPlatforms(const FName& InPlatformName, const bool bIsSupported);

	/** Clear the list of supported target platforms */
	virtual void ClearSupportedTargetPlatforms();

public:

	/** (Un)register a new type of template category to be shown on the new project page */
	virtual bool RegisterTemplateCategory(FName Type, FText Name, FText Description, const FSlateBrush* Icon, const FSlateBrush* Image);
	virtual void UnRegisterTemplateCategory(FName Type);

	// Non DLL-exposed access to template categories
	TSharedPtr<const FTemplateCategory> GetCategory(FName Type) const { return TemplateCategories.FindRef(Type); }

private:
	FAddCodeToProjectDialogOpenedEvent AddCodeToProjectDialogOpenedEvent;

	/** Map of template categories from type to ptr */
	FTemplateCategoryMap TemplateCategories;
};
