// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Implements a details view customization for the FDateTime structure.
 */
class FDateTimeStructCustomization
	: public IPropertyTypeCustomization
{
public:

	/**
	 * Creates an instance of this class.
	 *
	 * @return The new instance.
	 */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance( )
	{
		return MakeShareable(new FDateTimeStructCustomization());
	}

public:

	// IPropertyTypeCustomization interface

	virtual void CustomizeChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;
	virtual void CustomizeHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;

private:

	/** Handles getting the text color of the editable text box. */
	FSlateColor HandleTextBoxForegroundColor( ) const;

	/** Handles getting the text to be displayed in the editable text box. */
	FText HandleTextBoxText( ) const;

	/** Handles changing the value in the editable text box. */
	void HandleTextBoxTextChanged( const FText& NewText );

	/** Handles committing the text in the editable text box. */
	void HandleTextBoxTextCommited( const FText& NewText, ETextCommit::Type CommitInfo );

private:

	/** Holds a flag indicating whether the current input is a valid GUID. */
	bool InputValid;

	/** Holds a handle to the property being edited. */
	TSharedPtr<IPropertyHandle> PropertyHandle;

	/** Holds the text box for editing the Guid. */
	TSharedPtr<SEditableTextBox> TextBox;
};
