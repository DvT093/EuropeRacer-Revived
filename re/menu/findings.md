## GameMenu.dat (Delphi 5, Davilex Menulib), uitgezocht voor eracer-menu (9 nov 2026)
- Componenten: TGDI-familie (TGDIButton, TGDILabel, TGDIRadioButton, TGDIGraphic, TGDICheckBox, TGDIPanel, TGDISlider, TGDIMultiProgressBar) met TGDIButtonDefinition in TData. Alle GraphicNames zijn leeg in de DFM's; de code zet ze.
- Knopplaatjes: TData.DataModuleCreate 0x4a1044 -> 0x4a107c roept per definitie 0x496f8c(prefix) aan: prefix+"0" Up (zet ook de grootte), "1" Down, "2" Hovered+Focused, "3" Disabled, "4" Focused. De bedieningsknoppen (CoKe*/CoJo*) via 0x4971a8: "0" Up+Disabled, "1" Hovered+Down+Focused. Tabel in `src/menu/er/skin.cpp`.
- 0x4a15d0: de vier radio-definities krijgen hun tekst 35 px rechts van het rondje.
- Achtergrond: TBaseFrmCaption.SetBackground 0x492080, per formulier vanuit een eigen override met een vaste naam (BgQR1, BgE, BgQR2, ...). Het formulier krijgt de grootte van de achtergrond. Tabel in `skin.cpp`.
- Posities: Forms.ini + FormsA.ini `[TFrmX] Comp.Left/Top/...`, maar de meeste knopposities daarin zijn ontwerp-standaardwaarden. De echte plek zet code bij elk tonen (virtueel 0xe4):
  - TBaseFrmCaption 0x491f60: LogoImage (8,8), BtnQuit rechtsboven, BtnHelp links ervan.
  - TBaseFrmWizard 0x4928b8: BtnNext rechtsonder, BtnPrevious links ervan.
  - TBaseFrmPopUpWindow 0x48093c: BtnCancel rechtsonder, BtnOk links ervan (zonder Cancel: Ok rechtsonder).
  - TFrmReqQuit 0x482234: Cancel 30 px van rechts en 20 van onder, Ok 20 px links ervan.
  - TFrmMainWindow 0x4ab57c: Opties/Spelerslijst/Internet/High score vanaf links, 2 px tussenruimte, 4 px boven de onderrand.
