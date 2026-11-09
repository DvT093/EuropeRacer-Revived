# game.ini written by the menu before a race (spec)

Source: GameMenu.dat, unit `Modules\MdlGameInterface.pas`. Writer = `0x4981c4` (called via `0x497af4`: open ini `0x497654`, write, flush+free `0x4977d0` = TIniFile vtable+0x4c then Free).
Verified against the disassembly (objdump), not only the Ghidra C: several Ghidra outputs in `out_gi.c` drop arguments or the tail of a function (notably `0x499388`, whose real body runs to `0x4996d9`).

## 0. Conventions

- The menu opens an EXISTING file (`0x497654` errors `File "%s" not found`) and only overwrites the keys below through TIniFile (WritePrivateProfileString). Every other key in the template (`DAM0`, `VAL0`, `count`, `[LEAGUE]`, `COUNTRY04..07`, `Racer4/5`, `BEST TIMES`, ...) stays as it was, including stale `RACERS i=` lines for i >= NrOpponents from a previous run.
- File path = field self+0xC of TGameInterfaceModule. UNKNOWN where it is set; probably `[General] GameInterface=` of GameMenu.ini (string at 0x467c44), not traced.
- vtable: +4 WriteString, +0xC WriteInteger, +0x14 WriteBool (writes `1`/`0`). Integers are decimal.
- Section/key names are written exactly as below (Windows ini is case-insensitive, so `Sound` = template `[SOUND]`, `Nrlaps` = `Nrlaps`, `GameType` = `GAMETYPE`).
- `Random(n)` = `0x40343c` (0..n-1), `Randomize` = `0x403234` (assumed Delphi System RTL by use).
- P = current profile base = playerObj + 0xB10 * [playerObj+0x3760]. Global options live at playerObj+0x37xx (not per profile).
- GameType (`0x4a345c`, P+0xB1C): 0 QR, 1 CH, 2 AD.
- TRaceType (RTTI at 0x4a279a): 0 QR_TRACK, 1 QR_DUEL, 2 QR_TIMETRIAL, 3 QR_SUDDENDEATH, 4 QR_NORMAL, 5 QR_CHASE. QR uses 0-3 (menu choice), CH/AD rows use 4/5 (`Racetype` column).
- CarType: 0 CT_STANDARD, 1 CT_ATV, 2 CT_SPORT, 3 CT_DOWNLOADED. Clubs: 0 USA, 1 JAPAN, 2 RUSSIA, 3 GERMANY, 4 ENGLAND, 5 FRANCE, 6 ITALY, 7 HOLLAND. TUpgradeType: 0 US_NONE .. 3 US_STAGE3.

### Data the writer reads (GameMenu.ini, parsed records)
| Table | Record | Fields used |
|---|---|---|
| [TRACKS] (list idx = TrackID, `0x479c44`) | parser `0x4797a4` | +0x00 word = LT of 5th column (`LT_TRACKxx`), +0x14 RaceRegion, +0x18 PreferredCarType |
| [QUICKRACE] (`0x47a1a8(region, n)` = n-th QR row whose track has that region) | | +0 TrackID, +4 laps QR_TRACK, +8 laps DUEL, +0xC laps TIMETRIAL, +0x10 laps SUDDENDEATH (= +4+RaceType*4). The two opponent columns are never read by the writer. |
| [CHAMPIONSHIP] / [ADVENTURE] rows (parser `0x4792ac` / `0x4790ac`, 0x2C bytes) | | +0 TrackID, +4 NrOfLaps, +8 NrOfOpponents (ignored by writer), +0xC Racetype (unknown name -> 4), QR_NORMAL: +0x1C/+0x20/+0x24 = Qualify easy/normal/hard (positions), +0x10..0x18 = 0; QR_CHASE: +0x10/+0x14/+0x18 = Qualify * 1000 (ms), +0x1C.. = 0; +0x28 cops mask: bit0 HasNormalCops, bit1 HasEuroCops, bit2 HasSuperCop |
| CH row lookup `0x479e48(class, round)` | | the round-th CH row (0-based) whose track's PreferredCarType == class (rows are NOT addressed as class*5+round, but with the shipped ini this gives the same rows) |
| AD row lookup `0x479ca4(round)` | | AD row index = round |
| [CLUBS] (`0x49ee64(club, carType)`, record stride 0x9C) | | car ids: col 7 -> CT_STANDARD, col 8 -> CT_SPORT, col 9 -> CT_ATV (parser `0x49eb74` stores 1st/2nd/3rd id at +0x1C/+0x24/+0x20). +0x14 word = LT_ED_xx (driver name). |
| [CARS] (`0x4a5c80(type, n)`) | | n-th car (in ini order) with CarType == type; returns record, +0 = CarID |
| [TWEAK] | | `QR_UPGRADE` (enum name, default `US_NONE`; shipped `US_STAGE3`), `QR_TIMETRIAL_NR_LAPS` (default 5; shipped 3) |

