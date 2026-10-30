// The band: song form, harmony timeline, meters / polymeter, and the players (bed, bass, keys, soloist, drums).
// Every setting lives in S and L and is read live, so changes made while playing are picked up at the next note / bar.
'use strict';

// ======================= settings =======================
const S = {
  songId:'noc', key:2, progBase:[], progKey:2, prog:[], pendingProg:null,
  form:'nastroj',            // 'nastroj' (mood sections) | 'jam' (choruses with rotating solos)
  soloWho:'auto',            // 'auto' | layer id
  scale:'auto',
  baseTempo:64,
  meter:'4/4', meterMode:'fixed', meterSeq:['4/4'], meterPool:['4/4','3/4','5/4','6/8','7/8'],
};
const LAYER_DEF = [
  {id:'tlo',   name:'Tło',      opts:[['chor','Chór „oo”'],['chor_ah','Chór „ah”'],['syn_pad','Pad „mgła”'],['syn_smyczki','Smyczki synt.'],['kieliszki','Kieliszki']], def:'chor', vol:.75},
  {id:'bas',   name:'Bas',      opts:[['kontrabas','Kontrabas'],['syn_bas','Bas synt.']], def:'kontrabas', vol:.8},
  {id:'akordy',name:'Akordy',   opts:[['rhodes','Rhodes'],['fortepian','Fortepian (Steinway)'],['pianino','Pianino'],['fm_piano','FM piano'],['wibrafon','Wibrafon']], def:'rhodes', vol:.7},
  {id:'solo',  name:'Solista',  opts:[['sax_tenor','Sax tenor'],['sax_baryton','Sax baryton'],['trabka_tlumik','Trąbka z tłumikiem'],['trabka','Trąbka'],['wibrafon','Wibrafon'],['syn_dzwon','Dzwon FM'],['dzwonki','Dzwonki']], def:'sax_tenor', vol:.75},
  {id:'perk',  name:'Perkusja', opts:[['zestaw','Zestaw jazzowy'],['jazz','Orkiestrowa'],['miekka','Miękka (smyczek, shaker)']], def:'zestaw', vol:.7},
];
const L = {};   // id -> {on, inst, vol, int (intensity 0..1), meter ('wspólne' | meter id)}
for (const d of LAYER_DEF) L[d.id] = {on:true, inst:d.def, vol:d.vol, int:.35, meter:'wspólne'};

const LB = {};
const volCurve = v => v*v*1.6;
function layerBus(id){ return channelIn(id); }   // every layer plays into its mixer channel (js/mixer.js)
function layerInsts(id, v){
  if (id==='tlo') return v==='chor' ? ['chor_tenory_oo','chor_alty_oo'] : v==='chor_ah' ? ['chor_tenory_ah','chor_alty_ah'] : [v];
  if (id==='perk') return v==='zestaw' ? ['zestaw'] : v==='jazz' ? ['talerz','hihat','werbel','beben'] : ['talerz','shaker','mark_tree'];
  return [v];
}
// melodic instrument used when a layer takes a solo
function soloInst(id){
  if (id==='tlo'){ const v=L.tlo.inst; return v==='chor' ? 'chor_alty_oo' : v==='chor_ah' ? 'chor_alty_ah' : v==='syn_pad' ? 'syn_smyczki' : v; }
  return L[id].inst;
}
const a_isChoir = () => L.tlo.inst==='chor' || L.tlo.inst==='chor_ah';
const RANGE = {sax_tenor:[55,77], sax_baryton:[43,64], trabka_tlumik:[62,81], trabka:[60,82], wibrafon:[62,86], syn_dzwon:[62,88], dzwonki:[64,88],
  rhodes:[60,84], fortepian:[60,88], pianino:[60,86], fm_piano:[60,86], kontrabas:[36,57], syn_bas:[36,58], chor_alty_oo:[60,76], chor_alty_ah:[60,76],
  syn_smyczki:[60,84], kieliszki:[75,86]};
const rangeOf = inst => RANGE[inst] || [60,80];

function tempo(){ return Math.round(S.baseTempo * (0.88 + 0.24*M.ruch)); }
const compound = meterId => METERS[meterId].g.every(g=>g===3);
// The eighth-note pulse stays the same when the meter changes (like a real band in mixed meters): it is set by the
// song's main meter, not by the current bar. Tempo changes glide over about a bar instead of jumping.
let curTempo = null;
function eighth(){ return 60/(curTempo ?? tempo()) / (compound(S.meter) ? 3 : 2); }
function glideTempo(){ const target=tempo(); if (curTempo===null) curTempo=target; curTempo += (target-curTempo)*.12; if (Math.abs(target-curTempo)<.05) curTempo=target; }

// ======================= harmony helpers =======================
const layersOf = k => MAN && MAN[k] && MAN[k].layers ? LAYERS.filter(l=>MAN[k].layers[l]) : [];
function layerFor(k, level){ const av=layersOf(k); if (!av.length) return undefined; if (av.length===1) return av[0];
  return level > .66 ? av[av.length-1] : (av.includes('mf') && level > .42) ? 'mf' : av[0]; }
