// Samples, audio graph, mood macros and voices (sampled + synthesized).
'use strict';
// ======================= samples =======================
// Works with the repo's per-file library (sample/manifest.json) and with the packed preview (manifest.json + sprites).
let MAN = null, SPRITE = false;
const buffers = {}, loading = {}, levels = {}, sprites = {}, ready = {};
let ctx, bus, dryOut, revIn, conv, dlyIn, dly, dlyFb, fogF, comp, master, bedBus, bedRev;

async function loadManifest(){
  if (MAN) return MAN;
  try { const r = await fetch('manifest.json'); if (r.ok){ const m = await r.json(); if (Object.values(m)[0]?.sprite){ MAN = m; SPRITE = true; return MAN; } } } catch(e){}
  MAN = await (await fetch('sample/manifest.json')).json(); return MAN;
}
const rmsOf = b => { const d=b.getChannelData(0), n=Math.min(d.length,b.sampleRate|0); let s=0; for(let i=0;i<n;i++) s+=d[i]*d[i]; return Math.sqrt(s/n)+1e-6; };
function onsetOf(b){ const d=b.getChannelData(0), n=Math.min(d.length,b.sampleRate|0); let mx=0; for(let i=0;i<n;i++) mx=Math.max(mx,Math.abs(d[i]));
  const th=mx*.04; for(let i=0;i<n;i++) if(Math.abs(d[i])>th) return Math.max(0,i/b.sampleRate-.004); return 0; }
const decode = url => fetch(url).then(r=>{ if(!r.ok) throw new Error(url); return r.arrayBuffer(); }).then(a=>new Promise((res,rej)=>ctx.decodeAudioData(a,res,rej)));
function slice(sp, start, dur){
  const sr=sp.sampleRate, a=Math.max(0,Math.floor((start-.01)*sr)), n=Math.min(sp.length-a, Math.floor((dur+.12)*sr));
  const b=ctx.createBuffer(sp.numberOfChannels,n,sr); for(let c=0;c<sp.numberOfChannels;c++) b.copyToChannel(sp.getChannelData(c).subarray(a,a+n),c); return b;
}
function prep(b){ b._on=onsetOf(b); b._rms=rmsOf(b); return b; }
async function loadInst(key, layersWanted, quiet){
  if (!key || key.startsWith('syn_')) return;
  await loadManifest(); const I = MAN[key]; if (!I) return;
  const core = SPRITE && I.type==='percussion' && I.core;
  const lays = I.type==='percussion' ? ['all'] : Object.keys(I.layers).filter(l=>!layersWanted || layersWanted.includes(l));
  const jobs = lays.map(l=>{
    const id = key+'|'+l; if (loading[id]) return loading[id];
    const files = I.type==='percussion' ? (core ? core.sounds : I.sounds) : Object.values(I.layers[l]).flat();
    if (core){
      loading[id] = (sprites[core.sprite] ||= decode(core.sprite)).then(sp=>{ for (const f of files){ const [s,d]=core.seg[f]; buffers[`${key}/${f}`]=prep(slice(sp,s,d)); } ready[id]=true; });
    } else if (SPRITE){
      loading[id] = (sprites[I.sprite[l]] ||= decode(I.sprite[l])).then(sp=>{ for (const f of files){ const [s,d]=I.seg[f]; buffers[`${key}/${f}`]=prep(slice(sp,s,d)); } ready[id]=true; });
    } else {
      loading[id] = Promise.all(files.map(f=>decode(`sample/${key}/${f}`).then(b=>{ buffers[`${key}/${f}`]=prep(b); }))).then(()=>{ ready[id]=true; });
    }
    return loading[id];
  });
  const st = $('status'); if (!quiet) st.textContent = 'Wczytuję dźwięki…';
  try { await Promise.all(jobs); } finally { if (!quiet) st.textContent=''; }
  if (I.type==='melodic' && !levels[key]){
    const L = levels[key] = {}, lv = I.lvl || {}, single = Object.keys(I.layers).length===1;
    if (Object.keys(lv).length){ const ref = lv.f ?? lv.mf ?? lv.p; for (const l in lv) L[l] = Math.pow(10, ((single?0:LAYER_DB[l]) - (lv[l]-ref))/20); }
    else { // per-file library: measure loaded layers
      const avg={}; for (const l in I.layers){ const v=[]; for (const m in I.layers[l]) for (const f of I.layers[l][m]){ const b=buffers[`${key}/${f}`]; if(b) v.push(20*Math.log10(b._rms)); } if(v.length) avg[l]=v.reduce((a,b)=>a+b)/v.length; }
      const ref = avg.f ?? avg.mf ?? avg.p; for (const l in avg) L[l] = Math.pow(10, ((single?0:LAYER_DB[l]) - (avg[l]-ref))/20);
    }
  }
}

