// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "MacEvent.h"
#include "MacApplication.h"
#include "CocoaThread.h"
#include "CocoaWindow.h"

FMacEvent::FMacEvent(NSEvent* const Event)
: EventData(Event)
{
	check(Event);
	[Event retain];
	
	CocoaWindow = FMacApplication::FindMacEventWindow(Event);

	CGEventRef CGEvent = [Event CGEvent];
	CGPoint CGMouseLocation = CGEventGetLocation(CGEvent);
	MousePosition.X = CGMouseLocation.x;
	MousePosition.Y = CGMouseLocation.y;
}

FMacEvent::FMacEvent(NSNotification* const Notification, NSWindow* const Window)
: EventData(Notification)
{
	check(Notification);
	[Notification retain];

	CocoaWindow = [Window isKindOfClass:[FCocoaWindow class]] ? (FCocoaWindow*)Window : nullptr;

	CGEventRef Event = CGEventCreate(NULL);
	CGPoint CursorPos = CGEventGetLocation(Event);
	MousePosition.X = CursorPos.x;
	MousePosition.Y = CursorPos.y;
	CFRelease(Event);
}

void FMacEvent::SendToGameRunLoop(FMacEvent const* const Event, EMacEventSendMethod SendMethod, NSArray* SendModes)
{
	dispatch_block_t Block = ^{ FMacApplication::ProcessEvent(Event); };
	const bool bWait = (SendMethod == EMacEventSendMethod::Sync);
	
	GameThreadCall(Block, SendModes, bWait);
}
	
void FMacEvent::SendToGameRunLoop(NSEvent* const Event, EMacEventSendMethod SendMethod, NSArray* SendModes)
{
	if(MacApplication)
	{
		FMacEvent* MacEvent = new FMacEvent(Event);
		FMacEvent::SendToGameRunLoop(MacEvent, SendMethod, SendModes);
	}
}

void FMacEvent::SendToGameRunLoop(NSNotification* const Notification, NSWindow* const Window, EMacEventSendMethod SendMethod, NSArray* SendModes)
{
	if(MacApplication)
	{
		FMacEvent* MacEvent = new FMacEvent(Notification, Window);
		FMacEvent::SendToGameRunLoop(MacEvent, SendMethod, SendModes);
	}
}

FMacEvent::~FMacEvent(void)
{
	if(EventData)
	{
		[EventData release];
		EventData = nil;
	}
}

NSEvent* FMacEvent::GetEvent(void) const
{
	if(EventData && [EventData isKindOfClass:[NSEvent class]])
	{
		return (NSEvent*)EventData;
	}
	else
	{
		return nil;
	}
}

NSNotification* FMacEvent::GetNotification(void) const
{
	if(EventData && [EventData isKindOfClass:[NSNotification class]])
	{
		return (NSNotification*)EventData;
	}
	else
	{
		return nil;
	}
}

FCocoaWindow* FMacEvent::GetWindow(void) const
{
	return CocoaWindow;
}

FVector2D FMacEvent::GetMousePosition(void) const
{
	return MousePosition;
}
