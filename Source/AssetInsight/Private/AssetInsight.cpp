#include "AssetInsight.h"

#include "AssetRegistry/AssetData.h"
#include "ContentBrowserMenuContexts.h"
#include "Framework/Docking/TabManager.h"
#include "Interfaces/IPluginManager.h"
#include "LevelEditor.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

// 这里请改成你实际定义 EAssetInsightAnalysisMode 的头文件
#include "UI/SAssetInsightWindow.h"
#include "UI/SUnusedAssetScannerWindow.h"

#define LOCTEXT_NAMESPACE "FAssetInsightEditorModule"

const FName FAssetInsightEditorModule::AssetInsightTabName(TEXT("AssetInsight"));
const FName FAssetInsightEditorModule::UnusedAssetScannerTabName(TEXT("UnusedAssetScanner"));

namespace AssetInsightStyle
{
	const FName StyleSetName(TEXT("AssetInsightStyle"));
	const FName AssetInsightIcon(TEXT("AssetInsight.Menu.AssetInsight"));
	const FName AnalyzeDependencyIcon(TEXT("AssetInsight.Menu.AnalyzeDependency"));
	const FName AnalyzeReferencerIcon(TEXT("AssetInsight.Menu.AnalyzeReferencer"));
	const FName UnusedAssetsIcon(TEXT("AssetInsight.Menu.UnusedAssets"));
	const FName AssetInsightTabIcon(TEXT("AssetInsight.Tab.AssetInsight"));
	const FName UnusedAssetScannerTabIcon(TEXT("AssetInsight.Tab.UnusedAssetScanner"));

	TSharedPtr<FSlateStyleSet> StyleSet;

	void Register()
	{
		if (StyleSet.IsValid())
		{
			return;
		}

		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("AssetInsight"));
		if (!Plugin.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("[AssetInsight] Failed to find plugin while registering icons"));
			return;
		}

		StyleSet = MakeShared<FSlateStyleSet>(StyleSetName);
		StyleSet->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources"));

		const FVector2D SmallIcon(20.f, 20.f);
		const FVector2D LargeIcon(40.f, 40.f);

		StyleSet->Set(
			AssetInsightIcon,
			new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("AssetInsight_20"), TEXT(".png")), SmallIcon));
		StyleSet->Set(
			AnalyzeDependencyIcon,
			new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("AssetInsight_40"), TEXT(".png")), LargeIcon));
		StyleSet->Set(
			AnalyzeReferencerIcon,
			new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("AssetInsight_20"), TEXT(".png")), SmallIcon));
		StyleSet->Set(
			UnusedAssetsIcon,
			new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("UnusedAsset_40"), TEXT(".png")), LargeIcon));
		StyleSet->Set(
			AssetInsightTabIcon,
			new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("AssetInsight_40"), TEXT(".png")), LargeIcon));
		StyleSet->Set(
			UnusedAssetScannerTabIcon,
			new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("UnusedAsset_40"), TEXT(".png")), LargeIcon));

		FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
	}

	void Unregister()
	{
		if (!StyleSet.IsValid())
		{
			return;
		}

		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
		StyleSet.Reset();
	}
}

void FAssetInsightEditorModule::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] StartupModule"));
	AssetInsightStyle::Register();

	// 注册资产分析窗口
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		AssetInsightTabName,
		FOnSpawnTab::CreateRaw(this, &FAssetInsightEditorModule::SpawnAssetInsightTab))
		.SetDisplayName(LOCTEXT("AssetInsightTabTitle", "Asset Insight"))
		.SetIcon(FSlateIcon(AssetInsightStyle::StyleSetName, AssetInsightStyle::AssetInsightTabIcon))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// 注册未引用资源扫描窗口
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		UnusedAssetScannerTabName,
		FOnSpawnTab::CreateRaw(this, &FAssetInsightEditorModule::SpawnUnusedAssetScannerTab))
		.SetDisplayName(LOCTEXT("UnusedAssetScannerTabTitle", "Unused Asset Scanner"))
		.SetIcon(FSlateIcon(AssetInsightStyle::StyleSetName, AssetInsightStyle::UnusedAssetScannerTabIcon))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// 注册菜单启动回调
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAssetInsightEditorModule::RegisterMenus));

		UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] RegisterStartupCallback success"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] ToolMenu UI is disabled, RegisterMenus skipped"));
	}
}

void FAssetInsightEditorModule::ShutdownModule()
{
	UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] ShutdownModule"));

	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AssetInsightTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(UnusedAssetScannerTabName);
	AssetInsightStyle::Unregister();
}

TSharedRef<SDockTab> FAssetInsightEditorModule::SpawnAssetInsightTab(const FSpawnTabArgs& Args)
{
	UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] SpawnAssetInsightTab"));

	TSharedRef<SAssetInsightWindow> Window =
		SNew(SAssetInsightWindow);

	AssetInsightWindowWeak = Window;

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("AssetInsightWindowLabel", "Asset Insight"))
		[
			Window
		];
}

TSharedRef<SDockTab> FAssetInsightEditorModule::SpawnUnusedAssetScannerTab(const FSpawnTabArgs& Args)
{
	UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] SpawnUnusedAssetScannerTab"));

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("UnusedAssetScannerWindowLabel", "Unused Asset Scanner"))
		[
			SNew(SUnusedAssetScannerWindow)
		];
}

