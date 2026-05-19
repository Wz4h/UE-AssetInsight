#pragma once

#include "CoreMinimal.h"
#include "UnusedAssetScanner.h"
#include "Widgets/SCompoundWidget.h"

struct FUnusedAssetItem;
struct FUnusedAssetScanResult;

/**
 * 未引用资源扫描窗口
 * 作用：
 * 1. 提供 Scan 按钮触发扫描
 * 2. 调用 FUnusedAssetScanner 获取结果
 * 3. 显示摘要信息
 * 4. 显示未引用资源列表
 * 5. 支持双击跳转到 Content Browser
 */
class SUnusedAssetScannerWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUnusedAssetScannerWindow) {}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

private:
	struct FRootItem
	{
		FAssetInsightRootAsset RootAsset;
	};

	/** 构建顶部控制区 */
	TSharedRef<class SWidget> BuildTopPanel();

	/** 构建摘要区 */
	TSharedRef<class SWidget> BuildSummaryPanel();

	/** 构建列表区 */
	TSharedRef<class SWidget> BuildListPanel();

	/** 点击扫描按钮 */
	FReply OnScanClicked();

	/** 打开根资产选择窗口 */
	FReply OnOpenRootPickerClicked();
	FReply OnRemoveRootClicked(TSharedPtr<FRootItem> RootItem);
	FReply OnConfirmRemoveRootClicked();
	FReply OnCancelRemoveRootClicked();
	void OpenRemoveConfirm(TSharedPtr<FName> RootName);
	FUnusedAssetScanOptions GetScanOptions() const;

	/** 更新 UI */
	void UpdateUI(const FUnusedAssetScanResult& InResult);
	void UpdateRootSummary();
	void UpdateResultSummary(const FUnusedAssetScanResult& InResult);

	/** 保存手动根配置 */
	void SaveManualRoots(const TArray<FString>& Roots);

	void OnAddRootAssetsPicked(const TArray<FAssetData>& Assets);

	/** 生成列表行 */
	TSharedRef<class ITableRow> OnGenerateRow(
		TSharedPtr<FUnusedAssetItem> InItem,
		const TSharedRef<class STableViewBase>& OwnerTable);
	TSharedRef<class ITableRow> OnGenerateRootRow(
		TSharedPtr<FRootItem> InItem,
		const TSharedRef<class STableViewBase>& OwnerTable);

	/** 双击列表项时跳转到 Content Browser */
	void OnItemDoubleClicked(TSharedPtr<FUnusedAssetItem> InItem);

private:
	/** 当前扫描结果项 */
	TArray<TSharedPtr<FUnusedAssetItem>> Items;

	/** 列表控件 */
	TSharedPtr<class SListView<TSharedPtr<FUnusedAssetItem>>> ListView;

	/** 摘要文本 */
	TSharedPtr<class STextBlock> SummaryText;
	TSharedPtr<class SListView<TSharedPtr<FRootItem>>> RootListView;
	TArray<TSharedPtr<FRootItem>> RootItems;
	TSharedPtr<FName> PendingRemoveRoot;
	TWeakPtr<class SWindow> PendingRemoveWindow;
	FUnusedAssetScanOptions ScanOptions;

};
