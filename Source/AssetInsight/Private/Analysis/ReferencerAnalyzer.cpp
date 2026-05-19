#include "UI/Analysis/ReferencerAnalyzer.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"

FAssetAnalysisResult FReferencerAnalyzer::BuildReferencerTree(
	const FAssetData& InRootAsset,
	int32 MaxDepth,
	const FAssetReferencerTreeOptions& Options
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
	RootNode->ReferencerCategory = EAssetReferencerCategory::Root;

	FAssetAnalysisSummary Summary;
	Summary.RootAsset = InRootAsset.PackageName;
	Summary.TreeType = TEXT("Referencer");
	Summary.MaxDepth = MaxDepth;
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

	const FAssetReferencerInfo Info = BuildReferencerInfo(Registry, InRootAsset.PackageName, Options);
	RootNode->NumPackageReferencers = Info.NumPackageReferencers;
	RootNode->NumManageReferencers = Info.NumManageReferencers;
	RootNode->NumSemanticReferencers = Info.NumSemanticReferencers;
	RootNode->bHard = Info.bHard;
	RootNode->bSoft = Info.bSoft;
	RootNode->bGame = Info.bGame;
	RootNode->bEditorOnly = Info.bEditorOnly;
	RootNode->bBuild = Info.bBuild;

	TSet<FName> UniqueSet;
	UniqueSet.Add(InRootAsset.PackageName);

	TArray<FAssetDependency> PackageReferencers;
	TArray<FAssetDependency> ManageReferencers;
	TArray<FAssetDependency> SemanticReferencers;

	if (Options.bShowPackageReferencers)
	{
		const UE::AssetRegistry::FDependencyQuery PackageQuery =
			Options.bIncludeSoftPackageReferencers
			? UE::AssetRegistry::FDependencyQuery(UE::AssetRegistry::EDependencyQuery::NoRequirements)
			: UE::AssetRegistry::FDependencyQuery(UE::AssetRegistry::EDependencyQuery::Hard);

		GetReferencersByCategory(
			Registry,
			InRootAsset.PackageName,
			UE::AssetRegistry::EDependencyCategory::Package,
			PackageQuery,
			PackageReferencers
		);
		AddReferencerChildren(Registry, RootNode, PackageReferencers, EAssetReferencerCategory::Package, UniqueSet, Summary);
	}

	if (Options.bShowManageReferencers)
	{
		GetReferencersByCategory(
			Registry,
			InRootAsset.PackageName,
			UE::AssetRegistry::EDependencyCategory::Manage,
			UE::AssetRegistry::FDependencyQuery(),
			ManageReferencers
		);
		AddReferencerChildren(Registry, RootNode, ManageReferencers, EAssetReferencerCategory::Manage, UniqueSet, Summary);
	}

	if (Options.bShowSemanticReferencers)
	{
		GetReferencersByCategory(
			Registry,
			InRootAsset.PackageName,
			UE::AssetRegistry::EDependencyCategory::SearchableName,
			UE::AssetRegistry::FDependencyQuery(),
			SemanticReferencers
		);
		AddReferencerChildren(Registry, RootNode, SemanticReferencers, EAssetReferencerCategory::Semantic, UniqueSet, Summary);
	}

	Result.RootNode = RootNode;
	Result.Summary = Summary;

	return Result;
}

FAssetReferencerInfo FReferencerAnalyzer::BuildReferencerInfo(
	IAssetRegistry& Registry,
	const FName& PackageName,
	const FAssetReferencerTreeOptions& Options
)
{
	FAssetReferencerInfo Info;
	Info.AssetPackageName = PackageName;

	TArray<FAssetDependency> PackageReferencers;
	TArray<FAssetDependency> ManageReferencers;
	TArray<FAssetDependency> SemanticReferencers;

	const UE::AssetRegistry::FDependencyQuery PackageQuery =
		Options.bIncludeSoftPackageReferencers
		? UE::AssetRegistry::FDependencyQuery(UE::AssetRegistry::EDependencyQuery::NoRequirements)
		: UE::AssetRegistry::FDependencyQuery(UE::AssetRegistry::EDependencyQuery::Hard);

	GetReferencersByCategory(
		Registry,
		PackageName,
		UE::AssetRegistry::EDependencyCategory::Package,
		PackageQuery,
		PackageReferencers
	);

	GetReferencersByCategory(
		Registry,
		PackageName,
		UE::AssetRegistry::EDependencyCategory::Manage,
		UE::AssetRegistry::FDependencyQuery(),
		ManageReferencers
	);

	GetReferencersByCategory(
		Registry,
		PackageName,
		UE::AssetRegistry::EDependencyCategory::SearchableName,
		UE::AssetRegistry::FDependencyQuery(),
		SemanticReferencers
	);

	Info.NumPackageReferencers = PackageReferencers.Num();
	Info.NumManageReferencers = ManageReferencers.Num();
	Info.NumSemanticReferencers = SemanticReferencers.Num();

	for (const FAssetDependency& Ref : PackageReferencers)
	{
		Info.PackageReferencerNames.Add(Ref.AssetId.PackageName);
		Info.bHard |= EnumHasAnyFlags(Ref.Properties, UE::AssetRegistry::EDependencyProperty::Hard);
		Info.bSoft |= !EnumHasAnyFlags(Ref.Properties, UE::AssetRegistry::EDependencyProperty::Hard);
		Info.bGame |= EnumHasAnyFlags(Ref.Properties, UE::AssetRegistry::EDependencyProperty::Game);
		Info.bEditorOnly |= !EnumHasAnyFlags(Ref.Properties, UE::AssetRegistry::EDependencyProperty::Game);
		Info.bBuild |= EnumHasAnyFlags(Ref.Properties, UE::AssetRegistry::EDependencyProperty::Build);
	}

	for (const FAssetDependency& Ref : ManageReferencers)
	{
		Info.ManageReferencerNames.Add(Ref.AssetId.PackageName);
	}

	for (const FAssetDependency& Ref : SemanticReferencers)
	{
		Info.SemanticReferencerNames.Add(Ref.AssetId.PackageName);
	}

	return Info;
}

