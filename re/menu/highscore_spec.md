# High score tables: insert, popups, .HS file (GameMenu.dat, MdlScores)

Source: disassembly of `iso/EuropeRacer/Menu/GameMenu.dat` (llvm-objdump on the PE, VAs direct), 9 Nov 2026. Score object = global+0x34 (`TMdlScores`). "rec" offsets below are relative to the start of one row.

## 1. Tables in memory (score object offsets)

| Obj offset | Table | Rows | Row size | Filled from ini | Insert function |
|---|---|---|---|---|---|
| +0x010 | Adventure | 5 | 0x28 | `[SCORE_ADVENTURE]` | `0x495f74` |
| +0x0D8 | Championship ATV (car class 1) | 5 | 0x28 | `[SCORE_CC_ATV]` | `0x496108` |
| +0x1A0 | UNKNOWN (5 x 0x28; never loaded, cleared, inserted or read by any code found; stays zero) | 5 | 0x28 | none | none |
| +0x268 | Championship SPORT (car class 2) | 5 | 0x28 | `[SCORE_CC_SPORT]` | `0x4962a4` |
| +0x330 | Championship STANDARD (car class 0) | 5 | 0x28 | `[SCORE_CC_STANDARD]` | `0x496440` |
| +0x3F8 | Lap records, 4 regions x 0x198 | count + 10 slots | 0x28 | `[SCORE_LAP]` | `0x4965dc` |
| +0xA58 | Track records, 4 regions x 0x198 | count + 10 slots | 0x28 | `[SCORE_TRACK]` | `0x496700` |

Car class = TCarType (RTTI): 0 CT_STANDARD, 1 CT_ATV, 2 CT_SPORT, 3 CT_DOWNLOADED. Region = TRaceRegion (RTTI): 0 TC_WESTEUROPE, 1 TC_MIDDLEEUROPE, 2 TC_SOUTHEUROPE, 3 TC_DOWNLOADED.

### 1.1 Score row (adventure / championship), 0x28 bytes

| rec+ | Type | Field | ini default (`0x495928`) | Real insert |
|---|---|---|---|---|
| 0x00 | ShortString[8] (9 B) | name | 1st field, cut to 8 chars (`_PStrNCpy` cl=8) | profile name, cut to 8 |
| 0x09 | 3 B | padding | - | stack garbage |
| 0x0C | int32 | score (points) | 2nd field | club block +0x24 (points) |
| 0x10 | int32 | position, 0-based (display adds 1) | 3rd field | club block +0x28 (rank, 0 = leader) |
| 0x14 | int32 | club (TClubs) | `Random(7)` | player club rec+0x1D4 (real value) |
| 0x18 | double | TDateTime | `Now` at load | `Now` at insert |
| 0x20 | uint32 | total time ms | 0x05265818 | club block +0x2C (real total time) |
| 0x24 | 4 B | padding | - | garbage |

Club block = championship record + club*56 (CH record `0x4a335c` = P+0x2C+class*0x1EC; AD record `0x4a3328` = P+0x7E0), club = rec+0x1D4. So the real insert stores real club, time and date; only the ini defaults are fake.

Clear row (`0x495750`): name empty, score/position/club 0, date 0.0, time 0xFFFFFFFF.

### 1.2 Region block (lap / track), 0x198 bytes

| Block+ | Field |
|---|---|
| 0x000 | byte count (subrange 0..9; see 1.4) |
| 0x001..0x007 | padding |
| 0x008 + i*0x28 | slot i = 0..9 |

| Slot+ | Type | Field | ini default (`0x4959fc`) | Real update |
|---|---|---|---|---|
| 0x00 | int32 | track id ([TRACKS] index) | 1st field | same track id |
| 0x04 | ShortString[8] | name | 2nd field, cut to 8 chars | profile name |
| 0x0D | 3 B | padding | - | garbage |
| 0x10 | int32 | car type (TCarType) | `Random(3)` | current car class `0x4a3828` (QR P+0x9D8, CH P+0x7DC, AD P+0x9B8) |
| 0x14 | 4 B | padding | - | garbage |
| 0x18 | double | TDateTime | `Now` | `Now` |
| 0x20 | uint32 | time ms | 3599990 if first in region, else previous slot's time (so always 3599990) | race time |

Clear (`0x495794`): every region: count 0; every slot: name empty, car type 0, date 0.0, time 0xFFFFFFFF (track id not touched).

## 2. Championship / adventure insert (`0x495f74`, `0x496108`, `0x4962a4`, `0x496440`)

