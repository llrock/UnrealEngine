// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../GenericErrorReport.h"

/**
 * Helper that works with Mac Error Reports
 */
class FMacErrorReport : public FGenericErrorReport
{
public:
	/**
	 * Default constructor: creates a report with no files
	 */
	FMacErrorReport()
	{
	}
	
	/**
	 * Load helper modules
	 */
	static void Init();
	
	/**
	 * Unload helper modules
	 */
	static void ShutDown();

	/**
	 * Discover all files in the crash report directory
	 * @param Directory Full path to directory containing the report
	 */
	explicit FMacErrorReport(const FString& Directory);

	/**
	 * Parse the callstack from the Apple-style crash report log
	 * @return UE4 crash diagnosis text
	 */
	FText DiagnoseReport() const;

	/**
	 * Get the name of the crashed app from the report (hides implementation in FGenericErrorReport)
	 */
	FString FindCrashedAppName() const;

	/**
	 * Look for the most recent Mac Error Report
	 * @return Full path to the most recent report, or an empty string if none found
	 */
	static FString FindMostRecentErrorReport();
};
