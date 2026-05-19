#include "UI/SAssetInsightWindow.h"
#include "AssetInsight.h"
#include "AssetRegistry/AssetData.h"
#include "UI/SAssetAnalysisSummary.h"
#include "UI/SAssetAnalysisTree.h"
#include "UI/Analysis/DependencyAnalyzer.h"
#include "UI/Analysis/ReferencerAnalyzer.h"
#include "UI/Analysis/AssetAnalysisTypes.h"

#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"

#include "Widgets/Text/STextBlock.h"

namespace
{
	FString JsonEscape(const FString& Value)
	{
		FString Escaped = Value.Replace(TEXT("\\"), TEXT("\\\\"));
		Escaped = Escaped.Replace(TEXT("\""), TEXT("\\\""));
		return Escaped;
	}

	FString NodeTags(const FAssetTreeNode& Node)
	{
		TArray<FString> Tags;
		if (Node.ReferencerCategory == EAssetReferencerCategory::Package) Tags.Add(TEXT("Package Ref"));
		if (Node.ReferencerCategory == EAssetReferencerCategory::Manage) Tags.Add(TEXT("Manage Ref"));
		if (Node.ReferencerCategory == EAssetReferencerCategory::Semantic) Tags.Add(TEXT("SearchableName Ref"));
		if (Node.bHard) Tags.Add(TEXT("Hard"));
		if (Node.bSoft) Tags.Add(TEXT("Soft"));
		if (Node.bGame) Tags.Add(TEXT("Game"));
		if (Node.bEditorOnly) Tags.Add(TEXT("EditorOnly"));
		if (Node.bBuild) Tags.Add(TEXT("Build"));
		if (Node.bHasManageReferences) Tags.Add(TEXT("Manage Ref"));
		if (Node.bHasSemanticReferences) Tags.Add(TEXT("SearchableName Ref"));
		return FString::Join(Tags, TEXT(","));
	}

	FString ReferencerCategoryToString(EAssetReferencerCategory Category)
	{
		switch (Category)
		{
		case EAssetReferencerCategory::Package:
			return TEXT("Package");
		case EAssetReferencerCategory::Manage:
			return TEXT("Manage");
		case EAssetReferencerCategory::Semantic:
			return TEXT("SearchableName");
		default:
			return TEXT("Root");
		}
	}

	void AppendJsonNode(const TSharedPtr<FAssetTreeNode>& Node, FString& Out, int32 Indent)
	{
		if (!Node.IsValid())
		{
			return;
		}

		const FString Pad = FString::ChrN(Indent, TEXT('\t'));
		const FString ChildPad = FString::ChrN(Indent + 1, TEXT('\t'));
		Out += Pad + TEXT("{\n");
		Out += ChildPad + FString::Printf(TEXT("\"PackageName\": \"%s\",\n"), *JsonEscape(Node->PackageName.ToString()));
		Out += ChildPad + FString::Printf(TEXT("\"bHard\": %s,\n"), Node->bHard ? TEXT("true") : TEXT("false"));
		Out += ChildPad + FString::Printf(TEXT("\"bSoft\": %s,\n"), Node->bSoft ? TEXT("true") : TEXT("false"));
		Out += ChildPad + FString::Printf(TEXT("\"bGame\": %s,\n"), Node->bGame ? TEXT("true") : TEXT("false"));
		Out += ChildPad + FString::Printf(TEXT("\"bEditorOnly\": %s,\n"), Node->bEditorOnly ? TEXT("true") : TEXT("false"));
		Out += ChildPad + FString::Printf(TEXT("\"bBuild\": %s,\n"), Node->bBuild ? TEXT("true") : TEXT("false"));
		Out += ChildPad + FString::Printf(TEXT("\"bHasManageReferences\": %s,\n"), Node->bHasManageReferences ? TEXT("true") : TEXT("false"));
		Out += ChildPad + FString::Printf(TEXT("\"bHasSemanticReferences\": %s,\n"), Node->bHasSemanticReferences ? TEXT("true") : TEXT("false"));
		Out += ChildPad + TEXT("\"Dependencies\": [\n");
		for (int32 Index = 0; Index < Node->Children.Num(); ++Index)
		{
			AppendJsonNode(Node->Children[Index], Out, Indent + 2);
			if (Index + 1 < Node->Children.Num())
			{
				Out += TEXT(",");
			}
			Out += TEXT("\n");
		}
		Out += ChildPad + TEXT("]\n");
		Out += Pad + TEXT("}");
	}