void FReferencerAnalyzer::GetReferencersByCategory(
	IAssetRegistry& Registry,
	const FName& PackageName,
	UE::AssetRegistry::EDependencyCategory Category,
	UE::AssetRegistry::FDependencyQuery Query,
	TArray<FAssetDependency>& OutReferencers
)
{
	OutReferencers.Reset();

	TArray<FAssetDependency> Referencers;
	Registry.GetReferencers(
		FAssetIdentifier(PackageName),
		Referencers,
		Category,
		Query
	);

	TSet<FName> SeenPackages;
	for (const FAssetDependency& Ref : Referencers)
	{
		if (Ref.AssetId.PackageName.IsNone())
		{
			continue;
		}

		const FString PackageString = Ref.AssetId.PackageName.ToString();
		if (!FPackageName::IsValidLongPackageName(PackageString))
		{
			continue;
		}

		if (SeenPackages.Contains(Ref.AssetId.PackageName))
		{
			continue;
		}

		SeenPackages.Add(Ref.AssetId.PackageName);
		OutReferencers.Add(Ref);
	}
}

void FReferencerAnalyzer::AddReferencerChildren(
	IAssetRegistry& Registry,
	TSharedPtr<FAssetTreeNode> RootNode,
	const TArray<FAssetDependency>& Referencers,
	EAssetReferencerCategory Category,
	TSet<FName>& UniqueSet,
	FAssetAnalysisSummary& Summary
)
{
	if (!RootNode.IsValid())
	{
		return;
	}

	for (const FAssetDependency& Ref : Referencers)
	{
		const FName RefPackage = Ref.AssetId.PackageName;

		TSharedPtr<FAssetTreeNode> ChildNode = MakeShared<FAssetTreeNode>();
		ChildNode->PackageName = RefPackage;
		ChildNode->DisplayName = BuildDisplayName(Registry, RefPackage);
		ChildNode->ReferencerCategory = Category;

		if (Category == EAssetReferencerCategory::Package)
		{
			ApplyPackageReferencerProperties(Ref, ChildNode);
		}
		else if (Category == EAssetReferencerCategory::Manage)
		{
			ChildNode->bHasManageReferences = true;
		}
		else if (Category == EAssetReferencerCategory::Semantic)
		{
			ChildNode->bHasSemanticReferences = true;
		}

		RootNode->Children.Add(ChildNode);
		Summary.ExpandedNodeCount++;

		if (!UniqueSet.Contains(RefPackage))
		{
			UniqueSet.Add(RefPackage);
			Summary.UniqueNodeCount++;

			if (RefPackage.ToString().StartsWith(TEXT("/Engine")))
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

void FReferencerAnalyzer::ApplyPackageReferencerProperties(
	const FAssetDependency& Referencer,
	TSharedPtr<FAssetTreeNode> Node
)
{
	if (!Node.IsValid())
	{
		return;
	}

	Node->bHard = EnumHasAnyFlags(Referencer.Properties, UE::AssetRegistry::EDependencyProperty::Hard);
	Node->bSoft = !Node->bHard;
	Node->bGame = EnumHasAnyFlags(Referencer.Properties, UE::AssetRegistry::EDependencyProperty::Game);
	Node->bEditorOnly = !Node->bGame;
	Node->bBuild = EnumHasAnyFlags(Referencer.Properties, UE::AssetRegistry::EDependencyProperty::Build);
}

FString FReferencerAnalyzer::BuildDisplayName(
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