// ======================= audio graph =======================
function initAudio(){
  if (ctx) return;
  ctx = new (window.AudioContext||window.webkitAudioContext)({latencyHint:'playback'});
  bus = ctx.createGain();
  master = ctx.createGain(); master.gain.value = .8;
  comp = ctx.createDynamicsCompressor(); comp.threshold.value=-14; comp.ratio.value=2.5; comp.attack.value=.02; comp.release.value=.3;
  fogF = ctx.createBiquadFilter(); fogF.type='lowpass'; fogF.Q.value=.5;
  dryOut = ctx.createGain(); revIn = ctx.createGain(); conv = ctx.createConvolver(); conv.buffer = makeIR(3.6);
  dlyIn = ctx.createGain(); dly = ctx.createDelay(3); dlyFb = ctx.createGain();
  const dLp = ctx.createBiquadFilter(); dLp.type='lowpass'; dLp.frequency.value=2200;
  const dHp = ctx.createBiquadFilter(); dHp.type='highpass'; dHp.frequency.value=200;
  dlyIn.connect(dly); dly.connect(dLp); dLp.connect(dHp); dHp.connect(dlyFb); dlyFb.connect(dly);
  bus.connect(dryOut); bus.connect(revIn); bus.connect(dlyIn); dHp.connect(dryOut); dHp.connect(revIn);
  // background layer bus: low cut + a little presence, reverb but no delay (keeps it clear instead of mushy)
  bedBus = ctx.createGain(); const bHp=ctx.createBiquadFilter(); bHp.type='highpass'; bHp.frequency.value=140;
  const bPk=ctx.createBiquadFilter(); bPk.type='peaking'; bPk.frequency.value=2800; bPk.Q.value=.8; bPk.gain.value=3;
  bedBus.connect(bHp); bHp.connect(bPk); bPk.connect(dryOut); bedRev = ctx.createGain(); bPk.connect(bedRev); bedRev.connect(revIn);
  revIn.connect(conv); conv.connect(fogF); dryOut.connect(fogF); fogF.connect(comp); comp.connect(master); master.connect(ctx.destination);
  applyMacros();
}
function makeIR(sec){
  const sr=ctx.sampleRate, len=Math.floor(sr*sec), b=ctx.createBuffer(2,len,sr);
  for (let c=0;c<2;c++){ const d=b.getChannelData(c); let lp=0; for (let i=0;i<len;i++){ const t=i/len; lp=lp*.62+(Math.random()*2-1)*.38; d[i]=lp*Math.pow(1-t,3.2)*(i<sr*.03?i/(sr*.03):1); } }
  return b;
}

// ======================= macros =======================
const M = { noc:.55, nap:.4, ruch:.35 };
function applyMacros(){
  M.noc = $('m_noc').value/100; M.nap = $('m_nap').value/100; M.ruch = $('m_ruch').value/100;
  if (!ctx) return; const t = ctx.currentTime;
  dryOut.gain.setTargetAtTime(1 - M.noc*.4, t, .1);
  revIn.gain.setTargetAtTime(.25 + M.noc*.65, t, .1);
  dlyIn.gain.setTargetAtTime(.12 + M.noc*.3 + M.nap*.1, t, .1);
  dlyFb.gain.setTargetAtTime(.3 + M.noc*.3, t, .1);
  dly.delayTime.setTargetAtTime(60/tempo()*.75, t, .3);
  fogF.frequency.setTargetAtTime(18000*Math.pow(3000/18000, Math.max(0,(M.noc-.3)/.7)), t, .15);   // fog only darkens in the upper half of 'Noc'
  bedRev.gain.setTargetAtTime(.35 + M.noc*.4, t, .1);
  master.gain.setTargetAtTime($('vol').value/100, t, .05);
}
['m_noc','m_nap','m_ruch','vol','tempo'].forEach(id=>$(id).addEventListener('input', applyMacros));

