"""Build a web-ready sample library: lib/<instrument>/<file>.mp3 + lib/manifest.json.
Melodic instruments: files named <midi>_<layer>[_rr<n>].mp3, pitch verified by autocorrelation."""
import numpy as np, glob, os, re, json, subprocess, sys, warnings
from scipy.io import wavfile
warnings.filterwarnings('ignore')
SR = 44100
OUT = '/home/claude/jw/lib'
V = '/home/claude/src/vcsl'
PC = {'c':0,'d':2,'e':4,'f':5,'g':7,'a':9,'b':11,'h':11}

def note_to_midi(s):
    m = re.fullmatch(r'([A-Ga-g])(#|s|b)?(-?\d)', s)
    if not m: return None
    p = PC[m[1].lower()] + (1 if m[2] in ('#','s') else -1 if m[2]=='b' else 0)
    return 12*(int(m[3])+1) + p

def read(p):
    if p.endswith('.flac'):
        raw = subprocess.run(['ffmpeg','-loglevel','error','-i',p,'-f','f32le','-ac','2','-ar',str(SR),'-'],capture_output=True).stdout
        x = np.frombuffer(raw,np.float32).reshape(-1,2); return x
    sr, d = wavfile.read(p)
    if np.issubdtype(d.dtype, np.integer): d = d.astype(np.float32)/(2**(8*d.dtype.itemsize-1))
    d = d.astype(np.float32)
    if d.ndim == 1: d = d[:,None]
    if sr != SR:
        from scipy.signal import resample_poly
        d = resample_poly(d, SR, sr, axis=0).astype(np.float32)
    return d

def f0(x):
    x = x.mean(1)
    st = int(.15*SR); seg = x[st:st+16384] if len(x) > st+16384 else x[:16384]
    seg = seg - seg.mean(); n = len(seg)
    if n < 2048: return None
    X = np.fft.rfft(seg*np.hanning(n), 2*n); ac = np.fft.irfft(np.abs(X)**2)[:n]; ac /= ac[0]+1e-9
    lo, hi = int(SR/2100), int(SR/38)
    seg_ac = ac[lo:hi]
    # first strong peak (avoids octave-down errors of plain argmax)
    best = np.argmax(seg_ac); thr = seg_ac[best]*.85
    for i in range(1,len(seg_ac)-1):
        if seg_ac[i] > thr and seg_ac[i] >= seg_ac[i-1] and seg_ac[i] >= seg_ac[i+1]: best = i; break
    return SR/(lo+best)

def trim(x, maxlen, th=.004):
    a = np.abs(x).max(1); idx = int(np.argmax(a > th*max(a.max(),1e-6)))
    x = x[max(0, idx-64):]
    if maxlen and len(x) > maxlen*SR:
        x = x[:int(maxlen*SR)].copy(); n = int(.5*SR); x[-n:] *= (np.linspace(1,0,n)**2)[:,None]
    # cut trailing near-silence
    a = np.abs(x).max(1); last = len(a)-int(np.argmax(a[::-1] > 1e-4*max(a.max(),1e-6)))
    return x[:max(last, int(.05*SR))]

def enc(x, path, stereo, gain):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp = '/tmp/_b.wav'
    y = x*gain if stereo else x.mean(1, keepdims=True)*gain
    wavfile.write(tmp, SR, np.clip(y,-1,1).astype(np.float32))
    subprocess.run(['ffmpeg','-y','-loglevel','error','-i',tmp,'-b:a', '160k' if stereo else '112k', path], check=True)

manifest = {}
report = []

def melodic(key, label, files, stereo=False, maxlen=10, check=True):
    """files: list of (path, written_midi, layer, rr)"""
    items = [(p, m, l, rr, trim(read(p), maxlen)) for p, m, l, rr in files]
    # verify octave convention with pitch detection: use the median offset (in octaves)
    off = 0
    if check:
        diffs = []
        for p, m, l, rr, x in items:
            hz = f0(x)
            if hz: diffs.append(69+12*np.log2(hz/440) - m)
        d = np.array(diffs); octs = np.round(d/12)
        off = int(np.median(octs))*12
        bad = int(np.sum(np.abs(d - off) > 1.2))
        report.append(f'{key}: {len(items)} plików, przesunięcie oktawy {off:+d}, niezgodnych {bad}')
    peak = max(np.abs(x).max() for *_, x in items)
    entry = {'label': label, 'type': 'melodic', 'layers': {}}
    for p, m, l, rr, x in items:
        mm = m + off
        name = f'{mm}_{l}' + (f'_rr{rr}' if rr else '') + '.mp3'
        enc(x, f'{OUT}/{key}/{name}', stereo, .89/peak)
        entry['layers'].setdefault(l, {}).setdefault(str(mm), []).append(name)
    manifest[key] = entry

