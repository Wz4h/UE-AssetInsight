#include "UnusedAssetScanner.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Engine/AssetManagerTypes.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Settings/ProjectPackagingSettings.h"

namespace
{
	bool NormalizeToPackageName(const FString& InPath, FName& OutPackageName)
	{
		if (InPath.IsEmpty())
		{
			return false;
		}

		FString PackageName = InPath;
		if (InPath.Contains(TEXT(".")))
		{
			PackageName = FPackageName::ObjectPathToPackageName(InPath);
		}

		if (FPackageName::IsValidLongPackageName(PackageName))
		{
			OutPackageName = FName(*PackageName);
			return true;
		}

		if (FPackageName::TryConvertFilenameToLongPackageName(InPath, PackageName))
		{
			OutPackageName = FName(*PackageName);
			return true;
		}

		return false;
	}

	FString RootSourceToString(EAssetInsightRootSource Source)
	{
		switch (Source)
		{
		case EAssetInsightRootSource::DefaultMap:
			return TEXT("Default Map");
		case EAssetInsightRootSource::ManualPath:
			return TEXT("Manual Path");
		case EAssetInsightRootSource::AlwaysCookDirectory:
			return TEXT("Always Cook");
		case EAssetInsightRootSource::PrimaryAsset:
			return TEXT("Primary Asset");
		default:
			return TEXT("Unknown");
		}
	}

	void AddRootIfValid(const FString& RootPath, EAssetInsightRootSource Source, const FString& SourceDetail, TArray<FAssetInsightRootAsset>& OutRoots)
	{
		FName PackageName;
		if (NormalizeToPackageName(RootPath, PackageName))
		{
			FAssetInsightRootAsset Root;
			Root.PackageName = PackageName;
			Root.Source = Source;
			Root.SourceDetail = SourceDetail;
			OutRoots.Add(Root);
		}
	}

	FString NormalizeCookDirectoryPath(const FString& InPath)
	{
		FString Path = InPath;
		Path.TrimStartAndEndInline();

		if (Path.IsEmpty())
		{
			return FString();
		}

		if (!Path.StartsWith(TEXT("/")))
		{
			Path = FString::Printf(TEXT("/Game/%s"), *Path);
		}

		FName PackageName;
		return NormalizeToPackageName(Path, PackageName) ? PackageName.ToString() : FString();
	}

	void DeduplicateRootAssets(TArray<FAssetInsightRootAsset>& InOutRoots)
	{
		TSet<FString> SeenKeys;
		TArray<FAssetInsightRootAsset> UniqueRoots;

		for (const FAssetInsightRootAsset& Root : InOutRoots)
		{
			if (Root.PackageName.IsNone())
			{
				continue;
			}

			const FString Key = FString::Printf(
				TEXT("%s|%d"),
				*Root.PackageName.ToString(),
				static_cast<int32>(Root.Source));

			if (SeenKeys.Contains(Key))
			{
				continue;
			}

			SeenKeys.Add(Key);
			UniqueRoots.Add(Root);
		}

		InOutRoots = MoveTemp(UniqueRoots);
	}

	void CollectDefaultMapRoots(TArray<FAssetInsightRootAsset>& OutRoots)
	{
		if (!GConfig)
		{
			return;
		}

		const TCHAR* Section = TEXT("/Script/EngineSettings.GameMapsSettings");
		auto ReadAndAdd = [&](const TCHAR* Key)
		{
			FString Value;
			if (GConfig->GetString(Section, Key, Value, GEngineIni) ||
				GConfig->GetString(Section, Key, Value, GGameIni))
			{
				AddRootIfValid(Value, EAssetInsightRootSource::DefaultMap, Value, OutRoots);
			}
		};

		ReadAndAdd(TEXT("GameDefaultMap"));
		ReadAndAdd(TEXT("EditorStartupMap"));
		ReadAndAdd(TEXT("ServerDefaultMap"));
	}

	void CollectManualRoots(TArray<FAssetInsightRootAsset>& OutRoots)
	{
		TArray<FString> RootStrings;
		if (GConfig)
		{
			TArray<FString> TempRoots;
			GConfig->GetArray(TEXT("AssetInsight"), TEXT("ManualRootPackages"), TempRoots, GEditorPerProjectIni);
			RootStrings.Append(TempRoots);
			TempRoots.Reset();
			GConfig->GetArray(TEXT("AssetInsight"), TEXT("ManualRootPackages"), TempRoots, GGameIni);
			RootStrings.Append(TempRoots);
		}

		for (const FString& Root : RootStrings)
		{
			AddRootIfValid(Root, EAssetInsightRootSource::ManualPath, Root, OutRoots);
		}
	}