All four are the same code on a different table:
1. Copy the 5 rows to a local array; put pointers to them in a TList, then append a pointer to the new entry (index 5).
2. `TList.Sort` (Delphi QuickSort, not stable) with comparator `0x495668`.
3. Copy list items 0..4 back into the table. Item 5 is dropped (the old 5th row or the new entry).
4. Return 0 if item 5 is the new entry (not in), 2 if item 0 is the new entry (new top), else 1. Callers only test nonzero.

Comparator `0x495668(a, b)`, first difference wins:

| Step | Field | a before b when | Compare |
|---|---|---|---|
| 1 | score +0x0C | a.score > b.score | signed |
| 2 | position +0x10 | a.position > b.position (higher number first, as coded) | signed |
| 3 | time +0x20 | a.time < b.time | unsigned |
| 4 | date +0x18 | a.date > b.date (newer first) | double |
| 5 | - | equal | 0 |

Tie rule in practice: a new entry with equal score, position and time beats an older row (newer date). A cleared row (score 0, time 0xFFFFFFFF, date 0) loses to any entry with score >= 0 and a lower time.

Caller entry build (`TFrmCHResultWin` `0x4bf288`, `TFrmADResultWin` `0x4b7a94`): name, points, rank, club, Now, time from the player club block (section 1.1). CH picks the table by car class `0x4a3828`: 0 -> `0x496440` (STANDARD), 1 -> `0x496108` (ATV), 2 -> `0x4962a4` (SPORT); other values: no insert. AD always uses the adventure table (class ignored).

## 3. Lap / track record update (`0x4965dc` lap, `0x496700` track)

Arguments: self, track id, pointer to a 0x28 slot record (copied first).
1. region = TRACKS[track id].RaceRegion (`0x479c44`, +0x14).
2. For i = 0 .. count-1 of that region: if slot.track id == track id and slot.time > new.time (unsigned, strict), overwrite the whole slot with the new record and set result = true.
3. Return result (byte).

So: no insert, no sort, no shifting, count never changes. A track with no slot from the ini can never get a record. Equal time does not replace. Two slots with the same track id would both be replaced.

Default ini gap: `[SCORE_LAP]` and `[SCORE_TRACK]` have 20 rows each and no row for track 15 (Prague, TC_MIDDLEEUROPE). Records on Prague are never stored. Region TC_DOWNLOADED has no rows.

Caller (`0x4c1a4c` QR, `0x4be0dc` CH qualified, `0x4bd898` CH disqualified, `0x4b5a20` AD qualified, `0x4b51f4` AD disqualified; identical code):
1. Only if race result `Finished` byte != 0 (result = game.ini parse object +0x18, getter `0x499ff8`).
2. Build {track id `0x4a4c90`, name, car class `0x4a3828`, Now}.
3. time = best lap (result +0x0C) -> lap update `0x4965dc`.
4. Same record, time = TrackTime (result +0x08) -> track update `0x496700`.
5. Popup call: both true -> `0x490c38`; else lap -> `0x490c20`; track -> `0x490c2c`.

Race result fields (`0x497c54`, from game.ini `[PLAYER]`): +0x00 Finished, +0x04 Position, +0x08 TrackTime, +0x0C best lap = minimum of `lap_1_time .. lap_<lapcount>_time`, start 0x7FFFFFFF (no laps -> 0x7FFFFFFF, never a record). A negative time raises a range error before the update.

Not checked for lap/track records: `TFrmCHResultWin` and `TFrmADResultWin` (last race of a won championship), QR not finished (FS_QRESC), CH/AD escape forms.

## 4. When the checks run and which popup appears

All checks run in the wizard form's after-show hook (VMT slot +0xF8, wired by TBaseFrmWizard create `0x492b7c` into the event at +0x330, fired by `0x492978` after the page is made visible).

| Form | Hook | Order |
|---|---|---|
| TFrmQRResultWin | `0x4c1bcc` | lap/track check (3) |
| TFrmCHResultQualified / Disqualified | `0x4be25c` / `0x4bda18` | lap/track check |
| TFrmADResultQualified / Disqualified | `0x4b5ba0` / `0x4b51dc` | lap/track check |
| TFrmCHResultWin | `0x4bf408` | vcall +0x80 (probably Repaint; not verified), championship insert + popup `0x4bf288`, reset championship `0x4a4f38`, `0x492b44` (UNKNOWN purpose) |
| TFrmADResultWin | `0x4b7bb4` | vcall +0x80, adventure insert + popup `0x4b7a94`, reset `0x4a4f90`, `0x492b44` |