let compLay='p';
function compLayer(k, level){ const av=layersOf(k); if (!av.length) return undefined;
  if (compLay==='p' && level>.62) compLay = av.includes('mf') ? 'mf' : av[av.length-1]; else if (compLay!=='p' && level<.45) compLay='p';
  if (level>.82 && av.includes('f')) return 'f';
  return av.includes(compLay) ? compLay : av[0]; }
function bassNote(pc, lo=36, hi=47){ let m=lo+((pc-lo)%12+12)%12; return m>hi ? m-12 : m; }
function snap(m, pcs){ for (let d=0; d<7; d++){ if (pcs.includes(((m+d)%12+12)%12)) return m+d; if (pcs.includes(((m-d)%12+12)%12)) return m-d; } return m; }
// notes a soloist may use over chord c (scale setting can override the chord scale)
function soloScale(c){ if (S.scale==='auto' || !SCALES[S.scale].iv) return chordScale(c); return SCALES[S.scale].iv.map(i=>(S.key+i)%12); }
function soloTargets(c){ const sc=soloScale(c), ct=chordPcs(c).filter(p=>sc.includes(p)); return ct.length ? ct : sc; }
let prevVoicing=[53,57,60,64];
function voicing(c, level, top=70){
  const T=TYPES[c.type]; let ivs=[...T.guide, 7, 14]; if (level>.45) ivs=[...T.guide, ...T.col]; if (level>.75) ivs=[...T.guide, ...T.col, 7];
  ivs=[...new Set(ivs)].slice(0,4);
  const center = prevVoicing.reduce((a,b)=>a+b)/prevVoicing.length;
  const notes = ivs.map(iv=>{ const pc=(c.root+iv)%12; let best=null; for (let m=50;m<=top;m++) if (m%12===pc && (best===null || Math.abs(m-center)<Math.abs(best-center))) best=m; return best; }).sort((a,b)=>a-b);
  for (let i=1;i<notes.length;i++) if (notes[i]-notes[i-1]<1) notes[i]+=12;
  prevVoicing=notes; return notes;
}

// ======================= song / harmony timeline =======================
function setSong(id){
  const song = SONGS.find(x=>x.id===id); if (!song) return;
  S.songId=id; S.progBase=parseProgression(song.chords).chords; S.progKey=song.key; queueProg();
  return song;
}
function setCustom(text){ const r=parseProgression(text); if (!r.chords.length) return r; S.songId='custom'; S.progBase=r.chords; S.progKey=S.key; queueProg(); return r; }
function setKey(k){
  S.key=k;
  if (!running || S.pendingProg){ queueProg(); return; }
  S.prog = transpose(S.progBase, S.key - S.progKey);   // same chords, same place in the form – just in the new key
  T.rechord = true;                                      // re-voice the sounding chord on the next beat
}
function queueProg(){ S.pendingProg = transpose(S.progBase, S.key - S.progKey); if (!running) { S.prog=S.pendingProg; S.pendingProg=null; } }

// ======================= form =======================
const SECTIONS = [
  {name:'Mgła',    bars:4,  tlo:1, bas:0, akordy:0, solo:0, perk:0},
  {name:'Wejście', bars:8,  tlo:1, bas:1, akordy:1, solo:0, perk:.5},
  {name:'Noc',     bars:16, tlo:1, bas:1, akordy:1, solo:1, perk:1},
  {name:'Szczyt',  bars:8,  tlo:1, bas:2, akordy:1, solo:1.4, perk:1.3, needs:.35},
  {name:'Oddech',  bars:8,  tlo:1, bas:0, akordy:1, solo:.5, perk:0, soloBy:'akordy'},
];
const JAM_ALL = {tlo:1, bas:1, akordy:1, solo:1, perk:1};

// ======================= transport state =======================
let running=false, timer=null, nextT=0, step=0;
const T = {               // timeline
  meter:'4/4', barStart:0, barLen:8, bar:0, chordIdx:null, rechord:false, E:.3, soloHeat:0,
  ci:0, chordRemain:0, chord:null, events:[], chorus:0, chorusStart:false,
  sec:0, secBar:0, secStartStep:0, alignStep:0, jamStage:0, jamSolo:0, trade:false,
};
let userUntil=0;                          // the player on the touch strip has the stage until this time
const players = {};                       // per-layer solo phrase state
function resetTransport(){
  step=0; curTempo=null; Object.assign(T,{barStart:0, barLen:8, bar:0, ci:0, chordRemain:0, chord:null, chordIdx:null, rechord:false, events:[], chorus:0, sec:0, secBar:0, secStartStep:0, alignStep:0, jamStage:0, jamSolo:0, trade:false, E:.3, soloHeat:0});
  if (S.pendingProg){ S.prog=S.pendingProg; S.pendingProg=null; }
  for (const id in players) delete players[id];
}

// which meter does a layer play in, and where is it in its bar?
function polyLayers(){ return Object.keys(L).filter(id=>L[id].on && L[id].meter!=='wspólne'); }
function layerBeat(id){
  const lm = L[id].meter;
  if (lm==='wspólne') return beatInfo(T.meter, step - T.barStart);
  return beatInfo(lm, step - T.alignStep);
}
function polyCycle(){ let c=METERS[T.meter].len; for (const id of polyLayers()) c=lcm(c, METERS[L[id].meter].len); return c; }

