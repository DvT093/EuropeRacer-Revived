# Players, Options and persistence in GameMenu.dat (spec)

Source: GameMenu.dat (Delphi 5). Read from the disassembly (llvm `objdump -d` on the PE, VA addresses) plus `out_methods.c` / `out_score.c` / `out_gi.c`, `fields.json` (published field offsets), `dfm.json`, `Forms.ini` / `FormsA.ini`, `lt_enum.txt`, NL `Language.ini`. No Ghidra run, no builds.
Offset conventions as in `gameini_spec.md`: `obj` = player database object (global+0x18, class `TDatabaseModule`, instance size 0x37A8). Profile i record `R(i) = obj + 0x10 + i*0xB10`. `P = obj + 0xB10*[obj+0x3760]` (so `R = P + 0x10`; the P column below matches gameini_spec).

## 1. Persistence

### 1.1 Files
| File | Name built by | Path | Content |
|---|---|---|---|
| Player database | `0x4679b0`: `ChangeFileExt(ExtractFileName(Application.ExeName), '.EUR')` | **relative**, current directory (no path prepended) -> `GameMenu.EUR` | header + raw dump of obj+0x10 .. obj+0x37A8 |
| High scores | `0x467a2c`: same with `'.HS'` | relative -> `GameMenu.HS` | header + raw dump of score object +0x10 .. +0x10B8 |
| GameMenu.INI | `0x467b50`: `ExtractFilePath(ExeName) + ChangeFileExt(ExtractFileName(ExeName), '.INI')` | next to the exe | read-only for players/options, see 1.5 |

Which directory is current when the launcher starts the menu: UNKNOWN (not traced; probably the Menu folder).

### 1.2 GameMenu.EUR format (load `0x4a30e4`, save `0x4a3244`)
Untyped file, record size 1, little endian.

| Offset | Size | Content |
|---|---|---|
| 0 | 9 | ShortString `"\x08ERDBv0.0"` (DATA 0x4c8744) |
| 9 | 0x3798 (14232) | obj+0x10 .. obj+0x37A7 verbatim: 5 profiles (5 x 0xB10) then globals obj+0x3760..0x37A7 (0x48 bytes) |

Total 14241 bytes. Load: file missing -> defaults (`0x4a29bc`). Header differs -> defaults + `ShowMessage('database incorrect version.')`. Header ok -> BlockRead 0x3798 bytes, no further checks.

### 1.3 When
| Event | Code | What |
|---|---|---|
| Program start | `TManager` start `0x4616e0` -> LoadModules `0x461714` -> virtual Load (VMT slot 0x4a29a4 / 0x495650) | both `.EUR` and `.HS` read once |
| Program end | `TManager.Destroy 0x46130c` -> SaveModules `0x461804` -> virtual Save (0x4a29a8 / 0x495654) | both files rewritten (Rewrite = truncate) once |
No other save call exists. Every race restarts the menu (game.ini protocol), so a normal exit before a race saves. A crash loses all changes of that session.

### 1.4 GameMenu.HS format (load `0x49683c`, save `0x4969a4`, MdlScores)
Header ShortString `"\x08ERHSv0.3"` (DATA 0x4c80a8) + 0x10A8 bytes (score obj+0x10..+0x10B8). Missing file -> clear (`0x495878`) then load defaults from GameMenu.ini `[SCORE_*]` (`0x495c40`). Wrong header -> clear only (empty tables) + same ShowMessage. So `[SCORE_*]` in GameMenu.ini are factory defaults, never written.

| Score obj offset | Table | Layout |
|---|---|---|
| +0x010 | SCORE_ADVENTURE, 5 x 0x28 | rec: +0 name ShortString[8] (9 B), +0x0C score, +0x10 position, +0x14 club (Random(7) at ini load), +0x18 TDateTime (Now), +0x20 total time ms (ini load: 0x05265818) |
| +0x0D8 | SCORE_CC_ATV, 5 x 0x28 | same |
| +0x1A0 | 5 x 0x28, never loaded or cleared | UNKNOWN (probably CT_DOWNLOADED) |
| +0x268 | SCORE_CC_SPORT, 5 x 0x28 | same |
| +0x330 | SCORE_CC_STANDARD, 5 x 0x28 | same |
| +0x3F8 | SCORE_LAP, 4 regions x 0x198 | region = TRACKS[track].RaceRegion; +0 byte count, then 10 x 0x28 at +8: +0 track id, +4 name ShortString[8], +0x10 car type (Random(3) at ini load), +0x18 TDateTime, +0x20 time ms |
| +0xA58 | SCORE_TRACK, 4 x 0x198 | same |
How a race result is inserted into these tables: UNKNOWN (not traced).

