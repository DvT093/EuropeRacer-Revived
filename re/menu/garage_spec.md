# Garage spec (TFrmCHGarage / TFrmADGarage)

Source: GameMenu.dat, verified against disassembly (llvm `objdump -d --start-address=VA` on the PE) unless marked otherwise.
`P` = player object (`[0x4c8b40]+0x18`). Mode from `0x4a345c(P)`: 0 = QR, 1 = CH, 2 = AD.

## 1. CH vs AD

The two forms are byte-for-byte the same code (CH `0x4c0f9c..0x4c1350`, AD `0x4bca30..0x4bcde0`); both pass the same flag byte `0x7f` to the properties panel. The only difference is which record the getters return:

| Getter | Returns | CH (mode 1) | AD (mode 2) |
|---|---|---|---|
| `0x4a3c48` "club money block" | `rec + club*56 + 0x20` | rec = `0x4a335c` (P+0x2c+class*0x1ec) | rec = `0x4a3328` (P+0x7e0) |
| `0x4a3a54` "car-type slot" | `rec + club*56 + class*8` | same rec | same rec |

club = `*0x4a4a4c(P)` (player club, rec+0x1d4), class = `*0x4a3828(P)`.
So: money = `blk[+0x20]` (= `0x4a3c48[0]`), turbo = `blk[+0x34]` (= `0x4a3c48[+0x14]`), damage = `slot[+0]`, upgrade = `slot[+4]`.
Prices, items and rules are the same in CH and AD.

## 2. Offers (carousel items)

5 items, count = `0x4bbaa4` returns 5. Item index = shared int at form+0x364, **set to 2 on every form create** (so the carousel opens on the upgrade package).

| idx | Picture (`0x4bbabc`) | Text (TxtInfo) | Price (`0x4a54fc`) | Buy effect (`0x4bc06c`) | Buy enabled when (`0x4bbed0`) |
|---|---|---|---|---|---|
| 0 | `GNit50` | LT_GRG_NITROLARGE (0x176) "Nitro per 5 liter" | 1000 (`[0x4c878c]`) | turbo += 50, clamp to 100 | turbo < 100 AND money >= price |
| 1 | `GNit10` | LT_GRG_NITROSMALL (0x175) "Nitro per liter" | 250 (`[0x4c8788]`) | turbo += 10, clamp to 100 | turbo < 100 AND money >= price |
| 2 | `GPak1..4` by upgrade 0..3 | LT_GRG_UPGRADE1..4 (0x177..0x17a) | `[0x4c8778 + upgrade*4]` = 15000 / 25000 / 35000 / 0 | upgrade += 1 (Delphi range check: >3 raises; unreachable) | upgrade != 3 AND money >= price |
| 3 | `GRep10` | LT_GRG_REPAIRSMALL (0x173) "Reparatie per 5 %" | 250 (`[0x4c8790]`) | damage -= 5, clamp to 0 | damage > 0 AND money >= price |
| 4 | `GRep50` | LT_GRG_REPAIRLARGE (0x174) "Reparatie per 10%" | 1000 (`[0x4c8794]`) | damage -= 10, clamp to 0 | damage > 0 AND money >= price |

- Every buy first does `money -= price` (always, before the switch), then the effect. No partial repair: price is fixed even when damage < 5.
- Picture names do not match amounts (GRep10 = 5 %, GRep50 = 10 %; GNit50 = +50, GNit10 = +10). Code is authoritative.
- Upgrade text by stage: 0 "Sport Pack", 1 "Total Pack", 2 "Master Pack", 3 "Er zijn geen upgrades meer." (no price/money args).
- Price constants: DATA `0x4c8778..0x4c8797` = `15000, 25000, 35000, 0, 250, 1000, 250, 1000` (file offset VA-0x400e00). Only reader is `0x4a54fc` (byte search for each address); no ini value overrides them. GameMenu.ini [TWEAK] has nothing for the garage.
- Units: turbo 0..100 (start 100; "liter" text: +10 = 1 liter); damage in percent 0..100 (read from game.ini `[PLAYER] damage`, see gameini_spec.md).

## 3. Texts

| Control | Content | Filled by |
|---|---|---|
| TxtInfo (form+0x338) | `Format(LT_x, [priceStr, moneyStr])`, first %s = price ("Kosten"), second %s = money ("in kas"); both strings made with `Format('%d', ...)` (no thousands separator). LT_GRG_UPGRADE4 has no args. `%%` in the ini prints `%`. | `0x4bc3d8` |
| lblUpgradeCost (form+0x360) | `Format(LT_GRG_EURO "%d Euro", [price(idx)])`; for the maxed upgrade this is "0 Euro" | `0x4bbc90` |
| BtnBuy (form+0x350) | caption LT_GRG_KOPEN "Kopen" (DFM) | DFM |
| TxtContent | LT_GRG_ASKREPAIR "Wil je een upgrade kopen of een reparatie laten uitvoeren?" (DFM caption, no code touches it) | DFM |

