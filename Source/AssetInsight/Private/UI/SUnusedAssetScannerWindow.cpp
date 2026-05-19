
#include "UI/SUnusedAssetScannerWindow.h"

#include "UnusedAssetScanner.h"

#include "ContentBrowserModule.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IContentBrowserSingleton.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Styling/CoreStyle.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SSpacer.h"

#include "UI/SRootAssetPickerDialog.h"

#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FString RootSourceToText(EAssetInsightRootSource Source)
	{
		switch (Source)
		{
		case EAssetInsightRootSource::DefaultMap:
			return TEXT("Default Map");
		case EAssetInsightRootSource::ManualPath:
			return TEXT("Manual Path");
		case EAssetInsightRootSource::AlwaysCookDirectory:
			return TEXT("Always Cook");
		case EAssetInsightRootSource::PrimaryAsset:
			return TEXT("Primary Asset");
		default:
			return TEXT("Unknown");
		}
	}

	FLinearColor GetRootTypeColor(EAssetInsightRootSource Source)
	{
		switch (Source)
		{
		case EAssetInsightRootSource::DefaultMap:
			return FLinearColor(0.25f, 0.55f, 1.0f);
		case EAssetInsightRootSource::ManualPath:
			return FLinearColor(0.25f, 0.9f, 0.45f);
		case EAssetInsightRootSource::AlwaysCookDirectory:
			return FLinearColor(1.0f, 0.55f, 0.15f);
		case EAssetInsightRootSource::PrimaryAsset:
			return FLinearColor(0.75f, 0.45f, 1.0f);
		default:
			return FLinearColor::White;
		}
	}

	FString GetRootDisplayName(const FAssetInsightRootAsset& Root)
	{
		const FString PackageString = Root.PackageName.ToString();
		const FString ShortName = FPackageName::GetShortName(PackageString);
		return ShortName.IsEmpty() ? PackageString : ShortName;
	}

	FLinearColor GetReachableColor(bool bReachable)
	{
		return bReachable ? FLinearColor::White : FLinearColor(1.0f, 0.25f, 0.25f);
	}
}

void SUnusedAssetScannerWindow::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(8.f)
		[
			SNew(SVerticalBox)

			// 顶部控制区
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				BuildTopPanel()
			]

			// 摘要区
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				BuildSummaryPanel()
			]

			// 列表区
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				BuildListPanel()
			]
		]
	];

	UpdateRootSummary();
	OnScanClicked();
}

TSharedRef<SWidget> SUnusedAssetScannerWindow::BuildTopPanel()
{
	return SNew(SVerticalBox)

	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(0.f, 0.f, 0.f, 6.f)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Scan Unused Assets")))
			.OnClicked(this, &SUnusedAssetScannerWindow::OnScanClicked)
		]
	]

	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(0.f, 4.f, 0.f, 0.f)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 12.f, 0.f)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return ScanOptions.bIncludeDefaultMap ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { ScanOptions.bIncludeDefaultMap = NewState == ECheckBoxState::Checked; UpdateRootSummary(); })
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Include Default Map")))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 12.f, 0.f)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return ScanOptions.bIncludeManualRoots ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { ScanOptions.bIncludeManualRoots = NewState == ECheckBoxState::Checked; UpdateRootSummary(); })
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Include Manual Root Paths")))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 12.f, 0.f)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return ScanOptions.bIncludeAlwaysCookDirectories ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { ScanOptions.bIncludeAlwaysCookDirectories = NewState == ECheckBoxState::Checked; UpdateRootSummary(); })
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Include Always Cook Directories")))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return ScanOptions.bIncludePrimaryAssets ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { ScanOptions.bIncludePrimaryAssets = NewState == ECheckBoxState::Checked; UpdateRootSummary(); })
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Include AssetManager Primary Assets")))
			]
		]
	]

	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(0.f, 4.f, 0.f, 0.f)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 12.f, 0.f)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return ScanOptions.bIncludeSoftPackageDependencies ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { ScanOptions.bIncludeSoftPackageDependencies = NewState == ECheckBoxState::Checked; })
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Include Soft Package Dependencies")))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 12.f, 0.f)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return ScanOptions.bIncludeManageReferencers ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { ScanOptions.bIncludeManageReferencers = NewState == ECheckBoxState::Checked; })
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Include Manage Referencers")))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return ScanOptions.bIncludeSearchableNameReferencers ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { ScanOptions.bIncludeSearchableNameReferencers = NewState == ECheckBoxState::Checked; })
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Include SearchableName Referencers")))
			]
		]
	]
	;
}

