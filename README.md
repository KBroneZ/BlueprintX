# Blueprint-X

A Satisfactory mod (SML 3.12, game CL 513885+) to create a blueprint from **any size** of
area: place two corner markers and save everything between them as a blueprint. The server
collects the buildings itself, so it works on dedicated servers without hitting network
message limits.

**Status:** early development. Not released yet.

## Features

- **Blueprint-X Corner** marker (0.2.0): build menu > Special > Blueprint Designers; costs
  2 Iron Plates and is available from the start (no HUB milestone needed).

## Planned features

- The box between two corner markers becomes the blueprint area.
- Save from a small window (name, description, category) with a box preview.
- Works in single player and on dedicated servers (Windows client, Linux server).

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
