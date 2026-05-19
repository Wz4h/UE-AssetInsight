#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "AssetAnalysisTypes.h"

class FDependencyAnalyzer
{
public:

	/** 构建依赖树 + Summary */
	static FAssetAnalysisResult BuildDependencyTree(
		const FAssetData& InRootAsset,
		int32 MaxDepth = 5,
		const FAssetDependencyTreeOptions& Options = FAssetDependencyTreeOptions()
	);

private:

	static void BuildTreeRecursive(
		class IAssetRegistry& Registry,
		const FName& CurrentPackage,
		TSharedPtr<FAssetTreeNode> CurrentNode,
		TSet<FName>& CurrentPath,
		TSet<FName>& UniqueSet,
		TSet<FName>& ExpandedSet,
		FAssetAnalysisSummary& Summary,
		const FAssetDependencyTreeOptions& Options,
		int32 CurrentDepth,
		int32 MaxDepth
	);

	static void GetDependencies(
		class IAssetRegistry& Registry,
		const FName& PackageName,
		TArray<struct FAssetDependency>& OutDependencies,
		const FAssetDependencyTreeOptions& Options
	);

	static void ApplyDependencyProperties(
		const struct FAssetDependency& Dependency,
		TSharedPtr<FAssetTreeNode> Node,
		const FAssetDependencyTreeOptions& Options
	);

	static void ApplyHintFlags(
		class IAssetRegistry& Registry,
		TSharedPtr<FAssetTreeNode> Node,
		const FAssetDependencyTreeOptions& Options
	);

	static void AddDirectHintDependencies(
		class IAssetRegistry& Registry,
		const FName& CurrentPackage,
		TSharedPtr<FAssetTreeNode> CurrentNode,
		TSet<FName>& UniqueSet,
		FAssetAnalysisSummary& Summary,
		const FAssetDependencyTreeOptions& Options
	);

	static void AddHintDependencyChildren(
		class IAssetRegistry& Registry,
		TSharedPtr<FAssetTreeNode> CurrentNode,
		const TArray<struct FAssetDependency>& Dependencies,
		bool bManageDependency,
		TSet<FName>& UniqueSet,
		FAssetAnalysisSummary& Summary
	);

	static FString BuildDisplayName(
		class IAssetRegistry& Registry,
		const FName& PackageName
	);
};