## 1. Write order (0x4981c4)
1. `0x49759c` start-slot shuffle (no write): `Randomize; used[0..4]=0; for i=0..4: r=Random(4); repeat up to 5x: if !used[r] {used[r]=1; break} else r=(r+1)%5; slot[i]=r` (stored self+0xC4). slot[0] is never written (see UNKNOWN 1).
2. `0x4987d4` player block. 3. `0x49902c` NrOpponents + RACERS. 4. `0x499388` race options + BONUS. 5. `0x49a2d0` input. 6. `0x499894` resolution + GFX. 7. `0x499ad0` DIRECTX. 8. `0x499c0c` Sound. 9. `0x499df0` GameSim. Then self+0x14 := 1.

## 2. [PLAYER] / [ETAPPE] / [UPGRADES] PLAYER (0x4987d4 -> 0x498680 QR / 0x4984c8 CH / 0x4985a4 AD -> common 0x498310)

| Section | Key | Type | QR (0x498680) | CH (0x4984c8) | AD (0x4985a4) |
|---|---|---|---|---|---|
| PLAYER | Name | str | profile name (ShortString at P+0x10, `0x4a3524`) | same | same |
| PLAYER | Finished | bool | 0 | 0 | 0 |
| PLAYER | Money | int | 0 | 0 | 0 |
| ETAPPE | Etappe | int | track id = P+0x9F8 (`0x4a4c90`), the [TRACKS] id, no remapping | CH row(class, round).TrackID | AD row(round).TrackID |
| ETAPPE | Name | str | localized text of TRACKS[track]+0 (`LT_TRACKxx`) via language module `0x4a0be0` (global+0x40) | same | same |
| PLAYER | car | int | CarID = `0x4a5c80(class P+0x9D8, index P+0x9DC)` (n-th [CARS] row of the chosen class) | `[CLUBS][club].car[class]`, club = rec+0x1D4, class = P+0x7DC | `[CLUBS][club].car[class]`, club = AD rec+0x1D4, class = AD rec+0x1D8 |
| PLAYER | country | int | club owning that CarID (`0x49ee98`, cached P+0x9D4) | rec+0x1D4 (player club) | AD rec+0x1D4 |
| PLAYER | damage | int | 0 | rec[club*56 + class*8 + 0] | AD rec[club*56 + class*8 + 0] |
| PLAYER | TURBO_AMOUNT | int | 100 | rec[club*56 + 0x34] (reset value 100) | AD rec[club*56 + 0x34] |
| UPGRADES | PLAYER | int | GetEnumValue(TUpgradeType, [TWEAK] QR_UPGRADE) | (int8) rec[club*56 + class*8 + 4] | (int8) AD rec[club*56 + class*8 + 4] |

rec = CH record of the current class = P + 0x2C + class*0x1EC (`0x4a335c`); AD rec = P + 0x7E0 (`0x4a3328`). Per-club block = 56 bytes: 4 x {damage, upgrade} per CarType, +0x20 money, +0x24 points, +0x28 rank, +0x2C time, +0x30 ?, +0x34 turbo.

## 3. [OPTIONS] NrOpponents + [RACERS] / [UPGRADES] RacerN (0x49902c)

`NrOpponents` (`0x4a34ac`) depends only on RaceType, NOT on the ini opponent columns:

| RaceType | TRACK | DUEL | TIMETRIAL | SUDDENDEATH | NORMAL | CHASE |
|---|---|---|---|---|---|---|
| NrOpponents | 4 | 1 | 0 | 4 | 4 | 0 |

Error if <0 or >4. Per racer i (`0x4988ec`), 0-based:

| Section | Key | Value |
|---|---|---|
| RACERS | `i` (`%d`) | `"%d,%d,%d"` = carId, slot[i+1] (from step 1), 0 |
| RACERS | `country%.2d` (i) -> `country00..03` | club of that racer (int8) |
| UPGRADES | `Racer%d` (i) -> `Racer0..3` | upgrade (int8) |

How carId / club / upgrade are chosen:

