// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#define LOG_TO_UELOG_AS_WELL 0

// helpful text macros
#define TEXT_NULL TEXT("NULL")
#define TEXT_TRUE TEXT("TRUE")
#define TEXT_FALSE TEXT("FALSE")
#define TEXT_EMPTY TEXT("")
#define TEXT_CONDITION(Condition) ((Condition) ? TEXT_TRUE : TEXT_FALSE)
#define TEXT_SELF_IF_TRUE(Self) (Self ? TEXT(#Self) : TEXT_EMPTY)
#define TEXT_NAME(Object) ((Object != NULL) ? *(Object->GetName()) : TEXT_NULL)

DECLARE_LOG_CATEGORY_EXTERN(LogVisual, Warning, All);

class FJsonValue;

struct FVisLogSelfDrawingElement
{
	virtual void Draw(class UCanvas* Canvas) const = 0;
};

struct ENGINE_API FVisLogEntry
{
	float TimeStamp;
#if ENABLE_VISUAL_LOG
	FVector Location;
	
	struct FLogLine
	{
		FString Line;
		FName Category;
		TEnumAsByte<ELogVerbosity::Type> Verbosity;
		int64 UserData;
		FName TagName;

		FLogLine(const FName& InCategory, ELogVerbosity::Type InVerbosity, const FString& InLine)
			: Line(InLine), Category(InCategory), Verbosity(InVerbosity), UserData(0)
		{}

		FLogLine(const FName& InCategory, ELogVerbosity::Type InVerbosity, const FString& InLine, int64 InUserData)
			: Line(InLine), Category(InCategory), Verbosity(InVerbosity), UserData(InUserData)
		{}
	};
	TArray<FLogLine> LogLines;
	
	struct FStatusCategory
	{
		TArray<FString> Data;
		FString Category;

		FORCEINLINE void Add(const FString& Key, const FString& Value)
		{
			Data.Add(FString(Key).AppendChar(TEXT('|')) + Value);
		}

		FORCEINLINE bool GetDesc(int32 Index, FString& Key, FString& Value) const
		{
			if (Data.IsValidIndex(Index))
			{
				int32 SplitIdx = INDEX_NONE;
				if (Data[Index].FindChar(TEXT('|'), SplitIdx))
				{
					Key = Data[Index].Left(SplitIdx);
					Value = Data[Index].Mid(SplitIdx + 1);
					return true;
				}
			}

			return false;
		}
	};
	TArray<FStatusCategory> Status;

	struct FElementToDraw
	{
		enum EElementType {
			Invalid = 0,
			SinglePoint, // individual points. 
			Segment, // pairs of points 
			Path,	// sequence of point
			Box,
			// note that in order to remain backward compatibility in terms of log
			// serialization new enum values need to be added at the end
		};

		FString Description;
		FName Category;
		TEnumAsByte<ELogVerbosity::Type> Verbosity;
		TArray<FVector> Points;
	protected:
		friend struct FVisLogEntry;
		uint8 Type;
	public:
		uint8 Color;
		union
		{
			uint16 Thicknes;
			uint16 Radius;
		};

		FElementToDraw(EElementType InType = Invalid) : Verbosity(ELogVerbosity::All), Type(InType), Color(0xff), Thicknes(0)
		{}

		FElementToDraw(const FString& InDescription, const FColor& InColor, uint16 InThickness, const FName& InCategory) : Category(InCategory), Verbosity(ELogVerbosity::All), Type(uint16(Invalid)), Thicknes(InThickness)
		{
			if (InDescription.IsEmpty() == false)
			{
				Description = InDescription;
			}
			SetColor(InColor);
		}

		FORCEINLINE void SetColor(const FColor& InColor) 
		{ 
			Color = ((InColor.DWColor() >> 30) << 6)
				| (((InColor.DWColor() & 0x00ff0000) >> 22) << 4)
				| (((InColor.DWColor() & 0x0000ff00) >> 14) << 2)
				|  ((InColor.DWColor() & 0x000000ff) >> 6);
		}

		FORCEINLINE EElementType GetType() const
		{
			return EElementType(Type);
		}

		FORCEINLINE void SetType(EElementType InType)
		{
			Type = InType;
		}

		FORCEINLINE FColor GetFColor() const
		{ 
			return FColor(
				((Color & 0xc0) << 24)
				| ((Color & 0x30) << 18)
				| ((Color & 0x0c) << 12)
				| ((Color & 0x03) << 6)
				);
		}
	};
	TArray<FElementToDraw> ElementsToDraw;

