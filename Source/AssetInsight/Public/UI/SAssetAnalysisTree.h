#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "Analysis/AssetAnalysisTypes.h"

/**
 * SAssetAnalysisTree
 *
 * 资源分析树视图（View）
 *
 * 职责：
 * 1. 接收分析层生成的 FAssetTreeNode 树结构
 * 2. 使用 STreeView 展示依赖树 / 引用树
 * 3. 显示节点标签（Engine / Cycle / DepthLimit）
 * 4. 支持双击节点后同步到 Content Browser
 *
 * 不负责：
 * - 不负责生成树
 * - 不负责分析逻辑
 * - 不负责统计摘要
 */
class SAssetAnalysisTree : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAssetAnalysisTree) {}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	/** 设置整棵树的数据 */
	void SetTreeData(TSharedPtr<FAssetTreeNode> InRootNode);

	/** 清空树数据 */
	void ClearTree();

private:
	/** 生成单行显示 */
	TSharedRef<ITableRow> OnGenerateRow(
		TSharedPtr<FAssetTreeNode> InItem,
		const TSharedRef<STableViewBase>& OwnerTable);

	/** 获取子节点 */
	void OnGetChildren(
		TSharedPtr<FAssetTreeNode> InItem,
		TArray<TSharedPtr<FAssetTreeNode>>& OutChildren) const;

	/** 双击节点时跳转到 Content Browser */
	void OnItemDoubleClicked(TSharedPtr<FAssetTreeNode> InItem);

private:
	/** TreeView 根节点数组 */
	TArray<TSharedPtr<FAssetTreeNode>> RootItems;

	/** 当前根节点 */
	TSharedPtr<FAssetTreeNode> RootNode;

	/** TreeView 控件 */
	TSharedPtr<STreeView<TSharedPtr<FAssetTreeNode>>> TreeViewWidget;
};