	void CollectAlwaysCookRoots(TArray<FAssetInsightRootAsset>& OutRoots)
	{
		const UProjectPackagingSettings* PackagingSettings = GetDefault<UProjectPackagingSettings>();
		if (!PackagingSettings)
		{
			return;
		}

		for (const FDirectoryPath& Directory : PackagingSettings->DirectoriesToAlwaysCook)
		{
			const FString PackagePath = NormalizeCookDirectoryPath(Directory.Path);
			if (PackagePath.IsEmpty())
			{
				continue;
			}

			FAssetInsightRootAsset Root;
			Root.PackageName = FName(*PackagePath);
			Root.Source = EAssetInsightRootSource::AlwaysCookDirectory;
			Root.SourceDetail = PackagePath;
			OutRoots.Add(Root);
		}
	}

	void CollectPrimaryAssetRoots(TArray<FAssetInsightRootAsset>& OutRoots)
	{
		if (!UAssetManager::IsInitialized())
		{
			return;
		}

		UAssetManager& AssetManager = UAssetManager::Get();

		TArray<FPrimaryAssetTypeInfo> TypeInfos;
		AssetManager.GetPrimaryAssetTypeInfoList(TypeInfos);

		for (const FPrimaryAssetTypeInfo& TypeInfo : TypeInfos)
		{
			TArray<FPrimaryAssetId> PrimaryAssetIds;
			if (!AssetManager.GetPrimaryAssetIdList(TypeInfo.PrimaryAssetType, PrimaryAssetIds))
			{
				continue;
			}

			for (const FPrimaryAssetId& PrimaryAssetId : PrimaryAssetIds)
			{
				FAssetData AssetData;
				if (AssetManager.GetPrimaryAssetData(PrimaryAssetId, AssetData) && AssetData.IsValid())
				{
					FAssetInsightRootAsset Root;
					Root.PackageName = AssetData.PackageName;
					Root.Source = EAssetInsightRootSource::PrimaryAsset;
					Root.SourceDetail = PrimaryAssetId.ToString();
					OutRoots.Add(Root);
					continue;
				}

				const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(PrimaryAssetId);
				FName PackageName;
				if (NormalizeToPackageName(AssetPath.ToString(), PackageName))
				{
					FAssetInsightRootAsset Root;
					Root.PackageName = PackageName;
					Root.Source = EAssetInsightRootSource::PrimaryAsset;
					Root.SourceDetail = PrimaryAssetId.ToString();
					OutRoots.Add(Root);
				}
			}
		}
	}

	void CollectRootEntries(IAssetRegistry& Registry, const FUnusedAssetScanOptions& Options, TArray<FAssetInsightRootAsset>& OutRoots)
	{
		TArray<FAssetInsightRootAsset> CollectedRoots;

		if (Options.bIncludeDefaultMap)
		{
			CollectDefaultMapRoots(CollectedRoots);
		}

		if (Options.bIncludeManualRoots)
		{
			CollectManualRoots(CollectedRoots);
		}

		if (Options.bIncludeAlwaysCookDirectories)
		{
			CollectAlwaysCookRoots(CollectedRoots);
		}

		if (Options.bIncludePrimaryAssets)
		{
			CollectPrimaryAssetRoots(CollectedRoots);
		}

		DeduplicateRootAssets(CollectedRoots);
		OutRoots.Append(CollectedRoots);
	}

	void AddReachableRootAsset(
		const FName& PackageName,
		const FAssetInsightRootAsset& Root,
		TMap<FName, FAssetInsightReachableInfo>& OutReachable,
		TArray<FName>& Stack)
	{
		if (PackageName.IsNone() || OutReachable.Contains(PackageName))
		{
			return;
		}

		FAssetInsightReachableInfo Info;
		Info.PackageName = PackageName;
		Info.FirstRootSource = Root.Source;
		Info.FirstReachedFrom = NAME_None;

		OutReachable.Add(PackageName, Info);
		Stack.Add(PackageName);
	}