// ======================= voices =======================
const MONO = new Set(['sax_tenor','sax_baryton','trabka_tlumik','trabka']);
const RELEASE = {pianino:.4,rhodes:.45,fm_piano:.4,wibrafon:.7,dzwonki:.9,kieliszki:.8,kontrabas:.18,sax_tenor:.3,sax_baryton:.25,trabka_tlumik:.3,trabka:.3};
const ATTACK = {chor_alty_oo:.5,chor_tenory_oo:.5,chor_alty_ah:.5,chor_tenory_ah:.5,kieliszki:.3};
const lastVoice = {};
function nearest(list, m){ let b=list[0]; for (const r of list) if (Math.abs(r-m)<Math.abs(b-m)) b=r; return b; }

// ---------- expression: what samples lack – breath, vibrato, living pitch and swells ----------
// vib: rate (Hz), depth (cents), delay (s) before it blooms · air: breath noise mixed into the tone · drift: random tuning (cents)
// swell: long notes grow a little and relax (messa di voce) · inhale: audible breath before a phrase · click: key/valve noise · trem: amplitude wobble
const EXPR = {
  sax_tenor:     {vib:[5.1,16,.28], air:[.035,2300,.9], drift:5, swell:.12, inhale:.05, click:.03},
  sax_baryton:   {vib:[4.7,14,.32], air:[.04,1500,.9],  drift:5, swell:.1,  inhale:.06, click:.035},
  trabka_tlumik: {vib:[5.4,10,.32], air:[.05,3200,1.1], drift:4, swell:.12, inhale:.045, click:.02},
  trabka:        {vib:[5.5,12,.3],  air:[.03,3000,1],   drift:4, swell:.1,  inhale:.045, click:.02},
  chor_alty_oo:  {vib:[5.0,14,.45], air:[.02,1700,.7],  drift:7, swell:.15, inhale:.03},
  chor_alty_ah:  {vib:[5.0,14,.45], air:[.02,1900,.7],  drift:7, swell:.15, inhale:.03},
  chor_tenory_oo:{vib:[4.8,12,.5],  air:[.015,1400,.7], drift:7, swell:.12},
  chor_tenory_ah:{vib:[4.8,12,.5],  air:[.015,1600,.7], drift:7, swell:.12},
  kontrabas:     {drift:4},
  wibrafon:      {trem:[5.2,.18]},
  rhodes:        {drift:2, trem:[4.2,.06]},
  pianino:       {drift:2}, fm_piano:{drift:1}, dzwonki:{drift:2}, kieliszki:{drift:6, swell:.1, vib:[3.5,8,.6]},
};
let noiseBuf=null;
function noise(){ if (!noiseBuf){ noiseBuf=ctx.createBuffer(1, ctx.sampleRate*2, ctx.sampleRate); const d=noiseBuf.getChannelData(0); let b=0; for (let i=0;i<d.length;i++){ b=b*.6+(Math.random()*2-1)*.4; d[i]=b; } } return noiseBuf; }
// a soft inhale before a phrase ("hhh"), filtered like air through the mouth
function inhale(t, level, out){
  const n=ctx.createBufferSource(); n.buffer=noise(); const bp=ctx.createBiquadFilter(); bp.type='bandpass'; bp.frequency.value=rnd(900,1400); bp.Q.value=.9;
  const g=ctx.createGain(); g.gain.value=0; n.connect(bp); bp.connect(g); g.connect(out);
  const len=rnd(.22,.36); g.gain.setValueAtTime(0,t); g.gain.linearRampToValueAtTime(level, t+len*.55); g.gain.linearRampToValueAtTime(0, t+len);
  bp.frequency.setValueAtTime(bp.frequency.value, t); bp.frequency.linearRampToValueAtTime(bp.frequency.value*1.4, t+len);
  n.start(t, Math.random()*1.5); n.stop(t+len+.05);
}
function click(t, level, out){ // key pad / valve noise: tiny filtered tick
  const n=ctx.createBufferSource(); n.buffer=noise(); const hp=ctx.createBiquadFilter(); hp.type='bandpass'; hp.frequency.value=rnd(1800,3200); hp.Q.value=2;
  const g=ctx.createGain(); n.connect(hp); hp.connect(g); g.connect(out); g.gain.setValueAtTime(level,t); g.gain.setTargetAtTime(0,t+.002,.008); n.start(t, Math.random()); n.stop(t+.06);
}
const holdAt = (param, at) => { if (param.cancelAndHoldAtTime) param.cancelAndHoldAtTime(at); else param.cancelScheduledValues(at); };