- Teksten: captions zijn LT_-sleutels uit `Langdata/<taal>/menu/Language.ini [Text_<taal>]`. Knopfont = font van de TGDIButtonDefinition (bv. radio: Hilde -27 vet zwart; gdfButtonT1: Arial -11 vet wit), tenzij Forms.ini het per knop zet. Font Hilde = `FONTDATA/HIS_____.TTF` op de CD (alleen regular; vet = synthetisch).
- Flow: `GameMenu.ini [FS_Definition]` + `[FS_xxx]` (TFlowMatrix, FlowMatrix.pas).
- Afsluitvraag TFrmReqQuit: GDIFormShow 0x482404 zet BtnRadio1 (terug naar hoofdmenu) aan.
- Archieven: `Graphics.IMG` (322 TGA's, type 2, 32 bit met alpha), `Music.IMG` (13 wavs: ER_S_00-03 klikgeluiden, muziek per club), `Langdata/<taal>/menu/Voices.IMG`. Elk item: u32 grootte + zlib.
- Game-interface (MdlGameInterface.pas): sleutels gevonden als strings (o.a. `[PLAYER] car/Name/country/damage/TURBO_AMOUNT/Money/Finished/Position/TrackTime/lapcount/lap_%d_time`, `[ETAPPE] Etappe`, `[OPTIONS] NrOpponents/Difficulty/GameType/RaceMode/Nrlaps`, cops, `[BONUS]`, `[RACERS] Racer%d`, `[General] ExitCode`), secties per sleutel nog niet uit de code bevestigd.
### Schermen vullen (9 nov 2026, nacht)
- Teksten in code zijn `TLanguageText`-enumwaarden; de namen staan als RTTI vanaf 0x9e384 (409 stuks), uitgelezen naar `re/menu/lt_enum.txt` (niet in git). Voorbeeld: 0xb LT_MSG_QRDIFFICULTY, 0xd8-0xda LT_INF_GT_*, 0xdf-0xe1 LT_INF_DF_*, 0xe3-0xe5 LT_INF_CC_*, 0xe9-0xf0 LT_INF_CB_* (in [CLUBS]-volgorde), 0x150-0x152 LT_TXT_CC_*.
- De schermen zijn opgebouwd uit kleine controllers: radiogroep 0x48aa78, tekst per radio in TxtInfo 0x4aafc8, plaatje per radio 0x48af24, carrousel van drie polaroids (vorige, huidige, volgende) met knoppen, basis 0x4b893c (tracks 0x4c38d0), tabcontrol 0x4875f0 (tabs naast elkaar vanaf de eerste). Event-handlers zijn alleen via pointers bereikbaar: Ghidra maakt er geen functies van; `re/menu/l5.txt` (functiestarts `55 8B EC` op 4-byte-grens) + DecompList lost dat op. Per formulier gegroepeerd in `re/menu/forms/<klasse>.c` (niet in git).
- Plaatjes: `PolE` = lege polaroid (geen vorige/volgende), `trk%.2dA` trackfoto (geen trk15a: Praag zit niet in [QUICKRACE]), `car%.2dA` auto, `cbimgdO<club>` coureur (8 = J.B. Black), `cbimgcO<club>` clubfoto, `CBLogo<club>` klein logo, `dfimg0-2` moeilijkheid, `LMark<track>` stadscollage, `MensMal` briefje onder de eigenschapsbalken, `Stats0/1` balkjes, `GRep10/50`, `GNit10/50`, `GPak1-4` garage-aanbod, `opct0-3` besturing, `OpRe0-2` resolutie, `sld` schuifbalk.
- Achtergronden hebben zwarte gaten van 160x160 waar een polaroid hoort (garage: de clubfoto op ImgClub).
- Sommige 32-bit TGA's hebben overal alfa 0 (dfimg*, opp00, res1): het menu tekent die ondoorzichtig.
- Radiogroepen gelden per container (RadioGroupNumber + parent), anders schakelen de tabs van Opties de radio's in hun panelen uit.
- Nog niet ontcijferd: welke film bij welke flow (GameMenu.ini [MOVIES] is leeg; nu op bestandsnaam gekoppeld), garageprijzen en kopen, de high score-tabellen (TGameScorePageEditor + [SCORE_LAYOUT-*]), de uitslagschermen, ShowStatus, spelerslijst.

## Score-tabellen: welke layout bij welk control (uit strings, 9 nov 2026)
Bron: stringvolgorde in GameMenu.dat (elke `SCORE_LAYOUT-x` staat direct na het .pas-pad van zijn unit). Nog niet in Ghidra bevestigd.

| Layout | Unit / control | Kolommen |
|---|---|---|
| A | `CustomControls\ControlScore.pas` (TCtrlScore, high score Avontuur/Championship) | NAME, SCORE, POSITION, CLUB, TIME |
| B | `CustomControls\ControlGameResults.pas` (uitslag na race, achtergrond `BGRes`, `CBLogo%d`) | NAME, TRACKTIME |
| C | `CustomControls\ControlEndRanking.pas` (eindstand, `BGStdna`, `CBLogo%d`, `Res1`; niet in Quick Race) | NAME, CLUB, SCORE, TIME |
| D | `CustomControls\ControlStatusRanking.pas` (ShowStatus-ranking) | NAME, CLUB, SCORE |
| E | `Editors\ControlGameScore.pas` (TCtrlGameScore, high score ronde/track; niet voor QR/CH-download) | TRACK, NAME, TRACKTIME, CARTYPE |

- Data: `[SCORE_ADVENTURE]`, `[SCORE_CC_STANDARD/ATV/SPORT]` = `naam,score?,positie?` (5 regels); `[SCORE_LAP]`/`[SCORE_TRACK]` = `track,naam` per league. Betekenis van de getallen nog uit `Modules\MdlScores.pas` halen (Ghidra).
- Font-hoogtes zijn negatief (Windows LOGFONT: tekenhoogte in px).

## High score-tabellen (Ghidra, 9 nov 2026, gebouwd)
- Laden: MdlScores 0x495c40. `[SCORE_ADVENTURE]`/`[SCORE_CC_*]`: `naam,score,positie` (0x495928); naam = string[7] (dus "Alejand"); club = Random(7) en totale tijd = vaste 0x05265818 ms (23:59:58), beide bij het laden gezet, niet uit de ini.
- `[SCORE_LAP]`/`[SCORE_TRACK]`: `track,naam` (0x4959fc), per regio van de track max 10; autotype Random(3) (teksten LT_CD_STANDARD/ATV/SPORT, enum 0x67-0x69 via 0x4a0a9c); tijd 3599990 ms of die van de vorige regel.
- Tekenen: TCtrlScore (0x4846a8), font Hilde. Pagina 1 = layout A (0x4850f4), kolommen naam, score, positie+1, club (clubnaam via 0x49eef4), tijd hh:mm:ss (0x468018). Pagina 5 = layout E (0x486084): naam, track, autotype, tijd mm:ss:hh (0x4680c8), max 7 rijen (0x4865ac lap, 0x486844 track), in Pg5Page1.
- Niet geverifieerd: tekstkleur (zwart aangenomen), verticale plaatsing in de rij (bovenkant), en of pagina 1 echt over hetzelfde lijntjesvlak valt (de kolommen van layout A lopen niet gelijk met de lijnen in de achtergrond).

## Na de race (Ghidra, 9 nov 2026, Quick Race gebouwd)
- Het origineel stopt vóór de race; de launcher draait de game en start het menu opnieuw met `/DEFAULT` (0x493f84; `/FIRSTTIME` = FS_FIRSTTIME). Dan leest 0x497b18 `game.ini` en kiest 0x494218 de flow.
- Uitslag (0x497c54, 0x497ea8): `[PLAYER] Finished/Position (0 = eerste)/TrackTime/lapcount/lap_N_time` (beste ronde = laagste), `damage`, `TURBO_AMOUNT`, `Money`, `[UPGRADES] PLAYER`; `[OPTIONS] NrOpponents` (max 4) racers als `[RACERS] i=auto,positie,tijd` plus `country0i` = club van die racer. Vóór de race schrijft het menu `i=auto,x,0` en `country0i` (0x4988ec).
- Winnaar (0x49a09c): speler als Position 0, anders de eerste racer met positie 0, anders "No winner!".
- Flow (0x494218), speltype uit de spelerstaat (0 QR, 1 CH, 2 AD):
  - QR: Finished -> FS_QRWIN (3), anders FS_QRESC (16). FS_QRLOOSE wordt nooit gekozen.
  - CH: niet gefinisht -> FS_CHESC; gekwalificeerd = Position < grens uit een tabel per competitie en moeilijkheid (0x479e48). Niet de laatste race: gekwalificeerd of speltype 5 -> FS_CHQUALIFIED, anders FS_CHDISQUALIFIED. Laatste race: gekwalificeerd -> FS_CHWIN, anders DISQUALIFIED.
  - AD: hetzelfde met FS_ADESC/ADQUALIFIED/ADDISQUALIFIED/ADWIN (tabel 0x479ca4, aantal races 0x479dfc).
- TFrmQRResultWin (0x4c16ec): speler wint -> LT_MSG_QR_YOUWON + `Chaw8`; duel tegen de politie -> LT_MSG_QR_PS_WON met LT_ED_CHIEFPOLICE + `Chaw9`; anders LT_MSG_QR_PS_WON met de coureur van de club (0x49ee2c) + `ChaW<club>`. Foto in WinnerImage, BtnPrevious uit, btnTryAgain (0x4c168c) = dezelfde race opnieuw.
- Aanname: in 0x4c16ec zijn `0x4a4400()==1` en `0x4a4128()==1` gelezen als "QR-type duel" en "tegenstander = politie". Niet hard bewezen (spelerrecord-offsets 0x27a/0x27d).
- Niet geverifieerd: of de tekst in TxtContent (Hilde -27, vak 300x120) bij een lange naam ook in het origineel na drie regels wordt afgekapt.
- CH/AD-kwalificatieschermen (0x4bdd24 e.a.): gewonnen -> LT_MSG_xx_YOUWON + ChaW8, gekwalificeerd -> LT_MSG_xx_QUALIFIED met de winnaar, niet -> LT_MSG_xx_NOT_QUALIFIED. Alle CH/AD-uitslagschermen zetten `CBLogo<club van de speler>` in LogoImage (SetLogo 0x492178). btnTryAgain (0x4bd7ac, 0x4c168c) toont TFrmRace opnieuw zonder de flow te wisselen.
- Kwalificatiegrens = `Qualify-Easy/Normal/Hard` van de race in `[CHAMPIONSHIP]`/`[ADVENTURE]`: gekwalificeerd als Position (0 = eerste) kleiner is. Bij QR_CHASE staan daar 300/270/240: wat de race dan als Position schrijft, is niet bekend.
- Kampioenschapsstand (record per autoklasse in het spelerobject, CH +0x2c + klasse*0x1ec, AD +0x7e0): +0x1c0 vier tegenstanders (willekeurig, nooit de eigen club, 0x4a2a50), +0x1d4 club van de speler, +0x1e4 (AD +0x1e8) ronde; per club (stap 56 bytes) +0x20 geld, +0x24 punten, +0x28 rang, +0x2c totale tijd, +0x34 turbo (start 100), en per klasse schade/upgrade. Reset 0x4a2be8.
- Punten en geld per plek (DATA 0x4c8750/0x4c8764, 0x4a517c): 30/25/15 punten, 5000/2500/1000. Let op: DATA-sectie heeft een andere offset dan CODE (bestand = VA - 0x400e00).
- Optellen (0x4be724, bij het maken van TFrmCHResultRanking): speler punten + geld (positie + [PLAYER] Money) + tijd + turbo, schade/upgrades per klasse; racers punten + tijd. Rang (0x4beb50): speler + 4 tegenstanders op punten (TList.Sort). Foto: Res1 als de speler leidt, anders cbimgdo<club>.
- Volgende op de stand (0x4beff8): ronde +1 (0x4a5054) zolang er races over zijn; anders speelmodus 2 en rebase naar FS_xxWIN.
- Flow-rebase: de gewone Volgende/Vorige (0x492ad0/0x492a8c) schakelt eerst naar de speelflow van het speltype met hetzelfde formulier (0x4941e4 -> 0x4940ac: QR STANDARD/DUEL, CH NEW/STANDARD, AD NEW/STANDARD; modus 2 -> WIN). Zo gaat ShowTrack aan het eind van FS_CHQUALIFIED verder in FS_CHSTANDARD. De uitslagformulieren stappen zonder rebase.
- Tabellen: elke cel is zo breed als zijn kolom (tot de volgende x) en wordt daar afgeknipt. Niet geverifieerd tegen het origineel, wel logisch voor de labels van TCtrlScore.
- ShowStatus (0x4c0954, AD gelijk): races over = races van de klasse - ronde; < 2 -> LT_MSG_ADSTATUS2, anders LT_MSG_ADSTATUS met LT_TXT_CB_<club> (tabel DATA 0x4c8aac) en het aantal. ImgClub = `car%.2da` van de clubauto voor de klasse. PnlRanking = layout D (0x4ba80c: speler + 4 tegenstanders, naam/punten/club, gesorteerd 0x4ba544). BtnRestart (0x4c0c38) vraagt LT_MSG_RESTART met LT_YES/LT_NO in TFrmRequester (0x4910a0), reset de competitie (0x4a4f38) en gaat naar FS_CHNEW bij TFrmPlayMovie.

## game.ini voor de race (9 nov 2026, gebouwd)
- Volledige specificatie per sleutel: `re/menu/gameini_spec.md` (uit de disassembly gecontroleerd; Ghidra mist daar argumenten en de staart van 0x499388).
- Gebouwd in `er/gameini.cpp`: PLAYER/ETAPPE/UPGRADES, NrOpponents per racetype {4,1,0,4,4,0}, RACERS `auto,startplek,0` + `country0i` + `Racer%d`, race-opties, cops, verkeer, bonussen, GameSim Qualify. Geschreven bij het tonen van TFrmRace naar de overlay (`$XDG_DATA_HOME/eracer/install/Ini/game.ini`, gedeelde `red::Install`).
- Nog niet: Input, GFX OPTIONS, Resolution, DIRECTX, Sound (komen pas als Opties bewaard worden; tot dan blijven de templatewaarden staan).
- QR_CHASE: de kwalificatiegrens op positie is 0 (de ini-waarden zijn tijden in ms op een ander veld), dus na de race kwalificeert alleen Position < 0. Wat de race bij een achtervolging als Position schrijft, is onbekend tot de race geport is.
- Upgrades van tegenstanders (0x4bea24, na de rangschikking): trap k zodra de ronde >= pad[klasse*3+k-1]-1. De exacte indexering in 0x4bea24 is gelezen uit verminkte Ghidra-uitvoer; niet hard bewezen.

## Garage (9 nov 2026, gebouwd)
- Specificatie: `re/menu/garage_spec.md`. Vijf aanbiedingen, rondlopend, start op de upgrade: nitro 5 l (1000, turbo +50), nitro 1 l (250, +10), upgrade (15000/25000/35000, trap +1), reparatie 5 % (250), reparatie 10 % (1000). Kopen alleen als het iets doet en het geld er is; geen bevestiging.
- Eigenschappenpaneel 0x4b7ce4: MpbCarProperties krijgt de maat van MensMal en ImgProperties (het briefje) komt op de plek van de balken; ImgClub = `car%.2da` van de clubauto. 7 rijen: upgrade/3, snelheid, acceleratie, grip, sterkte (op de huidige trap), schade, nitro.
- Niet gebouwd: de waardetekst "%3d/%3d" naast de balken (plek onbekend).
