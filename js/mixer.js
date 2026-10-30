// Mixer: a channel strip for every layer (EQ, filter, drive, compression, colour, pan, sends) and a master section
// with the space (real impulse responses of rooms, halls, clubs, tunnels and streets), its imperfections,
// master EQ, tape / cassette colour and a limiter. Settings are remembered on the phone.
'use strict';

const MX_DEFAULT_CH = {pan:0, mute:false, solo:false, hpf:30, low:0, mid:0, midF:900, high:0, drive:0, comp:.2, color:'none', colorMix:.6, rev:.5, dly:0};
const MX = {
  ch: {
    tlo:   {...MX_DEFAULT_CH, hpf:140, mid:1, midF:2800, rev:.75, pan:0},
    bas:   {...MX_DEFAULT_CH, hpf:30, rev:.25, comp:.35, pan:0},
    akordy:{...MX_DEFAULT_CH, hpf:110, high:1, rev:.55, dly:.1, pan:-.18},
    solo:  {...MX_DEFAULT_CH, hpf:110, mid:2, midF:2600, rev:.7, dly:.7, pan:.15},
    perk:  {...MX_DEFAULT_CH, hpf:40, rev:.35, comp:.3, pan:.05},
  },
  master: {space:'studio_klub', spaceMix:.6, grime:.25, low:0, mid:0, midF:1200, high:0, color:'none', colorMix:.5, tape:0, comp:.35, width:1},
};
try { const saved = JSON.parse(localStorage.getItem('jw_mixer')||'null'); if (saved){ for (const id in saved.ch||{}) Object.assign(MX.ch[id]||{}, saved.ch[id]); Object.assign(MX.master, saved.master||{}); } } catch(e){}
function saveMixer(){ try { localStorage.setItem('jw_mixer', JSON.stringify(MX)); } catch(e){} }

let IRS = [];                     // list from ir/ir.json
const irBuffers = {};
const CH = {};                    // id -> nodes
let MS = null;                    // master nodes

async function loadIRList(){ if (!IRS.length){ try { IRS = await (await fetch('ir/ir.json')).json(); } catch(e){ IRS=[]; } } return IRS; }
async function loadIR(id){
  if (!id || id==='none') return null;
  if (!irBuffers[id]){
    const meta = IRS.find(x=>x.id===id); if (!meta) return null;
    irBuffers[id] = decode(meta.file).then(trimIR).then(async b=>{ b._g = await irGain(b); return b; });
  }
  return irBuffers[id];
}
function trimIR(b){ // remove the codec's leading silence so a colour insert does not delay the beat
  const d=b.getChannelData(0); let mx=0; for (let i=0;i<Math.min(d.length,b.sampleRate);i++) mx=Math.max(mx,Math.abs(d[i]));
  let s=0; while (s<d.length && Math.abs(d[s])<mx*.05) s++; s=Math.max(0,s-4);
  const n=b.length-s, out=ctx.createBuffer(b.numberOfChannels, n, b.sampleRate);
  for (let c=0;c<b.numberOfChannels;c++) out.copyToChannel(b.getChannelData(c).subarray(s), c);
  return out;
}
// how much louder/quieter a convolver with this IR makes a sound → used to keep every space and colour at the same level
async function irGain(b){
  try {
    const sr=b.sampleRate, oc=new OfflineAudioContext(2, Math.floor(sr*1.2), sr);
    const nb=oc.createBuffer(1, sr, sr), d=nb.getChannelData(0); let pk=0; for (let i=0;i<d.length;i++){ pk=pk*.7+(Math.random()*2-1)*.3; d[i]=pk; }
    const src=oc.createBufferSource(); src.buffer=nb; const cv=oc.createConvolver(); cv.buffer=b; src.connect(cv); cv.connect(oc.destination); src.start();
    const out=await oc.startRendering(), o=out.getChannelData(0);
    let ei=0, eo=0; for (let i=0;i<d.length;i++) ei+=d[i]*d[i]; for (let i=Math.floor(sr*.1); i<Math.floor(sr*1.1); i++) eo+=o[i]*o[i];
    return clamp(Math.sqrt(ei/Math.max(eo,1e-12)), .05, 20);
  } catch(e){ return 1; }
}
let synthIRGain = null;
// soft saturation with unity gain for quiet signals: it only rounds off the peaks (no jump in loudness)
function driveCurve(amount){ const n=2048, g=1+amount*8, c=new Float32Array(n); for (let i=0;i<n;i++){ const x=(i/(n-1)*2-1)*1.5; c[i]=Math.tanh(g*x)/g; } return c; }