function noteOn(key, m, o={}){
  if (key.startsWith('syn_')) return synthOn(key, m, o);
  const I = MAN[key]; if (!I) return null;
  const t = o.when ?? ctx.currentTime, vel = clamp(o.vel ?? .6, 0, 1);
  const av = LAYERS.filter(l=>I.layers[l] && ready[key+'|'+l]);
  if (!av.length) return null;
  // wanted layer not downloaded yet → use the nearest one that is, instead of staying silent
  let layer = o.layer || LAYERS.filter(l=>I.layers[l])[Math.min(2, Math.floor(vel*3))];
  if (!av.includes(layer)){ const w=LAYERS.indexOf(layer); layer = av.reduce((a,b)=>Math.abs(LAYERS.indexOf(b)-w)<Math.abs(LAYERS.indexOf(a)-w)?b:a); }
  const root = nearest(Object.keys(I.layers[layer]).map(Number), m);
  const file = pick(I.layers[layer][root]), buf = buffers[`${key}/${file}`]; if (!buf) return null;
  const isMono = o.mono ?? MONO.has(key), chan = o.ch || key;   // ch: separate legato channel per musical role
  const prev = isMono ? lastVoice[chan] : null, legato = !!(o.legato && prev && prev.alive);
  const X = EXPR[key] || {}, feel = o.feel ?? .5;                // feel: how expressive this player is right now (0..1)
  const cents = X.drift ? (Math.random()*2-1)*X.drift : 0;
  const rate = Math.pow(2,(m-root)/12 + cents/1200);
  const out = o.bus || bus;
  const s = ctx.createBufferSource(); s.buffer = buf;
  const tone = ctx.createBiquadFilter(); tone.type='lowpass'; tone.frequency.value = 2200 + vel*vel*13000;   // softer = darker, like a real player
  const g = ctx.createGain(); g.gain.value = 0; s.connect(tone); tone.connect(g); g.connect(out);
  const amp = (o.gain ?? 1) * (levels[key]?.[layer] ?? 1) * (I.gain?.[file] ?? 1) * (.42 + .58*vel) * .8;
  s.playbackRate.setValueAtTime(rate, t);
  if (legato && Math.abs(prev.m-m)<=4){ s.playbackRate.setValueAtTime(rate*Math.pow(2,(prev.m-m)/12), t); s.playbackRate.exponentialRampToValueAtTime(rate, t+.06+.015*Math.abs(prev.m-m)); }
  else if (o.scoop){ s.playbackRate.setValueAtTime(rate*Math.pow(2,-o.scoop/12), t); s.playbackRate.exponentialRampToValueAtTime(rate, t+.11+o.scoop*.04); }
  const off = buf._on + (legato?.07:0), atk = legato ? .045 : (o.attack ?? ATTACK[key] ?? .006);
  g.gain.setValueAtTime(0,t); g.gain.linearRampToValueAtTime(amp, t+atk);
  // long notes breathe: a gentle swell and relax instead of a flat level
  if (X.swell){ const sw=X.swell*(.6+feel*.8); g.gain.linearRampToValueAtTime(amp*(1+sw), t+atk+.55); g.gain.setTargetAtTime(amp*(1-sw*.6), t+atk+.6, 1.4); }
  s.start(t, off);
  const extra=[];                                                 // nodes to stop with the note
  // vibrato that blooms after the attack, deeper with feeling; slightly irregular rate
  if (X.vib && (o.vib ?? true)){
    const [vr, vd, vdel] = X.vib, depth = vd*(.55 + feel*.9);
    const lfo=ctx.createOscillator(); lfo.frequency.value = vr*rnd(.92,1.08);
    const lg=ctx.createGain(); lg.gain.value=0; lfo.connect(lg); lg.connect(s.playbackRate);
    const k = rate*(Math.pow(2, depth/1200)-1);
    lg.gain.setValueAtTime(0, t+vdel); lg.gain.linearRampToValueAtTime(k, t+vdel+.6);
    lfo.frequency.setValueAtTime(lfo.frequency.value, t+vdel); lfo.frequency.linearRampToValueAtTime(vr*rnd(1.0,1.12), t+vdel+2);
    lfo.start(t); extra.push(lfo);
  }
  // breath inside the tone: more audible when soft (subtone), follows the note
  if (X.air){
    const [ag, af, aq] = X.air, n=ctx.createBufferSource(); n.buffer=noise(); n.loop=true;
    const bp=ctx.createBiquadFilter(); bp.type='bandpass'; bp.frequency.value=af*rnd(.9,1.1); bp.Q.value=aq;
    const ng=ctx.createGain(); ng.gain.value=0; n.connect(bp); bp.connect(ng); ng.connect(g);
    const lvl = ag*(1.4-vel*.8)/Math.max(.2, amp) * amp;          // relative to the note
    ng.gain.setValueAtTime(0,t); ng.gain.linearRampToValueAtTime(lvl*2.2, t+atk+.03); ng.gain.setTargetAtTime(lvl, t+atk+.05, .15);
    n.start(t, Math.random()*1.5); extra.push(n);
  }
  if (X.click && !legato && o.click !== false) click(t-.004, X.click*(.5+vel), out);
  else if (X.click && legato && chance(.5)) click(t, X.click*.5, out);
  // gentle tremolo (vibraphone motor, Rhodes)
  if (X.trem){ const tl=ctx.createOscillator(); tl.frequency.value=X.trem[0]; const tg=ctx.createGain(); tg.gain.value=amp*X.trem[1]; tl.connect(tg); tg.connect(g.gain); tl.start(t); extra.push(tl); }
  if (legato) prev.release(t+.03, .035);
  const stopAll = at => { try{ s.stop(at); }catch(e){} extra.forEach(x=>{ try{ x.stop(at); }catch(e){} }); };
  const v = { m, alive:true, amp,
    release(at, tau){ if(!this.alive) return; at=Math.max(at ?? ctx.currentTime, t+atk); this.alive=false; tau = tau ?? (RELEASE[key] ?? .15)/3;
      holdAt(g.gain, at); g.gain.setTargetAtTime(0, at, tau);
      if (X.vib && MONO.has(key)){ holdAt(s.playbackRate, at); s.playbackRate.setTargetAtTime(rate*Math.pow(2,-8/1200), at, tau*2); }   // pitch sags as the air stops
      stopAll(at+tau*8); },
    fall(at, semis, len){ holdAt(s.playbackRate, at); s.playbackRate.exponentialRampToValueAtTime(rate*Math.pow(2,-semis/12), at+len);
      holdAt(g.gain, at); g.gain.linearRampToValueAtTime(.0001, at+len); this.alive=false; stopAll(at+len+.05); },
    swell(at, to, len){ holdAt(g.gain, at); g.gain.linearRampToValueAtTime(to, at+len); } };
  if (isMono) lastVoice[chan] = v;
  glow(key, vel, t);
  return v;
}
function hit(key, name, o={}){
  const buf = buffers[`${key}/${name}.mp3`]; if (!buf) return;
  const t=o.when ?? ctx.currentTime, s=ctx.createBufferSource(), g=ctx.createGain();
  s.buffer=buf; s.playbackRate.value=o.rate ?? 1; g.gain.value=o.gain ?? .8; s.connect(g); g.connect(o.bus || bus); s.start(t, buf._on);
  if (o.dur){ g.gain.setValueAtTime(g.gain.value, t+o.dur); g.gain.setTargetAtTime(0, t+o.dur, .08); s.stop(t+o.dur+.6); }
}
const mtof = m => 440*Math.pow(2,(m-69)/12);
function synthOn(key, m, o={}){
  const t=o.when ?? ctx.currentTime, vel=clamp(o.vel ?? .6,0,1), f=mtof(m), gmul=o.gain ?? 1;
  const out=ctx.createGain(); out.gain.value=0; out.connect(o.bus || bus);
  const nodes=[]; const osc=(type,freq,det=0)=>{ const x=ctx.createOscillator(); x.type=type; x.frequency.value=freq; x.detune.value=det; nodes.push(x); return x; };
  let amp=.2, atk=.01, rel=.3;
  if (key==='syn_pad'){
    // warm, clean pad: two detuned triangles + a quiet filtered saw, slow gentle filter movement
    const fl=ctx.createBiquadFilter(); fl.type='lowpass'; fl.Q.value=.5; fl.frequency.value=1100+vel*1200;
    const lfo=osc('sine',.07+Math.random()*.04), lg=ctx.createGain(); lg.gain.value=150; lfo.connect(lg); lg.connect(fl.frequency);
    for (const d of [-7,7]) osc('triangle',f,d+rnd(-1,1)).connect(fl);
    const sw=osc('sawtooth',f,rnd(-3,3)), swg=ctx.createGain(); swg.gain.value=.35; sw.connect(swg); swg.connect(fl);
    fl.connect(out); amp=.11*(.6+.4*vel)*gmul; atk=o.attack ?? 1.5; rel=2.2;
  } else if (key==='syn_smyczki'){
    const fl=ctx.createBiquadFilter(); fl.type='lowpass'; fl.frequency.value=1300+vel*2400; fl.Q.value=.4;
    const vib=osc('sine',5.2), vg=ctx.createGain(); vg.gain.value=6; vib.connect(vg);
    for (const d of [-6,5]){ const x=osc('sawtooth',f,d); vg.connect(x.detune); x.connect(fl); }
    fl.connect(out); amp=.075*(.5+.5*vel)*gmul; atk=o.attack ?? .5; rel=.8;
  } else if (key==='syn_bas'){
    const fl=ctx.createBiquadFilter(); fl.type='lowpass'; fl.Q.value=2; fl.frequency.setValueAtTime(220+vel*900,t); fl.frequency.exponentialRampToValueAtTime(140,t+.35);
    const a=osc('sine',f), b=osc('triangle',f*2), bg=ctx.createGain(); bg.gain.value=.25; a.connect(fl); b.connect(bg); bg.connect(fl); fl.connect(out);
    amp=.45*(.5+.5*vel)*gmul; atk=.008; rel=.18;
  } else { // syn_dzwon
    const car=osc('sine',f), mod=osc('sine',f*3.5), mg=ctx.createGain();
    mg.gain.setValueAtTime(f*(1.5+vel*2.5),t); mg.gain.exponentialRampToValueAtTime(f*.08,t+2.2); mod.connect(mg); mg.connect(car.frequency); car.connect(out);
    const c2=osc('sine',f*2.001), g2=ctx.createGain(); g2.gain.value=.12; c2.connect(g2); g2.connect(out);
    amp=.16*(.4+.6*vel)*gmul; atk=.003; rel=1.2;
  }
  out.gain.setValueAtTime(0,t); out.gain.linearRampToValueAtTime(amp,t+atk);
  if (key==='syn_dzwon') out.gain.setTargetAtTime(0,t+atk,1.1);
  nodes.forEach(n=>{ n.start(t); if (key==='syn_dzwon') n.stop(t+6); });
  glow(key, vel, t);
  return { m, alive:true, amp,
    release(at,tau){ if(!this.alive) return; this.alive=false; at=Math.max(at ?? ctx.currentTime, t+.01); tau=tau ?? rel/3;
      out.gain.cancelScheduledValues(at); out.gain.setValueAtTime(amp,at); out.gain.setTargetAtTime(0,at,tau); nodes.forEach(n=>{ try{n.stop(at+tau*8)}catch(e){} }); },
    fall(at){ this.release(at); }, swell(){} };
}
