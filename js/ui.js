// Controls: song & harmony, rhythm & meters, layers (instrument, solo, volume, intensity, own meter), play strip.
'use strict';

const UI = {
  sectionShown(label){ $('sec').textContent = label; },
  chordShown(name){ $('chord').textContent = '♪ '+name; buildStrip(); },
  meterShown(m){ $('meterTag').textContent = m; updatePolyInfo(); },
  songChanged(){ $('song').value = S.songId; },
};

// ---------- song & harmony ----------
function fillSelect(sel, items){ sel.innerHTML=''; for (const [v,t] of items){ const o=document.createElement('option'); o.value=v; o.textContent=t; sel.appendChild(o); } }
(function buildSongControls(){
  const sel=$('song'); const groups={};
  for (const s of SONGS){ (groups[s.group] ||= []).push(s); }
  for (const g in groups){ const og=document.createElement('optgroup'); og.label=g;
    for (const s of groups[g]){ const o=document.createElement('option'); o.value=s.id; o.textContent=`${s.name} · ${s.meter}`; og.appendChild(o); } sel.appendChild(og); }
  const oc=document.createElement('option'); oc.value='custom'; oc.textContent='Własna progresja'; sel.appendChild(oc);
  fillSelect($('key'), NAMES_FLAT.map((n,i)=>[i, n]));
  fillSelect($('scale'), Object.entries(SCALES).map(([k,v])=>[k, v.name]));
  fillSelect($('meter'), Object.keys(METERS).map(k=>[k,k]));
  sel.value=S.songId; $('key').value=S.key; $('meter').value=S.meter; $('tempo').value=S.baseTempo;
  setSong(S.songId); showCustomFromSong();
})();
function showCustomFromSong(){ const sg=SONGS.find(x=>x.id===S.songId); if (sg) $('custom').value = sg.chords; }
function toast(msg){ $('status').textContent = msg; clearTimeout(toast.t); toast.t = setTimeout(()=>{ if ($('status').textContent===msg) $('status').textContent=''; }, 2500); }
const fromNextBar = () => running ? toast('Zmiana od następnego taktu') : null;

$('song').onchange = e=>{
  const id=e.target.value;
  if (id==='custom'){ $('custom').closest('details').open = true; $('custom').focus(); return; }
  const sg=setSong(id); showCustomFromSong();
  // a song brings its own key, meter and tempo; everything can still be changed afterwards
  S.key=sg.key; $('key').value=sg.key; queueProg();
  S.meter=sg.meter; $('meter').value=sg.meter; S.meterMode='fixed'; $('meterMode').value='fixed'; $('seqBox').classList.add('hidden');
  S.baseTempo=sg.tempo; $('tempo').value=sg.tempo; showTempo(); if (ctx) applyMacros();
  if (sg.group!=='Nastrojowe' && S.form==='nastroj'){ S.form='jam'; $('form').value='jam'; }
  fromNextBar();
};
$('key').onchange = e=>{ setKey(+e.target.value); fromNextBar(); };
$('form').onchange = e=>{ S.form=e.target.value; T.secBar=0; T.sec=0; T.chorus=0; fromNextBar(); };
$('soloWho').onchange = e=>{ S.soloWho=e.target.value; refreshSoloButtons(); };
$('scale').onchange = e=>{ S.scale=e.target.value; buildStrip(); };
$('applyCustom').onclick = ()=>{
  const r=setCustom($('custom').value), info=$('customInfo');
  if (!r.chords.length){ info.textContent='Nie rozpoznałem żadnego akordu.'; info.classList.add('err'); return; }
  info.classList.toggle('err', r.errors.length>0);
  info.textContent = `${r.chords.length} akordów, ${progBars(r.chords)} taktów` + (r.errors.length ? ` · pominięte: ${r.errors.join(' ')}` : '');
  $('song').value='custom'; fromNextBar();
};