	void AddAlwaysCookDirectoryReachableAssets(
		IAssetRegistry& Registry,
		const FAssetInsightRootAsset& Root,
		TMap<FName, FAssetInsightReachableInfo>& OutReachable,
		TArray<FName>& Stack)
	{
		TArray<FAssetData> AssetsInDirectory;
		Registry.GetAssetsByPath(
			Root.PackageName,
			AssetsInDirectory,
			/*bRecursive*/ true,
			/*bIncludeOnlyOnDiskAssets*/ false
		);

		for (const FAssetData& Asset : AssetsInDirectory)
		{
			if (!Asset.IsValid())
			{
				continue;
			}

			AddReachableRootAsset(Asset.PackageName, Root, OutReachable, Stack);
		}
	}

	void BuildReachableSet(
		IAssetRegistry& Registry,
		const FUnusedAssetScanOptions& Options,
		TArray<FAssetInsightRootAsset>& OutRootAssets,
		TMap<FName, FAssetInsightReachableInfo>& OutReachable)
	{
		CollectRootEntries(Registry, Options, OutRootAssets);
		DeduplicateRootAssets(OutRootAssets);

		TArray<FName> Stack;
		for (const FAssetInsightRootAsset& Root : OutRootAssets)
		{
			if (Root.Source == EAssetInsightRootSource::AlwaysCookDirectory)
			{
				AddAlwaysCookDirectoryReachableAssets(Registry, Root, OutReachable, Stack);
				continue;
			}

			AddReachableRootAsset(Root.PackageName, Root, OutReachable, Stack);
		}

		while (Stack.Num() > 0)
		{
			const FName Current = Stack.Pop(EAllowShrinking::No);

			TArray<FName> Dependencies;
			Registry.GetDependencies(
				Current,
				Dependencies,
				UE::AssetRegistry::EDependencyCategory::Package,
				Options.bIncludeSoftPackageDependencies
					? UE::AssetRegistry::EDependencyQuery::NoRequirements
					: UE::AssetRegistry::EDependencyQuery::Hard
			);

			for (const FName& Dep : Dependencies)
			{
				if (OutReachable.Contains(Dep))
				{
					continue;
				}

				const FAssetInsightReachableInfo* ParentInfo = OutReachable.Find(Current);
				FAssetInsightReachableInfo Info;
				Info.PackageName = Dep;
				Info.FirstRootSource = ParentInfo ? ParentInfo->FirstRootSource : EAssetInsightRootSource::ManualPath;
				Info.FirstReachedFrom = Current;
				OutReachable.Add(Dep, Info);
				Stack.Add(Dep);
			}
		}
	}

	int32 CountReferencers(
		IAssetRegistry& Registry,
		const FName& PackageName,
		UE::AssetRegistry::EDependencyCategory Category,
		UE::AssetRegistry::EDependencyQuery Query = UE::AssetRegistry::EDependencyQuery::NoRequirements)
	{
		TArray<FName> Referencers;
		Registry.GetReferencers(PackageName, Referencers, Category, Query);
		return Referencers.Num();
	}
}

void FUnusedAssetScanner::GetRootPackages(TArray<FName>& OutRoots, const FUnusedAssetScanOptions& Options)
{
	FAssetRegistryModule& ARM =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetInsightRootAsset> RootAssets;
	CollectRootEntries(ARM.Get(), Options, RootAssets);

	TSet<FName> UniqueRoots;
	for (const FAssetInsightRootAsset& Root : RootAssets)
	{
		UniqueRoots.Add(Root.PackageName);
	}

	OutRoots = UniqueRoots.Array();
	OutRoots.Sort([](const FName& A, const FName& B)
	{
		return A.LexicalLess(B);
	});
}

void FUnusedAssetScanner::GetRootAssets(TArray<FAssetInsightRootAsset>& OutRoots, const FUnusedAssetScanOptions& Options)
{
	FAssetRegistryModule& ARM =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	CollectRootEntries(ARM.Get(), Options, OutRoots);
	DeduplicateRootAssets(OutRoots);
	OutRoots.Sort([](const FAssetInsightRootAsset& A, const FAssetInsightRootAsset& B)
	{
		if (A.PackageName != B.PackageName)
		{
			return A.PackageName.LexicalLess(B.PackageName);
		}

		return RootSourceToString(A.Source) < RootSourceToString(B.Source);
	});
}