	struct FHistogramSample
	{
		FHistogramSample() : Verbosity(ELogVerbosity::All) {}

		FName Category;
		TEnumAsByte<ELogVerbosity::Type> Verbosity;
		FName GraphName;
		FName DataName;
		FVector2D SampleValue;
	};
	TArray<FHistogramSample>	HistogramSamples;

	struct FDataBlock
	{
		FDataBlock() : Verbosity(ELogVerbosity::All) {}

		FName TagName;
		FName Category;
		TEnumAsByte<ELogVerbosity::Type> Verbosity;
		TArray<uint8>	Data;
	};
	TArray<FDataBlock>	DataBlocks;


	FVisLogEntry(const class AActor* Actor, TArray<TWeakObjectPtr<UObject> >* Children);
	FVisLogEntry(TSharedPtr<FJsonValue> FromJson);
	
	TSharedPtr<FJsonValue> ToJson() const;

	// path
	void AddElement(const TArray<FVector>& Points, const FName& CategoryName, const FColor& Color = FColor::White, const FString& Description = TEXT(""), uint16 Thickness = 0);
	// location
	void AddElement(const FVector& Point, const FName& CategoryName, const FColor& Color = FColor::White, const FString& Description = TEXT(""), uint16 Thickness = 0);
	// segment
	void AddElement(const FVector& Start, const FVector& End, const FName& CategoryName, const FColor& Color = FColor::White, const FString& Description = TEXT(""), uint16 Thickness = 0);
	// box
	void AddElement(const FBox& Box, const FName& CategoryName, const FColor& Color = FColor::White, const FString& Description = TEXT(""), uint16 Thickness = 0);

	// histogram sample
	void AddHistogramData(const FVector2D& DataSample, const FName& CategoryName, const FName& GraphName, const FName& DataName);

	// Custom data block
	void AddDataBlock(const FString& TagName, const TArray<uint8>& BlobDataArray, const FName& CategoryName);

	// find index of status category
	FORCEINLINE int32 FindStatusIndex(const FString& CategoryName)
	{
		for (int32 TestCategoryIndex = 0; TestCategoryIndex < Status.Num(); TestCategoryIndex++)
		{
			if (Status[TestCategoryIndex].Category == CategoryName)
			{
				return TestCategoryIndex;
			}
		}

		return INDEX_NONE;
	}
#endif // ENABLE_VISUAL_LOG
};

#if ENABLE_VISUAL_LOG

#if LOG_TO_UELOG_AS_WELL
#define TO_TEXT_LOG(CategoryName, Verbosity, Format, ...) FMsg::Logf(__FILE__, __LINE__, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity, Format, ##__VA_ARGS__);
#else
#define TO_TEXT_LOG(CategoryName, Verbosity, Format, ...) 
#endif // LOG_TO_UELOG_AS_WELL

#define REDIRECT_TO_VLOG(Dest) FVisualLog::Get().RedirectToVisualLog(this, Dest)
#define REDIRECT_OBJECT_TO_VLOG(Src, Dest) FVisualLog::Get().RedirectToVisualLog(Src, Dest)

