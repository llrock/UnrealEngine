// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Media.h"
#include "MediaSoundWave.h"


/**
 * Implements a details view customization for the UMediaSoundWave class.
 */
class FMediaSoundWaveCustomization
	: public IDetailCustomization
{
public:

	// IDetailCustomization interface

	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override;

public:

	/**
	 * Creates an instance of this class.
	 *
	 * @return The new instance.
	 */
	static TSharedRef<IDetailCustomization> MakeInstance( )
	{
		return MakeShareable(new FMediaSoundWaveCustomization());
	}
};