function nextMeter(sectionChange){
  if (S.meterMode==='seq' && S.meterSeq.length) return S.meterSeq[T.bar % S.meterSeq.length];
  if (S.meterMode==='section'){ if (sectionChange || !T.meter) return pick(S.meterPool); return T.meter; }
  return S.meter;
}

// who solos right now? → layer id or null
function soloist(){
  if (ctx && ctx.currentTime < userUntil) return 'user';
  if (S.soloWho!=='auto') return L[S.soloWho]?.on ? S.soloWho : null;
  if (S.form==='jam'){
    if (T.trade){ const lead=jamSoloists().find(x=>x!=='perk') || 'solo'; return Math.floor((T.secBar-1)/4)%2===0 ? lead : 'perk'; }
    if (T.jamStage===0) return null;
    const list=jamSoloists(); return list.length ? list[T.jamSolo % list.length] : null;
  }
  const s=SECTIONS[T.sec]; if (!s.solo) return null;
  const by = s.soloBy && L[s.soloBy].on ? s.soloBy : 'solo';
  return L[by].on ? by : null;
}
function jamSoloists(){ return ['solo','akordy','bas','tlo','perk'].filter(id=>L[id].on && (id!=='tlo' || S.soloWho==='tlo')); }
function sectionLevels(){ return S.form==='jam' ? JAM_ALL : SECTIONS[T.sec]; }

// ======================= dynamics =======================
// T.E is the band's energy (0..1). It follows the form slowly – a crescendo into the climax, a hush in 'Oddech',
// an arc across each solo chorus – plus the mood knobs. Every player reads it, so the band swells and settles together.
const SECTION_ENERGY = {'Mgła':.18, 'Wejście':.38, 'Noc':.55, 'Szczyt':.85, 'Oddech':.28};
function energyTarget(){
  let base;
  if (S.form==='jam'){
    const len = Math.max(1, progBars(S.prog)), p = clamp((T.secBar-1)/len, 0, 1);
    base = T.trade ? .72 : T.jamStage===0 ? .4 : .38 + .45*Math.sin(Math.PI*Math.min(1, p*1.15));   // build through the chorus, ease at the end
  } else {
    const s = SECTIONS[T.sec], nx = SECTIONS[(T.sec+1)%SECTIONS.length], p = clamp((T.secBar-1)/s.bars, 0, 1);
    base = lerp(SECTION_ENERGY[s.name], SECTION_ENERGY[nx.name], p>.75 ? (p-.75)*2 : 0);              // lean into the next part
  }
  const avgInt = (L.solo.int + L.akordy.int + L.bas.int + L.perk.int)/4;
  return clamp(base*.75 + M.nap*.25 + (avgInt-.35)*.25 + T.soloHeat*.15, 0, 1);
}
function updateEnergy(){ T.E += (energyTarget() - T.E) * .3; T.soloHeat *= .7; }
const dyn = (base, spread=.5) => clamp(base * (1 - spread/2 + spread*T.E), .05, 1);   // scale a velocity by the band energy

// ======================= bar & chord bookkeeping =======================
const at = (t, fn) => setTimeout(()=>{ if (running) fn(); }, Math.max(0,(t-ctx.currentTime)*1000));
function newBar(t){
  let sectionChange=false;
  if (S.pendingProg){ S.prog=S.pendingProg; S.pendingProg=null; T.ci=0; T.chordRemain=0; T.chorus=0; sectionChange=true; }
  T.secBar++;
  if (S.form==='nastroj'){
    let s=SECTIONS[T.sec];
    const aligned = !polyLayers().length || ((step - T.alignStep) % polyCycle() === 0);
    if (T.secBar > s.bars && (aligned || T.secBar > s.bars+4)){
      do { T.sec=(T.sec+1)%SECTIONS.length; s=SECTIONS[T.sec]; } while (s.needs && M.nap < s.needs);
      T.secBar=1; sectionChange=true;
      if (T.sec===0 && S.songId!=='custom' && SONGS.find(x=>x.id===S.songId)?.group==='Nastrojowe' && chance(.5)){
        const pool=SONGS.filter(x=>x.group==='Nastrojowe'); const sg=pick(pool); S.songId=sg.id; S.progBase=parseProgression(sg.chords).chords; S.progKey=sg.key;
        S.prog=transpose(S.progBase, S.key-S.progKey); T.ci=0; T.chordRemain=0; at(t, ()=>UI.songChanged());
      }
    }
  }
  // meter of this bar
  const m = nextMeter(sectionChange);
  if (m!==T.meter){
    T.meter=m; at(t, ()=>UI.meterShown(m));
    T.alignStep = step;                                   // everybody meets on this downbeat
    for (const id in players){ const P=players[id]; P.phrase = P.phrase.filter(n=>n.at < step); P.next = step + METERS[m].starts[1] || step+2; }
  }
  T.barLen = METERS[T.meter].len; T.barStart = step; T.bar++;
  // chords falling into this bar
  T.events=[]; T.chorusStart=false; let pos=0;
  while (pos < 1-1e-6){
    if (T.chordRemain <= 1e-6){
      if (!S.prog.length) break;
      if (T.ci % S.prog.length === 0 && T.ci>0) { T.chorus++; T.chorusStart = pos===0; }
      const c = S.prog[T.ci % S.prog.length]; T.ci++; T.chordRemain=c.bars;
      // put the change on the nearest beat
      const want = pos*T.barLen, starts=METERS[T.meter].starts; let st=starts[0]; for (const s0 of starts) if (Math.abs(s0-want)<Math.abs(st-want)) st=s0;
      T.events.push({pos: st, idx:(T.ci-1) % S.prog.length});
    }
    const take=Math.min(T.chordRemain, 1-pos); T.chordRemain-=take; pos+=take;
  }
  if (T.ci===1 && T.events.length && T.events[0].pos===0) T.chorusStart = true;
  if (S.form==='jam' && T.chorusStart){ jamAdvance(); sectionChange=true; T.secBar=1; }
  if (sectionChange){ T.alignStep = step; T.secStartStep = step; }
  const label = S.form==='jam' ? jamLabel() : SECTIONS[T.sec].name;
  at(t, ()=>UI.sectionShown(label));
}
function jamAdvance(){
  // chorus 1: band only (intro), then one chorus per soloist, then trading fours with the drums, then again
  const n = jamSoloists().length || 1;
  if (T.chorus===0){ T.jamStage=0; T.trade=false; return; }
  const k = T.chorus-1;
  if (k < n){ T.jamStage=1; T.jamSolo=k; T.trade=false; }
  else if (k===n && L.perk.on && progBars(S.prog)>=8){ T.jamStage=2; T.trade=true; T.jamSolo=0; }
  else { T.chorus=0; T.jamStage=0; T.trade=false; }
}
function jamLabel(){ const w=soloist(); const names={solo:'solista',akordy:'akordy',bas:'bas',tlo:'tło',perk:'perkusja'};
  return T.trade ? 'Czwórki z perkusją' : T.jamStage===0 ? 'Wstęp' : 'Solo: '+(names[w]||'—'); }
