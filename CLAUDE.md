# Europe Racer - native Mac-port (decomp)

Doel: Europe Racer (Davilex, v1.0.0.24, 13-04-2001, NL: "Europe Racing") **native** op Apple Silicon, volledig moddable, 60 fps. Zelfde aanpak als het zusterproject `../Vakantieracer`: static recompilation met SwARc (x86 -> C++ -> clang arm64), daarna functie voor functie herschrijven naar leesbare C. Lees eerst `../Vakantieracer/CLAUDE.md`: methode, tooling en regels gelden hier ook. Dit bestand noemt alleen wat anders is.

## Doel en takenlijst
- Volgorde: Vakantie Racer eerst speelbaar (dat bouwt de gedeelde lagen), dan pas hier echt beginnen. Voorbereiding (inventaris, Ghidra-analyse als er geen VR-build draait) mag nu al.

## Welke exe we porten (besloten 9 nov 2026: "gewoon de laatste pakken")
**`dl/nocd/spel.exe`**: de officiële NoCD van Davilex (`Disc/Europe-Racing_NoCD_Win_EN-FR-DE-ES-IT-NL.zip`, 15-06-2002, 2.334.720 bytes, sha256 `72c2de41...2f015f`). Alle adressen (`// 0x4xxxxx`), symbolen en mods verwijzen naar deze exe. Gebruik nooit de CD- of patch-exe als bron.
- Onbeveiligd: geen `stxt`-secties, normale code (entropie 6.66), echte importtabel van 192 functies, entry 0x5b2c70.
- Het is de laatste versie: de CD-exe (v1.0.0.24, april 2001) en de exe uit Patch 3 (`dl/patch3/f001c52f.bin`, uit de Wise-installer gehaald met een zlib-scan) zijn beide **SafeDisc**-verpakt (secties `stxt774`/`stxt371`, `.text` volledig versleuteld). De drie hebben exact dezelfde sectiegroottes (`.text` 1.977.987, `.rdata` 143.055, `.data` 1.028.488): Patch 3 is dezelfde gamecode in een nieuwere SafeDisc-verpakking voor XP, en de NoCD is die code zonder verpakking, met dezelfde datum als de patch.
- `secdrv.sys`/`drvmgt.dll` op de CD (de SafeDisc-driver) hebben we dus niet nodig.
- De exe nooit committen of publiceren, net als de rest van `iso/`.

## Platformoppervlak van de NoCD-exe
192 imports: `mss32` 51 (Miles), KERNEL32 91, USER32 30, WINMM 8, GDI32 7, DDRAW 3, DINPUT 1, ADVAPI32 1. Van de 141 niet-Miles-imports gebruikt Vakantie Racer er al 133. Nieuw in SwARc: `DeleteFileA`, `MoveFileA`, `MessageBoxA` en de console-functies (`GetConsoleMode`, `SetConsoleMode`, `PeekConsoleInputA`, `ReadConsoleInputA`, `GetNumberOfConsoleInputEvents`, waarschijnlijk alleen CRT). De echte nieuwe klus is Miles: 51 `AIL_*`-functies (samples, 3D-geluid, ASI-mp3-decoder, filters).
- 219 originele bronbestandnamen gelekt via asserts (`D:\work\Europa Racer\Code\Game\...`), in `re/source_files.txt`. Let op: het pad heet hier niet `RED\Game\prog` zoals bij VR.
- Strings die bij 60 fps helpen: `vchunks: %d  FPSMULT:%f`, `frameRate: %f Real: %f`, `fps = %f`. `FPSMULT` wijst op natuurkunde die met de framerate meeschaalt; uitzoeken vóór we op 60 fps vastzetten.