#define UE_VLOG(Object, CategoryName, Verbosity, Format, ...) \
{ \
	SCOPE_CYCLE_COUNTER(STAT_VisualLog); \
	static_assert((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) < ELogVerbosity::NumVerbosity && ELogVerbosity::Verbosity > 0, "Verbosity must be constant and in range."); \
	if (FVisualLog::Get().IsRecording() && (!FVisualLog::Get().IsAllBlocked() || FVisualLog::Get().InWhitelist(CategoryName.GetCategoryName()))) \
	{ \
		const AActor* MyOwnerActor = FVisualLog::Get().GetVisualLogRedirection(Object); \
		ensure(MyOwnerActor != NULL); \
		if (MyOwnerActor) \
		{  \
			FVisualLog::Get().LogLine(MyOwnerActor, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity, FString::Printf(Format, ##__VA_ARGS__)); \
		} \
	} \
	if (UE_LOG_CHECK_COMPILEDIN_VERBOSITY(CategoryName, Verbosity)) \
	{ \
		if (!CategoryName.IsSuppressed(ELogVerbosity::Verbosity) && (Object) != NULL) \
		{ \
			TO_TEXT_LOG(CategoryName, Verbosity, Format, ##__VA_ARGS__); \
		} \
	} \
}

#define UE_CVLOG(Condition, Object, CategoryName, Verbosity, Format, ...) if(Condition) \
{ \
	UE_VLOG(Object, CategoryName, Verbosity, Format, ##__VA_ARGS__); \
}

#define UE_VLOG_SEGMENT_THICK(Object, CategoryName, Verbosity, SegmentStart, SegmentEnd, Color, Thickness, DescriptionFormat, ...) \
{ \
	SCOPE_CYCLE_COUNTER(STAT_VisualLog); \
	static_assert((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) < ELogVerbosity::NumVerbosity && ELogVerbosity::Verbosity > 0, "Verbosity must be constant and in range."); \
	if (FVisualLog::Get().IsRecording() && (!FVisualLog::Get().IsAllBlocked() || FVisualLog::Get().InWhitelist(CategoryName.GetCategoryName()))) \
	{ \
		const AActor* OwnerActor = FVisualLog::Get().GetVisualLogRedirection(Object); \
		ensure(OwnerActor != NULL); \
		if (OwnerActor) \
		{  \
			FVisualLog::Get().GetEntryToWrite(OwnerActor)->AddElement(SegmentStart, SegmentEnd, CategoryName.GetCategoryName(), Color, FString::Printf(DescriptionFormat, ##__VA_ARGS__), Thickness); \
		} \
	} \
}

#define UE_VLOG_SEGMENT(Object, CategoryName, Verbosity, SegmentStart, SegmentEnd, Color, DescriptionFormat, ...) \
	UE_VLOG_SEGMENT_THICK(Object, CategoryName, Verbosity, SegmentStart, SegmentEnd, Color, 0, DescriptionFormat, ##__VA_ARGS__)

#define UE_VLOG_LOCATION(Object, CategoryName, Verbosity, Location, Radius, Color, DescriptionFormat, ...) \
{ \
	SCOPE_CYCLE_COUNTER(STAT_VisualLog); \
	static_assert((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) < ELogVerbosity::NumVerbosity && ELogVerbosity::Verbosity > 0, "Verbosity must be constant and in range."); \
	if (FVisualLog::Get().IsRecording() && (!FVisualLog::Get().IsAllBlocked() || FVisualLog::Get().InWhitelist(CategoryName.GetCategoryName()))) \
	{ \
		const AActor* OwnerActor = FVisualLog::Get().GetVisualLogRedirection(Object); \
		ensure(OwnerActor != NULL); \
		if (OwnerActor) \
		{  \
			FVisualLog::Get().GetEntryToWrite(OwnerActor)->AddElement(Location, CategoryName.GetCategoryName(), Color, FString::Printf(DescriptionFormat, ##__VA_ARGS__), Radius); \
		} \
	} \
}

#define UE_VLOG_BOX(Object, CategoryName, Verbosity, Box, Color, DescriptionFormat, ...) \
{ \
	SCOPE_CYCLE_COUNTER(STAT_VisualLog); \
	static_assert((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) < ELogVerbosity::NumVerbosity && ELogVerbosity::Verbosity > 0, "Verbosity must be constant and in range."); \
	if (FVisualLog::Get().IsRecording() && (!FVisualLog::Get().IsAllBlocked() || FVisualLog::Get().InWhitelist(CategoryName.GetCategoryName()))) \
	{ \
		const AActor* OwnerActor = FVisualLog::Get().GetVisualLogRedirection(Object); \
		ensure(OwnerActor != NULL); \
		if (OwnerActor) \
		{  \
			FVisualLog::Get().GetEntryToWrite(OwnerActor)->AddElement(Box, CategoryName.GetCategoryName(), Color, FString::Printf(DescriptionFormat, ##__VA_ARGS__)); \
		} \
	} \
}

#define UE_VLOG_HISTOGRAM(Object, CategoryName, Verbosity, GraphName, DataName, Data) \
{ \
	SCOPE_CYCLE_COUNTER(STAT_VisualLog); \
	static_assert((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) < ELogVerbosity::NumVerbosity && ELogVerbosity::Verbosity > 0, "Verbosity must be constant and in range."); \
	if (FVisualLog::Get().IsRecording() && (!FVisualLog::Get().IsAllBlocked() || FVisualLog::Get().InWhitelist(CategoryName.GetCategoryName()))) \
	{ \
		const AActor* OwnerActor = FVisualLog::Get().GetVisualLogRedirection(Object); \
		ensure(OwnerActor != NULL); \
		if (OwnerActor) \
		{  \
			FVisualLog::Get().GetEntryToWrite(OwnerActor)->AddHistogramData(Data, CategoryName.GetCategoryName(), GraphName, DataName); \
		} \
	} \
}

namespace VisualLogJson
{
	static const FString TAG_NAME = TEXT("Name");
	static const FString TAG_FULLNAME = TEXT("FullName");
	static const FString TAG_ENTRIES = TEXT("Entries");
	static const FString TAG_TIMESTAMP = TEXT("TimeStamp");
	static const FString TAG_LOCATION = TEXT("Location");
	static const FString TAG_STATUS = TEXT("Status");
	static const FString TAG_STATUSLINES = TEXT("StatusLines");
	static const FString TAG_CATEGORY = TEXT("Category");
	static const FString TAG_LINE = TEXT("Line");
	static const FString TAG_VERBOSITY = TEXT("Verb");
	static const FString TAG_LOGLINES = TEXT("LogLines");
	static const FString TAG_DESCRIPTION = TEXT("Description");
	static const FString TAG_TYPECOLORSIZE = TEXT("TypeColorSize");
	static const FString TAG_POINTS = TEXT("Points");
	static const FString TAG_ELEMENTSTODRAW = TEXT("ElementsToDraw");
	static const FString TAG_TAGNAME = TEXT("DataBlockTagName");
	static const FString TAG_USERDATA = TEXT("UserData");

	static const FString TAG_HISTOGRAMSAMPLES = TEXT("HistogramSamples");
	static const FString TAG_HISTOGRAMSAMPLE = TEXT("Sample");
	static const FString TAG_HISTOGRAMGRAPHNAME = TEXT("GraphName");
	static const FString TAG_HISTOGRAMDATANAME = TEXT("DataName");

	static const FString TAG_DATABLOCK = TEXT("DataBlock");
	static const FString TAG_DATABLOCK_DATA = TEXT("DataBlockData");

	static const FString TAG_LOGS = TEXT("Logs");
}

struct ENGINE_API FActorsVisLog
{
	static const int VisLogInitialSize = 8;

	FName Name;
	FString FullName;
	TArray<TSharedPtr<FVisLogEntry> > Entries;

	FActorsVisLog(const class AActor* Actor, TArray<TWeakObjectPtr<UObject> >* Children);
	FActorsVisLog(TSharedPtr<FJsonValue> FromJson);

	TSharedPtr<FJsonValue> ToJson() const;
};

DECLARE_DELEGATE_RetVal(FString, FVisualLogFilenameGetterDelegate);

struct FVisualLogExtensionInterface
{
	virtual void OnTimestampChange(float Timestamp, class UWorld* InWorld, class AActor* HelperActor) = 0;
	virtual void DrawData(class UWorld* InWorld, class UCanvas* Canvas, class AActor* HelperActor, const FName& TagName, const FVisLogEntry::FDataBlock& DataBlock, float Timestamp) = 0;
	virtual void DisableDrawingForData(class UWorld* InWorld, class UCanvas* Canvas, class AActor* HelperActor, const FName& TagName, const FVisLogEntry::FDataBlock& DataBlock, float Timestamp) = 0;
	virtual void LogEntryLineSelectionChanged(TSharedPtr<struct FLogEntryItem> SelectedItem, int64 UserData, FName TagName) = 0;
};


/** This class is to capture all log output even if the Visual Logger is closed */
class ENGINE_API FVisualLog : public FOutputDevice
{
public:
	typedef TMap<const class AActor*, TSharedPtr<FActorsVisLog> > FLogsMap;
	typedef TMap<const class AActor*, TArray<TWeakObjectPtr<UObject> > > FLogRedirectsMap;

	DECLARE_DELEGATE_TwoParams(FOnNewLogCreatedDelegate, const AActor*, TSharedPtr<FActorsVisLog>);

	FVisualLog();
	~FVisualLog();
	
	static FVisualLog& Get()
	{
		static FVisualLog StaticLog;
		return StaticLog;
	}

	void Cleanup(bool bReleaseMemory = false);

	void Redirect(class UObject* Source, const class AActor* NewRedirection);
	
	const class AActor* GetVisualLogRedirection(const class UObject* Source);

	void RedirectToVisualLog(const class UObject* Src, const class AActor* Dest);

	void LogLine(const class AActor* Actor, const FName& CategoryName, ELogVerbosity::Type Verbosity, const FString& Line, int64 UserData = 0, FName TagName = NAME_Name);

	const FLogsMap* GetLogs() const { return &LogsMap; }

	void RegisterNewLogsObserver(const FOnNewLogCreatedDelegate& NewObserver) { OnNewLogCreated = NewObserver; }
	void ClearNewLogsObserver() { OnNewLogCreated = NULL; }

	void SetIsRecording(bool NewRecording, bool bRecordToFile = false);
	FORCEINLINE bool IsRecording() const { return !!bIsRecording; }
	void SetIsRecordingOnServer(bool NewRecording) { bIsRecordingOnServer = NewRecording; }
	FORCEINLINE bool IsRecordingOnServer() const { return !!bIsRecordingOnServer; }
	void DumpRecordedLogs();

	FORCEINLINE bool InWhitelist(FName InName) const { return Whitelist.Find(InName) != INDEX_NONE; }
	FORCEINLINE bool IsAllBlocked() const { return !!bIsAllBlocked; }
	FORCEINLINE void BlockAllLogs(bool bBlock = true) { bIsAllBlocked = bBlock; }
	FORCEINLINE void AddCategortyToWhiteList(FName InCategory) { Whitelist.AddUnique(InCategory); }
	FORCEINLINE void ClearWhiteList() { Whitelist.Reset(); }

	FArchive* FileAr;

	FVisLogEntry* GetEntryToWrite(const class AActor* Actor);

	/** highly encouradged to use FVisualLogFilenameGetterDelegate::CreateUObject with this */
	void SetLogFileNameGetter(const FVisualLogFilenameGetterDelegate& InLogFileNameGetter) { LogFileNameGetter = InLogFileNameGetter; }

	TMap<FName, FVisualLogExtensionInterface*> AllExtensions;
	void RegisterExtension(FName TagName, FVisualLogExtensionInterface* ExtensionInterface) { AllExtensions.Add(TagName, ExtensionInterface); }
	void UnregisterExtension(FName TagName, FVisualLogExtensionInterface* ExtensionInterface) { AllExtensions.Remove(TagName); }
	FVisualLogExtensionInterface* GetExtensionForTag(const FName TagName) { return AllExtensions.Contains(TagName) ? AllExtensions[TagName] : NULL; }
	TMap<FName, FVisualLogExtensionInterface*>& GetAllExtensions() { return AllExtensions; }
protected:

	FString GetLogFileFullName() const;

	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) override;

	void LogElementImpl(FVisLogSelfDrawingElement* Element);

	FORCEINLINE_DEBUGGABLE TSharedPtr<FActorsVisLog> GetLog(const class AActor* Actor)
	{
		TSharedPtr<FActorsVisLog>* Log = LogsMap.Find(Actor);
		if (Log == NULL)
		{
			Log = &(LogsMap.Add(Actor, MakeShareable(new FActorsVisLog(Actor, RedirectsMap.Find(Actor)))));
			OnNewLogCreated.ExecuteIfBound(Actor, *Log);
		}
		return *Log;
	}
			
private:
	/** All log entries since this module has been started (via Start() call)
	 *	or since the last extraction-with-removal (@todo name function here) */
	FLogsMap LogsMap;

	FLogRedirectsMap RedirectsMap;

	TArray<FName>	Whitelist;

	FOnNewLogCreatedDelegate OnNewLogCreated;

	float StartRecordingTime;

	int32 bIsRecording : 1;
	int32 bIsRecordingOnServer : 1;
	int32 bIsRecordingToFile : 1;
	int32 bIsAllBlocked : 1;

	FVisualLogFilenameGetterDelegate LogFileNameGetter;
};

#else
	#define UE_VLOG(Actor, CategoryName, Verbosity, Format, ...) UE_LOG(CategoryName, Verbosity, Format, ##__VA_ARGS__)
	#define UE_CVLOG(Condition, Actor, CategoryName, Verbosity, Format, ...) UE_CLOG(Condition, CategoryName, Verbosity, Format, ##__VA_ARGS__)
	#define UE_VLOG_SEGMENT(Actor, CategoryName, Verbosity, SegmentStart, SegmentEnd, Color, DescriptionFormat, ...)
	#define UE_VLOG_LOCATION(Actor, CategoryName, Verbosity, Location, Radius, Color, DescriptionFormat, ...)
	#define UE_VLOG_BOX(Actor, CategoryName, Verbosity, Box, Color, DescriptionFormat, ...) 
	#define UE_VLOG_HISTOGRAM(Actor, CategoryName, Verbosity, GraphName, DataName, Data)
	#define REDIRECT_TO_VLOG(Dest)
	#define REDIRECT_OBJECT_TO_VLOG(Src, Destination) 
#endif //ENABLE_VISUAL_LOG