| Mode | Who | carId | country | upgrade |
|---|---|---|---|---|
| QR TRACK / SUDDENDEATH (`0x498c80`) | 4 distinct random clubs != player club (`used[8]`, used[playerClub]=1, `r=Random(8)`, probe `(r+1)%8` up to 8x); first NrOpponents written | `[CLUBS][club].car[TRACKS[track].PreferredCarType]` | that club | QR_UPGRADE (same as player) |
| QR DUEL, opponent type P+0x9E8 = 0 (club driver) | opponent club P+0x9E4 (`0x4a3fc0`) | type = track PreferredCarType; if opponentClub == playerClub AND player class == that type: type := random other one (0 -> 1/2, 1 -> 0/2, 2 -> 1/0, Random(2)). car = `[CLUBS][opp].car[type]` | opponent club | 0 |
| QR DUEL, opponent type = 1 (police chief) | | 44 (0x2C, not in [CARS]) | 2 | 0 |
| QR DUEL, other opponent type | nothing written | | | |
| QR TIMETRIAL | nothing written (NrOpponents 0) | | | |
| CH (`0x498a90`) | rec+0x1C0[i] (4 clubs drawn at reset by `0x4a2a50`: same random algorithm, never the player club) | `[CLUBS][opp].car[TRACKS[track].PreferredCarType]` | opp | (int8) rec[opp*56 + playerClass*8 + 4] |
| AD (`0x498b88`) | AD rec+0x1C0[i] (`0x4a2b1c`) | same as CH | opp | (int8) AD rec[opp*56 + ADclass*8 + 4] |

Opponent damage (rec[opp*56+class*8]) is read in CH/AD but not written.

## 4. Race options + BONUS (0x499388, real end 0x4996d9)

| Section | Key | Value | Source |
|---|---|---|---|
| OPTIONS | Difficulty | int 0/1/2 (easy/normal/hard) | `0x4a3df4`: QR P+0x9F0, CH rec+0x1D8, AD rec+0x1DC |
| OPTIONS | GameType | 0/1/2 | P+0xB1C |
| OPTIONS | RaceMode | TRaceType 0..5 | `0x4a4400`: QR P+0x9EC, CH row(class,round)+0xC, AD row(round)+0xC |
| OPTIONS | Nrlaps | laps | QR TIMETRIAL: [TWEAK] QR_TIMETRIAL_NR_LAPS (def 5). Else `0x4a462c`: QR = QR row(region P+0x9F4, index P+0x9E0) field 1+RaceType; CH/AD = row+4 |
| PLAYER | lapcount | same laps | |
| PLAYER | `lap_%d_time` for d = 1..laps | 2147483647 (0x7FFFFFFF) | |
| OPTIONS | LocalCops | 0/1 | QR: RaceType == QR_TRACK; CH/AD: row+0x28 bit0 (`0x4a524c`) |
| OPTIONS | EuroCops | 0/1 | QR: RaceType == QR_TRACK; CH/AD: bit1 (`0x4a5334`) |
| OPTIONS | SuperCops | 0/1 | QR: 0; CH/AD: bit2 (`0x4a541c`) |
| OPTIONS | Traffic / Opponents / Pedestrians / ghostcar | 0/1 | table below (`0x499230`) |
| BONUS | RepairBonus / MoneyBonus / TurboBonus / InvulnerableBonus | 0/1 | bits 1/2/4/8 of the mask below |

| RaceType | Traffic | Opponents | Pedestrians | ghostcar | bonus mask (Repair,Money,Turbo,Invuln) |
|---|---|---|---|---|---|
| 0 TRACK | 1 | 1 | 1 | 0 | 0x0D = 1,0,1,1 |
| 1 DUEL | 0 | 1 | 1 | 0 | 0x0D = 1,0,1,1 |
| 2 TIMETRIAL | 0 | 0 | 1 | 1 | 0x05 = 1,0,1,0 |
| 3 SUDDENDEATH | 0 | 1 | 1 | 0 | 0x0D = 1,0,1,1 |
| 4 NORMAL | 1 | 1 | 1 | 0 | 0x0F = 1,1,1,1 |
| 5 CHASE | 1 | 0 | 1 | 0 | 0x0F = 1,1,1,1 |

Pedestrians is forced to 0 when global option playerObj+0x379C == 1 (`0x4a5978`, a "pedestrians off" setting; which menu control sets it is UNKNOWN).

## 5. [InputGeneral] / [InputKeyBinds] (0x49a2d0)

dev = P+0xA04 (`0x4a557c`).

