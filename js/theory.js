// Music theory: chord symbols, scales, meters and the song library.
// Everything here is plain data + small functions, so new chords, scales, meters and songs can be added in one place.
'use strict';

// ---------- chord types ----------
// iv: chord intervals, sc: chord-scale for melodies, guide: 3rd + 7th (voice leading, choir), col: colour tones added with tension
const TYPES = {
  maj:     {iv:[0,4,7],          sc:[0,2,4,5,7,9,11], guide:[4,7],  col:[14,9],  suf:''},
  '6':     {iv:[0,4,7,9],        sc:[0,2,4,5,7,9,11], guide:[4,9],  col:[14,7],  suf:'6'},
  maj7:    {iv:[0,4,7,11],       sc:[0,2,4,5,7,9,11], guide:[4,11], col:[14,9],  suf:'maj7'},
  maj9:    {iv:[0,4,7,11,14],    sc:[0,2,4,5,7,9,11], guide:[4,11], col:[14,21], suf:'maj9'},
  maj7s11: {iv:[0,4,7,11,14,18], sc:[0,2,4,6,7,9,11], guide:[4,11], col:[14,18], suf:'maj7♯11'},
  m:       {iv:[0,3,7],          sc:[0,2,3,5,7,9,10], guide:[3,7],  col:[14,10], suf:'m'},
  m6:      {iv:[0,3,7,9],        sc:[0,2,3,5,7,9,11], guide:[3,9],  col:[14,11], suf:'m6'},
  m7:      {iv:[0,3,7,10],       sc:[0,2,3,5,7,9,10], guide:[3,10], col:[14,17], suf:'m7'},
  m9:      {iv:[0,3,7,10,14],    sc:[0,2,3,5,7,9,10], guide:[3,10], col:[14,17], suf:'m9'},
  m11:     {iv:[0,3,7,10,14,17], sc:[0,2,3,5,7,9,10], guide:[3,10], col:[14,17], suf:'m11'},
  mmaj7:   {iv:[0,3,7,11],       sc:[0,2,3,5,7,9,11], guide:[3,11], col:[14,9],  suf:'m(maj7)'},
  m7b5:    {iv:[0,3,6,10],       sc:[0,2,3,5,6,8,10], guide:[3,10], col:[14,17], suf:'m7♭5'},
  dim7:    {iv:[0,3,6,9],        sc:[0,2,3,5,6,8,9,11], guide:[3,9], col:[14,11], suf:'dim7'},
  dom7:    {iv:[0,4,7,10],       sc:[0,2,4,5,7,9,10], guide:[4,10], col:[14,21], suf:'7'},
  dom9:    {iv:[0,4,7,10,14],    sc:[0,2,4,5,7,9,10], guide:[4,10], col:[14,21], suf:'9'},
  dom13:   {iv:[0,4,7,10,14,21], sc:[0,2,4,5,7,9,10], guide:[4,10], col:[21,14], suf:'13'},
  dom7b9:  {iv:[0,4,7,10,13],    sc:[0,1,4,5,7,8,10], guide:[4,10], col:[13,20], suf:'7♭9'},
  dom7s9:  {iv:[0,4,7,10,15],    sc:[0,1,3,4,6,8,10], guide:[4,10], col:[15,20], suf:'7♯9'},
  dom7alt: {iv:[0,4,10,13,15,20],sc:[0,1,3,4,6,8,10], guide:[4,10], col:[13,20], suf:'7alt'},
  dom7s11: {iv:[0,4,7,10,14,18], sc:[0,2,4,6,7,9,10], guide:[4,10], col:[14,18], suf:'7♯11'},
  dom9sus: {iv:[0,5,7,10,14],    sc:[0,2,4,5,7,9,10], guide:[5,10], col:[14,21], suf:'9sus'},
};
// quality text → type (checked longest first)
const QUAL = [
  ['maj7#11','maj7s11'],['maj7♯11','maj7s11'],['maj9','maj9'],['maj7','maj7'],['ma7','maj7'],['Δ','maj7'],['M7','maj7'],
  ['m(maj7)','mmaj7'],['mmaj7','mmaj7'],['m7b5','m7b5'],['m7♭5','m7b5'],['ø','m7b5'],['dim7','dim7'],['dim','dim7'],['°','dim7'],['o7','dim7'],
  ['m11','m11'],['m9','m9'],['m7','m7'],['m6','m6'],['-7','m7'],['-9','m9'],['-','m'],['m','m'],
  ['7alt','dom7alt'],['7b9','dom7b9'],['7♭9','dom7b9'],['7#9','dom7s9'],['7♯9','dom7s9'],['7#11','dom7s11'],['7♯11','dom7s11'],
  ['13','dom13'],['9sus','dom9sus'],['7sus','dom9sus'],['sus','dom9sus'],['9','dom9'],['7','dom7'],['69','6'],['6','6'],['','maj'],
];
const ROOTS = {C:0,D:2,E:4,F:5,G:7,A:9,B:11,H:11};
const NAMES_FLAT = ['C','D♭','D','E♭','E','F','G♭','G','A♭','A','B♭','H'];