| Popup function | Shown when | What it does |
|---|---|---|
| `0x490c20` (lap) | lap record only | **nothing**: empty procedure (push ebp; ret) |
| `0x490c2c` (track) | track record only | **nothing**: empty procedure |
| `0x490c38` (both) | lap and track | **nothing**: empty procedure |
| `0x490c44(owner form)` | championship / adventure insert returned 1 or 2 | `TFrmRequester` (owner = result form) modal; TxtContent (`0x4921e8`, keeps its Left/Top) = LT_MSG_GAMEHIGHSCORE (enum 27, 0x1B); BtnOk visible, caption LT_OK (enum 118, 0x76); BtnCancel stays hidden (DFM). Title not set (form default). Freed after close |

NL text: `LT_MSG_GAMEHIGHSCORE=Je hebt een Highscore gehaald.\nGefeliciteerd!`. How `\n` is turned into a line break: not checked here (same requester path as LT_MSG_DELETE). The data is updated regardless of the popup; nothing is saved until program end.

## 5. GameMenu.ini [SCORE_*] defaults -> tables (loader `0x495c40`)

Only used when GameMenu.HS is missing (clear `0x495878` first). Wrong header: clear only, no ini (all counts 0, so lap/track records become impossible).

| Section | Keys read | Into |
|---|---|---|
| `[SCORE_ADVENTURE]`, `[SCORE_CC_STANDARD]`, `[SCORE_CC_ATV]`, `[SCORE_CC_SPORT]` | `0`..`4` = `name,score,position` (ReadString, default '') | row i of +0x10 / +0x330 / +0xD8 / +0x268; one loop i = 0..4 over the four |
| `[SCORE_LAP]` / `[SCORE_TRACK]` | `Count` (ReadInteger, default 0; ini writes `COUNT`, case-insensitive), then `0`..`Count-1` = `trackid,name` | appended in ini order to the region of the track: slot = region.count, then count+1 |

Region count is a 0..9 subrange: writing the 10th slot of a region succeeds, then the increment to 10 raises ERangeError and aborts loading (UNKNOWN what the caller does with that exception). Effective limit 9 rows per region without error; the shipped ini has at most 7. Rows are not sorted; the record screen shows them in slot order (layout E, max 7 rows).

## 6. GameMenu.HS file (load `0x49683c`, save `0x4969a4`)

Load and save once (program start / `TManager` destroy). Save: Rewrite, BlockWrite 9 header bytes, BlockWrite 0x10A8 bytes from obj+0x10. Load: BlockRead 9, compare with header, BlockRead 0x10A8. File size 0x10B1 (4273). File offset = obj offset - 0x10 + 9.

| File offset | Size | Content |
|---|---|---|
| 0x000 | 9 | `08 'ERHSv0.3'` |
| 0x009 | 0xC8 | Adventure, 5 x score row |
| 0x0D1 | 0xC8 | CC ATV |
| 0x199 | 0xC8 | UNKNOWN table (zero) |
| 0x261 | 0xC8 | CC SPORT |
| 0x329 | 0xC8 | CC STANDARD |
| 0x3F1 | 4 x 0x198 | Lap regions 0..3 (count byte, 7 pad, 10 slots) |
| 0xA51 | 4 x 0x198 | Track regions 0..3 |
| 0x10B1 | - | end |

All integers little-endian; dates are 8-byte IEEE doubles (Delphi TDateTime); padding bytes are undefined, write them as zero.

## 7. Notes for the port

- `src/menu/er/screens_ch.cpp` cuts names to 7 chars (`string[7]`); the binary copies up to 8 (`0x4032f8` with cl=8). Whether TCtrlScore draws all 8: not checked.
- The port's region lookup should accept region 3 and keep the 0..9 slot rule; Prague (15) has no slot by default.
- Lap/track records give no message in the original; only the championship/adventure table gives the LT_MSG_GAMEHIGHSCORE requester.

## UNKNOWN

| Item | Why |
|---|---|
| Table at obj+0x1A0 | no code reference found in MdlScores; earlier guess CT_DOWNLOADED unproven |
| Meaning of position-descending tie rule | coded as a.position > b.position first; intent unclear, reproduce as coded |
| vcall +0x80 and `0x492b44` in the win hooks | not traced (not needed for the tables) |
| Effect of ERangeError at the 10th ini row | exception handling of the module loader not traced |
| `\n` rendering in the requester | language loader not traced here |
