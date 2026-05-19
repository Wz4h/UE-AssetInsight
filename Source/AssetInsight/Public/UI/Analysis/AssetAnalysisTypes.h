#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

enum class EAssetReferencerCategory : uint8
{
	Root,
	Package,
	Manage,
	Semantic
};

/** 树节点（已存在的话沿用） */
struct FAssetTreeNode
{
	FName PackageName;
	FString DisplayName;

	TArray<TSharedPtr<FAssetTreeNode>> Children;

	bool bIsCycle = false;
	bool bIsDepthLimited = false;
	bool bAlreadyVisited = false;

	bool bHard = false;
	bool bSoft = false;
	bool bGame = false;
	bool bEditorOnly = false;
	bool bBuild = false;

	bool bHasManageReferences = false;
	bool bHasSemanticReferences = false;
	bool bIsManageDependency = false;
	bool bIsSemanticDependency = false;

	EAssetReferencerCategory ReferencerCategory = EAssetReferencerCategory::Root;
	int32 NumPackageReferencers = 0;
	int32 NumManageReferencers = 0;
	int32 NumSemanticReferencers = 0;
};

struct FAssetDependencyTreeOptions
{
	bool bIncludeSoftPackageDependencies = true;
	bool bShowEditorOnly = true;
	bool bShowBuild = true;
	bool bShowManageHints = true;
	bool bShowSemanticHints = true;
};

struct FAssetReferencerTreeOptions
{
	bool bShowPackageReferencers = true;
	bool bIncludeSoftPackageReferencers = true;
	bool bShowManageReferencers = true;
	bool bShowSemanticReferencers = true;
};

struct FAssetReferencerInfo
{
	FName AssetPackageName;

	int32 NumPackageReferencers = 0;
	int32 NumManageReferencers = 0;
	int32 NumSemanticReferencers = 0;

	TArray<FName> PackageReferencerNames;
	TArray<FName> ManageReferencerNames;
	TArray<FName> SemanticReferencerNames;

	bool bHard = false;
	bool bSoft = false;
	bool bGame = false;
	bool bEditorOnly = false;
	bool bBuild = false;
};

/** 摘要 */
struct FAssetAnalysisSummary
{
	FName RootAsset;
	FString TreeType;          // "Dependency" / "Referencer"
	int32 MaxDepth = 0;

	int32 ExpandedNodeCount = 0;   // 树节点总数（包含重复展开）
	int32 UniqueNodeCount = 0;     // 唯一资产数

	int32 CycleCount = 0;
	int32 DepthLimitedCount = 0;

	int32 ProjectAssetCount = 0;
	int32 EngineAssetCount = 0;

	bool bDepthLimited = false;    // 是否发生过截断
};

/** 统一分析结果 */
struct FAssetAnalysisResult
{
	TSharedPtr<FAssetTreeNode> RootNode;
	FAssetAnalysisSummary Summary;
}; 

enum class EAssetInsightAnalysisMode : uint8
{
	Dependency,
	Referencer
};
