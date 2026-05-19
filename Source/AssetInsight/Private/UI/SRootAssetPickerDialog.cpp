#include "UI/SRootAssetPickerDialog.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

void SRootAssetPickerDialog::Construct(const FArguments& InArgs)
{
	TitleText = InArgs._TitleText;
	bAllowMultiple = InArgs._bAllowMultiple;
	OnAssetsPicked = InArgs._OnAssetsPicked;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(10.f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(TitleText)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 10.f, 0.f, 6.f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Choose Assets...")))
				.OnClicked(this, &SRootAssetPickerDialog::OnChooseAssetsClicked)
			]
		]
	];

	ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());

	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SRootAssetPickerDialog::OnAutoOpenPicker));
}

FReply SRootAssetPickerDialog::OnChooseAssetsClicked()
{
	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FOpenAssetDialogConfig DialogConfig;
	DialogConfig.DialogTitleOverride = TitleText;
	DialogConfig.DefaultPath = TEXT("/Game");
	DialogConfig.bAllowMultipleSelection = bAllowMultiple;

	SelectedAssets = ContentBrowserModule.Get().CreateModalOpenAssetDialog(DialogConfig);

	if (SelectedAssets.Num() == 0)
	{
		if (ParentWindow.IsValid())
		{
			ParentWindow.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	if (OnAssetsPicked.IsBound())
	{
		OnAssetsPicked.Execute(SelectedAssets);
	}

	if (ParentWindow.IsValid())
	{
		ParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

EActiveTimerReturnType SRootAssetPickerDialog::OnAutoOpenPicker(double, float)
{
	OnChooseAssetsClicked();
	return EActiveTimerReturnType::Stop;
}