TSharedRef<SWidget> SUnusedAssetScannerWindow::BuildSummaryPanel()
{
	return SNew(SBorder)
	.Padding(8.f)
	.BorderBackgroundColor(FLinearColor(0.12f, 0.18f, 0.34f, 1.0f))
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Root Packages")))
			.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 14))
			.Justification(ETextJustify::Center)
			.ColorAndOpacity(FLinearColor(0.85f, 0.92f, 1.0f))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Current roots used as reachability entry points")))
				.ColorAndOpacity(FLinearColor(0.72f, 0.82f, 1.0f))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(10.f, 0.f, 0.f, 0.f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("+")))
				.OnClicked(this, &SUnusedAssetScannerWindow::OnOpenRootPickerClicked)
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 6.f, 0.f, 8.f)
		[
			SAssignNew(RootListView, SListView<TSharedPtr<FRootItem>>)
			.ListItemsSource(&RootItems)
			.OnGenerateRow(this, &SUnusedAssetScannerWindow::OnGenerateRootRow)
			.SelectionMode(ESelectionMode::Single)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(SummaryText, STextBlock)
			.Text(FText::FromString(TEXT("Scanned: - | Unused: -")))
		]
	];
}

TSharedRef<SWidget> SUnusedAssetScannerWindow::BuildListPanel()
{
	return SNew(SBorder)
	.Padding(8.f)
	.BorderBackgroundColor(FLinearColor(0.34f, 0.12f, 0.12f, 1.0f))
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Likely Unused Assets")))
			.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 14))
			.Justification(ETextJustify::Center)
			.ColorAndOpacity(FLinearColor(1.0f, 0.86f, 0.86f))
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(ListView, SListView<TSharedPtr<FUnusedAssetItem>>)
			.ListItemsSource(&Items)
			.OnGenerateRow(this, &SUnusedAssetScannerWindow::OnGenerateRow)
			.OnMouseButtonDoubleClick(this, &SUnusedAssetScannerWindow::OnItemDoubleClicked)
		]
	];
}

FReply SUnusedAssetScannerWindow::OnScanClicked()
{
	const FUnusedAssetScanResult Result = FUnusedAssetScanner::ScanProjectUnusedAssets(GetScanOptions());
	UpdateUI(Result);
	UpdateResultSummary(Result);
	return FReply::Handled();
}

FUnusedAssetScanOptions SUnusedAssetScannerWindow::GetScanOptions() const
{
	return ScanOptions;
}

FReply SUnusedAssetScannerWindow::OnOpenRootPickerClicked()
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString(TEXT("Add Root Assets")))
		.ClientSize(FVector2D(520.f, 360.f))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	Window->SetContent(
		SNew(SRootAssetPickerDialog)
		.TitleText(FText::FromString(TEXT("Select assets to add as manual roots")))
		.bAllowMultiple(true)
		.OnAssetsPicked(FOnRootAssetsPicked::CreateSP(this, &SUnusedAssetScannerWindow::OnAddRootAssetsPicked))
	);

	FSlateApplication::Get().AddWindow(Window);
	return FReply::Handled();
}

void SUnusedAssetScannerWindow::UpdateUI(const FUnusedAssetScanResult& InResult)
{
	Items.Reset();

	for (const FUnusedAssetItem& Item : InResult.Items)
	{
		Items.Add(MakeShared<FUnusedAssetItem>(Item));
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SUnusedAssetScannerWindow::OnGenerateRow(
	TSharedPtr<FUnusedAssetItem> InItem,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	if (!InItem.IsValid())
	{
		return SNew(STableRow<TSharedPtr<FUnusedAssetItem>>, OwnerTable)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Invalid Item")))
		];
	}

	const FString PackageString = InItem->PackageName.ToString();
	const FString DetailTooltip = FString::Printf(
		TEXT("Path: %s\nPackageRef: %d\nManageRef: %d\nSearchableNameRef: %d\nReachable: %s"),
		*PackageString,
		InItem->PackageReferencerCount,
		InItem->ManageReferencerCount,
		InItem->SearchableNameReferencerCount,
		InItem->bReachableByRoot ? TEXT("true") : TEXT("false"));

	return SNew(STableRow<TSharedPtr<FUnusedAssetItem>>, OwnerTable)
	.ToolTipText(FText::FromString(DetailTooltip))
	[
		SNew(SBorder)
		.Padding(6.f, 3.f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(InItem->DisplayName))
				.ColorAndOpacity(GetReachableColor(InItem->bReachableByRoot))
				.ToolTipText(FText::FromString(DetailTooltip))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(12.f, 0.f, 0.f, 0.f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Copy Path")))
				.OnClicked_Lambda([PackageString]()
				{
					FPlatformApplicationMisc::ClipboardCopy(*PackageString);
					return FReply::Handled();
				})
			]
		]
	];
}

