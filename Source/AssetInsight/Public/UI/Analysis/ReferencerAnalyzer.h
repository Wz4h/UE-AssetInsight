#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetAnalysisTypes.h"

/**
 * FReferencerAnalyzer
 *
 * 生成引用树（谁引用了我）
 *
 * 特点：
 * - DFS 构建树
 * - 支持最大深度
 * - 支持循环检测
 * - 在构建树的同时统计 Summary
 */
class FReferencerAnalyzer
{
public:

	/** 构建引用树 + Summary */
	static FAssetAnalysisResult BuildReferencerTree(
		const FAssetData& InRootAsset,
		int32 MaxDepth = 5,
		const FAssetReferencerTreeOptions& Options = FAssetReferencerTreeOptions()
	);

private:

	static FAssetReferencerInfo BuildReferencerInfo(
		class IAssetRegistry& Registry,
		const FName& PackageName,
		const FAssetReferencerTreeOptions& Options
	);

	static void GetReferencersByCategory(
		class IAssetRegistry& Registry,
		const FName& PackageName,
		UE::AssetRegistry::EDependencyCategory Category,
		UE::AssetRegistry::FDependencyQuery Query,
		TArray<struct FAssetDependency>& OutReferencers
	);

	static void AddReferencerChildren(
		class IAssetRegistry& Registry,
		TSharedPtr<FAssetTreeNode> RootNode,
		const TArray<struct FAssetDependency>& Referencers,
		EAssetReferencerCategory Category,
		TSet<FName>& UniqueSet,
		FAssetAnalysisSummary& Summary
	);

	static void ApplyPackageReferencerProperties(
		const struct FAssetDependency& Referencer,
		TSharedPtr<FAssetTreeNode> Node
	);

	static FString BuildDisplayName(
		class IAssetRegistry& Registry,
		const FName& PackageName
	);
};