	void AppendReferencerJsonNode(const TSharedPtr<FAssetTreeNode>& Node, FString& Out, int32 Indent)
	{
		if (!Node.IsValid())
		{
			return;
		}

		const FString Pad = FString::ChrN(Indent, TEXT('\t'));
		const FString ChildPad = FString::ChrN(Indent + 1, TEXT('\t'));
		Out += Pad + TEXT("{\n");
		Out += ChildPad + FString::Printf(TEXT("\"AssetPackageName\": \"%s\",\n"), *JsonEscape(Node->PackageName.ToString()));
		Out += ChildPad + FString::Printf(TEXT("\"NumPackageReferencers\": %d,\n"), Node->NumPackageReferencers);
		Out += ChildPad + FString::Printf(TEXT("\"NumManageReferencers\": %d,\n"), Node->NumManageReferencers);
		Out += ChildPad + FString::Printf(TEXT("\"NumSemanticReferencers\": %d,\n"), Node->NumSemanticReferencers);
		Out += ChildPad + TEXT("\"Referencers\": [\n");
		for (int32 Index = 0; Index < Node->Children.Num(); ++Index)
		{
			const TSharedPtr<FAssetTreeNode>& Child = Node->Children[Index];
			if (!Child.IsValid())
			{
				continue;
			}

			const FString ItemPad = FString::ChrN(Indent + 2, TEXT('\t'));
			const FString FieldPad = FString::ChrN(Indent + 3, TEXT('\t'));
			Out += ItemPad + TEXT("{\n");
			Out += FieldPad + FString::Printf(TEXT("\"Category\": \"%s\",\n"), *ReferencerCategoryToString(Child->ReferencerCategory));
			Out += FieldPad + FString::Printf(TEXT("\"ReferencerName\": \"%s\",\n"), *JsonEscape(Child->PackageName.ToString()));
			Out += FieldPad + FString::Printf(TEXT("\"bHard\": %s,\n"), Child->bHard ? TEXT("true") : TEXT("false"));
			Out += FieldPad + FString::Printf(TEXT("\"bSoft\": %s,\n"), Child->bSoft ? TEXT("true") : TEXT("false"));
			Out += FieldPad + FString::Printf(TEXT("\"bGame\": %s,\n"), Child->bGame ? TEXT("true") : TEXT("false"));
			Out += FieldPad + FString::Printf(TEXT("\"bEditorOnly\": %s,\n"), Child->bEditorOnly ? TEXT("true") : TEXT("false"));
			Out += FieldPad + FString::Printf(TEXT("\"bBuild\": %s,\n"), Child->bBuild ? TEXT("true") : TEXT("false"));
			Out += FieldPad + TEXT("\"Referencers\": [\n");
			for (int32 ChildIndex = 0; ChildIndex < Child->Children.Num(); ++ChildIndex)
			{
				AppendReferencerJsonNode(Child->Children[ChildIndex], Out, Indent + 4);
				if (ChildIndex + 1 < Child->Children.Num())
				{
					Out += TEXT(",");
				}
				Out += TEXT("\n");
			}
			Out += FieldPad + TEXT("]\n");
			Out += ItemPad + TEXT("}");
			if (Index + 1 < Node->Children.Num())
			{
				Out += TEXT(",");
			}
			Out += TEXT("\n");
		}
		Out += ChildPad + TEXT("]\n");
		Out += Pad + TEXT("}");
	}

	void AppendMarkdownNode(const TSharedPtr<FAssetTreeNode>& Node, FString& Out, int32 Depth)
	{
		if (!Node.IsValid())
		{
			return;
		}

		const FString Indent = FString::ChrN(Depth * 2, TEXT(' '));
		const FString Tags = NodeTags(*Node);
		Out += FString::Printf(TEXT("%s- %s%s\n"), *Indent, *Node->PackageName.ToString(), Tags.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" [%s]"), *Tags));
		for (const TSharedPtr<FAssetTreeNode>& Child : Node->Children)
		{
			AppendMarkdownNode(Child, Out, Depth + 1);
		}
	}

	void AppendCsvRows(const TSharedPtr<FAssetTreeNode>& Node, FString& Out)
	{
		if (!Node.IsValid())
		{
			return;
		}

		for (const TSharedPtr<FAssetTreeNode>& Child : Node->Children)
		{
			if (Child.IsValid())
			{
				Out += FString::Printf(
					TEXT("%s,%s,%d,%d,%d,%d,%d,%d,%d\n"),
					*Node->PackageName.ToString(),
					*Child->PackageName.ToString(),
					Child->bHard ? 1 : 0,
					Child->bSoft ? 1 : 0,
					Child->bGame ? 1 : 0,
					Child->bEditorOnly ? 1 : 0,
					Child->bBuild ? 1 : 0,
					Child->bHasManageReferences ? 1 : 0,
					Child->bHasSemanticReferences ? 1 : 0);
				AppendCsvRows(Child, Out);
			}
		}
	}

	void AppendReferencerCsvRows(const TSharedPtr<FAssetTreeNode>& Root, FString& Out)
	{
		if (!Root.IsValid())
		{
			return;
		}

		for (const TSharedPtr<FAssetTreeNode>& Child : Root->Children)
		{
			if (!Child.IsValid())
			{
				continue;
			}

			Out += FString::Printf(
				TEXT("%s,%s,%s,%d,%d,%d,%d,%d\n"),
				*Root->PackageName.ToString(),
				*ReferencerCategoryToString(Child->ReferencerCategory),
				*Child->PackageName.ToString(),
				Child->bHard ? 1 : 0,
				Child->bSoft ? 1 : 0,
				Child->bGame ? 1 : 0,
				Child->bEditorOnly ? 1 : 0,
				Child->bBuild ? 1 : 0);
			AppendReferencerCsvRows(Child, Out);
		}
	}
}