LT numbering: TLanguageText value = line number in `lt_enum.txt` **minus 1** (LT_APPTITLE is line 1 = value 0; e.g. 0x173 = 371 = line 372 LT_GRG_REPAIRSMALL).

## 4. Carousel behaviour (`0x4b8758` base, `0x4b893c` refresh)

| Slot | Shows | Alpha |
|---|---|---|
| ImgCarPrev (+0x34c) | item (idx-1+5) mod 5 | 100 |
| ImgCarCur (+0x348) | item idx | 255 |
| ImgCarNext (+0x344) | item (idx+1) mod 5 | 100 |

- BtnCarNext (+0x33c) OnClick `0x4b8660`: idx = (idx-1+5) mod 5. BtnCarPrev (+0x340) OnClick `0x4b86e4`: idx = (idx+1) mod 5. (Verified from push order at `0x4c11d8..0x4c1224`; the button names are the reverse of what they do to the index.) Wraps around.
- Both arrow clicks play sound effect 3 at the sfx volume (P+0xB14).
- After any index change or buy, `0x486ce0` notifies all registered observers (global list `0x4ca1e8`), so TxtInfo, lblUpgradeCost, BtnBuy enable state and the properties panel all refresh.

## 5. Buying flow

1. BtnBuy OnClick = `0x4bc06c` (set by `0x4bbdd8`; button set Enabled = true at create, then the enable rule above).
2. Plays sound effect 0 at the sfx volume (P+0xB14, via the profile copy `0x4a3404`).
3. money -= price; apply effect (table above); notify observers.
- No confirmation popup, no message box, no "not enough money" text: the button is just disabled.

Sound index -> file: `0x494e54(mgr, n, vol)`. Index 0/3 are assumed to follow the [SOUND_EFFECT] order (0 SNDFX_OK `ER_S_00`, 3 SNDFX_PHOTO `ER_S_03`). UNKNOWN (not verified): the loader that fills the sound table at mgr+0xc was not traced.

## 6. Rest of the form

- Background `BgGar`; logo `CBLogo%d` (club number) (`0x4c0f9c`).
- Music: `0x495024(mgr, club, musicVol P+0xB10)` = club track `SNDCB_<club>` ([SOUND_MUSIC]); does not restart if already playing.
- ImgClub (+0x354): bitmap `car%.2da`, number = clubRecord(`0x49edcc(club)`)[+0xc + class*4] (the player's car for this class).
- Properties panel (`0x4b7ce4`, update `0x4b813c`, MpbCarProperties +0x358, ImgProperties +0x35c, bar images Stats0/Stats1, `MensMal`), range 0..100, 7 rows (flag 0x7f = all), value text `"%3d/%3d"`:

| Row | Label | Value / max | Bar |
|---|---|---|---|
| 0 | LT_CPROP_UPGRADE | upgrade / 3 | upgrade*33 |
| 1-4 | LT_CPROP_SPEED, _ACCELERATION, _GRIP, _STRENGTH | car table byte `[0x4a5db0(carId)+10+upgrade*4+k]`, k=0..3 / 100 | same value |
| 5 | LT_CPROP_DAMAGE | damage / 100 | damage |
| 6 | LT_CPROP_NITRO | turbo / 100 (QR: fixed 100) | turbo |

  carId = `0x4a39dc(P)`. It shows the **current** stage only, no preview of the next package. k order = the 4 per-stage values of [CARS] (assumed same order as the labels; the [CARS] loader was not traced).

## 7. Round / upgrade path

- The garage code (`0x4b7ce4..0x4bc774`, both forms) never reads the round (rec+0x1e4/+0x1e8) or [CHUPGRADEPATH]/[ADUPGRADEPATH]. The offer list, prices and rules are the same every round.
- The garage appears every round: FS_CHNEW/FS_CHSTANDARD (after ShowTrack, before Race) and FS_ADNEW/FS_ADSTANDARD (after SelectCar, before Race).
- UNKNOWN: what [CH/AD]UPGRADEPATH drives. It is parsed by the club loader (string refs at `0x49e9d9` / `0x49ea01`), outside the garage; most likely opponent upgrades per round, not checked.

## 8. UNKNOWNs

| Item | Why |
|---|---|
| Sound file for effects 0 and 3 | sound-table loader not traced; mapping inferred from [SOUND_EFFECT] order |
| Order of the 4 per-stage car properties | [CARS] parser not traced |
| What the upgrade paths do | outside the garage code |
| Screen positions | DFM positions are design-time; the runtime layout comes from Forms.ini/FormsA.ini (not re-read here) |
