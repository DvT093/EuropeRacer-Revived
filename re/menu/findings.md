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
