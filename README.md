# Jesienna Warszawa

Generatywna aplikacja muzyczna w klimacie ambient / dark jazz.

- `index.html` – odsłuchiwacz biblioteki: wszystkie instrumenty, syntezatory, delay, pogłos i nocna scena „Noc na Pradze”.
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
