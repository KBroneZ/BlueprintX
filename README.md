# Blueprint-X

A Satisfactory mod (SML 3.12, game CL 513885+) to create a blueprint from **any size** of
area: place two corner markers and save everything between them as a blueprint. The server
collects the buildings itself, so it works on dedicated servers without hitting network
message limits.

**Status:** early development. Not released yet.

## Features

- **Blueprint-X Corner** marker (0.2.0): build menu > Special > Blueprint Designers; costs
  2 Iron Plates and is available from the start (no HUB milestone needed).
- **Save window** (0.4.0): interact (`E`) with a Corner, pick the other corner (nearest first),
  set the extra height, name and description, and save. A box outline shows the area.
- The server collects and saves the buildings; tested in single player and on a Linux
  dedicated server (1674 buildings in one blueprint, synced to other players).
- Debug chat command: `/bxsave <name> [h=<metres>]` saves between the two nearest Corners.

## Install (manual, until it is on ficsit.app)

The mod is a Game Feature plugin, so the folder goes one level deeper than older mods:

- Game (Steam): `<Satisfactory>/FactoryGame/Mods/GameFeatures/BlueprintX/BlueprintX.uplugin`
- Dedicated server: `<server>/FactoryGame/Mods/GameFeatures/BlueprintX/BlueprintX.uplugin`

The folder must be named exactly `BlueprintX`. Server and players need the same version.
Satisfactory Mod Manager does not know this mod yet and may remove it when it syncs a profile.

## Planned features

- Category selection in the window.
- Build gun mode to pick the two corners by aiming.

## Building

Set up the Satisfactory modding environment (https://docs.ficsit.app), place this folder in
`<SatisfactoryModLoader>/Mods/GameFeatures/BlueprintX`, build the editor and package with
Alpakit.

## Credits

- Concept of corner-defined areas: **Area Actions** by mircearoata (GPL-3.0).
- **Quick Blueprint** by SirDigby (GPL-3.0) showed that saving blueprints outside the Blueprint
  Designer is possible.
- Built on the Satisfactory Mod Loader (SML) and its Alpakit templates.

## License

GPL-3.0. See [LICENSE](LICENSE).
