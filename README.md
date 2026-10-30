# Jesienna Warszawa

Generatywna aplikacja muzyczna w klimacie ambient / dark jazz.

- `index.html` + `css/` + `js/` – aplikacja: generatywny zespół jazzowo-ambientowy.
  - **Utwór i harmonia:** nastrojowe progresje, standardy jazzowe i bluesy (tylko akordy, solówki są improwizowane), tonacja, forma (części nastrojowe albo jam session z solówkami po kolei i czwórkami z perkusją), wybór solisty, skala solówek, własna progresja wpisywana tekstem.
  - **Rytm:** tempo, metrum (od 2/4 do 12/8), zmiany metrum według sekwencji albo losowo co część, polirytm (każda warstwa może grać we własnym metrum, a wszystkie spotykają się na „raz”).
  - **Warstwy:** tło, bas, akordy, solista, perkusja – każda z wyborem instrumentu, przyciskiem Solo, głośnością, intensywnością (siła, gęstość, ozdobniki, swobodniejszy rytm) i własnym metrum.
  - Pokrętła nastroju Noc / Napięcie / Ruch i pasek do grania razem z muzyką.
- `odsluch/` – odsłuchiwacz biblioteki: każdy instrument osobno, syntezatory i efekty.
- `sample/` – biblioteka sampli. Każda nuta i każdy dźwięk to osobny plik MP3. Spis plików jest w `sample/manifest.json`.
- `narzedzia/` – skrypty, którymi pocięto i przygotowano sample.

## Nazwy plików

Instrumenty melodyczne: `sample/<instrument>/<nuta MIDI>_<dynamika>[_rr<n>].mp3`, np. `sample/rhodes/62_mf.mp3`.
60 = C4 (środkowe C), dynamiki to `p`, `mf` i `f`, a `rr` to warianty tego samego dźwięku.
Perkusja i efekty mają nazwy opisowe, np. `sample/talerz/suscymb1_hit_stick_pp1.mp3`.

## Licencje

Szczegóły i wymagane podpisy są w [`sample/LICENCJE.md`](sample/LICENCJE.md). W skrócie:

- VCSL, VSCO 2 CE, kontrabas D. Smolkena, Bear Sax: CC0.
- Chór (epic-choir / VocalSet): CC BY 4.0, wymagane podanie autorów.
- Rhodes (jRhodes3d, Jeff Learman): CC BY-NC-SA 4.0, tylko do użytku niekomercyjnego.
- Zestaw perkusyjny (Salamander Drumkit, Alexander Holm): CC BY-SA 3.0, wymagane podanie autora.

## Jak rozbudowywać

Kod jest podzielony tak, żeby nowe rzeczy dopisywać w jednym miejscu:

| Plik | Co tam jest | Jak dodać coś nowego |
|---|---|---|
| `js/theory.js` | akordy, skale, metra, biblioteka utworów | nowy utwór = jedna linijka w `SONGS` (akordy tekstem, np. `'Dm7 G7 Cmaj7:2'`); nowa skala w `SCALES`; nowe metrum w `METERS` (podział taktu na grupy ósemek, np. `[2,2,3]`) |
| `js/engine.js` | forma, oś czasu akordów i taktów, gracze (tło, bas, akordy, solówki, perkusja) | nowy instrument do warstwy w `LAYER_DEF`; zakres solówek w `RANGE`; nowe części formy w `SECTIONS`; nowe rytmy solówek w `CELLS_*`; motywy solówki perkusji w `DRUM_MOTIFS` |
| `js/audio.js` | wczytywanie sampli, efekty, głosy sampli i syntezatorów | nowy syntezator w `synthOn` |
| `js/ui.js` | panel sterowania | nowe kontrolki |
| `js/sky.js` | animowane tło | |

Zmiany w trakcie grania działają od razu (głośność, intensywność, solo, skala) albo od następnego taktu (utwór, tonacja, metrum, progresja).