void FAssetInsightEditorModule::RegisterMenus()
{
	UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] RegisterMenus"));

	if (!UToolMenus::IsToolMenuUIEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] ToolMenu UI disabled in RegisterMenus"));
		return;
	}

	FToolMenuOwnerScoped OwnerScoped(this);

	// =========================
	// Content Browser 右键菜单
	// =========================
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu");
		if (Menu)
		{
			UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] Extend ContentBrowser.AssetContextMenu success"));

			FToolMenuSection& Section = Menu->FindOrAddSection("AssetContextAdvancedActions");

			Section.AddSubMenu(
				"AssetInsightSubMenu",
				LOCTEXT("AssetInsight_Label", "Asset Insight"),
				LOCTEXT("AssetInsight_Tooltip", "Analyze asset dependency and referencer trees."),
				FNewToolMenuDelegate::CreateLambda([this](UToolMenu* SubMenu)
				{
					FToolMenuSection& SubSection = SubMenu->AddSection("AssetInsightActions");

					SubSection.AddMenuEntry(
						"AssetInsight_AnalyzeDependency",
						LOCTEXT("AnalyzeDependency_Label", "Analyze Dependency Tree"),
						LOCTEXT("AnalyzeDependency_Tooltip", "Open Asset Insight window and analyze dependency tree."),
						FSlateIcon(),
						FToolMenuExecuteAction::CreateRaw(this, &FAssetInsightEditorModule::ExecuteAnalyzeDependency));

					SubSection.AddMenuEntry(
						"AssetInsight_AnalyzeReferencer",
						LOCTEXT("AnalyzeReferencer_Label", "Analyze Referencer Tree"),
						LOCTEXT("AnalyzeReferencer_Tooltip", "Open Asset Insight window and analyze referencer tree."),
						FSlateIcon(),
						FToolMenuExecuteAction::CreateRaw(this, &FAssetInsightEditorModule::ExecuteAnalyzeReferencer));
				}),
				false,
				FSlateIcon(AssetInsightStyle::StyleSetName, AssetInsightStyle::AssetInsightIcon));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[AssetInsight] Failed to extend ContentBrowser.AssetContextMenu"));
		}
	}

	// =========================
	// Tools 主菜单入口（比工具栏更稳）
	// =========================
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		if (Menu)
		{
			UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] Extend LevelEditor.MainMenu.Tools success"));

			FToolMenuSection& Section = Menu->FindOrAddSection("AssetInsight");

			Section.AddMenuEntry(
				"OpenUnusedAssetScanner",
				LOCTEXT("OpenUnusedAssetScanner_Label", "Unused Assets"),
				LOCTEXT("OpenUnusedAssetScanner_Tooltip", "Open the Unused Asset Scanner window."),
				FSlateIcon(AssetInsightStyle::StyleSetName, AssetInsightStyle::UnusedAssetsIcon),
				FUIAction(FExecuteAction::CreateRaw(this, &FAssetInsightEditorModule::OpenUnusedAssetScannerTab))
			);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[AssetInsight] Failed to extend LevelEditor.MainMenu.Tools"));
		}
	}
}

void FAssetInsightEditorModule::ExecuteAnalyzeDependency(const FToolMenuContext& InContext)
{
	UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] ExecuteAnalyzeDependency"));

	const UContentBrowserAssetContextMenuContext* AssetContext =
		InContext.FindContext<UContentBrowserAssetContextMenuContext>();

	if (!AssetContext || AssetContext->SelectedAssets.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] No selected asset for dependency analysis"));
		return;
	}

	OpenWindowAndAnalyze(AssetContext->SelectedAssets[0], EAssetInsightAnalysisMode::Dependency);
}

void FAssetInsightEditorModule::ExecuteAnalyzeReferencer(const FToolMenuContext& InContext)
{
	UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] ExecuteAnalyzeReferencer"));

	const UContentBrowserAssetContextMenuContext* AssetContext =
		InContext.FindContext<UContentBrowserAssetContextMenuContext>();

	if (!AssetContext || AssetContext->SelectedAssets.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] No selected asset for referencer analysis"));
		return;
	}

	OpenWindowAndAnalyze(AssetContext->SelectedAssets[0], EAssetInsightAnalysisMode::Referencer);
}

void FAssetInsightEditorModule::OpenWindowAndAnalyze(const FAssetData& InAsset, EAssetInsightAnalysisMode InMode)
{
	UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] OpenWindowAndAnalyze: %s"), *InAsset.AssetName.ToString());

	FGlobalTabmanager::Get()->TryInvokeTab(AssetInsightTabName);

	TSharedPtr<SAssetInsightWindow> Window = AssetInsightWindowWeak.Pin();
	if (!Window.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[AssetInsight] AssetInsightWindow is invalid after invoking tab"));
		return;
	}

	Window->AnalyzeAsset(InAsset, InMode);
}

void FAssetInsightEditorModule::OpenUnusedAssetScannerTab()
{
	UE_LOG(LogTemp, Warning, TEXT("[AssetInsight] OpenUnusedAssetScannerTab"));
	FGlobalTabmanager::Get()->TryInvokeTab(UnusedAssetScannerTabName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAssetInsightEditorModule, AssetInsight)
