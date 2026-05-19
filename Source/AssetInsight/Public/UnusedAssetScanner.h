#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

/**
 * 单个未引用资源项
 *
 * 注意：
 * 这里的“未引用”是基于 AssetRegistry 静态引用关系判断，
 * 即没有任何 Referencers 的资源（可能存在软引用/代码引用漏检）
 */
struct FUnusedAssetItem
{
	/** 完整资源数据（用于 Content Browser 跳转等功能） */
	FAssetData AssetData;

	/** 资源包名，例如：/Game/Test/BP_Test */
	FName PackageName;

	/** UI 显示名称 */
	FString DisplayName;

	/** 是否为 Engine 资源（v1 默认不扫描 Engine） */
	bool bIsEngineAsset = false;

	bool bReachableByRoot = false;
	int32 PackageReferencerCount = 0;
	int32 ManageReferencerCount = 0;
	int32 SearchableNameReferencerCount = 0;
};

enum class EAssetInsightRootSource : uint8
{
	DefaultMap,
	ManualPath,
	AlwaysCookDirectory,
	PrimaryAsset
};

struct FAssetInsightRootAsset
{
	FName PackageName;
	EAssetInsightRootSource Source = EAssetInsightRootSource::ManualPath;
	FString SourceDetail;
};

struct FAssetInsightReachableInfo
{
	FName PackageName;
	EAssetInsightRootSource FirstRootSource = EAssetInsightRootSource::ManualPath;
	FName FirstReachedFrom;
};

struct FUnusedAssetScanOptions
{
	bool bIncludeDefaultMap = true;
	bool bIncludeManualRoots = true;
	bool bIncludeAlwaysCookDirectories = true;
	bool bIncludePrimaryAssets = true;
	bool bIncludeSoftPackageDependencies = true;
	bool bIncludeManageReferencers = true;
	bool bIncludeSearchableNameReferencers = true;
};

/**
 * 扫描结果
 */
struct FUnusedAssetScanResult
{
	/** 未引用资源列表 */
	TArray<FUnusedAssetItem> Items;

	/** 扫描到的总资源数 */
	int32 TotalScannedAssetCount = 0;

	/** 未引用资源数量 */
	int32 UnusedAssetCount = 0;

	/** /Game 资源数量 */
	int32 ProjectAssetCount = 0;

	/** /Engine 资源数量 */
	int32 EngineAssetCount = 0;

	TArray<FAssetInsightRootAsset> RootAssets;
	TMap<FName, FAssetInsightReachableInfo> ReachableAssets;

	int32 DefaultMapRootCount = 0;
	int32 ManualRootCount = 0;
	int32 AlwaysCookRootCount = 0;
	int32 PrimaryAssetRootCount = 0;
	int32 ReachableAssetCount = 0;
};

/**
 * FUnusedAssetScanner
 *
 * 无状态分析器：
 * 从 AssetRegistry 中筛选出“没有 Referencers 的资源”
 *
 * v1规则：
 * - 只扫描 /Game
 * - Referencers 为空 → 判定为“可能未引用”
 */
class FUnusedAssetScanner
{
public:

	/** 扫描项目中未引用资源（v1） */
	static FUnusedAssetScanResult ScanProjectUnusedAssets(const FUnusedAssetScanOptions& Options = FUnusedAssetScanOptions());
	static void GetRootPackages(TArray<FName>& OutRoots, const FUnusedAssetScanOptions& Options = FUnusedAssetScanOptions());
	static void GetRootAssets(TArray<FAssetInsightRootAsset>& OutRoots, const FUnusedAssetScanOptions& Options = FUnusedAssetScanOptions());

private:

	/** 获取 /Game 下所有资源 */
	static void GetAllProjectAssets(class IAssetRegistry& InAssetRegistry, TArray<FAssetData>& OutAssets);

	/** 构建结果项 */
	static FUnusedAssetItem BuildItem(const FAssetData& InAssetData);
};