// ---------- channel strip ----------
function buildChannel(id){
  const n = {};
  n.in = ctx.createGain();                                    // volume (layer slider)
  n.drive = ctx.createWaveShaper(); n.drive.oversample='2x';
  n.hpf = ctx.createBiquadFilter(); n.hpf.type='highpass'; n.hpf.Q.value=.6;
  n.low = ctx.createBiquadFilter(); n.low.type='lowshelf'; n.low.frequency.value=140;
  n.mid = ctx.createBiquadFilter(); n.mid.type='peaking'; n.mid.Q.value=.9;
  n.high = ctx.createBiquadFilter(); n.high.type='highshelf'; n.high.frequency.value=7000;
  n.comp = ctx.createDynamicsCompressor(); n.comp.attack.value=.008; n.comp.release.value=.2; n.comp.knee.value=8;
  n.colorDry = ctx.createGain(); n.colorWet = ctx.createGain(); n.colorConv = ctx.createConvolver();
  n.pan = ctx.createStereoPanner ? ctx.createStereoPanner() : ctx.createGain();
  n.mute = ctx.createGain();
  n.sendRev = ctx.createGain(); n.sendDly = ctx.createGain();
  n.in.connect(n.drive); n.drive.connect(n.hpf); n.hpf.connect(n.low); n.low.connect(n.mid); n.mid.connect(n.high); n.high.connect(n.comp);
  n.comp.connect(n.colorDry); n.colorDry.connect(n.pan);
  n.comp.connect(n.colorConv); n.colorConv.connect(n.colorWet); n.colorWet.connect(n.pan);
  n.pan.connect(n.mute); n.mute.connect(dryOut); n.mute.connect(n.sendRev); n.mute.connect(n.sendDly);
  n.sendRev.connect(revIn); n.sendDly.connect(dlyIn);
  CH[id] = n; applyChannel(id);
  return n;
}
function channelIn(id){ return (CH[id] || buildChannel(id)).in; }
function anySolo(){ return Object.values(MX.ch).some(c=>c.solo); }
async function applyChannel(id){
  const n=CH[id], c=MX.ch[id]; if (!n) return; const t=ctx.currentTime, set=(p,v)=>p.setTargetAtTime(v,t,.03);
  set(n.in.gain, volCurve(L[id].vol));
  n.drive.curve = c.drive>0 ? driveCurve(c.drive) : null;
  set(n.hpf.frequency, c.hpf); set(n.low.gain, c.low); set(n.mid.gain, c.mid); set(n.mid.frequency, c.midF); set(n.high.gain, c.high);
  n.comp.threshold.value = -6 - c.comp*30; n.comp.ratio.value = 1 + c.comp*7;
  if (n.pan.pan) set(n.pan.pan, c.pan);
  const audible = !c.mute && (!anySolo() || c.solo); set(n.mute.gain, audible ? 1 : 0);
  set(n.sendRev.gain, c.rev); set(n.sendDly.gain, c.dly);
  if (c.color && c.color!=='none'){ const b=await loadIR(c.color); if (b && n.colorConv.buffer!==b) n.colorConv.buffer=b; set(n.colorWet.gain, c.colorMix*(b?._g ?? 1)); set(n.colorDry.gain, 1-c.colorMix); }
  else { set(n.colorWet.gain, 0); set(n.colorDry.gain, 1); }
}
function applyAllChannels(){ for (const id in MX.ch) if (CH[id]) applyChannel(id); }