// "Cm7", "F#7b9:2" (2 bars), "Bbmaj7:.5" (half a bar), "%" repeats the previous chord, "|" is ignored
function parseChord(tok){
  const [sym, durTxt] = tok.split(':');
  const m = sym.match(/^([A-H])([#b♯♭]?)(.*)$/); if (!m) return null;
  let root = ROOTS[m[1]] + (m[2]==='#'||m[2]==='♯' ? 1 : m[2]==='b'||m[2]==='♭' ? -1 : 0);
  root = (root+12)%12;
  const q = m[3].replace(/\s/g,'');
  const hit = QUAL.find(([k])=>q.startsWith(k)) || ['','maj'];
  const bars = durTxt ? Math.max(.25, parseFloat(durTxt.replace(',','.')) || 1) : 1;
  return {root, type: hit[1], bars};
}
function parseProgression(text){
  const out = []; const errors = [];
  for (const tok of text.replace(/\|/g,' ').split(/\s+/).filter(Boolean)){
    if (tok.startsWith('%')){ if (out.length){ const d=tok.split(':')[1]; out.push({...out[out.length-1], bars: d ? parseFloat(d) : 1}); } continue; }
    const c = parseChord(tok); if (c) out.push(c); else errors.push(tok);
  }
  return {chords: out, errors};
}
const transpose = (prog, by) => prog.map(c=>({...c, root:(c.root+by+120)%12}));
const chordName = c => NAMES_FLAT[c.root] + TYPES[c.type].suf;
const chordPcs = c => TYPES[c.type].iv.map(i=>(c.root+i)%12);
const chordScale = c => TYPES[c.type].sc.map(i=>(c.root+i)%12);
const progBars = prog => prog.reduce((a,c)=>a+c.bars, 0);

// ---------- scales for soloists (relative to the key) ----------
const SCALES = {
  auto:     {name:'Automatycznie (do akordu)'},
  blues:    {name:'Bluesowa', iv:[0,3,5,6,7,10]},
  pent_min: {name:'Pentatonika molowa', iv:[0,3,5,7,10]},
  pent_maj: {name:'Pentatonika durowa', iv:[0,2,4,7,9]},
  dorian:   {name:'Dorycka', iv:[0,2,3,5,7,9,10]},
  aeolian:  {name:'Eolska (moll naturalny)', iv:[0,2,3,5,7,8,10]},
  harm_min: {name:'Moll harmoniczny', iv:[0,2,3,5,7,8,11]},
  phrygian: {name:'Frygijska', iv:[0,1,3,5,7,8,10]},
  major:    {name:'Durowa', iv:[0,2,4,5,7,9,11]},
  lydian:   {name:'Lidyjska', iv:[0,2,4,6,7,9,11]},
  mixo:     {name:'Miksolidyjska', iv:[0,2,4,5,7,9,10]},
  hirajoshi:{name:'Hirajoshi (japońska)', iv:[0,2,3,7,8]},
};

// ---------- meters: beats as groups of eighth notes ----------
// g: eighths per beat, strong: beat indexes with accents
const METERS = {
  '4/4':  {g:[2,2,2,2],           strong:[0,2]},
  '3/4':  {g:[2,2,2],             strong:[0]},
  '2/4':  {g:[2,2],               strong:[0]},
  '5/4':  {g:[2,2,2,2,2],         strong:[0,3]},
  '6/4':  {g:[2,2,2,2,2,2],       strong:[0,3]},
  '7/4':  {g:[2,2,2,2,2,2,2],     strong:[0,4]},
  '5/8':  {g:[3,2],               strong:[0]},
  '6/8':  {g:[3,3],               strong:[0]},
  '7/8':  {g:[2,2,3],             strong:[0]},
  '9/8':  {g:[3,3,3],             strong:[0]},
  '11/8': {g:[3,3,3,2],           strong:[0,2]},
  '12/8': {g:[3,3,3,3],           strong:[0,2]},
};
for (const k in METERS){ const m=METERS[k]; m.len = m.g.reduce((a,b)=>a+b,0); m.starts=[]; let p=0; for (const g of m.g){ m.starts.push(p); p+=g; } }
// where in its bar is eighth `pos`? → {beat, sub (0 = on the beat), groupLen, onBeat, strong, last}
function beatInfo(meterId, pos){
  const m = METERS[meterId]; pos = ((pos % m.len) + m.len) % m.len;
  let beat = m.starts.length-1; while (m.starts[beat] > pos) beat--;
  const sub = pos - m.starts[beat];
  return {beat, sub, groupLen:m.g[beat], onBeat: sub===0, strong: sub===0 && m.strong.includes(beat), beats:m.g.length, lastBeat: beat===m.g.length-1, pos, len:m.len};
}
const gcd = (a,b)=> b ? gcd(b, a%b) : a;
const lcm = (a,b)=> a/gcd(a,b)*b;

// ---------- song library (harmony only – the solos are improvised, no themes are reproduced) ----------
// key: key the chords are written in (pc), so the song can be transposed; meter + tempo are suggestions applied when chosen
const SONGS = [
  {group:'Nastrojowe', id:'noc',      name:'Nocna mgła',          key:2,  meter:'4/4', tempo:64, chords:'Dm9 Gm9 Ebmaj7#11 A7b9'},
  {group:'Nastrojowe', id:'wisla',    name:'Nad Wisłą',           key:2,  meter:'4/4', tempo:60, chords:'Dm9:2 Bbmaj9 A7b9'},
  {group:'Nastrojowe', id:'praga',    name:'Praga nocą',          key:2,  meter:'4/4', tempo:66, chords:'Gm9 C9sus Fmaj9 Bbmaj7#11 Em7b5 A7b9 Dm9:2'},
  {group:'Nastrojowe', id:'frygijska',name:'Frygijski dryf',      key:2,  meter:'4/4', tempo:58, chords:'Dm11:2 Ebmaj7#11:2'},
  {group:'Nastrojowe', id:'deszcz',   name:'Deszcz na Pradze',    key:2,  meter:'6/8', tempo:62, chords:'Dm9 Fmaj9 Bbmaj7#11 A7b9'},
  {group:'Nastrojowe', id:'latarnie', name:'Latarnie (5/4)',      key:2,  meter:'5/4', tempo:70, chords:'Dm9 Dm9 Bbmaj7#11 A7alt'},
  {group:'Standardy jazzowe', id:'autumn', name:'Autumn Leaves (w g-moll)', key:7, meter:'4/4', tempo:110,
    chords:'Cm7 F7 Bbmaj7 Ebmaj7 Am7b5 D7b9 Gm7:2 Cm7 F7 Bbmaj7 Ebmaj7 Am7b5 D7b9 Gm7:2 Am7b5 D7b9 Gm7:2 Cm7 F7 Bbmaj7 Ebmaj7 Am7b5 D7b9 Gm7:.5 C7:.5 Fm7:.5 Bb7:.5 Am7b5 D7b9 Gm7:2'},
  {group:'Standardy jazzowe', id:'bossa', name:'Blue Bossa', key:0, meter:'4/4', tempo:120,
    chords:'Cm7:2 Fm7:2 Dm7b5 G7b9 Cm7:2 Ebm7 Ab7 Dbmaj7:2 Dm7b5 G7b9 Cm7 Dm7b5:.5 G7b9:.5'},
  {group:'Standardy jazzowe', id:'sowhat', name:'So What (modalnie)', key:2, meter:'4/4', tempo:120,
    chords:'Dm11:16 Ebm11:8 Dm11:8'},
  {group:'Standardy jazzowe', id:'take5', name:'Take Five – vamp (5/4)', key:3, meter:'5/4', tempo:150,
    chords:'Ebm7 Bbm7 Ebm7 Bbm7 Ebm7 Bbm7 Ebm7 Bbm7 Bmaj7 Abm7 Bbm7 Ebm7 Abm7 Db7 Gbmaj7:2'},
  {group:'Standardy jazzowe', id:'attya', name:'All the Things You Are', key:8, meter:'4/4', tempo:130,
    chords:'Fm7 Bbm7 Eb7 Abmaj7 Dbmaj7 G7 Cmaj7:2 Cm7 Fm7 Bb7 Ebmaj7 Abmaj7 D7 Gmaj7:2 Am7 D7 Gmaj7:2 F#m7b5 B7 Emaj7 C7alt Fm7 Bbm7 Eb7 Abmaj7 Dbmaj7 Dbm7 Cm7 Bdim7 Bbm7 Eb7 Abmaj7:2'},
  {group:'Standardy jazzowe', id:'summer', name:'Summertime (uproszczone)', key:9, meter:'4/4', tempo:72,
    chords:'Am6:4 Dm6:2 Am6:2 Bm7b5 E7b9 Am6 Dm6 Am6 E7b9 Am6:2'},
  {group:'Standardy jazzowe', id:'rhythm', name:'Rhythm Changes (A)', key:10, meter:'4/4', tempo:170,
    chords:'Bbmaj7:.5 G7:.5 Cm7:.5 F7:.5 Dm7:.5 G7:.5 Cm7:.5 F7:.5 Fm7:.5 Bb7:.5 Ebmaj7:.5 Ab7:.5 Dm7:.5 G7:.5 Cm7:.5 F7:.5'},
  {group:'Blues', id:'blues_f', name:'Jazz blues w F', key:5, meter:'4/4', tempo:120,
    chords:'F7 Bb7 F7 Cm7:.5 F7:.5 Bb7 Bdim7 F7 D7b9 Gm7 C7 F7:.5 D7:.5 Gm7:.5 C7:.5'},
  {group:'Blues', id:'blues_min', name:'Molowy blues w C', key:0, meter:'4/4', tempo:96,
    chords:'Cm7:4 Fm7:2 Cm7:2 Ab7 G7alt Cm7 G7alt'},
  {group:'Blues', id:'blues_slow', name:'Wolny blues 12/8 w G', key:7, meter:'12/8', tempo:56,
    chords:'G7 C7 G7 G7 C7 C7 G7 E7 Am7 D7 G7 D7'},
  {group:'Blues', id:'blues_bird', name:'Bird blues w F', key:5, meter:'4/4', tempo:150,
    chords:'Fmaj7 Em7b5:.5 A7:.5 Dm7:.5 G7:.5 Cm7:.5 F7:.5 Bb7 Bbm7:.5 Eb7:.5 Am7:.5 D7:.5 Abm7:.5 Db7:.5 Gm7 C7 F7:.5 D7:.5 Gm7:.5 C7:.5'},
  {group:'Blues', id:'blues_basic', name:'Prosty blues w E (shuffle)', key:4, meter:'12/8', tempo:72,
    chords:'E7 A7 E7 E7 A7 A7 E7 E7 B7 A7 E7 B7'},
];