## Verschillen met Vakantie Racer (bekend uit de inventaris, 9 nov 2026)
| Onderdeel | Vakantie Racer | Europe Racer |
|---|---|---|
| Game-exe | `spel.dat`, onbeveiligd, 137 imports | `dl/nocd/spel.exe` (officiële NoCD), 192 imports, waarvan 51 Miles |
| Geluid | DirectSound direct | **Miles Sound System** (`Mss32.dll` + `redist/*.m3d` 3D-providers + `Mp3dec.asi`, 203 mp3's in het archief) |
| Launcher | `VRacer.exe` | `ER.dat` (PE, 80 imports, leest `App.Ini`: taal + `CreateProcess` van menu en game) |
| Menu | `MenuData/Menu.exe`, Delphi | `Menu/GameMenu.dat` (1 MB), **ook Delphi** (`CODE`/`DATA`/`BSS`-secties, DirectDraw + DirectSound + AVI), leest bestanden via `Menu/Rcs.dll` (`xvga_src_fopen` e.d.) |
| Levels | `etappe0-5` | `Etappe00-20` (21 etappes, `.cmp/.pmp/.ibi`, `Vdyns/`) |
| Talen | alleen NL | 6 talen in `Langdata/<taal>/` (Game: `RcsLang.*` + `ini/events.ini`; menu: `Voices.*`, `Language.ini`; Media: AVI's). De installer kopieert één taal naast `EuropeRacer/`; de port moet die samenvoeging zelf doen |

- **Archief is identiek**: `res/rcs.IND` leest direct met `../Vakantieracer/re/scripts/rcs_list.py` (6.364 entries: 5.295 `.d3d`-modellen, 459 tga, 404 wav, 203 mp3). De RED-laag is dus echt herbruikbaar.
- Zelfde datafamilie in `Game/`: `Cars0-2.dat`, `Phys0-2.dat`, `Bonus.dso`, `Ini/CAR_CFG.INI`, `car_snd.ini`, `anmtypes.ini`. Nieuw: `Cars.ini`, `atv.ini` (quad), `ANMOBJ*`/`CHRANM*`-animaties, `aigoals.ini`/`AIShortTerm.ini`/`behavior.ini`.

## Drie lagen (afspraak uit Vakantie Racer, hier de tweede gebruiker)
1. **Algemeen Windows/DirectX/Miles**: in SwARc (`../SwARc-dx5`, branch `legacy-dx5`), zonder gamenamen. Miles hoort hier ook: óf `Mss32.dll` mee-liften (x86 DLL), óf de gebruikte `AIL_*`-functies nabouwen op SDL. Het gaat om 51 `AIL_*`-functies (lijst: `python3 -I ../Vakantieracer/re/scripts/pe_imports.py dl/nocd/spel.exe`). Nabouwen op SDL houdt het native en moddable; mee-liften is sneller speelbaar maar dan zit Miles' eigen DirectSound-code erin.
2. **RED-engine** (archief, `.d3d`, ini-structuur, auto-/aanhangertabellen, Delphi-menu-onderdelen): generiek, naar `../Davilex-RED` zodra beide games het gebruiken. Tot die repo er is: importeer uit `../Vakantieracer/re/scripts/`, kopieer niet.
3. **Europe Racer-specifiek** (adressen in spel.exe, GameConfig, menuschermen, taalkeuze): alleen hier.
Bij elke nieuwe functie: vraag eerst in welke laag hij hoort.

## Menu (eracer-menu)
- Native vervanger van `Menu/GameMenu.dat` (Delphi). Leest bij elke start de formulieren (DFM), plaatjes (`Graphics.IMG`), geluid (`Music.IMG`, `Voices.IMG`), teksten (`Langdata/<taal>/menu/Language.ini`), layout (`Forms.ini` + `FormsA.ini`) en flow (`GameMenu.ini`) uit de eigen CD. Er staat geen speldata in git.
- **De engine is gedeeld** met Vakantie Racer en A2 Racer IV: `../Davilex-RED/menu/engine/` (git-repo, eigen commits). Wijzig de engine alleen daar. Overstapnotitie: `../Davilex-RED/menu/MIGRATION.md`. Testen in de originele look: `ERACER_MENU_UPGRADES=0 ERACER_MENU_SIZE=800x600`.
- Bouwen: `cmake --preset release` (eenmalig), dan `cmake --build --preset release --target eracer-menu` (licht). Starten: `out/build/release/eracer-menu --game-dir iso/EuropeRacer [--lang NL] [--mods DIR] [--script FILE] [--shot FILE]`. Script-regels: `<ms> click <component>`, `hover`, `key`, `type`, `shot <png>`, `quit`.
- Code: `er/skin.cpp` (plaatjesnamen die het origineel in code zet, met adressen), `er/game.cpp` (wizard, knopplaatsing per formulier, afsluitvraag), `er/flow.cpp` (FS_-flow), `er/gamedata.cpp` (GameMenu.ini-tabellen, taal). Bevindingen: `re/menu/findings.md`.
- Stand 9 nov 2026 (nacht): alle 55 formulieren bouwen. Gevuld met de echte plaatjes en teksten: hoofdmenu, Quick Race (type, moeilijkheid, track, auto met eigenschapsbalken, tegenstander), Championship en Avontuur (film, type, moeilijkheid, club, trackcollage, autokeuze, garage-aanbod), Opties (vier tabs met plaatjes per keuze), Spelerslijst, Internet, High score (tabs plus de tabellen uit `[SCORE_*]` met layout A en E). Code: `er/screens_qr.cpp`, `er/screens_ch.cpp`, helpers (radio-koppeling, carrousel, panelen) in `er/game.cpp`. Na de race (9 nov 2026, middag): uitslag uit `game.ini` (`er/race.cpp`), flow na de race, Quick Race-winnaar, Championship/Avontuur met kwalificatie, uitslag, stand, rondes, status en herstarten (`er/league.cpp`, `er/screens_result.cpp`, tabellen in `er/table.cpp`, scherp in 4K). Testen zonder race: `ERACER_RACE_RESULT=<ini>` (zie `re/menu/findings.md` > Na de race). `game.ini` voor de race (`er/gameini.cpp`, spec `re/menu/gameini_spec.md`) wordt geschreven naar de SwARc-overlay. Nog niet: de race starten (de game zelf is nog niet geport), Input/GFX/Sound in game.ini, high score-invoer en recordmeldingen, spelers opslaan, opties bewaren. Garage met prijzen en kopen is gebouwd (`re/menu/garage_spec.md`). Film per flow is op bestandsnaam gekoppeld, niet uit de code.

## Wat er is
- `Disc/` - de twee zips zoals gedownload (ISO-versie + Patch 3). Bron, niet aankomen.
- `dl/` - uitgepakt: `EuropeRacer.iso` (687 MB, ISO 9660, label `ER`), `ESPatch_3.EXE` (Wise-installer), `readme.txt`, `nocd/spel.exe` (**de exe die we porten**), `patch3/` (twee PE's uit de patch: de beveiligde spel.exe en een hulpprogramma van 160 KB).
- `re/source_files.txt` - 219 gelekte bronbestandnamen.
- `iso/` - inhoud van de CD (561 MB, 678 bestanden). Gekopieerd via `hdiutil attach` (bsdtar gaf een lege map). Hoofdmap van het spel: `iso/EuropeRacer/` (`ER.dat`, `App.Ini`, `Game/`, `Menu/`, `res/`).
- Nog niet: git-historie, Ghidra-project, symbolen, build. Volg bij opzetten exact de structuur van `../Vakantieracer` (`CMakeLists.txt`, `CMakePresets.json`, `lift.sh`, `run.sh`, `re/`, `symbols/`).

## Regels
- Originele data, ISO, patch en (ontsleutelde) exe's nooit committen of publiceren.
- Elke gedecompileerde functie: originele adres in een comment (`// 0x4xxxxx`).
- Verifieer gedrag tegen het origineel (screenshots/waarden), niet op gevoel.