FUnusedAssetScanResult FUnusedAssetScanner::ScanProjectUnusedAssets(const FUnusedAssetScanOptions& Options)
{
	FUnusedAssetScanResult Result;

	FAssetRegistryModule& ARM =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	IAssetRegistry& Registry = ARM.Get();

	TArray<FAssetData> AllAssets;
	GetAllProjectAssets(Registry, AllAssets);

	BuildReachableSet(Registry, Options, Result.RootAssets, Result.ReachableAssets);

	TSet<FName> DefaultMapRoots;
	TSet<FName> ManualRoots;
	TSet<FName> AlwaysCookRoots;
	TSet<FName> PrimaryAssetRoots;
	for (const FAssetInsightRootAsset& Root : Result.RootAssets)
	{
		switch (Root.Source)
		{
		case EAssetInsightRootSource::DefaultMap:
			DefaultMapRoots.Add(Root.PackageName);
			break;
		case EAssetInsightRootSource::ManualPath:
			ManualRoots.Add(Root.PackageName);
			break;
		case EAssetInsightRootSource::AlwaysCookDirectory:
			AlwaysCookRoots.Add(Root.PackageName);
			break;
		case EAssetInsightRootSource::PrimaryAsset:
			PrimaryAssetRoots.Add(Root.PackageName);
			break;
		default:
			break;
		}
	}

	Result.DefaultMapRootCount = DefaultMapRoots.Num();
	Result.ManualRootCount = ManualRoots.Num();
	Result.AlwaysCookRootCount = AlwaysCookRoots.Num();
	Result.PrimaryAssetRootCount = PrimaryAssetRoots.Num();

	Result.TotalScannedAssetCount = AllAssets.Num();
	Result.ProjectAssetCount = AllAssets.Num();
	Result.EngineAssetCount = 0;
	Result.ReachableAssetCount = Result.ReachableAssets.Num();

	for (const FAssetData& Asset : AllAssets)
	{
		if (!Asset.IsValid())
		{
			continue;
		}

		const FString ClassPath = Asset.AssetClassPath.ToString();
		if (ClassPath.Contains(TEXT("ObjectRedirector")))
		{
			continue;
		}

		FUnusedAssetItem Item = BuildItem(Asset);
		Item.bReachableByRoot = Result.ReachableAssets.Contains(Asset.PackageName);
		Item.PackageReferencerCount = CountReferencers(
			Registry,
			Asset.PackageName,
			UE::AssetRegistry::EDependencyCategory::Package,
			Options.bIncludeSoftPackageDependencies
				? UE::AssetRegistry::EDependencyQuery::NoRequirements
				: UE::AssetRegistry::EDependencyQuery::Hard);

		if (Options.bIncludeManageReferencers)
		{
			Item.ManageReferencerCount = CountReferencers(
				Registry,
				Asset.PackageName,
				UE::AssetRegistry::EDependencyCategory::Manage);
		}

		if (Options.bIncludeSearchableNameReferencers)
		{
			Item.SearchableNameReferencerCount = CountReferencers(
				Registry,
				Asset.PackageName,
				UE::AssetRegistry::EDependencyCategory::SearchableName);
		}

		const bool bHasAnyReferencer =
			Item.PackageReferencerCount > 0 ||
			Item.ManageReferencerCount > 0 ||
			Item.SearchableNameReferencerCount > 0;

		if (!Item.bReachableByRoot && !bHasAnyReferencer)
		{
			Result.Items.Add(Item);
		}
	}

	Result.UnusedAssetCount = Result.Items.Num();

	Result.Items.Sort([](const FUnusedAssetItem& A, const FUnusedAssetItem& B)
	{
		return A.DisplayName < B.DisplayName;
	});

	return Result;
}

void FUnusedAssetScanner::GetAllProjectAssets(IAssetRegistry& InAssetRegistry, TArray<FAssetData>& OutAssets)
{
	OutAssets.Reset();

	const FName GamePath(TEXT("/Game"));

	InAssetRegistry.GetAssetsByPath(
		GamePath,
		OutAssets,
		/*bRecursive*/ true,
		/*bIncludeOnlyOnDiskAssets*/ false
	);
}

FUnusedAssetItem FUnusedAssetScanner::BuildItem(const FAssetData& InAssetData)
{
	FUnusedAssetItem Item;

	Item.AssetData = InAssetData;
	Item.PackageName = InAssetData.PackageName;
	Item.DisplayName = InAssetData.AssetName.ToString();
	Item.bIsEngineAsset = InAssetData.PackagePath.ToString().StartsWith(TEXT("/Engine"));

	return Item;
}