// ---------- master ----------
// fogF (mood) → EQ → colour (IR) → tape (wow/flutter, saturation, hiss) → compressor → limiter → volume
function initMixer(){
  if (MS) return;
  MS = {};
  MS.revReturn = ctx.createGain();
  conv.disconnect(); conv.connect(MS.revReturn); MS.revReturn.connect(fogF);
  fogF.disconnect(); comp.disconnect();
  MS.low=ctx.createBiquadFilter(); MS.low.type='lowshelf'; MS.low.frequency.value=120;
  MS.mid=ctx.createBiquadFilter(); MS.mid.type='peaking'; MS.mid.Q.value=.8;
  MS.high=ctx.createBiquadFilter(); MS.high.type='highshelf'; MS.high.frequency.value=8000;
  MS.colorDry=ctx.createGain(); MS.colorWet=ctx.createGain(); MS.colorConv=ctx.createConvolver();
  MS.tapeIn=ctx.createGain(); MS.wow=ctx.createDelay(.1); MS.wow.delayTime.value=.012;
  MS.wowLfo=ctx.createOscillator(); MS.wowLfo.frequency.value=.55; MS.wowDepth=ctx.createGain();
  MS.flutLfo=ctx.createOscillator(); MS.flutLfo.frequency.value=6.3; MS.flutDepth=ctx.createGain();
  MS.wowLfo.connect(MS.wowDepth); MS.wowDepth.connect(MS.wow.delayTime); MS.flutLfo.connect(MS.flutDepth); MS.flutDepth.connect(MS.wow.delayTime);
  MS.wowLfo.start(); MS.flutLfo.start();
  MS.tapeDry=ctx.createGain(); MS.tapeWet=ctx.createGain(); MS.sat=ctx.createWaveShaper(); MS.sat.oversample='2x';
  MS.tapeTone=ctx.createBiquadFilter(); MS.tapeTone.type='lowpass'; MS.tapeTone.frequency.value=12000;
  MS.hiss=ctx.createBufferSource(); MS.hiss.buffer=noise(); MS.hiss.loop=true; MS.hissF=ctx.createBiquadFilter(); MS.hissF.type='highpass'; MS.hissF.frequency.value=3000; MS.hissG=ctx.createGain(); MS.hissG.gain.value=0;
  MS.hiss.connect(MS.hissF); MS.hissF.connect(MS.hissG); MS.hiss.start();
  MS.limiter=ctx.createDynamicsCompressor(); MS.limiter.threshold.value=-1.5; MS.limiter.knee.value=0; MS.limiter.ratio.value=20; MS.limiter.attack.value=.002; MS.limiter.release.value=.12;
  // wiring
  fogF.connect(MS.low); MS.low.connect(MS.mid); MS.mid.connect(MS.high);
  MS.high.connect(MS.colorDry); MS.high.connect(MS.colorConv); MS.colorConv.connect(MS.colorWet);
  MS.colorDry.connect(MS.tapeIn); MS.colorWet.connect(MS.tapeIn);
  MS.tapeIn.connect(MS.tapeDry); MS.tapeIn.connect(MS.wow); MS.wow.connect(MS.sat); MS.sat.connect(MS.tapeTone); MS.tapeTone.connect(MS.tapeWet);
  MS.tapeDry.connect(comp); MS.tapeWet.connect(comp); MS.hissG.connect(comp);
  comp.connect(MS.limiter); MS.limiter.connect(master);
  // imperfections of the place: room tone / hum / distant city, filtered by the space itself
  MS.amb=ctx.createBufferSource(); MS.amb.buffer=noise(); MS.amb.loop=true; MS.ambF=ctx.createBiquadFilter(); MS.ambF.type='lowpass'; MS.ambF.frequency.value=500;
  MS.ambLfo=ctx.createOscillator(); MS.ambLfo.frequency.value=.07; MS.ambLfoG=ctx.createGain(); MS.ambLfoG.gain.value=0; MS.ambLfo.connect(MS.ambLfoG);
  MS.ambG=ctx.createGain(); MS.ambG.gain.value=0; MS.ambLfoG.connect(MS.ambG.gain);
  MS.hum=ctx.createOscillator(); MS.hum.frequency.value=50; MS.hum.type='sawtooth'; MS.humF=ctx.createBiquadFilter(); MS.humF.type='lowpass'; MS.humF.frequency.value=300; MS.humG=ctx.createGain(); MS.humG.gain.value=0;
  MS.amb.connect(MS.ambF); MS.ambF.connect(MS.ambG); MS.ambG.connect(revIn); MS.ambG.connect(dryOut);
  MS.hum.connect(MS.humF); MS.humF.connect(MS.humG); MS.humG.connect(dryOut);
  MS.amb.start(); MS.ambLfo.start(); MS.hum.start();
  // existing layer buses now feed the channel strips
  applyMaster();
}
const GRIME = { // what an imperfect place adds: background rumble colour, hum, how much it breathes
  'Studia i pokoje':   {f:300, amb:.002, hum:.0006, lfo:.2},
  'Sale i świątynie':  {f:250, amb:.003, hum:0,     lfo:.3},
  'Kluby i kawiarnie': {f:700, amb:.008, hum:.0025, lfo:.6},
  'Tunele i garaże':   {f:160, amb:.012, hum:.0012, lfo:.8},
  'Ulice':             {f:400, amb:.014, hum:0,     lfo:1},
  'Pogłosy sprzętowe': {f:2000,amb:.002, hum:.0015, lfo:.1},
};
async function applyMaster(){
  if (!MS) return; const m=MX.master, t=ctx.currentTime, set=(p,v)=>p.setTargetAtTime(v,t,.05);
  await loadIRList();
  const sp = IRS.find(x=>x.id===m.space);
  if (synthIRGain===null) synthIRGain = await irGain(conv.buffer);
  let spG = 1;
  if (sp){ const b=await loadIR(sp.id); if (b){ if (conv.buffer!==b) conv.buffer=b; spG = b._g / synthIRGain; } }
  set(MS.revReturn.gain, m.spaceMix*1.4*spG);                       // every space sits at a similar level
  const gr = GRIME[sp?.group] || GRIME['Studia i pokoje'];
  set(MS.ambF.frequency, gr.f); set(MS.ambG.gain, gr.amb*m.grime*3); set(MS.ambLfoG.gain, gr.amb*m.grime*3*gr.lfo); set(MS.humG.gain, gr.hum*m.grime*3);
  set(MS.low.gain, m.low); set(MS.mid.gain, m.mid); set(MS.mid.frequency, m.midF); set(MS.high.gain, m.high);
  if (m.color && m.color!=='none'){ const b=await loadIR(m.color); if (b && MS.colorConv.buffer!==b) MS.colorConv.buffer=b; set(MS.colorWet.gain, m.colorMix*(b?._g ?? 1)); set(MS.colorDry.gain, 1-m.colorMix); }
  else { set(MS.colorWet.gain, 0); set(MS.colorDry.gain, 1); }
  // tape: pitch wobble (wow & flutter), soft saturation, darker top, hiss
  const tp=m.tape;
  set(MS.tapeWet.gain, tp>0 ? 1 : 0); set(MS.tapeDry.gain, tp>0 ? 0 : 1);
  set(MS.wowDepth.gain, tp*.0018); set(MS.flutDepth.gain, tp*.00025);
  MS.sat.curve = tp>0 ? driveCurve(.05 + tp*.35) : null;
  set(MS.tapeTone.frequency, 16000 - tp*9000); set(MS.hissG.gain, tp*tp*.012);
  comp.threshold.value = -6 - m.comp*22; comp.ratio.value = 1.5 + m.comp*4;
  saveMixer();
}

