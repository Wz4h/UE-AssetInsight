#include "UI/Analysis/DependencyAnalyzer.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "UI/Analysis/AssetAnalysisTypes.h"

FAssetAnalysisResult FDependencyAnalyzer::BuildDependencyTree(
	const FAssetData& InRootAsset,
	int32 MaxDepth,
	const FAssetDependencyTreeOptions& Options
)
{
	FAssetAnalysisResult Result;

	if (!InRootAsset.IsValid())
	{
		return Result;
	}

	FAssetRegistryModule& ARM =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	IAssetRegistry& Registry = ARM.Get();

	TSharedPtr<FAssetTreeNode> RootNode = MakeShared<FAssetTreeNode>();
	RootNode->PackageName = InRootAsset.PackageName;
	RootNode->DisplayName = InRootAsset.AssetName.ToString();
	ApplyHintFlags(Registry, RootNode, Options);

	FAssetAnalysisSummary Summary;
	Summary.RootAsset = InRootAsset.PackageName;
	Summary.TreeType = TEXT("Dependency");
	Summary.MaxDepth = MaxDepth;

	TSet<FName> CurrentPath;
	CurrentPath.Add(InRootAsset.PackageName);

	TSet<FName> UniqueSet;
	UniqueSet.Add(InRootAsset.PackageName);

	TSet<FName> ExpandedSet;

	Summary.ExpandedNodeCount = 1;
	Summary.UniqueNodeCount = 1;

	if (InRootAsset.PackagePath.ToString().StartsWith(TEXT("/Engine")))
	{
		Summary.EngineAssetCount++;
	}
	else
	{
		Summary.ProjectAssetCount++;
	}

	BuildTreeRecursive(
		Registry,
		InRootAsset.PackageName,
		RootNode,
		CurrentPath,
		UniqueSet,
		ExpandedSet,
		Summary,
		Options,
		0,
		MaxDepth
	);

	Result.RootNode = RootNode;
	Result.Summary = Summary;

	return Result;
}

void FDependencyAnalyzer::BuildTreeRecursive(
	IAssetRegistry& Registry,
	const FName& CurrentPackage,
	TSharedPtr<FAssetTreeNode> CurrentNode,
	TSet<FName>& CurrentPath,
	TSet<FName>& UniqueSet,
	TSet<FName>& ExpandedSet,
	FAssetAnalysisSummary& Summary,
	const FAssetDependencyTreeOptions& Options,
	int32 CurrentDepth,
	int32 MaxDepth
)
{
	if (!CurrentNode.IsValid())
	{
		return;
	}

	if (CurrentDepth >= MaxDepth)
	{
		CurrentNode->bIsDepthLimited = true;
		Summary.DepthLimitedCount++;
		Summary.bDepthLimited = true;
		return;
	}

	if (ExpandedSet.Contains(CurrentPackage))
	{
		CurrentNode->bAlreadyVisited = true;
		return;
	}
	ExpandedSet.Add(CurrentPackage);

	AddDirectHintDependencies(Registry, CurrentPackage, CurrentNode, UniqueSet, Summary, Options);

	TArray<FAssetDependency> Dependencies;
	GetDependencies(Registry, CurrentPackage, Dependencies, Options);

	for (const FAssetDependency& Dep : Dependencies)
	{
		if (Dep.AssetId.PackageName.IsNone())
		{
			continue;
		}

		const FName DepPackage = Dep.AssetId.PackageName;
		TSharedPtr<FAssetTreeNode> ChildNode = MakeShared<FAssetTreeNode>();
		ChildNode->PackageName = DepPackage;
		ChildNode->DisplayName = BuildDisplayName(Registry, DepPackage);
		ApplyDependencyProperties(Dep, ChildNode, Options);
		ApplyHintFlags(Registry, ChildNode, Options);

		CurrentNode->Children.Add(ChildNode);
		Summary.ExpandedNodeCount++;

		if (!UniqueSet.Contains(DepPackage))
		{
			UniqueSet.Add(DepPackage);
			Summary.UniqueNodeCount++;

			if (DepPackage.ToString().StartsWith(TEXT("/Engine")))
			{
				Summary.EngineAssetCount++;
			}
			else
			{
				Summary.ProjectAssetCount++;
			}
		}

		if (CurrentPath.Contains(DepPackage))
		{
			ChildNode->bIsCycle = true;
			Summary.CycleCount++;
			continue;
		}

		if (ExpandedSet.Contains(DepPackage))
		{
			ChildNode->bAlreadyVisited = true;
			continue;
		}

		CurrentPath.Add(DepPackage);
		BuildTreeRecursive(
			Registry,
			DepPackage,
			ChildNode,
			CurrentPath,
			UniqueSet,
			ExpandedSet,
			Summary,
			Options,
			CurrentDepth + 1,
			MaxDepth
		);
		CurrentPath.Remove(DepPackage);
	}
}

void FDependencyAnalyzer::GetDependencies(
	IAssetRegistry& Registry,
	const FName& PackageName,
	TArray<FAssetDependency>& OutDependencies,
	const FAssetDependencyTreeOptions& Options
)
{
	OutDependencies.Reset();

	const UE::AssetRegistry::FDependencyQuery Query =
		Options.bIncludeSoftPackageDependencies
		? UE::AssetRegistry::FDependencyQuery(UE::AssetRegistry::EDependencyQuery::NoRequirements)
		: UE::AssetRegistry::FDependencyQuery(UE::AssetRegistry::EDependencyQuery::Hard);

	Registry.GetDependencies(
		FAssetIdentifier(PackageName),
		OutDependencies,
		UE::AssetRegistry::EDependencyCategory::Package,
		Query
	);
}

