// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "RigDetails.h"
#include "Animation/Rig.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE	"RigDetails"

// Table Headers for node list
#define NODE_TABLE_DISPLAYNAME	TEXT("DisplayName")
#define NODE_TABLE_NODENAME		TEXT("NodeName")
#define NODE_TABLE_PARENTNAME	TEXT("ParentName")


TSharedRef<IDetailCustomization> FRigDetails::MakeInstance()
{
	return MakeShareable(new FRigDetails);
}

void FRigDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilderPtr = &DetailBuilder;
	TArray<TWeakObjectPtr<UObject>> Objects;

	DetailBuilder.GetObjectsBeingCustomized(Objects);

	// if more than one, do not customize?
	if (Objects.Num() > 1)
	{
		return;
	}

	ItemBeingEdited = Objects[0];

	IDetailCategoryBuilder& NodeCategory = DetailBuilder.EditCategory("Node");
	IDetailCategoryBuilder& TransformBaseCategory = DetailBuilder.EditCategory("Constraint Setup");

	TransformBasesPropertyHandle = DetailBuilder.GetProperty("TransformBases");
	NodesPropertyHandle = DetailBuilder.GetProperty("Nodes");

	// since now we can't really resize the array, we'll just allocate everything here
	// if you reallocate, TSharedPtr<FString> for combo box won't work because
	uint32 NumElement = 0;
	check (FPropertyAccess::Fail != NodesPropertyHandle->AsArray()->GetNumElements(NumElement));

	if ( NumElement > 0 )
	{
		DisplayNameTextBoxes.AddZeroed(NumElement);
	}

	check (FPropertyAccess::Fail != TransformBasesPropertyHandle->AsArray()->GetNumElements(NumElement));
	if ( NumElement > 0 )
	{
		ParentSpaceOptionList.AddZeroed(NumElement);
		// I need two per each - translation/orientation
		ParentSpaceComboBoxes.AddZeroed(NumElement*2);
	}

	TSharedRef<FDetailArrayBuilder> NodeArrayBuilder = MakeShareable(new FDetailArrayBuilder(NodesPropertyHandle.ToSharedRef()));
	NodeArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FRigDetails::GenerateNodeArrayElementWidget, &DetailBuilder));

	NodeCategory.AddCustomBuilder( NodeArrayBuilder, false );

	TSharedRef<FDetailArrayBuilder> TransformBaseArrayBuilder = MakeShareable(new FDetailArrayBuilder(TransformBasesPropertyHandle.ToSharedRef()));
	TransformBaseArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FRigDetails::GenerateTransformBaseArrayElementWidget, &DetailBuilder));

	// add custom menu
	// -> set all to world
	// -> set all to default parent
	TransformBaseCategory.AddCustomRow(TEXT(""))
	[
		// two button 1. view 2. save to base pose
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(5)
		.HAlign(HAlign_Center)
		[
			SNew(SButton)
			.OnClicked(FOnClicked::CreateSP(this, &FRigDetails::OnSetAllToWorld))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Text(LOCTEXT("SetAllToWorld_ButtonLabel", "Set All Constraints to World"))
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(5)
		.HAlign(HAlign_Center)
		[
			SNew(SButton)
			.OnClicked(FOnClicked::CreateSP(this, &FRigDetails::OnSetAllToParent))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Text(LOCTEXT("SetAllToParent_ButtonLabel", "Set All Constraints to Parent"))
		]
	];

	TransformBaseCategory.AddCustomBuilder( TransformBaseArrayBuilder, false );
}

void FRigDetails::GenerateNodeArrayElementWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder, IDetailLayoutBuilder* DetailLayout)
{
	TSharedRef<IPropertyHandle> DisplayNameProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNode, DisplayName)).ToSharedRef();
	TSharedRef<IPropertyHandle> NodeNameProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNode, Name)).ToSharedRef();
	TSharedRef<IPropertyHandle> ParentNameProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNode, ParentName)).ToSharedRef();
	TSharedRef<IPropertyHandle> AdvancedProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNode, bAdvanced)).ToSharedRef();

	TSharedPtr<SEditableTextBox> DisplayTextBox;

	// the interface will be node [display name] [parent node]
	// delegate for display name
	FString NodeName, ParentNodeName, DisplayString;
	check (NodeNameProp->GetValueAsDisplayString(NodeName) != FPropertyAccess::Fail);
	check (ParentNameProp->GetValueAsDisplayString(ParentNodeName) != FPropertyAccess::Fail);
	check (DisplayNameProp->GetValueAsDisplayString(DisplayString) != FPropertyAccess::Fail);

	ChildrenBuilder.AddChildContent(TEXT(""))
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.Padding(5, 2)
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(100)
			.Content()
			[
				SNew(STextBlock)
				.Text(NodeName)
				.Font(DetailLayout->GetDetailFontBold())
			]
		]

		+SHorizontalBox::Slot()
		.Padding(5, 2)
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(150)
			.Content()
			[
				SNew(STextBlock)
				.Text(FString::Printf(TEXT(" [Parent : %s] "), *ParentNodeName))
				.Font(DetailLayout->GetDetailFont())
			]
		]

		+SHorizontalBox::Slot()
		.Padding(5, 2)
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(FString(TEXT("Display Name")))
			.Font(DetailLayout->GetDetailFontBold())
		]

		+SHorizontalBox::Slot()
		.Padding(5, 2)
		.AutoWidth()
		.HAlign(HAlign_Left)
		[
			SNew(SBox)
			.WidthOverride(250)
			.Content()
			[
				SAssignNew(DisplayTextBox, SEditableTextBox)
				.Text( TAttribute<FText>::Create( TAttribute<FText>::FGetter::CreateSP(this, &FRigDetails::GetDisplayName, DisplayNameProp) ) )
				.Font(DetailLayout->GetDetailFont())
				.OnTextChanged(this, &FRigDetails::OnDisplayNameChanged, DisplayNameProp, ArrayIndex)
				.OnTextCommitted(this, &FRigDetails::OnDisplayNameCommitted, DisplayNameProp, ArrayIndex)
				.MinDesiredWidth(200)
			]
		]

		+SHorizontalBox::Slot()
		.Padding(5, 2)
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(FString(TEXT("Advanced")))
			.Font(DetailLayout->GetDetailFontBold())
		]

		+SHorizontalBox::Slot()
		.Padding(5, 2)
		.FillWidth(1)
		.HAlign(HAlign_Left)
		[
			SNew(SBox)
			.WidthOverride(250)
			.Content()
			[
				SNew(SCheckBox)
				.IsChecked(this, &FRigDetails::AdvancedCheckBoxIsChecked, AdvancedProp)
				.OnCheckStateChanged(this, &FRigDetails::OnAdvancedCheckBoxStateChanged, AdvancedProp)
			]
		]
	];

	DisplayNameTextBoxes[ArrayIndex] = DisplayTextBox;
};

FText FRigDetails::GetDisplayName(TSharedRef<IPropertyHandle> DisplayNameProp) const
{
	FText DisplayText;
	
	check (FPropertyAccess::Success == DisplayNameProp->GetValueAsDisplayText(DisplayText));

	return DisplayText;
}

void FRigDetails::ValidErrorMessage(const FString & DisplayString, int32 ArrayIndex)
{
	if(DisplayNameTextBoxes.IsValidIndex(ArrayIndex))
	{
		DisplayNameTextBoxes[ArrayIndex]->SetError(TEXT(""));

		if(DisplayString.Len() == 0)
		{
			DisplayNameTextBoxes[ArrayIndex]->SetError(TEXT("Name can't be empty"));
		}
		else
		{
			// verify if this name is unique
			URig * Rig = Cast<URig>(ItemBeingEdited.Get());
			if(Rig)
			{
				FString NewText = DisplayString;
				// make sure that name is unique
				const TArray<FNode> & Nodes = Rig->GetNodes();
				int32 NodeIndex = 0;
				for(auto Node : Nodes)
				{
					if(NodeIndex++ != ArrayIndex && Node.DisplayName == NewText)
					{
						DisplayNameTextBoxes[ArrayIndex]->SetError(TEXT("Name should be unique."));
					}
				}
			}
		}
	}
}

void FRigDetails::OnDisplayNameChanged(const FText& Text, TSharedRef<IPropertyHandle> DisplayNameProp, int32 ArrayIndex)
{
	// still set it since you don't know what they come up with 
	DisplayNameProp->SetValueFromFormattedString(Text.ToString());
	ValidErrorMessage(Text.ToString(), ArrayIndex);
}

void FRigDetails::OnDisplayNameCommitted(const FText& Text, ETextCommit::Type CommitType, TSharedRef<IPropertyHandle> DisplayNameProp, int32 ArrayIndex)
{
	// @todo error check here? I basically need mirror string to avoid the issue
	DisplayNameProp->SetValueFromFormattedString(Text.ToString());
}

void FRigDetails::GenerateTransformBaseArrayElementWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder, IDetailLayoutBuilder* DetailLayout)
{
	TSharedRef<IPropertyHandle> NodeNameProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTransformBase, Node)).ToSharedRef();
	TSharedPtr<IPropertyHandleArray> ConstraintsProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTransformBase, Constraints))->AsArray();

	// translation
	TSharedPtr<IPropertyHandleArray> TransformConstraintsProp_T = ConstraintsProp->GetElement(0)->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTransformBaseConstraint, TransformConstraints))->AsArray();
	TSharedRef<IPropertyHandle> ParentNameProp_T = TransformConstraintsProp_T->GetElement(0)->GetChildHandle(GET_MEMBER_NAME_CHECKED(FRigTransformConstraint, ParentSpace)).ToSharedRef();

	// orientation
	TSharedPtr<IPropertyHandleArray> TransformConstraintsProp_R = ConstraintsProp->GetElement(1)->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTransformBaseConstraint, TransformConstraints))->AsArray();
	TSharedRef<IPropertyHandle> ParentNameProp_R = TransformConstraintsProp_R->GetElement(0)->GetChildHandle(GET_MEMBER_NAME_CHECKED(FRigTransformConstraint, ParentSpace)).ToSharedRef();

	// the interface will be node [display name] [parent node]
	// delegate for display name
	FString NodeName, ParentNodeName_T, ParentNodeName_R;
	check(NodeNameProp->GetValueAsDisplayString(NodeName) != FPropertyAccess::Fail);
	check(ParentNameProp_T->GetValueAsDisplayString(ParentNodeName_T) != FPropertyAccess::Fail);
	check(ParentNameProp_R->GetValueAsDisplayString(ParentNodeName_R) != FPropertyAccess::Fail);

	// create string list for picking parent node
	// make sure you don't include itself and find what is curretn selected item
	TArray<TSharedPtr<FString>> & ParentNodeOptions = ParentSpaceOptionList[ArrayIndex];

	ParentNodeOptions.Empty();
	ParentNodeOptions.Add(MakeShareable(new FString(URig::WorldNodeName.ToString())));
	URig * Rig = Cast<URig>(ItemBeingEdited.Get());
	check (Rig);
	const TArray<FNode> & Nodes = Rig->GetNodes();
	if ( Nodes.Num() <= 0 )
	{
		return;
	}

	int32 NodeIndex = 0, ParentIndex_T=0, ParentIndex_R=0;
	const FNode & CurNode = Nodes[ArrayIndex];
	for(auto Node : Nodes)
	{
		if (NodeIndex != ArrayIndex)
		{
			ParentNodeOptions.Add(MakeShareable(new FString(Node.Name.ToString())));

			if (Node.Name.ToString() == ParentNodeName_T)
			{
				ParentIndex_T = ParentNodeOptions.Num()-1;
			}

			if(Node.Name.ToString() == ParentNodeName_R)
			{
				ParentIndex_R = ParentNodeOptions.Num()-1;
			}
		}

		NodeIndex++;
	}

	ChildrenBuilder.AddChildContent(TEXT(""))
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.Padding(5, 2)
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(100)
			.Content()
			[
				SNew(STextBlock)
				.Text(NodeName)
				.Font(DetailLayout->GetDetailFontBold())
			]
		]

		+SHorizontalBox::Slot()
		.Padding(5, 2)
		.AutoWidth()
		[
			SNew(SVerticalBox)

			+SVerticalBox::Slot()
			.Padding(2)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.Padding(2)
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(100)
					.Content()
					[
						SNew(STextBlock)
						.Text(FString(TEXT("Translation")))
						.Font(DetailLayout->GetDetailFontBold())
					]
				]

				+SHorizontalBox::Slot()
				.Padding(2)
				.FillWidth(1)
				[
					SNew(SBox)
					.WidthOverride(250)
					.Content()
					[

						SAssignNew(ParentSpaceComboBoxes[ArrayIndex*2], SComboBox< TSharedPtr<FString> >)
						.OptionsSource(&ParentNodeOptions)
						.InitiallySelectedItem(ParentNodeOptions[ParentIndex_T])
						.OnSelectionChanged(this, &FRigDetails::OnParentSpaceSelectionChanged, ParentNameProp_T)
						.OnGenerateWidget(this, &FRigDetails::MakeItemWidget)
						.OnComboBoxOpening(this, &FRigDetails::OnComboBoxOopening, ParentNameProp_T, ArrayIndex, true)
						.HasDownArrow(true)
						[
							SNew(STextBlock)
							.Text(this, &FRigDetails::GetSelectedTextLabel, ParentNameProp_T)
						]
					]
				]
			]

			+SVerticalBox::Slot()
			.Padding(2)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.Padding(2)
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(100)
					.Content()
					[
						SNew(STextBlock)
						.Text(FString(TEXT("Orientation")))
						.Font(DetailLayout->GetDetailFontBold())
					]
				]

				+SHorizontalBox::Slot()
				.Padding(2)
				.FillWidth(1)
				[
					SNew(SBox)
					.WidthOverride(250)
					.Content()
					[
						SAssignNew(ParentSpaceComboBoxes[ArrayIndex*2+1], SComboBox< TSharedPtr<FString> >)
						.OptionsSource(&ParentNodeOptions)
						.InitiallySelectedItem(ParentNodeOptions[ParentIndex_R])
						.OnSelectionChanged(this, &FRigDetails::OnParentSpaceSelectionChanged, ParentNameProp_R)
						.OnGenerateWidget(this, &FRigDetails::MakeItemWidget)
						.OnComboBoxOpening(this, &FRigDetails::OnComboBoxOopening, ParentNameProp_R, ArrayIndex, true)
						.HasDownArrow(true)
						[
							SNew(STextBlock)
							.Text(this, &FRigDetails::GetSelectedTextLabel, ParentNameProp_R)
						]
					]
				]
			]
		]
	];
}

void FRigDetails::OnParentSpaceSelectionChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo, TSharedRef<IPropertyHandle> ParentSpacePropertyHandle)
{
	if (SelectInfo == ESelectInfo::OnKeyPress || SelectInfo == ESelectInfo::OnMouseClick)
	{
		if(SelectedItem.IsValid())
		{
			ParentSpacePropertyHandle->SetValueFromFormattedString(*SelectedItem.Get());
		}
	}
}

FReply FRigDetails::OnSetAllToWorld()
{
	URig * Rig = Cast<URig>(ItemBeingEdited.Get());
	check(Rig);

	const FScopedTransaction Transaction(LOCTEXT("SetAllToWorld_Action", "Set All Transform Constraints to World"));
	Rig->Modify();
	Rig->SetAllConstraintsToWorld();
	DetailBuilderPtr->ForceRefreshDetails();

	return FReply::Handled();
}

FReply FRigDetails::OnSetAllToParent()
{
	URig * Rig = Cast<URig>(ItemBeingEdited.Get());
	check(Rig);

	const FScopedTransaction Transaction(LOCTEXT("SetAllToParent_Action", "Set All Transform Constraints to Parent"));
	Rig->Modify();
	Rig->SetAllConstraintsToParents();
	DetailBuilderPtr->ForceRefreshDetails();

	return FReply::Handled();
}

/** Called to create a widget for each string */
TSharedRef<SWidget> FRigDetails::MakeItemWidget(TSharedPtr<FString> StringItem)
{
	check(StringItem.IsValid());

	return SNew(STextBlock)
		.Text(*StringItem.Get());
}
/** Helper method to get the text for a given item in the combo box */
FString FRigDetails::GetSelectedTextLabel(TSharedRef<IPropertyHandle> ParentSpacePropertyHandle) const
{
	FString DisplayText;

	if (ParentSpacePropertyHandle->GetValueAsDisplayString(DisplayText) != FPropertyAccess::Fail)
	{
		return DisplayText;
	}

	return TEXT("Unknown");
}

void FRigDetails::OnComboBoxOopening(TSharedRef<IPropertyHandle> ParentSpacePropertyHandle, int32 ArrayIndex, bool bTranslation)
{
	FString PropertyValue = GetSelectedTextLabel(ParentSpacePropertyHandle);

	// now find exact data
	TArray<TSharedPtr<FString>> & ParentOptions = ParentSpaceOptionList[ArrayIndex];
	TSharedPtr<FString> SelectedItem;
	for (auto Option : ParentOptions)
	{
		if (*Option.Get() == PropertyValue)
		{
			SelectedItem = Option;
			break;
		}
	}

	int32 ComboBoxIndex = (bTranslation)? ArrayIndex*2 : ArrayIndex*2+1;
	TSharedPtr< SComboBox<TSharedPtr<FString>> > ComboBox = ParentSpaceComboBoxes[ComboBoxIndex];

	ComboBox->SetSelectedItem(SelectedItem);
}

void FRigDetails::OnAdvancedCheckBoxStateChanged(ESlateCheckBoxState::Type NewState, TSharedRef<IPropertyHandle> PropertyHandle)
{
	bool bValue = (NewState == ESlateCheckBoxState::Checked)? true : false;
	PropertyHandle->SetValue(bValue);
}

ESlateCheckBoxState::Type FRigDetails::AdvancedCheckBoxIsChecked(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	bool bValue = false;
	// multi value doesn't work in array, so i'm not handling multi value
	if (PropertyHandle->GetValue(bValue) != FPropertyAccess::Fail)
	{
		return (bValue)? ESlateCheckBoxState::Checked: ESlateCheckBoxState::Unchecked;
	}

	return ESlateCheckBoxState::Undetermined;
}
#undef LOCTEXT_NAMESPACE