| Section | Key | Value |
|---|---|---|
| InputGeneral | InputDevice | table 0x4c80b4[dev]: 0 `Keyboard`, 1 `DigitalController`, 2 `AnalogController`, 3 `AnalogController` |
| InputGeneral | Sensitivity | int P+0xA44+dev*0x40 (`0x4a581c`) |
| InputGeneral | ForceFeedback | (P+0xA40+dev*0x40 != 0) ? 1 : 0 (`0x4a57d8`) |
| InputGeneral | ForceFeedbackLevel | only if that value != 0: (int8)value - 1 |
| InputKeyBinds | MoveForward, MoveBack, TurnLeft, TurnRight, HandBrake, Turbo, Horn, Reset, Escape, Camera1..Camera5 (14, table 0x4c80c4) | `0x49a1bc`: b = binding P+0xA08+dev*0x40+key*4; code = scanTable[b].code (table PTR 0x4c8d64, 12-byte entries, 0x71 entries); if code > 0: comma-joined TScanCodes enum names (RTTI 0x4a18c4) of every entry with that code, else empty string |

Camera6..10 in the template are not written. The TScanCodes name list (e.g. `KB_Up`, `JS_Left`) is in the RTTI at 0x4a18c4; not dumped here.

## 6. Resolution + [GFX OPTIONS] (0x499894), all global (playerObj+...)

| Section | Key | Value |
|---|---|---|
| OPTIONS | Resolution | table 0x4c810c[ [+0x3768] ] = {3, 4, 7} for index 0/1/2 |
| GFX OPTIONS | mipmapping | [+0x3770] |
| GFX OPTIONS | gamma | [+0x376C] |
| GFX OPTIONS | particles | 1 - [+0x3798] (inverted) |
| GFX OPTIONS | FILTER | [+0x3778] == 0 ? 0 : 2 |
| GFX OPTIONS | alpha_blending | [+0x377C] |
| GFX OPTIONS | WaitVBL | [+0x3784] |

## 7. [DIRECTX] (0x499ad0)
| Key | Value |
|---|---|
| Index | [+0x3764] (`0x4a5940`) |
| Name | Strings[Index] of the device list in the DirectX module (global+0x4C, `0x4806a8`) |
| guid | Objects[Index] converted to text by `0x480530`. Exact text format UNKNOWN (not decompiled). |

## 8. [Sound] (0x499c0c)
| Key | Value |
|---|---|
| MUSICVOLUME / SFXVOLUME / POLICEVOLUME | per profile: P+0xB10 / P+0xB14 / P+0xB18 |
| hardware | (int8) global [+0x3788] |
| system | (int8) global [+0x378C] |
| SoundQUality | (int8) global [+0x3790] |
| ON | global [+0x3794] |

`hardware` and `system` do not exist in the template (it has `SpeakerType`/`SoundSystem`); the menu adds them. Whether the game reads them is UNKNOWN.

## 9. [GameSim] Qualify (0x499df0)

| GameType + RaceMode | Qualify (string) |
|---|---|
| CH, QR_NORMAL | IntToStr(row+0x1C+Difficulty*4) = Qualify-Easy/Normal/Hard position, row = first CH row with TrackID == track (`0x479fa4`) |
| CH, QR_CHASE | `0x4680c8`(row+0x10+Difficulty*4 ms) = time text (mm:ss:hh per findings.md) |
| AD | same with AD row by TrackID (`0x479d08`) |
| QR (RaceMode 0-3) or anything else | `-` |

## 10. Keys in the template NOT written by the menu
`[RACERS] count`, `DAMn`, `VALn`, `COUNTRY04..07`, `[UPGRADES] Racer4/5`, `[ETAPPE] british`, `[PLAYER] tracktime/position/copland/radarbonus/etappeNN_*`, `[OPTIONS] Invulnerable/BonusCountDown/fog/...`, `[LEAGUE] league`, `[InputKeyBinds] Camera6..10`, `[InputGeneral] ForceFeedback*Level`.

## 11. UNKNOWN
1. Player start slot: slot[0] of the shuffle is computed but never written; how the game places the player is unknown. It is also an assumption that the middle RACERS value is the start position (template uses 0..5 in order; after the race the game writes the finishing position there).
2. Ini file path (self+0xC) not traced.
3. DIRECTX `guid` text format.
4. AD car class (AD rec+0x1D8): whether the AD flow updates it per round to the track's car type is not checked; the writer just uses it for the player car and the opponents' upgrade index, while opponents' cars use the track's PreferredCarType.
5. Which menu controls set the global flags +0x3774 (unused here), +0x379C (pedestrians off), +0x3788/+0x378C.
6. TScanCodes names and scan table contents (0x4c8d64) not dumped.
7. The meaning of the race type `QR_SUDDENDEATH` for the game side (menu treats it like TRACK without traffic).

