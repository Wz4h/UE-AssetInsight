#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FAssetData;
struct FTopLevelAssetPath;

DECLARE_DELEGATE_OneParam(FOnRootAssetsPicked, const TArray<FAssetData>&);

class SRootAssetPickerDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRootAssetPickerDialog) {}
		SLATE_ARGUMENT(FText, TitleText)
		SLATE_ARGUMENT(bool, bAllowMultiple)
		SLATE_EVENT(FOnRootAssetsPicked, OnAssetsPicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnChooseAssetsClicked();
	EActiveTimerReturnType OnAutoOpenPicker(double, float);

private:
	FText TitleText;
	bool bAllowMultiple = true;
	FOnRootAssetsPicked OnAssetsPicked;

	TArray<FAssetData> SelectedAssets;

	TWeakPtr<class SWindow> ParentWindow;
};