function nextChordAfter(){
  // chord that follows the one sounding now
  const pos = step - T.barStart, later = T.events.find(e=>e.pos>pos);
  if (later) return S.prog[later.idx];
  return S.prog.length ? S.prog[T.ci % S.prog.length] : T.chord;
}
function chordEndsThisBeat(){ // is the next beat a chord change? (for bass approach notes)
  const bi=beatInfo(T.meter, step-T.barStart), nextPos = bi.pos + bi.groupLen;
  if (nextPos >= T.barLen) return T.chordRemain <= 1e-6; return T.events.some(e=>e.pos===nextPos);
}

// ======================= scheduler =======================
function tick(t, silent){
  if (step===0 || step - T.barStart >= T.barLen) newBar(t);
  const gb = beatInfo(T.meter, step - T.barStart);
  const ev = T.events.find(e=>e.pos===gb.pos);
  if (ev){ T.chordIdx = ev.idx; T.chord = S.prog[ev.idx]; if (!silent) chordChanged(t, eighth()); }
  else if (T.rechord && gb.onBeat && T.chordIdx!=null){ T.rechord=false; T.chord = S.prog[T.chordIdx % S.prog.length]; if (!silent) chordChanged(t, eighth()); }
  if (gb.pos===0) updateEnergy();
  return gb;
}
function schedule(){
  if (nextT < ctx.currentTime - .05){            // the phone was busy: jump ahead silently but keep bars and chords in step
    while (nextT < ctx.currentTime + .05){ tick(nextT, true); glideTempo(); nextT += eighth(); step++; }
  }
  const ahead = document.hidden ? 2.5 : .6;
  while (nextT < ctx.currentTime + ahead){
    glideTempo();
    const e8 = eighth(), gb = tick(nextT, false);
    const swing = (gb.groupLen===2 && gb.sub===1) ? e8*lerp(.32, .12, clamp((curTempo-60)/150,0,1)) : 0;
    const t = nextT + swing;
    if (T.chord){
      const lv=sectionLevels(), who=soloist();
      if (L.perk.on && lv.perk)     playDrums(t, e8, lv, who);
      if (L.bas.on && lv.bas)       who==='bas' ? playSolo('bas', t, e8, nextT) : playBass(t, e8, lv, who);
      if (L.akordy.on && lv.akordy) { if (who==='akordy') { playSolo('akordy', t, e8, nextT); leftHand(t, e8); } else playComp(t, e8, who); }
      if (L.tlo.on && lv.tlo && who==='tlo') playSolo('tlo', t, e8, nextT);
      if (L.solo.on && lv.solo && who==='solo') playSolo('solo', t, e8, nextT);
      if (who!=='solo' && players.solo) players.solo.next = Math.max(players.solo.next, step+2);
    }
    nextT += e8; step++;
  }
}

// timing feel: small human drift, more (and sometimes clearly off the grid) with intensity
function human(id, t, e8){
  // drums keep the time (tight), bass almost; soloists and keys breathe more freely and, with intensity, push and drag
  const it=L[id].int, anchor = id==='perk' ? .3 : id==='bas' ? .5 : id==='akordy' ? .7 : 1;
  let d = (Math.random()+Math.random()-1) * (.002 + it*.008) * anchor;               // a few ms – feels human, stays in time
  if (id!=='perk' && it>.65 && chance((it-.65)*.25*anchor)) d += (chance(.6)?1:-1) * e8 * rnd(.06, .16);   // only when really fired up
  return Math.max(ctx.currentTime+.005, t + d);
}

