
#include "UI/SAssetAnalysisSummary.h"

#include "Widgets/Layout/SBorder.h"

#include "Widgets/Text/STextBlock.h"

void SAssetAnalysisSummary::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(6.0f)
		[
			SNew(SVerticalBox)

			// Asset
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildRow(TEXT("Asset"), AssetText)
			]

			// Type
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildRow(TEXT("Type"), TypeText)
			]

			// Depth
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildRow(TEXT("Max Depth"), DepthText)
			]

			// Nodes
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot().FillWidth(0.5f)
				[
					BuildRow(TEXT("Nodes"), NodeText)
				]

				+ SHorizontalBox::Slot().FillWidth(0.5f)
				[
					BuildRow(TEXT("Unique"), UniqueText)
				]
			]

			// Cycle / DepthLimit
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot().FillWidth(0.5f)
				[
					BuildRow(TEXT("Cycle"), CycleText)
				]

				+ SHorizontalBox::Slot().FillWidth(0.5f)
				[
					BuildRow(TEXT("DepthLimit"), DepthLimitText)
				]
			]

			// Project / Engine
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot().FillWidth(0.5f)
				[
					BuildRow(TEXT("Project"), ProjectText)
				]

				+ SHorizontalBox::Slot().FillWidth(0.5f)
				[
					BuildRow(TEXT("Engine"), EngineText)
				]
			]
		]
	];
}

TSharedRef<SWidget> SAssetAnalysisSummary::BuildRow(
	const FString& Key,
	TSharedPtr<STextBlock>& OutValueWidget
)
{
	return SNew(SHorizontalBox)

	+ SHorizontalBox::Slot()
	.AutoWidth()
	.Padding(0, 0, 8, 0)
	[
		SNew(STextBlock)
		.Text(FText::FromString(Key + TEXT(":")))
	]

	+ SHorizontalBox::Slot()
	.FillWidth(1.0f)
	[
		SAssignNew(OutValueWidget, STextBlock)
		.Text(FText::FromString(TEXT("-")))
	];
}

void SAssetAnalysisSummary::SetSummary(const FAssetAnalysisSummary& InSummary)
{
	if (AssetText.IsValid())
	{
		AssetText->SetText(FText::FromName(InSummary.RootAsset));
	}

	if (TypeText.IsValid())
	{
		TypeText->SetText(FText::FromString(InSummary.TreeType));
	}

	if (DepthText.IsValid())
	{
		DepthText->SetText(FText::AsNumber(InSummary.MaxDepth));
	}

	if (NodeText.IsValid())
	{
		NodeText->SetText(FText::AsNumber(InSummary.ExpandedNodeCount));
	}

	if (UniqueText.IsValid())
	{
		UniqueText->SetText(FText::AsNumber(InSummary.UniqueNodeCount));
	}

	if (CycleText.IsValid())
	{
		CycleText->SetText(FText::AsNumber(InSummary.CycleCount));
	}

	if (DepthLimitText.IsValid())
	{
		DepthLimitText->SetText(
			FText::FromString(InSummary.bDepthLimited ? TEXT("Yes") : TEXT("No"))
		);
	}

	if (ProjectText.IsValid())
	{
		ProjectText->SetText(FText::AsNumber(InSummary.ProjectAssetCount));
	}

	if (EngineText.IsValid())
	{
		EngineText->SetText(FText::AsNumber(InSummary.EngineAssetCount));
	}
}

void SAssetAnalysisSummary::ClearSummary()
{
	if (AssetText.IsValid()) AssetText->SetText(FText::FromString(TEXT("-")));
	if (TypeText.IsValid()) TypeText->SetText(FText::FromString(TEXT("-")));
	if (DepthText.IsValid()) DepthText->SetText(FText::FromString(TEXT("-")));

	if (NodeText.IsValid()) NodeText->SetText(FText::FromString(TEXT("-")));
	if (UniqueText.IsValid()) UniqueText->SetText(FText::FromString(TEXT("-")));

	if (CycleText.IsValid()) CycleText->SetText(FText::FromString(TEXT("-")));
	if (DepthLimitText.IsValid()) DepthLimitText->SetText(FText::FromString(TEXT("-")));

	if (ProjectText.IsValid()) ProjectText->SetText(FText::FromString(TEXT("-")));
	if (EngineText.IsValid()) EngineText->SetText(FText::FromString(TEXT("-")));
}