## 12. Getters identified
| Addr | Meaning | Evidence |
|---|---|---|
| 0x4a3328 | AD record = P+0x7E0 | disasm `lea [edx+8*eax+0x7e0]` |
| 0x4a335c | CH record of current class = P+0x2C+class*0x1EC | disasm, class from profile+0x7CC |
| 0x4a3404 | copy whole profile (0xB10 bytes from P+0x10) to buffer | `rep movsd 0x2c4` |
| 0x4a345c | GameType P+0xB1C | disasm; switch 0/1/2 in all writers |
| 0x4a3490 | playerObj+0x37A0 (set by 0x4a4930) | disasm, unused by writer |
| 0x4a34ac | NrOpponents by RaceType {4,1,0,4,4,0} | jump table 0x4a34cb |
| 0x4a3524 | profile name (ShortString P+0x10) | copies profile, string from buf+1 |
| 0x4a3554 | GetCurrentTrackIndex: QR P+0x9E0 (index within region in [QUICKRACE]) | error string; 2nd arg of 0x47a1a8 |
| 0x4a36c0 | GetCurrentCarIndex: QR P+0x9DC | error string; arg of 0x4a5c80 |
| 0x4a3828 | GetCurrentCarType: QR P+0x9D8, CH P+0x7DC, AD P+0x9B8 | error string, disasm |
| 0x4a3a54 / 0x4a3c48 | GetCurrentCarInfo / GetCurrentRacerInfo | error strings, not used by writer |
| 0x4a3df4 | GetCurrentDifficulty: QR P+0x9F0, CH rec+0x1D8, AD P+0x9BC (AD rec+0x1DC) | error string; index into Qualify[3] |
| 0x4a3fc0 | GetCurrentOpponent (duel club): QR P+0x9E4 | error string; country of duel racer |
| 0x4a4128 | GetCurrentOpponentType: QR P+0x9E8, 0 club / 1 police | error string; police branch writes car 44 |
| 0x4a4294 | GetCurrentRaceregion: QR P+0x9F4 | error string; 1st arg of 0x47a1a8 |
| 0x4a4400 | GetCurrentRaceType: QR P+0x9EC, CH/AD row+0xC | error string; TRaceType RTTI |
| 0x4a462c | GetCurrentNrOfLaps | error string; row+4 / QR row[1+type] |
| 0x4a4870 | QR record base P+0x9D4 | disasm |
| 0x4a4a4c | GetCurrentClub: QR club of car (cached P+0x9D4), CH rec+0x1D4, AD P+0x9B4 | error string |
| 0x4a4c90 | GetCurrentTrack: QR P+0x9F8, CH row.TrackID (cached rec+0x1E0), AD row.TrackID (cached P+0x9C4) | error string; written as Etappe |
| 0x4a524c / 0x4a5334 / 0x4a541c | Local / Euro / Super cops | bit test row+0x28 &1/&2/&4 |
| 0x4a557c | input device P+0xA04 | index into device-name table |
| 0x4a55b0 | resolution index playerObj+0x3768 | index into {3,4,7} |
| 0x4a55cc / 0x4a5600 / 0x4a5634 | music / sfx / police volume P+0xB10/B14/B18 | Sound keys |
| 0x4a569c | key binding P+0xA08+dev*0x40+key*4 | disasm |
| 0x4a57d8 / 0x4a581c | force feedback P+0xA40+dev*0x40 / sensitivity P+0xA44+dev*0x40 | disasm |
| 0x4a5860..0x4a59b0 | globals +0x3788 hardware, +0x378C system, +0x3778 filter, +0x376C gamma, +0x3770 mipmap, +0x3784 vbl, +0x3774 ?, +0x377C alpha, +0x3764 dx index, +0x3798 particles-off, +0x379C pedestrians-off, +0x3790 sound quality, +0x3794 sound on | `add eax, 0x37xx` per getter, mapped by the key it feeds |
| 0x4a5c80 | n-th car of a CarType in [CARS] | loop compares rec+4 with type |
| 0x49ee64 / 0x49ee98 | club car for CarType / club owning CarID | disasm, stride 0x9C |
| 0x479c44 / 0x479ca4 / 0x479e48 / 0x479fa4 / 0x479d08 / 0x47a1a8 | TRACKS[i] / AD row i / CH row (class, round) / CH row by track / AD row by track / QR row (region, n) | out_rank.c + disasm |