// ======================= bed (background chords) =======================
const held = {tlo:[], akordy:[]};
function chordChanged(t, e8){
  const c=T.chord, name=chordName(c);
  at(t, ()=>UI.chordShown(name, c));
  held.tlo.forEach(v=>v&&v.release(t+.3,.8)); held.tlo=[];
  const lv=sectionLevels();
  if (!(L.tlo.on && lv.tlo) || soloist()==='tlo') return;
  const Tt=TYPES[c.type], g1=(c.root+Tt.guide[0])%12, g2=(c.root+Tt.guide[1])%12, g3=(c.root+(Tt.col[0]%12))%12;
  const it=L.tlo.int, lvl=(.4 + M.noc*.3)*(.8+it*.4)*(1.1 - T.E*.3), dur = e8*T.barLen*Math.max(1, c.bars) + 1;
  if (a_isChoir() && chance(.6)) inhale(t-.35, .012*(.6+it), layerBus('tlo'));       // the choir breathes before a new chord
  const [a,b] = layerInsts('tlo', L.tlo.inst), bus=layerBus('tlo');
  if (b){ const o={when:t, vel:.4+it*.3, layer: it>.7?'f':'mf', gain:lvl*.8, bus};
    held.tlo.push(noteOn(a, snap(55,[g1]), o), noteOn(b, snap(63,[g2]), o), noteOn(b, snap(68,[g3]), {...o, gain:lvl*.55})); }
  else if (a.startsWith('syn_')){ held.tlo.push(noteOn(a, bassNote(c.root)+12, {when:t, vel:.4, gain:lvl*1.2, bus}), noteOn(a, snap(63,[g2]), {when:t, vel:.35, gain:lvl, bus})); }
  else held.tlo.push(noteOn(a, snap(78,[g2]), {when:t, vel:.4, gain:lvl, bus}));
  held.tlo.forEach(v=>v && v.release(t+dur, .8));
}

// ======================= bass =======================
function playBass(t, e8, lv, who){
  const b=layerBeat('bas'); if (!b.onBeat) return;
  const it=L.bas.int, inst=L.bas.inst, c=T.chord, sc=chordScale(c);
  if (who==='perk' && !(b.beat===0 && chance(.5))) return;             // drum solo: only occasional hits
  const walk = lv.bas>1 || M.ruch>.6 || it>.55 || S.form==='jam' && tempo()>100 || chance(it*.6);
  const two = b.strong || b.beat===0;
  if (!walk && !two) return;
  let m;
  if (b.beat===0 || T.events.some(e=>e.pos===step-T.barStart)) m = bassNote(c.root);
  else if (walk && chordEndsThisBeat()){ const nx=nextChordAfter(); m = bassNote(nx.root) + pick([-1,1,-2]); }
  else if (walk) m = snap(bassNote(c.root) + pick([2,3,4,5,7,9]), sc) + (chance(it*.25) ? 12 : 0);
  else m = bassNote((c.root+7)%12);
  const len = (walk ? b.groupLen : b.groupLen*2)*e8;
  const vel = dyn((b.beat===0 ? .52 : b.strong ? .47 : .4) + M.nap*.08 + it*.28, .6) * rnd(.95,1.05);
  const v = noteOn(inst, m, {bus:layerBus('bas'), when:human('bas',t,e8), vel, layer: layerFor(inst, .3 + it*.5 + M.nap*.2), gain: who && who!=='bas' ? .9 : 1});
  v && v.release(t+len*(it>.7 && chance(.3) ? .55 : .93), .05);
}

// ======================= keys comping =======================
function playComp(t, e8, who){
  const b=layerBeat('akordy'), it=L.akordy.int, inst=L.akordy.inst;
  const change = T.events.some(e=>e.pos===step-T.barStart);
  const quiet = who && who!=='akordy' ? .65 : 1;
  let p=0;
  if (b.beat===0 && b.onBeat) p=.8; else if (change) p=.9;
  else if (b.onBeat) p=(.08 + it*.35 + M.ruch*.1)*quiet;
  else if (b.lastBeat && b.sub===b.groupLen-1) p=(.15+it*.3)*quiet;       // anticipation
  else p = it*.12*quiet;
  if (who==='perk') p = b.beat===0 && b.onBeat && chance(.5) ? 1 : 0;
  if (!chance(p)) return;
  const listen = who && who!=='akordy' ? 1 - T.soloHeat*.35 : 1;
  const level = clamp(dyn(.32 + M.nap*.22 + it*.35, .7), .1, 1), vel = clamp(level*quiet*listen*(b.beat===0 ? 1 : .9) + rnd(-.03,.03), .08, 1);
  const cc = (b.lastBeat && !b.onBeat && chordEndsThisBeat()) ? nextChordAfter() : T.chord;
  held.akordy.forEach(v=>v&&v.release(t+.03, .12)); held.akordy=[];
  const lay = compLayer(inst, level), short = it>.55 && chance(it*.5), tt=human('akordy',t,e8);
  for (const m of voicing(cc, level, 68 + Math.round(it*8))){
    const v = noteOn(inst, m, {bus:layerBus('akordy'), when:tt+rnd(0,.008), vel, layer:lay, gain:.5});
    if (v){ v.release(t + (short ? e8*rnd(.8,1.6) : e8*rnd(6,7.5)), short ? .06 : .2); held.akordy.push(v); }
  }
}
function leftHand(t, e8){ // shells under a keys solo
  if (!T.events.some(e=>e.pos===step-T.barStart)) return;
  const c=T.chord, Tt=TYPES[c.type];
  for (const iv of Tt.guide){ const v=noteOn(L.akordy.inst, snap(52+((c.root+iv)%12), [(c.root+iv)%12]), {bus:layerBus('akordy'), when:t, vel:.3, layer:'p', gain:.4}); v&&v.release(t+e8*6, .2); }
}

