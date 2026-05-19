#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Analysis/AssetAnalysisTypes.h"

class SAssetAnalysisSummary;
class SAssetAnalysisTree;
class SEditableTextBox;
struct FAssetData;

/**
 * SAssetInsightWindow
 *
 * 资源分析主窗口（Controller）
 *
 * 职责：
 * 1. 接收外部输入（资产 + 分析模式）
 * 2. 调用分析器生成结果
 * 3. 将结果分发给 Summary 和 Tree 子控件
 * 4. 管理窗口整体布局
 */
class SAssetInsightWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAssetInsightWindow) {}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	/** 外部入口：分析指定资产 */
	void AnalyzeAsset(const FAssetData& InAsset, EAssetInsightAnalysisMode InMode);

private:
	/** 构建顶部控制区 */
	TSharedRef<class SWidget> BuildTopControlPanel();

	/** 构建主内容区 */
	TSharedRef<class SWidget> BuildMainContent();

	/** 执行依赖树分析 */
	void ApplyDependencyResult(const FAssetData& InAsset);

	/** 执行引用树分析 */
	void ApplyReferencerResult(const FAssetData& InAsset);

	/** 获取最大深度 */
	int32 GetMaxDepth() const;

	/** 清空结果 */
	FReply OnClearClicked();
	FReply OnExportJsonClicked();
	FReply OnExportMarkdownClicked();
	FReply OnExportCsvClicked();
	void ExportCurrentTree(const FString& Extension) const;
	FAssetDependencyTreeOptions GetDependencyTreeOptions() const;
	FAssetReferencerTreeOptions GetReferencerTreeOptions() const;

private:
	/** 摘要控件 */
	TSharedPtr<SAssetAnalysisSummary> SummaryWidget;

	/** 树控件 */
	TSharedPtr<SAssetAnalysisTree> TreeWidget;

	/** 最大深度输入框 */
	TSharedPtr<SEditableTextBox> MaxDepthTextBox;
	TSharedPtr<FAssetTreeNode> CurrentTreeRoot;
	FString CurrentExportBaseName = TEXT("AssetInsightTree");
	FAssetDependencyTreeOptions DependencyTreeOptions;
	FAssetReferencerTreeOptions ReferencerTreeOptions;
};