### 1.5 GameMenu.ini and registry
| Key | Read/write | Note |
|---|---|---|
| `[General] CurrentPlayer`, `PrevState` | never | the strings do not occur in GameMenu.dat; current player lives in GameMenu.EUR |
| `[TWEAK] QR_UPGRADE` | read at profile reset | copied to R+0x9F0 |
| `[SCORE_*]` | read only when GameMenu.HS is missing | |
| `[General] ExitCode` | written (launcher protocol, `0x497960`: 255/40/50/60 by state; `0x4a63b0`: 255 on start-up failure) | outside this spec |
| Registry | no writes (only RegOpenKeyEx/RegQueryValueEx/RegCloseKey imported; only the Delphi RTL `FPUMaskValue` key string present) | |

## 2. Data layout

### 2.1 Globals (obj+..., stored in .EUR)
| obj+ | Meaning | Default (`0x4a29bc`) | Set by control | game.ini (gameini_spec) |
|---|---|---|---|---|
| 0x3760 | current profile index 0..4 | 0 | player list OK / new player | - |
| 0x3764 | DirectX device index | 0 | cboVideoCard (Display) | [DIRECTX] Index/Name/guid |
| 0x3768 | resolution 0 low / 1 med / 2 high | 0 | btnLowRes/MedRes/HighRes | [OPTIONS] Resolution = {3,4,7}[v] |
| 0x376C | brightness 0 light / 1 dark | 0 | btnLight/btnDark | [GFX OPTIONS] gamma |
| 0x3770 | mipmapping 0/1 | 0 (not in defaults) | chkMIPMapping | mipmapping |
| 0x3774 | texture detail 0 min / 1 normal / 2 max | 0 (not in defaults) | btnMinimal/Normal/Maximal | not written |
| 0x3778 | bilinear filter 0/1 | 1 | chkBilinearFiltering | FILTER = v ? 2 : 0 |
| 0x377C | transparency 0/1 | 1 | chkTransparancy | alpha_blending |
| 0x3780 | ? | 1 | none found | none. UNKNOWN |
| 0x3784 | wait for VBL 0/1 | 0 | chkSynchronize | WaitVBL |
| 0x3788 | speakers 0 two / 1 four / 2 headphones | 0 (not in defaults) | btnTwoSpeakers/FourSpeakers/Headphones | [Sound] hardware |
| 0x378C | sound system 0 normal / 1 EAX / 2 A3D | 0 (not in defaults) | btnNormal/btnEAX/btnA3D | [Sound] system |
| 0x3790 | sound quality 0 low / 1 med / 2 high | 0 | btnLow/Med/High | SoundQUality |
| 0x3794 | sound on 0/1 | 1 | chkSoundEnabled | ON |
| 0x3798 | hide particles 0/1 | 0 (not in defaults) | chkHideParticles | particles = 1 - v |
| 0x379C | hide pedestrians 0/1 | 0 (not in defaults) | chkHidePedestrians | Pedestrians forced 0 when 1 |
| 0x37A0 | session/play mode: 0 = welcome pending, 1 = normal, 2 = competition won | 0 | `/FIRSTTIME` sets 0; main window show sets 1; ranking sets 2 | - |
| 0x37A4 | ? | 0 | none found | UNKNOWN |
"Not in defaults" = the reset never writes it; a fresh object is zero-filled, a corrupt-header reset keeps nothing else either (object is new), so 0.