// ======================= soloists (any layer) =======================
// rhythm cells in eighths; the second list (with sixteenths) unlocks with intensity
const CELLS_CALM = [[2,1,1,4],[1,1,2,4],[3,1,4],[2,2,4],[2,6],[4,2,2],[3,3,2],[1,1,1,1,4]];
const CELLS_HOT  = [[.5,.5,.5,.5,1,1],[1,.5,.5,1,1],[.5,.5,1,.5,.5,1],[1.5,.5,1,1],[.5,.5,.5,.5,.5,.5,1],[1,1,.5,.5,.5,.5]];
const CELLS_SLOW = [[4,4],[6,2],[3,5],[8]];
function playerFor(id){ return players[id] ||= {phrase:[], next: step + (id==='solo' ? 4 : 2), mel:null}; }
function planPhrase(id, inst){
  const P=playerFor(id), it=L[id].int, [lo,hi]=rangeOf(inst), lv=sectionLevels();
  const slow = id==='tlo' || inst==='kieliszki';
  const dens = clamp((.3 + M.nap*.4 + it*.4) * (lv.solo || 1), 0, 1.2);
  const bars = chance(.35 + dens*.35) ? 2 : 1, barLen=METERS[L[id].meter==='wspólne' ? T.meter : L[id].meter].len;
  const startOffs = METERS[L[id].meter==='wspólne' ? T.meter : L[id].meter].starts;
  let st = step + pick([0, ...startOffs.slice(0,2), 1]); const end = step + bars*barLen - 1;
  const peak = clamp(dyn(.3 + M.nap*.3 + it*.45, .7) + rnd(-.05,.05), .12, 1);
  const pool = slow ? CELLS_SLOW : (it>.45 && chance(it) ? [...CELLS_CALM, ...CELLS_HOT, ...CELLS_HOT] : CELLS_CALM);
  const notes=[];
  while (st < end){ for (const d of pick(pool)){ if (st>=end) break; notes.push({at:st, len:Math.min(d, end-st+1)}); st+=d; } }
  const dir=pick([-1,1]), lay=layerFor(inst, peak);
  notes.forEach((n,i)=>{ const p=i/Math.max(1,notes.length-1);
    n.vel=clamp(peak*(.82+.18*Math.sin(Math.PI*Math.min(1,p*1.15))) + (n.len<1 ? -.06 : 0), .1, 1); n.layer=lay; n.dir=dir; n.first=i===0; n.last=i===notes.length-1; });
  if (notes.length) notes[notes.length-1].len += slow ? 6 : 4;
  // with intensity: climb toward the top of the range and sometimes repeat a "cry" note
  if (it>.6 && notes.length>4 && chance(it*.5)){ const k=notes.length-3; notes.slice(k).forEach(n=>n.cry=true); }
  if (P.mel===null) P.mel = Math.round((lo+hi)/2);
  P.phrase = notes; P.next = step + (bars + (chance(dens) ? (it>.6?0:1) : 2))*barLen;
}
function playSolo(id, t, e8, tStraight){
  const inst=soloInst(id), P=playerFor(id), it=L[id].int, [lo,hi]=rangeOf(inst);
  if (step >= P.next) planPhrase(id, inst);
  const c=T.chord, sc=soloScale(c), legato = MONO.has(inst) || inst.startsWith('chor_') || inst==='syn_smyczki';
  const due = P.phrase.filter(n=>n.at>=step && n.at<step+1);
  for (const n of due){
    const strong = (n.at%1===0) && layerBeat(id).onBeat;
    let target;
    if (n.cry){ target = hi - pick([2,3,4]); }
    else { const leap = chance(.1 + it*.25) ? pick([3,4,5,7]) : pick([1,1,2,2,-1,3]); target = n.first ? P.mel : P.mel + n.dir*leap; }
    if (chance(.18)) n.dir*=-1;
    if (target>hi-1){ target=hi-3; n.dir=-1; } if (target<lo+1){ target=lo+3; n.dir=1; }
    const m = snap(target, (strong||n.last) ? soloTargets(c) : sc); P.mel=m;
    const frac = n.at-step, tt = human(id, frac===0 ? t : tStraight + frac*e8, e8), dur = n.len*e8;
    // dynamics inside the phrase: higher notes a little louder, swung off-beats lifted, quick passing notes lighter
    const mid=(lo+hi)/2, offbeat = (n.at%1===0) && !layerBeat(id).onBeat;
    let vel = n.vel + (m-mid)/(hi-lo)*.16 + (offbeat ? .04 : 0) - (n.len<1 ? .07 : 0) + (n.cry ? .1 : 0);
    vel = clamp(vel*rnd(.96,1.04), .08, 1);
    if (n.first && legato && !inst.startsWith('chor_')) inhale(tt-.32, (EXPR[inst]?.inhale ?? .03)*(.6+it*.6), layerBus(id));
    T.soloHeat = Math.max(T.soloHeat, vel*(.6+it*.4));
    const scoop = legato && (n.first || n.len>=2) && chance(.15 + it*.5) ? rnd(.4, .5+it*1.2) : 0;
    const v = noteOn(inst, m, {bus:layerBus(id), ch:'solo_'+id, mono:legato, when:tt, vel, layer:n.layer, feel:.35+it*.5+T.E*.2, vib: n.len>=1.5 || n.last,
      legato: legato && !n.first, scoop, attack: inst.startsWith('chor_') ? .12 : undefined});
    if (!v) continue;
    const i=P.phrase.indexOf(n), nx=P.phrase[i+1];
    if (n.last){
      if (legato && chance(.15 + it*.3)) v.fall(tt+dur*.7, pick([2,3,5]), .4+it*.2);
      else { v.swell(tt+dur*.3, v.amp*(.55+it*.3), dur*.6); v.release(tt+dur*.95, .2); }
    } else if (!legato || !nx || Math.abs(nx.at-(n.at+n.len))>.01) v.release(tt+dur*(it>.6 && n.len<=1 ? .7 : .92), legato ? .1 : undefined);
    if (nx) nx.dir=n.dir;
  }
}