void SAssetInsightWindow::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(8.0f)
		[
			SNew(SVerticalBox)

			// 顶部控制区
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				BuildTopControlPanel()
			]

			// 主内容区
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				BuildMainContent()
			]
		]
	];
}

void SAssetInsightWindow::AnalyzeAsset(const FAssetData& InAsset, EAssetInsightAnalysisMode InMode)
{
	if (!InAsset.IsValid())
	{
		return;
	}

	switch (InMode)
	{
	case EAssetInsightAnalysisMode::Dependency:
		ApplyDependencyResult(InAsset);
		break;

	case EAssetInsightAnalysisMode::Referencer:
		ApplyReferencerResult(InAsset);
		break;

	default:
		break;
	}
}

TSharedRef<SWidget> SAssetInsightWindow::BuildTopControlPanel()
{
	return SNew(SBorder)
		.Padding(6.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Max Depth")))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 12.f, 0.f)
			[
				SAssignNew(MaxDepthTextBox, SEditableTextBox)
				.MinDesiredWidth(80.f)
				.Text(FText::FromString(TEXT("5")))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 12.f, 0.f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Clear")))
				.OnClicked(this, &SAssetInsightWindow::OnClearClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return DependencyTreeOptions.bIncludeSoftPackageDependencies ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					const bool bChecked = NewState == ECheckBoxState::Checked;
					DependencyTreeOptions.bIncludeSoftPackageDependencies = bChecked;
					ReferencerTreeOptions.bIncludeSoftPackageReferencers = bChecked;
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Soft")))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return ReferencerTreeOptions.bShowPackageReferencers ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { ReferencerTreeOptions.bShowPackageReferencers = NewState == ECheckBoxState::Checked; })
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Package")))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return DependencyTreeOptions.bShowEditorOnly ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { DependencyTreeOptions.bShowEditorOnly = NewState == ECheckBoxState::Checked; })
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("EditorOnly")))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return DependencyTreeOptions.bShowBuild ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { DependencyTreeOptions.bShowBuild = NewState == ECheckBoxState::Checked; })
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Build")))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return DependencyTreeOptions.bShowManageHints ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					const bool bChecked = NewState == ECheckBoxState::Checked;
					DependencyTreeOptions.bShowManageHints = bChecked;
					ReferencerTreeOptions.bShowManageReferencers = bChecked;
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Manage")))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 12.f, 0.f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return DependencyTreeOptions.bShowSemanticHints ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					const bool bChecked = NewState == ECheckBoxState::Checked;
					DependencyTreeOptions.bShowSemanticHints = bChecked;
					ReferencerTreeOptions.bShowSemanticReferencers = bChecked;
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("SearchableName")))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("JSON")))
				.OnClicked(this, &SAssetInsightWindow::OnExportJsonClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("MD")))
				.OnClicked(this, &SAssetInsightWindow::OnExportMarkdownClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("CSV")))
				.OnClicked(this, &SAssetInsightWindow::OnExportCsvClicked)
			]
		];
}

TSharedRef<SWidget> SAssetInsightWindow::BuildMainContent()
{
	return SNew(SSplitter)

		+ SSplitter::Slot()
		.Value(0.30f)
		[
			SAssignNew(SummaryWidget, SAssetAnalysisSummary)
		]

		+ SSplitter::Slot()
		.Value(0.70f)
		[
			SAssignNew(TreeWidget, SAssetAnalysisTree)
		];
}

