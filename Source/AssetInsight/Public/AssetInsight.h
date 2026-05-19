#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

enum class EAssetInsightAnalysisMode : uint8;
class SAssetInsightWindow;

class FAssetInsightEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** 注册菜单与工具栏 */
	void RegisterMenus();

	/** 生成资产分析窗口 */
	TSharedRef<class SDockTab> SpawnAssetInsightTab(const class FSpawnTabArgs& Args);

	/** 生成未引用资源扫描窗口 */
	TSharedRef<class SDockTab> SpawnUnusedAssetScannerTab(const class FSpawnTabArgs& Args);

	/** 菜单执行：分析依赖树 */
	void ExecuteAnalyzeDependency(const struct FToolMenuContext& InContext);

	/** 菜单执行：分析引用树 */
	void ExecuteAnalyzeReferencer(const struct FToolMenuContext& InContext);

	/** 打开窗口并执行分析 */
	void OpenWindowAndAnalyze(const struct FAssetData& InAsset, EAssetInsightAnalysisMode InMode);

	/** 打开未引用资源扫描窗口 */
	void OpenUnusedAssetScannerTab();

private:
	/** 资产分析窗口 Tab ID */
	static const FName AssetInsightTabName;

	/** 未引用资源扫描窗口 Tab ID */
	static const FName UnusedAssetScannerTabName;

	/** 当前窗口弱引用 */
	TWeakPtr<SAssetInsightWindow> AssetInsightWindowWeak;
};