void FDependencyAnalyzer::ApplyDependencyProperties(
	const FAssetDependency& Dependency,
	TSharedPtr<FAssetTreeNode> Node,
	const FAssetDependencyTreeOptions& Options
)
{
	if (!Node.IsValid())
	{
		return;
	}

	Node->bHard = EnumHasAnyFlags(Dependency.Properties, UE::AssetRegistry::EDependencyProperty::Hard);
	Node->bSoft = !Node->bHard;
	Node->bGame = EnumHasAnyFlags(Dependency.Properties, UE::AssetRegistry::EDependencyProperty::Game);
	Node->bEditorOnly = Options.bShowEditorOnly && !Node->bGame;
	Node->bBuild = Options.bShowBuild && EnumHasAnyFlags(Dependency.Properties, UE::AssetRegistry::EDependencyProperty::Build);
}

void FDependencyAnalyzer::ApplyHintFlags(
	IAssetRegistry& Registry,
	TSharedPtr<FAssetTreeNode> Node,
	const FAssetDependencyTreeOptions& Options
)
{
	if (!Node.IsValid())
	{
		return;
	}

	if (Options.bShowManageHints)
	{
		TArray<FAssetDependency> ManageReferencers;
		Registry.GetReferencers(
			FAssetIdentifier(Node->PackageName),
			ManageReferencers,
			UE::AssetRegistry::EDependencyCategory::Manage,
			UE::AssetRegistry::FDependencyQuery()
		);
		Node->bHasManageReferences = ManageReferencers.Num() > 0;
	}

	if (Options.bShowSemanticHints)
	{
		TArray<FAssetDependency> SemanticReferencers;
		Registry.GetReferencers(
			FAssetIdentifier(Node->PackageName),
			SemanticReferencers,
			UE::AssetRegistry::EDependencyCategory::SearchableName,
			UE::AssetRegistry::FDependencyQuery()
		);
		Node->bHasSemanticReferences = SemanticReferencers.Num() > 0;
	}
}

void FDependencyAnalyzer::AddDirectHintDependencies(
	IAssetRegistry& Registry,
	const FName& CurrentPackage,
	TSharedPtr<FAssetTreeNode> CurrentNode,
	TSet<FName>& UniqueSet,
	FAssetAnalysisSummary& Summary,
	const FAssetDependencyTreeOptions& Options
)
{
	if (!CurrentNode.IsValid())
	{
		return;
	}

	if (Options.bShowManageHints)
	{
		TArray<FAssetDependency> ManageDependencies;
		Registry.GetDependencies(
			FAssetIdentifier(CurrentPackage),
			ManageDependencies,
			UE::AssetRegistry::EDependencyCategory::Manage,
			UE::AssetRegistry::FDependencyQuery()
		);
		AddHintDependencyChildren(Registry, CurrentNode, ManageDependencies, true, UniqueSet, Summary);
	}

	if (Options.bShowSemanticHints)
	{
		TArray<FAssetDependency> SemanticDependencies;
		Registry.GetDependencies(
			FAssetIdentifier(CurrentPackage),
			SemanticDependencies,
			UE::AssetRegistry::EDependencyCategory::SearchableName,
			UE::AssetRegistry::FDependencyQuery()
		);
		AddHintDependencyChildren(Registry, CurrentNode, SemanticDependencies, false, UniqueSet, Summary);
	}
}

void FDependencyAnalyzer::AddHintDependencyChildren(
	IAssetRegistry& Registry,
	TSharedPtr<FAssetTreeNode> CurrentNode,
	const TArray<FAssetDependency>& Dependencies,
	bool bManageDependency,
	TSet<FName>& UniqueSet,
	FAssetAnalysisSummary& Summary
)
{
	if (!CurrentNode.IsValid())
	{
		return;
	}

	TSet<FName> AddedPackages;
	for (const FAssetDependency& Dependency : Dependencies)
	{
		const FName DepPackage = Dependency.AssetId.PackageName;
		if (DepPackage.IsNone() || AddedPackages.Contains(DepPackage))
		{
			continue;
		}

		AddedPackages.Add(DepPackage);

		TSharedPtr<FAssetTreeNode> ChildNode = MakeShared<FAssetTreeNode>();
		ChildNode->PackageName = DepPackage;
		ChildNode->DisplayName = BuildDisplayName(Registry, DepPackage);
		ChildNode->bIsManageDependency = bManageDependency;
		ChildNode->bIsSemanticDependency = !bManageDependency;

		CurrentNode->Children.Add(ChildNode);
		Summary.ExpandedNodeCount++;

		if (!UniqueSet.Contains(DepPackage))
		{
			UniqueSet.Add(DepPackage);
			Summary.UniqueNodeCount++;

			if (DepPackage.ToString().StartsWith(TEXT("/Engine")))
			{
				Summary.EngineAssetCount++;
			}
			else
			{
				Summary.ProjectAssetCount++;
			}
		}
	}
}

FString FDependencyAnalyzer::BuildDisplayName(
	IAssetRegistry& Registry,
	const FName& PackageName
)
{
	TArray<FAssetData> Datas;
	Registry.GetAssetsByPackageName(PackageName, Datas);

	if (Datas.Num() > 0 && Datas[0].IsValid())
	{
		return Datas[0].AssetName.ToString();
	}

	return PackageName.ToString();
}