void SAssetInsightWindow::ApplyDependencyResult(const FAssetData& InAsset)
{
	const int32 MaxDepth = GetMaxDepth();

	const FAssetAnalysisResult Result =
		FDependencyAnalyzer::BuildDependencyTree(InAsset, MaxDepth, GetDependencyTreeOptions());

	if (!Result.RootNode.IsValid())
	{
		CurrentTreeRoot.Reset();
		CurrentExportBaseName = TEXT("AssetInsightTree");

		if (SummaryWidget.IsValid())
		{
			SummaryWidget->ClearSummary();
		}

		if (TreeWidget.IsValid())
		{
			TreeWidget->ClearTree();
		}

		return;
	}

	if (SummaryWidget.IsValid())
	{
		SummaryWidget->SetSummary(Result.Summary);
	}

	if (TreeWidget.IsValid())
	{
		TreeWidget->SetTreeData(Result.RootNode);
	}

	CurrentTreeRoot = Result.RootNode;
	CurrentExportBaseName = TEXT("DependencyTree");
}

void SAssetInsightWindow::ApplyReferencerResult(const FAssetData& InAsset)
{
	const int32 MaxDepth = GetMaxDepth();

	const FAssetAnalysisResult Result =
		FReferencerAnalyzer::BuildReferencerTree(InAsset, MaxDepth, GetReferencerTreeOptions());

	if (!Result.RootNode.IsValid())
	{
		CurrentTreeRoot.Reset();
		CurrentExportBaseName = TEXT("AssetInsightTree");

		if (SummaryWidget.IsValid())
		{
			SummaryWidget->ClearSummary();
		}

		if (TreeWidget.IsValid())
		{
			TreeWidget->ClearTree();
		}

		return;
	}

	if (SummaryWidget.IsValid())
	{
		SummaryWidget->SetSummary(Result.Summary);
	}

	if (TreeWidget.IsValid())
	{
		TreeWidget->SetTreeData(Result.RootNode);
	}

	CurrentTreeRoot = Result.RootNode;
	CurrentExportBaseName = TEXT("ReferencerTree");
}

int32 SAssetInsightWindow::GetMaxDepth() const
{
	if (!MaxDepthTextBox.IsValid())
	{
		return 5;
	}

	const FString Text = MaxDepthTextBox->GetText().ToString();

	int32 ParsedDepth = FCString::Atoi(*Text);
	if (ParsedDepth <= 0)
	{
		ParsedDepth = 5;
	}

	return ParsedDepth;
}

FReply SAssetInsightWindow::OnClearClicked()
{
	if (SummaryWidget.IsValid())
	{
		SummaryWidget->ClearSummary();
	}

	if (TreeWidget.IsValid())
	{
		TreeWidget->ClearTree();
	}

	CurrentTreeRoot.Reset();
	CurrentExportBaseName = TEXT("AssetInsightTree");

	return FReply::Handled();
}

FAssetDependencyTreeOptions SAssetInsightWindow::GetDependencyTreeOptions() const
{
	return DependencyTreeOptions;
}

FAssetReferencerTreeOptions SAssetInsightWindow::GetReferencerTreeOptions() const
{
	return ReferencerTreeOptions;
}

FReply SAssetInsightWindow::OnExportJsonClicked()
{
	ExportCurrentTree(TEXT("json"));
	return FReply::Handled();
}

FReply SAssetInsightWindow::OnExportMarkdownClicked()
{
	ExportCurrentTree(TEXT("md"));
	return FReply::Handled();
}

FReply SAssetInsightWindow::OnExportCsvClicked()
{
	ExportCurrentTree(TEXT("csv"));
	return FReply::Handled();
}

void SAssetInsightWindow::ExportCurrentTree(const FString& Extension) const
{
	if (!CurrentTreeRoot.IsValid())
	{
		return;
	}

	const FString ExportDir = FPaths::ProjectSavedDir() / TEXT("AssetInsight");
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*ExportDir);

	const FString FilePath = ExportDir / FString::Printf(TEXT("%s.%s"), *CurrentExportBaseName, *Extension);
	FString Content;

	if (Extension == TEXT("json"))
	{
		if (CurrentExportBaseName == TEXT("ReferencerTree"))
		{
			AppendReferencerJsonNode(CurrentTreeRoot, Content, 0);
		}
		else
		{
			AppendJsonNode(CurrentTreeRoot, Content, 0);
		}
		Content += TEXT("\n");
	}
	else if (Extension == TEXT("md"))
	{
		AppendMarkdownNode(CurrentTreeRoot, Content, 0);
	}
	else if (Extension == TEXT("csv"))
	{
		if (CurrentExportBaseName == TEXT("ReferencerTree"))
		{
			Content = TEXT("AssetPackageName,Category,ReferencerName,Hard,Soft,Game,EditorOnly,Build\n");
			AppendReferencerCsvRows(CurrentTreeRoot, Content);
		}
		else
		{
			Content = TEXT("Parent,Child,Hard,Soft,Game,EditorOnly,Build,Manage,SearchableName\n");
			AppendCsvRows(CurrentTreeRoot, Content);
		}
	}

	FFileHelper::SaveStringToFile(Content, *FilePath);
}
