# Jesienna Warszawa

Generatywna aplikacja muzyczna w klimacie ambient / dark jazz.

- `index.html` + `css/` + `js/` – aplikacja: generatywny zespół jazzowo-ambientowy.
  - **Utwór i harmonia:** nastrojowe progresje, standardy jazzowe i bluesy (tylko akordy, solówki są improwizowane), tonacja, forma (części nastrojowe albo jam session z solówkami po kolei i czwórkami z perkusją), wybór solisty, skala solówek, własna progresja wpisywana tekstem.
  - **Rytm:** tempo, metrum (od 2/4 do 12/8), zmiany metrum według sekwencji albo losowo co część, polirytm (każda warstwa może grać we własnym metrum, a wszystkie spotykają się na „raz”).
  - **Warstwy:** tło, bas, akordy, solista, perkusja – każda z wyborem instrumentu, przyciskiem Solo, głośnością, intensywnością (siła, gęstość, ozdobniki, swobodniejszy rytm) i własnym metrum.
  - Pokrętła nastroju Noc / Napięcie / Ruch i pasek do grania razem z muzyką.
- **Mikser** (przycisk „Mikser” u góry): dla każdej warstwy głośność, panorama, korektor (niskie / środek z wyborem częstotliwości / wysokie), filtr dołu, przester, kompresja, kolor (odpowiedź impulsowa sprzętu: kaseta, taśma szpulowa, gramofon, głośnik obrotowy, monitor, telefon) i wysyłki na przestrzeń i echo; master z wyborem przestrzeni, „niedoskonałością miejsca” (szum tła, buczenie, oddech miasta), korektorem, kolorem, efektem taśmy (falowanie, nasycenie, szum) i limiterem. Ustawienia zapisują się w telefonie.
- `ir/` – 32 prawdziwe odpowiedzi impulsowe (sale, kościół, Panteon, kawiarnie, piwnica, tunel pod mostem, garaż, klatka schodowa, zaułki, płyta i sprężyny pogłosowe, kaseta, taśma, gramofon, głośniki) z biblioteki [IR Library](https://github.com/itsmusician/IR-Library) Connera, licencja MIT (`ir/LICENSE-IR-Library.md`).
- `android/` + `.github/workflows/android.yml` – aplikacja na Androida: GitHub sam buduje plik APK po każdej zmianie (zakładka **Releases** → `jesienna-warszawa.apk`).
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

## Aplikacja na Androida

Po każdej zmianie w repozytorium GitHub Actions buduje APK (ok. 5–10 minut) i publikuje go w zakładce **Releases**.
Pobierz `jesienna-warszawa.apk` na telefonie i otwórz; za pierwszym razem Android poprosi o zgodę na instalację z tego źródła.
Kolejne wersje instalują się na poprzednią (wszystkie są podpisane tym samym kluczem z `android/jesienna.keystore` – to klucz tylko do prywatnej instalacji, nie do Sklepu Play).

## Wersja na PC (VST3 + aplikacja)

Folder `pc/` – wtyczka VST3 (FL Studio, Reaper, Ableton, Cubase…) i samodzielna aplikacja dla Windows, zbudowane na JUCE.
Instalator `JesiennaWarszawa-Setup.exe` budują GitHub Actions (`.github/workflows/pc.yml`) i wrzucają do zakładki **Releases** (tagi `pc-N`).

- **Gitara:** bramka, kompresor, przester (kostka), 7 modeli wzmacniaczy (oversampling ×4), 25 IR kolumn (21 prawdziwych 4×12 Greenback/V30 na CC0 + 4 syntetyczne komba), 5 charakterów mikrofonu, chorus/wibrato/tremolo, delay w tempie DAW, tuner.
- **Zespół:** perkusja (Salamander), kontrabas, Rhodes albo fortepian; 16 utworów (ballady, standardy, bluesy), styl swing/ballada/prosto/bossa, energia, swing. Gra w tempie, metrum i takcie projektu DAW (albo we własnym tempie).
- **Przestrzeń i wyjście:** 26 przestrzeni (kluby, tunele, ulice, kościoły, płyta, sprężyny), barwa (kasety, taśma szpulowa, gramofon, telefon), taśma (wow, nasycenie, szum), limiter, 8 gotowych brzmień.

Kod w `pc/` jest na licencji AGPL v3 (JUCE). Dane instalują się do `C:\ProgramData\Jesienna Warszawa`.
