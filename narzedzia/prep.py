import numpy as np, struct, subprocess, glob, os, re, json, sys
from scipy.io import wavfile
OUT='/home/claude/jw/site/s'
PCS={'c':0,'db':1,'d':2,'eb':3,'e':4,'f':5,'gb':6,'g':7,'ab':8,'a':9,'bb':10,'b':11}
PCS_UP={'C':0,'C#':1,'D':2,'D#':3,'E':4,'F':5,'F#':6,'G':7,'G#':8,'A':9,'A#':10,'B':11}
SR=44100
def readwav(p):
    sr,d=wavfile.read(p)
    d=d.astype(np.float32)
    if d.dtype!=np.float32 or np.abs(d).max()>2: pass
    if np.issubdtype(wavfile.read(p)[1].dtype,np.integer):
        d=d/ (2**(8*wavfile.read(p)[1].dtype.itemsize-1))
    if d.ndim>1: d=d.mean(1)
    if sr!=SR:
        from scipy.signal import resample_poly
        d=resample_poly(d,SR,sr).astype(np.float32)
    return d
def smpl_loop(p):
    f=open(p,'rb').read(); i=12
    while i<len(f):
        cid=f[i:i+4]; sz=struct.unpack('<I',f[i+4:i+8])[0]
        if cid==b'smpl':
            d=f[i+8:i+8+sz]; n=struct.unpack('<I',d[28:32])[0]
            if n: s,e=struct.unpack('<6I',d[36:60])[2:4]; return s,e
        i+=8+sz+(sz&1)
def f0(x):
    x=x[int(0.3*SR):int(0.3*SR)+8192] if len(x)>int(0.3*SR)+8192 else x[:8192]
    x=x-x.mean(); n=len(x)
    X=np.fft.rfft(x,2*n); ac=np.fft.irfft(X*np.conj(X))[:n]
    lo,hi=int(SR/1200),int(SR/45)
    lag=lo+np.argmax(ac[lo:hi]); return SR/lag
def midi_of(hz): return 69+12*np.log2(hz/440)
def trim_start(x,th=0.003):
    idx=np.argmax(np.abs(x)>th); return x[max(0,idx-64):]
def fade(x,maxlen):
    x=x[:int(maxlen*SR)].copy(); n=min(len(x),int(0.4*SR)); x[-n:]*=np.linspace(1,0,n)**2; return x
def enc(x,name,peak):
    tmp='/tmp/_t.wav'; wavfile.write(tmp,SR,(x/peak*0.89).astype(np.float32))
    subprocess.run(['ffmpeg','-y','-loglevel','error','-i',tmp,'-ac','1','-b:a','96k',f'{OUT}/{name}.mp3'],check=True)
manifest={}
def add(inst,layer,midi,name): manifest.setdefault(inst,{}).setdefault(layer,{})[midi]=name
report=[]
# --- Bear sax looped sustains -> extend to 9 s
SAX='/home/claude/sfzinstruments/karoryfer.bear-sax/Samples'
items=[]
for p in sorted(glob.glob(SAX+'/*_looped_p.wav')):
    b=os.path.basename(p); m=re.match(r'([a-g]b?)(\d)_looped',b); midi=12*(int(m[2])+2)+PCS[m[1]]
    for lay in ['p','f']:
        q=p.replace('_looped_p',f'_looped_{lay}'); x=readwav(q); ls,le=smpl_loop(q)
        body=x[:le+1]; loop=x[ls:le+1]
        while len(body)<9*SR: body=np.concatenate([body,loop])
        items.append(('sax',lay,midi,fade(body,9),f'sax_{lay}_{midi}'))
    det=midi_of(f0(readwav(p))); report.append(('sax',midi,round(det,2)))
# sax staccato vl3 rr1 (short)
for p in sorted(glob.glob(SAX+'/*_vl3_rr1.wav')):
    b=os.path.basename(p); m=re.match(r'([a-g]b?)(\d)_vl3',b); midi=12*(int(m[2])+2)+PCS[m[1]]
    items.append(('saxst','f',midi,fade(trim_start(readwav(p)),1.5),f'saxst_{midi}'))
# --- VSCO trumpet
V='/home/claude/sgossner/vsco-2-ce/Brass/Trumpet'
for d,inst in [('sus','tr'),('harmonM-sus','trh')]:
    for p in sorted(glob.glob(f'{V}/{d}/*.wav')):
        b=os.path.basename(p); m=re.search(r'_([A-G]#?)(\d)_v(\d)_',b)
        midi=12*(int(m[2])+2)+PCS_UP[m[1]]; lay='p' if m[3]=='1' else 'f'
        x=trim_start(readwav(p)); det=midi_of(f0(x))
        report.append((inst,b,midi,round(det,2)))
        items.append((inst,lay,midi,fade(x,8),f'{inst}_{lay}_{midi}'))
json.dump([list(map(str,r)) for r in report],open('/tmp/report.json','w'))
for r in report: print(r)
# loudness: normalize per instrument by the loudest layer so dynamics stay relative
peaks={}
for inst,lay,midi,x,name in items:
    rms=np.sqrt(np.mean(x[:int(2*SR)]**2)); peaks.setdefault(inst,[]).append(np.abs(x).max())
for inst,lay,midi,x,name in items:
    enc(x,name,max(peaks[inst])); add(inst,lay,midi,name)
json.dump(manifest,open(f'{OUT}/../manifest.json','w'),indent=0)
