#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Analysis/AssetAnalysisTypes.h"

/**
 * SAssetAnalysisSummary
 *
 * 显示分析结果摘要
 *
 * 职责：
 * - 接收 FAssetAnalysisSummary
 * - 结构化展示关键统计信息
 */
class SAssetAnalysisSummary : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAssetAnalysisSummary) {}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	/** 设置摘要 */
	void SetSummary(const FAssetAnalysisSummary& InSummary);

	/** 清空 */
	void ClearSummary();

private:

	/** 构建一行 Key-Value */
	TSharedRef<class SWidget> BuildRow(
		const FString& Key,
		TSharedPtr<class STextBlock>& OutValueWidget
	);

private:

	/** 各字段 UI */
	TSharedPtr<class STextBlock> AssetText;
	TSharedPtr<class STextBlock> TypeText;
	TSharedPtr<class STextBlock> DepthText;

	TSharedPtr<class STextBlock> NodeText;
	TSharedPtr<class STextBlock> UniqueText;

	TSharedPtr<class STextBlock> CycleText;
	TSharedPtr<class STextBlock> DepthLimitText;

	TSharedPtr<class STextBlock> ProjectText;
	TSharedPtr<class STextBlock> EngineText;
};