// ---------- rhythm & meters ----------
function showTempo(){ $('tempoVal').textContent = `${S.baseTempo} bpm` + (M.ruch!==.5 ? ` (z Ruchem: ${tempo()})` : ''); }
$('tempo').addEventListener('input', e=>{ S.baseTempo=+e.target.value; showTempo(); });
$('m_ruch').addEventListener('input', showTempo);
$('meter').onchange = e=>{ S.meter=e.target.value; if (S.meterMode!=='fixed'){ S.meterMode='fixed'; $('meterMode').value='fixed'; $('seqBox').classList.add('hidden'); } fromNextBar(); };
$('meterMode').onchange = e=>{ S.meterMode=e.target.value; $('seqBox').classList.toggle('hidden', S.meterMode!=='seq'); readSeq(); fromNextBar(); };
function readSeq(){ const v=$('meterSeq').value.split(/[\s,]+/).filter(x=>METERS[x]); S.meterSeq = v.length ? v : ['4/4']; }
$('meterSeq').addEventListener('change', ()=>{ readSeq(); fromNextBar(); });
function updatePolyInfo(){
  const poly=polyLayers(), el=$('polyInfo');
  if (!poly.length){ el.textContent='Polirytm: każda warstwa może grać we własnym metrum (w „Warstwach” → Metrum). Warstwy spotykają się na początku każdej części.'; return; }
  const c=polyCycle(); const names={tlo:'tło',bas:'bas',akordy:'akordy',solo:'solista',perk:'perkusja'};
  el.textContent = `Polirytm: ${poly.map(id=>`${names[id]} w ${L[id].meter}`).join(', ')}, reszta w ${T.meter}. Wszyscy spotykają się na „raz” co ${c} ósemek` + (S.form==='nastroj' ? ', a części kończą się w takim miejscu, żeby się zgadzało.' : ' i na początku każdego chorusu.');
}

// ---------- layers ----------
const LNAMES = {tlo:'Tło',bas:'Bas',akordy:'Akordy',solo:'Solista',perk:'Perkusja'};
function buildLayers(){
  const box=$('layers');
  for (const d of LAYER_DEF){
    const st=L[d.id], row=document.createElement('div'); row.className='layer'; row.style.display='block';
    const head=document.createElement('div'); head.className='lhead';
    const b=document.createElement('button'); b.className='tog on'; b.textContent='● '+d.name;
    b.onclick=()=>{ st.on=!st.on; b.classList.toggle('on',st.on); b.textContent=(st.on?'● ':'○ ')+d.name; if(!st.on) silence(d.id); updatePolyInfo(); };
    const s=document.createElement('select'); for (const [v,n] of d.opts){ const o=document.createElement('option'); o.value=v; o.textContent=n; s.appendChild(o); }
    s.value=st.inst; s.onchange=async()=>{ silence(d.id); st.inst=s.value; if (d.id==='solo') buildStrip(); if (ctx){ await loadLayer(d.id); loadLayer(d.id, true, true); if (d.id==='perk') loadInst('tom', null, true); } };
    const so=document.createElement('button'); so.className='solo-b'; so.textContent='Solo'; so.dataset.id=d.id;
    so.onclick=async()=>{ S.soloWho = S.soloWho===d.id ? 'auto' : d.id; $('soloWho').value=S.soloWho; refreshSoloButtons();
      if (ctx){ if (d.id==='perk') loadInst('tom', null, true); if (d.id==='tlo') loadInst(soloInst('tlo'), null, true); else loadLayer(d.id, true, true); } };
    head.append(b,s,so);
    const ctl=document.createElement('div'); ctl.className='lctl';
    const mk=(label, cls, val, fn, volId)=>{ const l=document.createElement('label'); l.textContent=label; const r=document.createElement('input'); r.type='range'; r.min=0; r.max=100; r.value=Math.round(val*100); if (cls) r.className=cls; if (volId) r.dataset.layervol=volId; r.oninput=()=>fn(r.value/100); l.appendChild(r); return l; };
    ctl.append(
      mk('Głośność', '', st.vol, v=>{ st.vol=v; if (ctx) layerBus(d.id).gain.setTargetAtTime(volCurve(v), ctx.currentTime, .05); }, d.id),
      mk('Intensywność', 'int', st.int, v=>{ st.int=v; }));
    const ml=document.createElement('label'); ml.textContent='Metrum warstwy'; ml.style.gridColumn='1 / -1';
    const ms=document.createElement('select'); ms.appendChild(new Option('wspólne (jak cały zespół)','wspólne')); for (const k of Object.keys(METERS)) ms.appendChild(new Option(k,k));
    ms.value=st.meter; ms.onchange=()=>{ st.meter=ms.value; if (players[d.id]) players[d.id].next=step; updatePolyInfo(); };
    ml.appendChild(ms); ctl.appendChild(ml);
    row.append(head, ctl); box.appendChild(row);
  }
}
function refreshSoloButtons(){ document.querySelectorAll('.solo-b').forEach(b=>b.classList.toggle('on', b.dataset.id===S.soloWho)); }