// ---------- UI ----------
const LNAME = {tlo:'Tło', bas:'Bas', akordy:'Akordy', solo:'Solista', perk:'Perkusja'};
function mxSlider(label, min, max, step, val, fmt, onInput){
  const w=document.createElement('label'); w.className='mxs';
  const top=document.createElement('span'); top.innerHTML=`${label} <b></b>`; const b=top.querySelector('b');
  const r=document.createElement('input'); r.type='range'; r.min=min; r.max=max; r.step=step; r.value=val; b.textContent=fmt(+val);
  r.oninput=()=>{ b.textContent=fmt(+r.value); onInput(+r.value); };
  w.append(top, r); return w;
}
const fmtDb = v => (v>0?'+':'')+v.toFixed(1)+' dB', fmtPct = v => Math.round(v*100)+'%', fmtHz = v => v>=1000 ? (v/1000).toFixed(1)+' kHz' : Math.round(v)+' Hz';
const fmtPan = v => Math.abs(v)<.03 ? 'środek' : (v<0 ? 'L ' : 'P ')+Math.round(Math.abs(v)*100);
function irSelect(kind, value, withNone, onChange){
  const s=document.createElement('select');
  if (withNone) s.appendChild(new Option('brak', 'none'));
  const groups={}; for (const ir of IRS.filter(x=>kind==='any' || x.kind===kind)) (groups[ir.group] ||= []).push(ir);
  for (const g in groups){ const og=document.createElement('optgroup'); og.label=g; for (const ir of groups[g]) og.appendChild(new Option(ir.name, ir.id)); s.appendChild(og); }
  s.value=value; s.onchange=()=>onChange(s.value); return s;
}
async function buildMixerUI(){
  await loadIRList();
  const box=$('mixer'); box.innerHTML='';
  // master
  const m=MX.master, mc=document.createElement('div'); mc.className='card';
  mc.innerHTML='<div class="label">Przestrzeń i master</div>';
  const spW=document.createElement('label'); spW.className='fld'; spW.textContent='Gdzie gra zespół';
  spW.appendChild(irSelect('space', m.space, false, v=>{ m.space=v; applyMaster(); }));
  const g=document.createElement('div'); g.className='mxgrid';
  g.append(
    mxSlider('Ilość przestrzeni', 0, 1, .01, m.spaceMix, fmtPct, v=>{ m.spaceMix=v; applyMaster(); }),
    mxSlider('Niedoskonałość miejsca', 0, 1, .01, m.grime, fmtPct, v=>{ m.grime=v; applyMaster(); }),
    mxSlider('Niskie', -12, 12, .5, m.low, fmtDb, v=>{ m.low=v; applyMaster(); }),
    mxSlider('Środek', -12, 12, .5, m.mid, fmtDb, v=>{ m.mid=v; applyMaster(); }),
    mxSlider('Częstotliwość środka', 200, 5000, 10, m.midF, fmtHz, v=>{ m.midF=v; applyMaster(); }),
    mxSlider('Wysokie', -12, 12, .5, m.high, fmtDb, v=>{ m.high=v; applyMaster(); }),
    mxSlider('Kompresja', 0, 1, .01, m.comp, fmtPct, v=>{ m.comp=v; applyMaster(); }),
    mxSlider('Taśma (falowanie, nasycenie, szum)', 0, 1, .01, m.tape, fmtPct, v=>{ m.tape=v; applyMaster(); }));
  const colW=document.createElement('label'); colW.className='fld'; colW.textContent='Kolor całości (sprzęt)';
  colW.appendChild(irSelect('color', m.color, true, v=>{ m.color=v; applyMaster(); }));
  const colM=mxSlider('Ilość koloru', 0, 1, .01, m.colorMix, fmtPct, v=>{ m.colorMix=v; applyMaster(); });
  mc.append(spW, g, colW, colM); box.appendChild(mc);
  // channels
  for (const id of ['solo','akordy','bas','perk','tlo']){
    const c=MX.ch[id], card=document.createElement('div'); card.className='card mxch';
    const head=document.createElement('div'); head.className='mxhead';
    head.innerHTML=`<b>${LNAME[id]}</b>`;
    const mu=document.createElement('button'); mu.className='mxb'+(c.mute?' on':''); mu.textContent='M'; mu.title='Wycisz';
    mu.onclick=()=>{ c.mute=!c.mute; mu.classList.toggle('on',c.mute); applyAllChannels(); saveMixer(); };
    const so=document.createElement('button'); so.className='mxb solo'+(c.solo?' on':''); so.textContent='S'; so.title='Słuchaj tylko tego';
    so.onclick=()=>{ c.solo=!c.solo; so.classList.toggle('on',c.solo); applyAllChannels(); saveMixer(); };
    head.append(mu, so);
    const g2=document.createElement('div'); g2.className='mxgrid';
    const ch=(k,v)=>{ c[k]=v; if (ctx) applyChannel(id); saveMixer(); };
    g2.append(
      mxSlider('Głośność', 0, 1, .01, L[id].vol, fmtPct, v=>{ L[id].vol=v; syncLayerVol(id); if (ctx) applyChannel(id); }),
      mxSlider('Panorama', -1, 1, .01, c.pan, fmtPan, v=>ch('pan',v)),
      mxSlider('Niskie', -12, 12, .5, c.low, fmtDb, v=>ch('low',v)),
      mxSlider('Środek', -12, 12, .5, c.mid, fmtDb, v=>ch('mid',v)),
      mxSlider('Częstotliwość środka', 200, 6000, 10, c.midF, fmtHz, v=>ch('midF',v)),
      mxSlider('Wysokie', -12, 12, .5, c.high, fmtDb, v=>ch('high',v)),
      mxSlider('Filtr dołu', 20, 400, 1, c.hpf, fmtHz, v=>ch('hpf',v)),
      mxSlider('Przester', 0, 1, .01, c.drive, fmtPct, v=>ch('drive',v)),
      mxSlider('Kompresja', 0, 1, .01, c.comp, fmtPct, v=>ch('comp',v)),
      mxSlider('Przestrzeń (wysyłka)', 0, 1.2, .01, c.rev, fmtPct, v=>ch('rev',v)),
      mxSlider('Echo (wysyłka)', 0, 1.2, .01, c.dly, fmtPct, v=>ch('dly',v)));
    const colW2=document.createElement('label'); colW2.className='fld'; colW2.textContent='Kolor (sprzęt / głośnik / taśma)';
    colW2.appendChild(irSelect('any', c.color, true, v=>ch('color',v)));
    const colM2=mxSlider('Ilość koloru', 0, 1, .01, c.colorMix, fmtPct, v=>ch('colorMix',v));
    const det=document.createElement('details'); const sum=document.createElement('summary'); sum.textContent='Korektor i efekty';
    det.append(sum, g2, colW2, colM2);
    card.append(head, det); box.appendChild(card);
  }
  const reset=document.createElement('button'); reset.className='btn'; reset.textContent='Przywróć ustawienia miksu';
  reset.onclick=()=>{ try{ localStorage.removeItem('jw_mixer'); }catch(e){} location.reload(); };
  const credit=document.createElement('div'); credit.className='note'; credit.style.marginTop='10px';
  credit.textContent='Przestrzenie i kolory to prawdziwe odpowiedzi impulsowe z biblioteki „IR Library” Connera (licencja MIT). Nagrania z miejsc publicznych mają naturalne szumy i niedoskonałości.';
  box.append(reset, credit);
}
function syncLayerVol(id){ const r=document.querySelector(`[data-layervol="${id}"]`); if (r) r.value=Math.round(L[id].vol*100); }