// ======================= drums =======================
const rr = (name, n) => `${name}_${1+Math.floor(Math.random()*n)}`;
function kit(name, n, o){ hit('zestaw', rr(name, n), {bus:layerBus('perk'), ...o}); }
function playDrums(t, e8, lv, who){
  const it=L.perk.int, b=layerBeat('perk'), dens=clamp(lv.perk*(.3+M.ruch*.6+it*.4), 0, 1.6);
  const tt=human('perk', t, e8);
  if (who==='perk'){ drumSolo(tt, e8, b); return; }
  const under = (who && who!=='perk' ? (who==='bas' ? .7 : .9) : 1) * (.72 + T.E*.45 + T.soloHeat*.15);
  const inst=L.perk.inst;
  if (inst==='zestaw'){
    const lastBar = S.form==='nastroj' ? T.secBar===SECTIONS[T.sec].bars : (T.chordRemain<=1e-6 && S.prog.length && T.ci % S.prog.length===0);
    const triple = b.groupLen===3;
    // ride: every beat; swing "skip" note before even beats (duple) or the last eighth of the group (triple / shuffle)
    let ride = b.onBeat || (triple ? b.sub===2 && dens>.3 : (b.sub===1 && b.beat%2===1 && dens>.35 && chance(.85)));
    if (!b.onBeat && it>.6 && chance((it-.6)*.8)) ride = true;
    if (ride && dens>.1){
      const acc = b.onBeat ? (b.beat%2===1 ? 1 : .8) : .6;
      kit(it>.55 && chance(it*.6) ? 'ride_mp' : 'ride_pp', 4, {when:tt, gain:.3*acc*(.75+it*.5)*under, rate:rnd(.99,1.01)});
      if (it>.75 && b.onBeat && chance(.15)) kit('ride_kopula', 3, {when:tt, gain:.14});
    }
    if (b.onBeat && b.beat%2===1 && dens>.2) kit('hihat_noga', 4, {when:tt, gain:.35*under});
    if (b.onBeat && dens>.5 && chance(.2+it*.3)) kit('stopa_p', 4, {when:tt, gain:(.2+it*.15)*under});
    if (b.onBeat && b.beat===0 && T.secBar===1 && dens>.3) kit(it>.5?'stopa_f':'stopa_p', it>.5?3:4, {when:tt, gain:.35});
    if (!b.onBeat && it>.5 && chance(it*.12)) kit('stopa_f', 3, {when:tt, gain:.3*under});                 // "bombs"
    if (b.onBeat && b.lastBeat && dens<=.6 && it<.5 && chance(.7)) kit('werbel_obrecz', 4, {when:tt, gain:.25});
    if (!b.onBeat && dens>.5 && chance(.1+it*.35)) kit('werbel_ghost', 4, {when:tt, gain:(.15+it*.08)*under});
    if (!b.onBeat && chance(it*it*.3)) kit(it>.8?'werbel_f':'werbel_mp', it>.8?3:4, {when:tt, gain:.25*under});   // comping accents
    if (lastBar && b.beat>=b.beats-2 && dens>.3){
      kit(b.lastBeat && b.onBeat ? 'werbel_mp' : 'werbel_ghost', 4, {when:tt, gain:b.lastBeat?.3:.2});
      if (it>.5) kit('werbel_ghost', 4, {when:tt+e8*.5, gain:.18});
    }
  } else if (inst==='jazz'){
    if (b.onBeat || (b.sub===1 && b.beat%2===1 && dens>.55 && chance(.8)))
      hit('talerz', chance(.85)?'suscymb1_hit_stick_pp1':'suscymb1_hit_stick_mp1', {bus:layerBus('perk'), when:tt, gain:.13*(b.beat%2?1:.75)*(.7+it*.5), dur:e8*3.5, rate:rnd(.98,1.02)});
    if (b.onBeat && b.beat%2===1 && dens>.3) hit('hihat', pick(['hihat_close_rr1','hihat_close_rr2']), {bus:layerBus('perk'), when:tt, gain:.25});
    if (dens>.8 && !b.onBeat && chance(.18+it*.2)) hit('werbel','snare2_taps_v4_rr1',{bus:layerBus('perk'), when:tt, gain:.08+it*.05});
    if (dens>.9 && b.beat===0 && b.onBeat && chance(.5)) hit('beben','bassdrum_hit_pp1',{bus:layerBus('perk'), when:tt, gain:.25});
  } else {
    if (b.beat===0 && b.onBeat && T.bar%2===0) hit('talerz', pick(['suscymb1_bow_13','suscymb1_bow_7']), {bus:layerBus('perk'), when:tt, gain:.08+M.noc*.06});
    if (b.onBeat && dens>.3) hit('shaker', pick(['mid_shakerdouble_down_rr1','mid_shakerdouble_up_rr1','mid_shakerdouble_down_rr2']), {bus:layerBus('perk'), when:tt, gain:.12+M.ruch*.1+it*.08});
  }
}
// drum solo: short motifs over the beat, busier and louder with intensity
const DRUM_MOTIFS = [
  [['werbel_ghost',0],['werbel_ghost',.5],['werbel_mp',1]],
  [['tom',0],['tom',.5],['werbel_mp',1],['stopa_p',1.5]],
  [['stopa_f',0],['werbel_f',1]],
  [['ride_mp',0],['stopa_p',.5],['ride_mp',1],['werbel_ghost',1.5]],
  [['werbel_mp',0],['tom',1],['tom',1.5]],
  [['werbel_ghost',0],['werbel_ghost',.5],['werbel_ghost',1],['werbel_ghost',1.5]],
];
function drumSolo(t, e8, b){
  const it=L.perk.int;
  if (!b.onBeat || chance(.15*(1-it))) { if (chance(.2)) kit('ride_pp',4,{when:t, gain:.18}); return; }
  if (b.beat===0 && chance(.5)) kit('ride_mp', 4, {when:t, gain:.3});
  const motif = pick(it>.6 ? DRUM_MOTIFS : DRUM_MOTIFS.slice(0,4));
  for (const [s, off] of motif){
    if (off >= b.groupLen) continue;
    const when = t + off*e8, g = (.22 + it*.2) * (off===0 ? 1 : .85);
    if (s==='tom') hit('tom', pick(['tomh_hits_v2_rr1','tomh_hits_v3_rr1','tomh_hits_v2_rr2','tomh_hits_v4_rr1']), {bus:layerBus('perk'), when, gain:g*.9, rate:rnd(.9,1.12)});
    else kit(s, s==='werbel_f'||s==='stopa_f' ? 3 : 4, {when, gain:g});
  }
}

