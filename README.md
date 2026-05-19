# AssetInsight

AssetInsight is an Unreal Engine plugin that provides three main features:

## Features

1. **Unused Assets**
   - Scan project assets that are not referenced anywhere.
   - Display Root Packages and Likely Unused Assets.
   - Highlight unreferenced assets in red.
   - Options for AlwaysCook, AssetManager, and SearchableName references.
   - Default display shows asset name only for clarity.

2. **Dependency Tree**
   - Visualize dependencies of a given asset.
   - Supports hard references, soft references, and Blueprint type references.
   - Tree view shows hierarchical structure of asset dependencies.

3. **Referencer Tree**
   - Visualize all assets that reference a given asset.
   - Supports hard references, soft references, and Manage references.
   - Shows type and source of each referencer.

## Installation

1. Copy `AssetInsight` folder into `YourProject/Plugins/`
2. Enable the plugin in Unreal Editor
3. Restart Unreal Editor

## Usage

1. Open AssetInsight from the Editor menu.
2. Select the feature (Unused Assets, Dependency Tree, Referencer Tree).
3. Configure options and scan.
4. Review results in the UI.

## Screenshots

*Insert screenshots of UI for each feature*

## Contributing

- Contributions welcome via pull requests.

## License

- MIT License