void SUnusedAssetScannerWindow::OnItemDoubleClicked(TSharedPtr<FUnusedAssetItem> InItem)
{
	if (!InItem.IsValid())
	{
		return;
	}

	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	IContentBrowserSingleton& ContentBrowser = ContentBrowserModule.Get();

	TArray<FAssetData> AssetsToSync;
	AssetsToSync.Add(InItem->AssetData);

	ContentBrowser.SyncBrowserToAssets(AssetsToSync);
}

void SUnusedAssetScannerWindow::UpdateRootSummary()
{
	if (!RootListView.IsValid())
	{
		return;
	}

	TArray<FAssetInsightRootAsset> Roots;
	FUnusedAssetScanner::GetRootAssets(Roots, GetScanOptions());

	RootItems.Reset();
	for (const FAssetInsightRootAsset& Root : Roots)
	{
		TSharedPtr<FRootItem> Item = MakeShared<FRootItem>();
		Item->RootAsset = Root;
		RootItems.Add(Item);
	}

	RootListView->RequestListRefresh();
}

TSharedRef<ITableRow> SUnusedAssetScannerWindow::OnGenerateRootRow(
	TSharedPtr<FRootItem> InItem,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	if (!InItem.IsValid())
	{
		return SNew(STableRow<TSharedPtr<FRootItem>>, OwnerTable)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Invalid Root")))
		];
	}

	const FString RootPath = InItem->RootAsset.PackageName.ToString();
	const FString SourceText = RootSourceToText(InItem->RootAsset.Source);
	const FString DetailText = InItem->RootAsset.SourceDetail.IsEmpty() ? RootPath : InItem->RootAsset.SourceDetail;

	return SNew(STableRow<TSharedPtr<FRootItem>>, OwnerTable)
	.ToolTipText(FText::FromString(DetailText))
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 2.f, 12.f, 2.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(GetRootDisplayName(InItem->RootAsset)))
			.ToolTipText(FText::FromString(RootPath))
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 2.f, 12.f, 2.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("[%s]"), *SourceText)))
			.ColorAndOpacity(GetRootTypeColor(InItem->RootAsset.Source))
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(0.f, 2.f, 0.f, 2.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(DetailText))
			.ToolTipText(FText::FromString(DetailText))
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(12.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("x")))
			.Visibility_Lambda([InItem]()
			{
				return (InItem.IsValid() && InItem->RootAsset.Source == EAssetInsightRootSource::ManualPath)
					? EVisibility::Visible
					: EVisibility::Collapsed;
			})
			.OnClicked(this, &SUnusedAssetScannerWindow::OnRemoveRootClicked, InItem)
		]
	];
}

FReply SUnusedAssetScannerWindow::OnRemoveRootClicked(TSharedPtr<FRootItem> RootItem)
{
	if (!RootItem.IsValid())
	{
		return FReply::Handled();
	}

	if (RootItem->RootAsset.Source != EAssetInsightRootSource::ManualPath)
	{
		return FReply::Handled();
	}

	OpenRemoveConfirm(MakeShared<FName>(RootItem->RootAsset.PackageName));
	return FReply::Handled();
}