// ---------- play strip ----------
let stripNotes=[];
function buildStrip(){
  const box=$('strip'), inst=L.solo.inst, [lo,hi]=rangeOf(inst);
  const c = T.chord || S.prog[0] || {root:S.key, type:'m9', bars:1}, sc=soloScale(c), cp=chordPcs(c);
  const all=[]; for (let m=lo+2; m<=hi; m++) if (sc.includes(m%12)) all.push(m);
  const st=Math.max(0, Math.floor((all.length-8)/2)); stripNotes=all.slice(st, st+8);
  if (box.children.length!==stripNotes.length){ box.innerHTML=''; stripNotes.forEach(()=>{ const d=document.createElement('div'); d.className='k'; d.innerHTML='<span class="n"></span>'; box.appendChild(d); }); }
  [...box.children].forEach((d,i)=>{ const m=stripNotes[i]; d.dataset.m=m; d.querySelector('.n').textContent=NAMES[m%12]; d.classList.toggle('ct', cp.includes(m%12)); });
}
function bindStrip(){
  const el=$('strip'), heldU=new Map();
  const velAt=(k,y)=>{ const r=k.getBoundingClientRect(); return clamp(1-(y-r.top)/r.height,0,1); };
  const play=(k, vel, legato)=>{ userUntil = ctx.currentTime + eighth(T.meter)*T.barLen*2; const inst=L.solo.inst;
    return noteOn(inst, +k.dataset.m, {bus:layerBus('solo'), ch:'user', vel, layer: layerFor(inst, vel), legato, scoop: !legato && MONO.has(inst) ? .5 : 0}); };
  el.addEventListener('pointerdown', async e=>{
    const k=e.target.closest('.k'); if(!k) return; e.preventDefault();
    initAudio(); await ctx.resume(); await loadManifest(); await loadLayer('solo');
    const vel=.2+velAt(k,e.clientY)*.8, v=play(k, vel, false);
    k.classList.add('hit'); heldU.set(e.pointerId,{v,k,vel}); try{ el.setPointerCapture(e.pointerId) }catch(_){}
  });
  el.addEventListener('pointermove', e=>{
    const h=heldU.get(e.pointerId); if(!h) return; const tt=document.elementFromPoint(e.clientX,e.clientY), k=tt&&tt.closest?tt.closest('.k'):null;
    if(!k||k===h.k) return; h.k.classList.remove('hit'); if(!MONO.has(L.solo.inst)) h.v&&h.v.release();
    const v=play(k, h.vel, true); k.classList.add('hit'); heldU.set(e.pointerId,{v,k,vel:h.vel});
  });
  const up=e=>{ const h=heldU.get(e.pointerId); if(!h) return; h.v&&h.v.release(); h.k.classList.remove('hit'); heldU.delete(e.pointerId); };
  el.addEventListener('pointerup',up); el.addEventListener('pointercancel',up);
}

// ---------- transport ----------
$('playBtn').onclick = async ()=>{
  const b=$('playBtn');
  if (running){ stop(); b.textContent='▶'; b.classList.remove('on'); $('sec').textContent='Pauza'; return; }
  b.disabled=true;
  try { await start(); }
  catch(err){ console.error(err); $('status').textContent='Nie udało się wczytać dźwięków. Odśwież stronę.'; b.disabled=false; return; }
  b.disabled=false; b.textContent='■'; b.classList.add('on'); applyMacros();
};

// ---------- band / mixer views ----------
let mixerBuilt=false;
$('viewBtn').onclick = async ()=>{
  const toMixer = !$('mixer').classList.contains('show');
  $('mixer').classList.toggle('show', toMixer); $('band').classList.toggle('hidden', toMixer);
  $('viewBtn').textContent = toMixer ? 'Zespół' : 'Mikser';
  if (toMixer){ await buildMixerUI(); mixerBuilt=true; } else document.querySelectorAll('[data-layervol]').forEach(r=>r.value=Math.round(L[r.dataset.layervol].vol*100));
  scrollTo(0,0);
};
buildLayers(); buildStrip(); bindStrip(); showTempo(); updatePolyInfo();

$('perf').value = PERF.mode; $('perf').onchange = () => { try { localStorage.setItem('jw_perf', $('perf').value); } catch (e){} $('status').textContent = 'Wydajność zmieni się po odświeżeniu aplikacji.'; };
$('instr').value = INSTR.mode; $('instr').onchange = () => { try { localStorage.setItem('jw_instr', $('instr').value); } catch (e){} $('status').textContent = 'Zmiana instrumentów zadziała po odświeżeniu aplikacji.'; };
