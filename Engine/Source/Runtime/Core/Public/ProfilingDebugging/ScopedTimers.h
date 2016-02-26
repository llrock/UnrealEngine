// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ThreadingBase.h"

/**
 * Utility stopwatch class for tracking the duration of some action (tracks 
 * time in seconds and adds it to the specified variable on destruction).
 */
class FDurationTimer
{
public:
	explicit FDurationTimer(double& AccumulatorIn)
		: StartTime(FPlatformTime::Seconds())
		, Accumulator(AccumulatorIn)
	{}

	double Start()
	{
		StartTime = FPlatformTime::Seconds();
		return StartTime;
	}

	double Stop()
	{
		double StopTime = FPlatformTime::Seconds();
		Accumulator += (StopTime - StartTime);
		StartTime = StopTime;
			
		return StopTime;
	}

	double GetAccumulatedTime() const
	{
		return Accumulator;
	}

private:
	/** Start time, captured in ctor. */
	double StartTime;
	/** Time variable to update. */
	double& Accumulator;
};

/**
 * Utility class for tracking the duration of a scoped action (the user 
 * doesn't have to call Start() and Stop() manually).
 */
class FScopedDurationTimer : private FDurationTimer
{
public:
	explicit FScopedDurationTimer(double& AccumulatorIn)
		: FDurationTimer(AccumulatorIn)
	{
		Start();
	}

	/** Dtor, updating seconds with time delta. */
	~FScopedDurationTimer()
	{
		Stop();
	}
};

/**
 * Utility class for tracking the duration of a scoped action to an accumulator in a thread-safe fashion.
 * Can accumulate into a 32bit or 64bit counter.
 * 
 * ThreadSafeCounterClass is expected to be a thread-safe type with a non-static member Add(uint32) that will work correctly if called from multiple threads simultaneously.
 */
template <typename ThreadSafeCounterClass = FThreadSafeCounter>
class TScopedDurationThreadSafeTimer
{
public:
	explicit TScopedDurationThreadSafeTimer(ThreadSafeCounterClass& InCounter)
		:Counter(InCounter)
		, StartCycles(FPlatformTime::Cycles())
	{
	}
	~TScopedDurationThreadSafeTimer()
	{
		Counter.Add(FPlatformTime::Cycles() - StartCycles);
	}
private:
	ThreadSafeCounterClass& Counter;
	int32 StartCycles;
};

/** Helper to create a TScopedDurationThreadSafeTimer using an existing FThreadSafeCounter. */
template <typename ThreadSafeCounterClass>
TScopedDurationThreadSafeTimer<ThreadSafeCounterClass> MakeScopedDurationThreadSafeTimer(ThreadSafeCounterClass& Counter)
{
	return TScopedDurationThreadSafeTimer<ThreadSafeCounterClass>(Counter);
}

typedef TScopedDurationThreadSafeTimer<> FScopedDurationThreadSafeTimer;
#if PLATFORM_HAS_64BIT_ATOMICS
typedef TScopedDurationThreadSafeTimer<FThreadSafeCounter64> FScopedDurationThreadSafeTimer64;
#endif

/**
 * Utility class for logging the duration of a scoped action (the user 
 * doesn't have to call Start() and Stop() manually).
 */
class FScopedDurationTimeLogger
{
public:
	explicit FScopedDurationTimeLogger(FString InMsg = TEXT("Scoped action"), FOutputDevice* InDevice = GLog)
		: Msg        (MoveTemp(InMsg))
		, Device     (InDevice)
		, Accumulator(0.0)
		, Timer      (Accumulator)
	{
		Timer.Start();
	}

	~FScopedDurationTimeLogger()
	{
		Timer.Stop();
		Device->Logf(TEXT("%s: %f secs"), *Msg, Accumulator);
	}

private:
	FString        Msg;
	FOutputDevice* Device;
	double         Accumulator;
	FDurationTimer Timer;
};
