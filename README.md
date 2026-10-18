# Blueprint-X

A Satisfactory mod (SML 3.12, game CL 513885+) to create a blueprint from **any size** of
area: place two corner markers and save everything between them as a blueprint. The server
collects the buildings itself, so it works on dedicated servers without hitting network
message limits.

**Status:** early development (0.1.0 is an empty skeleton). Not released yet.

## Planned features

- Corner marker buildable; the box between two markers becomes the blueprint area.
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
