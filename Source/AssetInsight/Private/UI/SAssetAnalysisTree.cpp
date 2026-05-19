
#include "UI/SAssetAnalysisTree.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"

#include "Widgets/Layout/SBorder.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

void SAssetAnalysisTree::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(4.0f)
		[
			SAssignNew(TreeViewWidget, STreeView<TSharedPtr<FAssetTreeNode>>)
			.TreeItemsSource(&RootItems)
			.OnGenerateRow(this, &SAssetAnalysisTree::OnGenerateRow)
			.OnGetChildren(this, &SAssetAnalysisTree::OnGetChildren)
			.OnMouseButtonDoubleClick(this, &SAssetAnalysisTree::OnItemDoubleClicked)
			.SelectionMode(ESelectionMode::Single)
		]
	];
}

void SAssetAnalysisTree::SetTreeData(TSharedPtr<FAssetTreeNode> InRootNode)
{
	RootNode = InRootNode;
	RootItems.Reset();

	if (RootNode.IsValid())
	{
		RootItems.Add(RootNode);
	}

	if (TreeViewWidget.IsValid())
	{
		TreeViewWidget->RequestTreeRefresh();

		// 默认展开根节点
		if (RootNode.IsValid())
		{
			TreeViewWidget->SetItemExpansion(RootNode, true);
		}
	}
}

void SAssetAnalysisTree::ClearTree()
{
	RootNode.Reset();
	RootItems.Reset();

	if (TreeViewWidget.IsValid())
	{
		TreeViewWidget->RequestTreeRefresh();
	}
}

TSharedRef<ITableRow> SAssetAnalysisTree::OnGenerateRow(
	TSharedPtr<FAssetTreeNode> InItem,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	if (!InItem.IsValid())
	{
		return SNew(STableRow<TSharedPtr<FAssetTreeNode>>, OwnerTable)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Invalid Node")))
		];
	}

	TSharedRef<SHorizontalBox> RowContent = SNew(SHorizontalBox);

	// 主名称
	RowContent->AddSlot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(FText::FromString(InItem->DisplayName))
	];

	// Engine 标签
	if (InItem->PackageName.ToString().StartsWith(TEXT("/Engine")))
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[Engine]")))
			.ColorAndOpacity(FLinearColor(0.4f, 0.6f, 1.f))
		];
	}

	if (InItem->ReferencerCategory == EAssetReferencerCategory::Package)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[Package Ref]")))
			.ColorAndOpacity(FLinearColor(0.45f, 0.8f, 1.f))
		];
	}

	if (InItem->bIsManageDependency)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[Manage Dep]")))
			.ColorAndOpacity(FLinearColor(0.7f, 0.55f, 1.f))
		];
	}

	if (InItem->bIsSemanticDependency)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[SearchableName Dep]")))
			.ColorAndOpacity(FLinearColor(1.f, 0.55f, 0.9f))
		];
	}

	if (InItem->ReferencerCategory == EAssetReferencerCategory::Manage)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[Manage Ref]")))
			.ColorAndOpacity(FLinearColor(0.7f, 0.55f, 1.f))
		];
	}

	if (InItem->ReferencerCategory == EAssetReferencerCategory::Semantic)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[SearchableName Ref]")))
			.ColorAndOpacity(FLinearColor(1.f, 0.55f, 0.9f))
		];
	}

	if (InItem->ReferencerCategory == EAssetReferencerCategory::Root &&
		(InItem->NumPackageReferencers > 0 || InItem->NumManageReferencers > 0 || InItem->NumSemanticReferencers > 0))
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(8.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(
				TEXT("[Package:%d Manage:%d SearchableName:%d]"),
				InItem->NumPackageReferencers,
				InItem->NumManageReferencers,
				InItem->NumSemanticReferencers)))
			.ColorAndOpacity(FLinearColor(0.75f, 0.75f, 0.75f))
		];
	}

	if (InItem->bHard)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[Hard]")))
			.ColorAndOpacity(FLinearColor(1.f, 0.55f, 0.35f))
		];
	}

	if (InItem->bSoft)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[Soft]")))
			.ColorAndOpacity(FLinearColor(0.35f, 0.8f, 1.f))
		];
	}

	if (InItem->bGame)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[Game]")))
			.ColorAndOpacity(FLinearColor(0.45f, 1.f, 0.45f))
		];
	}

	if (InItem->bEditorOnly)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[EditorOnly]")))
			.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
		];
	}

	if (InItem->bBuild)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[Build]")))
			.ColorAndOpacity(FLinearColor(1.f, 0.9f, 0.35f))
		];
	}

	if (InItem->bHasManageReferences && InItem->ReferencerCategory != EAssetReferencerCategory::Manage)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[Manage Ref]")))
			.ColorAndOpacity(FLinearColor(0.7f, 0.55f, 1.f))
		];
	}

	if (InItem->bHasSemanticReferences && InItem->ReferencerCategory != EAssetReferencerCategory::Semantic)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[SearchableName Ref]")))
			.ColorAndOpacity(FLinearColor(1.f, 0.55f, 0.9f))
		];
	}

	// Cycle 标签
	if (InItem->bIsCycle)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[Cycle]")))
			.ColorAndOpacity(FLinearColor::Red)
		];
	}

	// DepthLimit 标签
	if (InItem->bIsDepthLimited)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[DepthLimit]")))
			.ColorAndOpacity(FLinearColor::Yellow)
		];
	}

	if (InItem->bAlreadyVisited)
	{
		RowContent->AddSlot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[Visited]")))
			.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
		];
	}

	return SNew(STableRow<TSharedPtr<FAssetTreeNode>>, OwnerTable)
	[
		RowContent
	];
}

void SAssetAnalysisTree::OnGetChildren(
	TSharedPtr<FAssetTreeNode> InItem,
	TArray<TSharedPtr<FAssetTreeNode>>& OutChildren) const
{
	if (!InItem.IsValid())
	{
		return;
	}

	OutChildren.Append(InItem->Children);
}

void SAssetAnalysisTree::OnItemDoubleClicked(TSharedPtr<FAssetTreeNode> InItem)
{
	if (!InItem.IsValid())
	{
		return;
	}

	// 尝试根据 PackageName 找到对应 AssetData
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	IAssetRegistry& Registry = AssetRegistryModule.Get();

	TArray<FAssetData> FoundAssets;
	Registry.GetAssetsByPackageName(InItem->PackageName, FoundAssets);

	if (FoundAssets.Num() == 0)
	{
		return;
	}

	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	IContentBrowserSingleton& ContentBrowser = ContentBrowserModule.Get();

	ContentBrowser.SyncBrowserToAssets(FoundAssets);
}