def perc(key, label, files, stereo=True, maxlen=12):
    """files: list of (path, name)"""
    xs = [(n, trim(read(p), maxlen)) for p, n in files]
    peak = max(np.abs(x).max() for _, x in xs)
    entry = {'label': label, 'type': 'percussion', 'sounds': []}
    for n, x in xs:
        enc(x, f'{OUT}/{key}/{n}.mp3', stereo, .89/peak); entry['sounds'].append(n+'.mp3')
    manifest[key] = entry
    report.append(f'{key}: {len(xs)} dźwięków')

def g(pattern): return sorted(glob.glob(pattern))
def rx(path, pat): return re.search(pat, os.path.basename(path))

only = set(sys.argv[1:])
def want(k): return not only or k in only

# ---------- KEYS ----------
if want('pianino'):
    fs = []
    for p in g(f'{V}/Chordophones/Zithers/Upright Piano, Yamaha/Sustains/*_rr1.wav'):
        m = rx(p, r'_Sus_([A-G]#?-?\d)_vl(\d)_rr1'); fs.append((p, note_to_midi(m[1]), {'1':'p','2':'mf','3':'f'}[m[2]], 0))
    melodic('pianino', 'Pianino (Yamaha)', fs, maxlen=9)
if want('rhodes'):
    fs = []
    for p in g('/home/claude/src/rh3d/jRhodes3d-mono/*.flac'):
        m = rx(p, r'_(\d{3})__.*_(\d)\.flac')
        if m and m[2] in '135': fs.append((p, int(m[1]), {'1':'p','3':'mf','5':'f'}[m[2]], 0))
    melodic('rhodes', 'Rhodes (jRhodes3d)', fs, maxlen=8)
if want('fm_piano'):
    fs = []
    for p in g(f'{V}/Electrophones/TX81Z/FM Piano/*.wav'):
        m = rx(p, r'_([A-G]#?-?\d)_vl(\d)'); fs.append((p, note_to_midi(m[1]), {'1':'p','2':'mf','3':'f'}[m[2]], 0))
    melodic('fm_piano', 'FM Piano (TX81Z)', fs, maxlen=7)
if want('wibrafon'):
    fs = []
    for p in g(f'{V}/Idiophones/Struck Idiophones/Vibraphone/Soft Mallets/*_rr1_*.wav') + g(f'{V}/Idiophones/Struck Idiophones/Vibraphone/Hard Mallets/*_v3_rr1_*.wav'):
        m = rx(p, r'Vibes_(soft|hard)_([A-G]#?-?\d)_v(\d)'); lay = {'soft1':'p','soft2':'mf','hard3':'f'}[m[1]+m[3]]
        fs.append((p, note_to_midi(m[2]), lay, 0))
    melodic('wibrafon', 'Wibrafon', fs, maxlen=8)
    fs = [(p, note_to_midi(rx(p, r'_bowed_([A-G]#?-?\d)')[1]), 'mf', 0) for p in g(f'{V}/Idiophones/Struck Idiophones/Vibraphone/Bowed/*.wav')]
    melodic('wibrafon_smyczek', 'Wibrafon smyczkiem', fs, maxlen=10)
if want('dzwonki'):
    fs = [(p, note_to_midi(rx(p, r'sus_([A-G]#?-?\d)_')[1]), 'mf', 0) for p in g(f'{V}/Idiophones/Struck Idiophones/Hand Chimes/*.wav')]
    melodic('dzwonki', 'Dzwonki ręczne', fs, maxlen=8)
if want('kieliszki'):
    fs = []
    for p in g(f'{V}/Idiophones/Friction Idiophones/Wine Glasses/Sustains/Slow/*.wav'):
        m = rx(p, r'glass\d_([A-G]#?-?\d)_'); fs.append((p, note_to_midi(m[1]), 'mf', 0))
    melodic('kieliszki', 'Kieliszki (pocierane)', fs, stereo=True, maxlen=12)