// ======================= start / stop =======================
const FIRST = {tlo:['mf'], bas:['p','mf'], akordy:['p','mf'], solo:['p','mf'], perk:null};
async function loadLayer(id, quiet, all){
  const keys=[...layerInsts(id, L[id].inst)]; if (id==='tlo') keys.push(soloInst('tlo'));
  await Promise.all([...new Set(keys)].map(k=>loadInst(k, all ? null : FIRST[id], quiet)));
}
async function loadAll(){
  await loadLayer('tlo');
  (async ()=>{
    $('status').textContent = 'Dociągam instrumenty…';
    try {
      await loadLayer('bas', true); await loadLayer('akordy', true); await loadLayer('perk', true); await loadInst('mark_tree', null, true);
      await loadLayer('solo', true); await loadLayer('solo', true, true); await loadLayer('akordy', true, true); await loadLayer('bas', true, true);
      if (L.perk.inst==='zestaw') await loadInst('tom', null, true);
    } catch(e){} finally { $('status').textContent=''; }
  })();
}
function silence(id){ if (!ctx) return; (held[id]||[]).forEach(v=>v&&v.release(ctx.currentTime,.3)); held[id]=[];
  for (const k in lastVoice) if (k.includes(id) || k===L[id]?.inst) lastVoice[k]?.release(ctx.currentTime,.1); }
async function start(){
  initAudio(); await ctx.resume();
  await loadManifest(); await loadAll();
  resetTransport(); running=true; nextT=ctx.currentTime+.15;
  timer=setInterval(schedule, 50); schedule();
}
function stop(){ running=false; clearInterval(timer); ['tlo','akordy','solo','bas'].forEach(silence); Object.values(lastVoice).forEach(v=>v&&v.release(ctx.currentTime,.2)); }
document.addEventListener('visibilitychange', ()=>{ if (ctx && running && !document.hidden && ctx.state!=='running') ctx.resume(); });