### 2.2 Profile record R (0xB10 bytes)
| R+ | P+ | Size | Field | Profile reset `0x4a2dac` | New player `0x4a2fd4` |
|---|---|---|---|---|---|
| 0x000 | 0x010 | 1 | empty flag: 1 = free slot, 0 = in use | 1 | 0 |
| 0x001 | 0x011 | 9 | name, ShortString[8] | '' | name (truncated to 8) |
| 0x010 | 0x020 | 8 | entry date, TDateTime double | `Date` (no time) | caller's `Now` |
| 0x018 | 0x028 | 1 | byte flag, never read | 1 | 1 |
| 0x019 | 0x029 | 1 | byte flag, never read | 1 | 1 |
| 0x01C | 0x02C | 4 x 0x1EC | CH record per CarType 0..3 (2.3) | each `0x4a2be8` | via reset |
| 0x7CC | 0x7DC | 4 | CH current car class | 0 | |
| 0x7D0 | 0x7E0 | 0x1F4 | AD record (2.3) | `0x4a2cb8` | |
| 0x9C4..0x9EC | 0x9D4..0x9FC | 4 each | QR: cached club, car class, car index, track index, duel club, opponent type, race type, difficulty, region, track id, ? | all 0 except difficulty (R+0x9E0) = 1 | |
| 0x9F0 | 0xA00 | 4 | QR upgrade = GetEnumValue([TWEAK] QR_UPGRADE, default US_NONE); not read back (writer re-reads the ini) | ini | |
| 0x9F4 | 0xA04 | 4 | input device 0 keyboard / 1 gamepad / 2 joystick / 3 wheel | 0 | |
| 0x9F8 | 0xA08 | 4 x 0x40 | per device d: +d*0x40 + k*4 binding k = 0..13 (TInputCommand), +0x38 force feedback 0..3, +0x3C sensitivity 0..100 | **not touched** | `0x4a56e8`: defaults for all 4 devices (2.4), FF 0, sensitivity 50 |
| 0xAF8 | 0xB08 | 4 | ? never read | 10 | |
| 0xAFC | 0xB0C | 4 | show controls at race start 0/1 (reset writes the low byte only) | 1 | |
| 0xB00 / 0xB04 / 0xB08 | 0xB10 / 0xB14 / 0xB18 | 4 each | music / effects / voices volume 0..100 (voices -> game.ini POLICEVOLUME) | 50 / 50 / 50 | |
| 0xB0C | 0xB1C | 4 | GameType 0 QR / 1 CH / 2 AD (main window radios) | **not reset** (stale value of the slot's previous owner) | |

### 2.3 CH record (0x1EC) and AD record (0x1F4)
Per-club block c = 0..7 at rec + c*56: +0x00 + type*8 damage, +0x04 + type*8 upgrade (4 CarTypes), +0x20 money, +0x24 points, +0x28 rank, +0x2C time, +0x30 ?, +0x34 turbo. Reset: all 0, turbo 100.

| Field | CH rec+ | CH reset | AD rec+ | AD reset |
|---|---|---|---|---|
| 4 opponent clubs (random, never player club) | 0x1C0 | `0x4a2a50` | 0x1C0 | `0x4a2b1c` |
| player club | 0x1D4 | 0 | 0x1D4 | 0 |
| car class | (profile R+0x7CC) | | 0x1D8 | 0 |
| difficulty | 0x1D8 | 1 | 0x1DC | 1 |
| byte flag (meaning UNKNOWN) | 0x1DC | 1 | 0x1E0 | 1 |
| cached track | 0x1E0 | not reset | 0x1E4 (P+0x9C4 per gameini_spec) | not reset |
| round | 0x1E4 | 0 | 0x1E8 | 0 |
| ? | 0x1E8 | not reset | 0x1F0..0x1F3 (4 bytes) | 0 |
Note: the reset sets the player club to 0 first and then draws the opponents, so at reset club 0 (USA) is excluded. Whether the draw is redone after the player picks a club: not checked.

### 2.4 Default key bindings (`0x4a5710`, table DATA 0x4c8664, 14 dwords per device)
Index k = game.ini key order: 0 MoveForward, 1 MoveBack, 2 TurnLeft, 3 TurnRight, 4 HandBrake, 5 Turbo, 6 Horn, 7 Reset, 8 Escape, 9..13 Camera1..5.

| Device | 0..3 | 4 | 5 | 6 | 7 | 8 | 9..13 |
|---|---|---|---|---|---|---|---|
| 0 keyboard | KB_UP, KB_DOWN, KB_LEFT, KB_RIGHT | KB_LEFTALT | KB_LEFTCONTROL | KB_SPACE | KB_ENTER | KB_ESCAPE | KB_1..KB_5 |
| 1 gamepad | JS_UP, JS_DOWN, JS_LEFT, JS_RIGHT | JS_BUTTON1 | JS_BUTTON2 | JS_BUTTON3 | KB_ENTER | KB_ESCAPE | KB_1..KB_5 |
| 2 joystick, 3 wheel | JS_UP, JS_DOWN, JS_LEFT, JS_RIGHT | KB_LEFTALT | KB_LEFTCONTROL | KB_SPACE | KB_ENTER | KB_ESCAPE | KB_1..KB_5 |
Values are TInputCommand ordinals (RTTI 0x4a18c8, 0..112; KB_UP 78, KB_DOWN 79, KB_LEFT 76, KB_RIGHT 77, KB_LEFTALT 41, KB_LEFTCONTROL 30, KB_SPACE 43, KB_ENTER 29, KB_ESCAPE 12, KB_1..5 13..17, JS_LEFT 0, JS_RIGHT 1, JS_UP 2, JS_DOWN 3, JS_BUTTON1..3 4..6). Ordinal 112 (0x70) = "no key".

## 3. Players

### 3.1 Rules
| Item | Value |
|---|---|
| Profiles | 5 fixed slots |
| Name | 1..8 characters (TEdit MaxLength 8; stored ShortString[8]); empty -> LT_MSG_NONAME; any character accepted (no key filter); duplicates **not** checked (LT_MSG_NAMEERROR is never referenced) |
| "No players" test `0x4a48dc` | all 5 empty flags set |
| Current player | obj+0x3760; every getter uses it |

### 3.2 Start-up (main window show `0x4ab76c`, runs only while obj+0x37A0 == 0)
`/FIRSTTIME` (`0x493f84`) sets obj+0x37A0 = 0 and starts flow 15 = FS_FIRSTTIME (`TFrmPlayMovie`, `TFrmMainWindow`). `/DEFAULT` keeps the stored value (normally 1), so nothing below runs after a race. Start without argument: `0x493bdc`, flow UNKNOWN; the stored value applies.
1. If no players: loop { `TFrmNameEntryNew` modal (`0x4915f0`): TxtTitle LT_APPTITLE, TxtContent LT_MSG_FIRSTPLAYER, TxtEditTitle LT_MSG_NEWNAME. OK -> create player in **slot 0** with `Now`. Cancel -> `Application.Terminate` } until a player exists.
2. Else: `TFrmPlayerList` modal (`0x491724`, title LT_APPTITLE, header label LT_TI_PLAYERLIST at (10,0)). OK -> re-bind the main window radios to the new current player's GameType and refresh TxtInfo. Any other result -> `Application.Terminate`.
3. obj+0x37A0 := 1.

### 3.3 Main window "Spelerslijst" (`BtnPlayerListClick 0x4aba2c` -> `0x491808`)
Click sound, then `TFrmPlayerListDlg` modal (header LT_TI_PLAYERLIST at (10,0)). On mrOk: radio group re-bound to new current GameType (`0x48ad98`), TxtInfo update (`0x4ab1d0`), redraw. The main window radios (BtnRadio1..3) are bound to the GameType pointer P+0xB1C, so picking a game type writes the current profile immediately.

### 3.4 Player list form (TFrmPlayerList; TFrmPlayerListDlg = subclass without own code)
Texts (Forms.ini): TxtTitle LT_TI_PLAYERLIST, TxtContent LT_MSG_CHANGEPLAYER, btnNewPlayer LT_NEWPLAYER, btnDeletePlayer LT_DELPLAYER, txtName LT_PLAYERNAME, txtDate LT_ENTRYDATE, OK/Cancel LT_OK/LT_CANCEL.
Working copy, 5 rows (form+0x370 + row*0x28): name, date, empty flag, profile index, pending flags (bit0 delete, bit1 create). Nothing touches the database before OK.

| Step | Behaviour |
|---|---|
| Create `0x483658` | copy slot i -> row i (name, date, empty), flags 0; selected row = current player |
| Refresh `0x4834d4` | sort rows: used before empty, then newest date first (selection follows its row); row k -> btnRadio(k+1) caption = name, txtDate(k+1) = DateToStr(date); empty rows: captions '' and radio disabled; check selected row |
| Radio click | selected row = that radio |
| btnNewPlayer `0x4839f4` | first empty row; none -> LT_MSG_LISTFULL (OK only). Else `TFrmNameEntryDlg` modal (TxtTitle LT_APPTITLE at Left 20, TxtContent '' per Forms.ini, TxtEditTitle LT_MSG_NEWNAME). OK -> row name, date = Now, empty 0, flags \|= 2, select it, refresh |
| btnDeletePlayer `0x4838bc` | TFrmRequester: LT_MSG_DELETE formatted with the name twice, buttons LT_YES/LT_NO. Yes -> row empty 1, flags \|= 1, select first non-empty row, refresh |
| OK `0x483b68` | no non-empty row -> LT_MSG_NOPLAYER (stay). Else per row: bit0 -> profile reset `0x4a2dac(slot)`; bit1 -> `0x4a2fd4(name, slot, Now-at-OK)` (reset + name/date + current := slot + default bindings). Then current := selected row's slot; close mrOk |
| Cancel `0x483d70` | exact class TFrmPlayerList (start-up): LT_MSG_EXIT Yes/No, close only on Yes. TFrmPlayerListDlg: close at once. BtnQuit = same as Cancel |

Delete does not wipe the record: reset keeps bindings, FF, sensitivity and GameType of the old player; a new player in that slot gets fresh bindings but the stale GameType.

### 3.5 Name entry forms
| | TFrmNameEntryNew (first player) | TFrmNameEntryDlg (new player) |
|---|---|---|
| Enter in EditBox | clicks BtnOk | clicks BtnOk twice (inherited handler + own; harmless, but an empty name shows LT_MSG_NONAME twice) |
| OK | empty -> LT_MSG_NONAME, stay; else mrOk | same |
| Cancel | the confirm branch compares ClassName with 'TFrmPlayerList' and never matches -> closes at once | same |
| Result | ModalResult + EditBox.Text | same |

### 3.6 Texts (NL)
| Key | Text |
|---|---|
| LT_MSG_FIRSTPLAYER | Welkom bij het Europe Racing Avontuur!\nDus je gaat de uitdaging aan? Veel succes! |
| LT_MSG_NEWNAME | Vul hier een nieuwe naam in: |
| LT_MSG_CHANGEPLAYER | Kies een spelersnaam. |
| LT_MSG_NONAME / NOPLAYER / LISTFULL | Je bent vergeten een naam op te geven. Voer een naam in. / Er staan geen namen in de spelerslijst. Voeg een naam toe om te kunnen spelen. / De spelerslijst is vol. Om een nieuwe speler toe te voegen, moet je eerst een speler verwijderen. |
| LT_MSG_DELETE | Wil je %s echt verwijderen? Let op! Je verwijdert daarmee ook alle opgeslagen spellen van %s. |
| LT_MSG_EXIT | Wil je echt stoppen met het spel? |
| Unused | LT_MSG_FIRSTNAME, LT_MSG_SELECTPLAYER, LT_MSG_NAMEERROR (no reference found) |

## 4. Options

### 4.1 Mechanism
All option controls are bound by controller objects to a pointer into obj/P; a change writes the field immediately. Each dialog copies the fields on create and Cancel/BtnQuit writes the copies back. OK = `TFrmDialog_BtnOkClick` (keep). No Next/Previous. The main window ignores the result (`0x49192c`). Persistence only via `.EUR` at exit.
Controllers: tabs `0x487e9c` (tab radios <-> panels), radio group `0x48aa78` (value = 0-based radio index), image per value `0x48af24`, slider `0x48b2ec` (Position = value), checkbox+label `0x48b4fc` (0/1).

### 4.2 TFrmOptions (title LT_OPTIONS)
| Control (DFM) | Field | Range | Note |
|---|---|---|---|
| tabs btnControls / btnResolution / btnSound / btnGameSpeed | pnlControls / pnlResolution / pnlSound / pnlSpeed | | first tab active on open (assumed: no start index is passed) |
| btnKeyboard / btnGamePad / btnJoystick / btnSteeringWheel | P+0xA04 | 0..3 | create `0x48cda8`: no joystick (TJoystickex `0x48b834` false) -> value := 0 and the other three disabled |
| imgControlType | P+0xA04 | | `opct0..3` |
| chkShowControlsAtStartRace + lblShowControls | P+0xB0C | 0/1 | LT_OPT_SHOWCONTROLS; before a race `0x491a58` shows TFrmShowControls when 1 |
| btnControlSettings (LT_SETTINGS) | | | device from radios: keyboard/gamepad -> TFrmOptionsKeyboard(d), joystick/wheel -> TFrmOptionsSteer(d) |
| btnLowRes / btnMedRes / btnHighRes | obj+0x3768 | 0..2 | imgResolution `OpRe0..2` |
| btnDisplaySettings (LT_ADVANCED) | | | TFrmOptionsDisplay |
| sliMusicVolume | P+0xB10 | 0..100, step 1/5 | change callback `0x48c32c` sets live music volume; thumb image `sld` |
| sliSoundEffects | P+0xB14 | 0..100 | used for menu click sounds |
| sliVoices | P+0xB18 | 0..100 | -> POLICEVOLUME |
| btnSoundSettings (LT_ADVANCED) | | | TfrmOptionsSound |
| chkHidePedestrians | obj+0x379C | 0/1 | Spelsnelheid tab |
| chkHideParticles | obj+0x3798 | 0/1 | Spelsnelheid tab |
| lblGameSpeedInfo | | | LT_GAMESPEED_INFO |
| Button1 (plain TButton in pnlControls) | | | no handler, leftover |
"Spelsnelheid" (LT_GAMESPEED) is a performance tab only: the info text plus the two hide checkboxes. There is no speed value.
Cancel `0x48cbf0` restores: device, resolution, 3 volumes, show-controls, hide pedestrians, hide particles (backup `0x48cccc`, taken before the joystick check).

### 4.3 Sub dialogs
| Dialog | Controls -> field | Cancel restores |
|---|---|---|
| TfrmOptionsSound (LT_SOUND_ADVANCED, bg BGGTab) | btnTwoSpeakers/FourSpeakers/Headphones -> obj+0x3788; btnNormal/EAX/A3D -> obj+0x378C; btnLow/Med/High -> obj+0x3790; chkSoundEnabled -> obj+0x3794 | all 4 |
| TFrmOptionsDisplay (LT_RESOLUTION_ADVANCED, bg BGGTab) | tabs btnGeneral/btnProblems; btnLight/btnDark -> 0x376C; btnMinimal/Normal/Maximal -> 0x3774; chkMIPMapping -> 0x3770; chkBilinearFiltering -> 0x3778; chkTransparancy -> 0x377C; chkSynchronize -> 0x3784; cboVideoCard (made visible on show; items = DirectX device list) -> 0x3764 | all 7 |
| TFrmOptionsKeyboard(d), d = 0/1 | 14 buttons -> binding k: Accelerate 0, Brake 1, Left 2, Right 3, HandBrake 4, Turbo 5, Claxon 6, Reset 7, Escape 8, CamClose 9, CamFar 10, CamDriver 11, CamRear 12, Overlay 13 | bindings |
| TFrmOptionsSteer(d), d = 2/3 | same buttons for k = 4..13 (no 0..3: axes); btnForceOff/Min/Med/Max -> FF 0..3; sliSteerSensitivity -> sensitivity 0..100 | bindings, FF, sensitivity |
Key capture (keyboard form): click a button -> wait state. Next key: VK -> TInputCommand (`0x4a26c8`); already used by another action of this device -> LT_MSG_CONTROLERROR and wait ends; ordinal 112 -> ignored, keep waiting; else store, caption = key name, wait ends. Esc outside wait state removes focus. Gamepad buttons are read through TJoystickex when d = 1; joystick capture details UNKNOWN. btnDefaultValues -> `0x4a5710(d)`: device defaults of 2.4, and also FF 0 / sensitivity 50.

## 5. UNKNOWN
1. Current directory at start, so where GameMenu.EUR/.HS actually land (names are relative).
2. Meaning of R+0x18, R+0x19, R+0xAF8 (=10), R+0x9EC, CH +0x1DC / AD +0x1E0 flags, obj+0x3780, obj+0x37A4, score table +0x1A0: written by resets, no reader found (search: whole CODE for `[reg + 8*eax + disp]` and `add eax, disp`).
3. Flow chosen when the menu starts without `/FIRSTTIME` or `/DEFAULT` (`0x493bdc` not traced).
4. How race results enter the high score tables.
5. Joystick/gamepad button capture in the key dialogs and the TJoystickex detection itself.
6. Initial tab of TFrmOptions / TFrmOptionsDisplay (no index passed; 0 assumed).
7. Whether Forms.ini overrides the DFM caption LT_MSG_FIRSTPLAYER of TFrmNameEntryDlg.TxtContent with '' (assumed yes, as for other forms).