void SUnusedAssetScannerWindow::OnAddRootAssetsPicked(const TArray<FAssetData>& Assets)
{
	if (Assets.Num() == 0)
	{
		return;
	}

	TArray<FString> ExistingRoots;
	if (GConfig)
	{
		TArray<FString> TempRoots;
		GConfig->GetArray(TEXT("AssetInsight"), TEXT("ManualRootPackages"), TempRoots, GEditorPerProjectIni);
		ExistingRoots.Append(TempRoots);
		TempRoots.Reset();
		GConfig->GetArray(TEXT("AssetInsight"), TEXT("ManualRootPackages"), TempRoots, GGameIni);
		ExistingRoots.Append(TempRoots);
	}

	TSet<FString> RootSet(ExistingRoots);
	for (const FAssetData& Asset : Assets)
	{
		if (Asset.IsValid())
		{
			RootSet.Add(Asset.PackageName.ToString());
		}
	}

	TArray<FString> Roots = RootSet.Array();
	Roots.Sort();

	SaveManualRoots(Roots);
	UpdateRootSummary();
}

void SUnusedAssetScannerWindow::OpenRemoveConfirm(TSharedPtr<FName> RootName)
{
	PendingRemoveRoot = RootName;

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString(TEXT("Confirm Remove Root")))
		.ClientSize(FVector2D(360.f, 120.f))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	Window->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.f, 8.f, 8.f, 4.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Remove this root package?")))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.f, 0.f, 8.f, 6.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(RootName.IsValid() ? RootName->ToString() : TEXT("-")))
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSpacer)
			.Size(FVector2D(1.f, 1.f))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.f, 0.f, 8.f, 8.f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Confirm")))
				.OnClicked(this, &SUnusedAssetScannerWindow::OnConfirmRemoveRootClicked)
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SSpacer)
				.Size(FVector2D(1.f, 1.f))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Cancel")))
				.OnClicked(this, &SUnusedAssetScannerWindow::OnCancelRemoveRootClicked)
			]
		]
	);

	PendingRemoveWindow = Window;
	FSlateApplication::Get().AddWindow(Window);
}

FReply SUnusedAssetScannerWindow::OnConfirmRemoveRootClicked()
{
	if (!PendingRemoveRoot.IsValid() || !GConfig)
	{
		return FReply::Handled();
	}

	TArray<FString> ExistingRoots;
	{
		TArray<FString> TempRoots;
		GConfig->GetArray(TEXT("AssetInsight"), TEXT("ManualRootPackages"), TempRoots, GEditorPerProjectIni);
		ExistingRoots.Append(TempRoots);
		TempRoots.Reset();
		GConfig->GetArray(TEXT("AssetInsight"), TEXT("ManualRootPackages"), TempRoots, GGameIni);
		ExistingRoots.Append(TempRoots);
	}
	TSet<FString> RootSet(ExistingRoots);
	if (RootSet.Contains(PendingRemoveRoot->ToString()))
	{
		RootSet.Remove(PendingRemoveRoot->ToString());
	}

	TArray<FString> Roots = RootSet.Array();
	Roots.Sort();
	SaveManualRoots(Roots);
	UpdateRootSummary();

	PendingRemoveRoot.Reset();
	if (PendingRemoveWindow.IsValid())
	{
		PendingRemoveWindow.Pin()->RequestDestroyWindow();
	}
	PendingRemoveWindow.Reset();
	return FReply::Handled();
}

FReply SUnusedAssetScannerWindow::OnCancelRemoveRootClicked()
{
	PendingRemoveRoot.Reset();
	if (PendingRemoveWindow.IsValid())
	{
		PendingRemoveWindow.Pin()->RequestDestroyWindow();
	}
	PendingRemoveWindow.Reset();
	return FReply::Handled();
}

void SUnusedAssetScannerWindow::UpdateResultSummary(const FUnusedAssetScanResult& InResult)
{
	if (!SummaryText.IsValid())
	{
		return;
	}

	const FString SummaryStr = FString::Printf(
		TEXT("Root Assets: Default Map %d | Manual Paths %d | Always Cook %d | Primary Assets %d\nReachable Assets: %d | Scanned: %d | Likely Unused Assets: %d"),
		InResult.DefaultMapRootCount,
		InResult.ManualRootCount,
		InResult.AlwaysCookRootCount,
		InResult.PrimaryAssetRootCount,
		InResult.ReachableAssetCount,
		InResult.TotalScannedAssetCount,
		InResult.UnusedAssetCount
	);
	SummaryText->SetText(FText::FromString(SummaryStr));
}

void SUnusedAssetScannerWindow::SaveManualRoots(const TArray<FString>& Roots)
{
	if (!GConfig)
	{
		return;
	}

	GConfig->SetArray(TEXT("AssetInsight"), TEXT("ManualRootPackages"), Roots, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}