# ---------- WINDS / BASS / VOICE ----------
if want('sax_tenor'):
    fs = []
    for p in g(f'{V}/Aerophones/Reed Aerophones/Tenor Saxophone/Non-Vibrato/*.wav'):
        m = rx(p, r'_NV_Main_([A-G]#?-?\d)_vl(\d)'); fs.append((p, note_to_midi(m[1]), {'2':'p','3':'f'}[m[2]], 0))
    melodic('sax_tenor', 'Saksofon tenorowy', fs, maxlen=8)
    fs = []
    for p in g(f'{V}/Aerophones/Reed Aerophones/Tenor Saxophone/Staccato/*_rr1.wav'):
        m = rx(p, r'_Staccato_Main_([A-G]#?-?\d)_vl(\d)'); fs.append((p, note_to_midi(m[1]), {'1':'p','2':'f'}[m[2]], 0))
    melodic('sax_tenor_krotkie', 'Saksofon tenorowy – krótkie', fs, maxlen=2)
if want('kontrabas'):
    fs = []
    for p in g('/home/claude/src/dsmolken.double-bass/pizz/pizz_*.wav'):
        m = rx(p, r'pizz_([a-g]s?\d)_([pmf])([ab])\.wav')
        if m: fs.append((p, note_to_midi(m[1]), {'p':'p','m':'mf','f':'f'}[m[2]], 1 if m[3]=='a' else 2))
    melodic('kontrabas', 'Kontrabas pizzicato', fs, maxlen=6)
if want('chor'):
    for part, pl in [('A','alty'),('T','tenory')]:
        for vow in ['ah','oo']:
            fs = []
            for p in g(f'/home/claude/dl/choir/samples/{part}/{part}_{vow}_*.wav'):
                m = rx(p, rf'{part}_{vow}_(p|mf|f)_([A-G]s?\d)\.wav'); fs.append((p, note_to_midi(m[2]), m[1], 0))
            melodic(f'chor_{pl}_{vow}', f'Chór – {pl} „{vow}”', fs, stereo=True, maxlen=10)

# ---------- PERCUSSION ----------
if want('perkusja'):
    P = f'{V}/Idiophones/Struck Idiophones'; M = f'{V}/Membranophones/Struck Membranophones'
    def clean(s): return re.sub(r'(_Mid|_Main|\.wav)$','',re.sub(r'_Mid\.wav$|\.wav$','',s)).lower()
    perc('hihat', 'Hi-hat', [(p, clean(os.path.basename(p))) for p in g(f'{P}/Hi-Hat Cymbal/*.wav')])
    perc('talerz', 'Talerz zawieszony (uderzenia, smyczek, crescendo, tremolo)', [(p, clean(os.path.basename(p))) for p in g(f'{P}/Suspended Cymbal 1/*.wav') if 'rel' not in p])
    perc('werbel', 'Werbel', [(p, clean(os.path.basename(p))) for p in g(f'{M}/Snare Drum, Modern 1/*.wav') if '_rr1' in p or 'rr' not in p])
    perc('beben', 'Bęben basowy (orkiestrowy)', [(p, clean(os.path.basename(p))) for p in g(f'{M}/Bass Drum 2/*.wav') if '_rel' not in p])
    perc('tom', 'Tom (pałeczka i pałka filcowa)', [(p, clean(os.path.basename(p))) for p in g(f'{M}/Tom 1/*/*.wav') + g(f'{M}/Tom 1/*.wav')])
    perc('shaker', 'Shaker', [(p, clean(os.path.basename(p))) for p in g(f'{P}/Shaker, Small/*.wav') if '_2' not in os.path.basename(p)])
    perc('mark_tree', 'Mark tree (dzwoneczki)', [(p, clean(os.path.basename(p))) for p in g(f'{P}/Mark Trees/Legacy/*.wav')])
    perc('kontrabas_szumy', 'Kontrabas – stuki i szumy', [(p, clean(os.path.basename(p))) for p in g('/home/claude/src/dsmolken.double-bass/pizz/noises/*.wav')], stereo=False)

old = {}
if os.path.exists(f'{OUT}/manifest.json'): old = json.load(open(f'{OUT}/manifest.json'))
old.update(manifest)
json.dump(old, open(f'{OUT}/manifest.json','w'), ensure_ascii=False, indent=1)
print('\n'.join(report))
