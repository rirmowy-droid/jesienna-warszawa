// Instruments played by the synth engine instead of samples (phone default): pianos, Rhodes, FM piano,
// vibraphone, glasses, saxophones and trumpets. Same interface as a sampled voice (release, fall, swell), so the
// band code does not care which one plays. Double bass, choir, drums and bells stay sampled.
'use strict';
const ENGINE_PATCH = {
  fortepian: {gain: .5, env: [.001, .1, 1, .35], filt: {type: 'lp', f: 5200, q: .5, key: .5, vel: .9}, drift: 1,
    oscs: [{t: 'modal', p: {piano: true, n: PERF.lite ? 10 : 14, B: 3.5e-4, tau: 3.2, bright: .7, beat: .12, norm: .55}}, {t: 'click', lvl: .25, p: {f: 2500, len: .004}}]},
  pianino: {gain: .28, env: [.001, .1, 1, .3], filt: {type: 'lp', f: 4200, q: .5, key: .5, vel: .9}, drift: 2,
    oscs: [{t: 'modal', p: {piano: true, n: PERF.lite ? 9 : 12, B: 7e-4, tau: 2.2, bright: .6, beat: .3, hd: .5, norm: .7}}, {t: 'click', lvl: .3, p: {f: 2000, len: .005}}]},
  rhodes: {gain: .55, env: [.002, 4.5, 0, .45], filt: {type: 'lp', f: 3200, q: .6, key: .4, vel: .8}, drift: 1.5, trem: [4.2, .12],
    oscs: [{t: 'fm', p: {ratio: 1, idx: 1.6, idx2: .25, idxT: .5}}, {t: 'fm', lvl: .22, p: {ratio: 14, idx: 1.2, idx2: 0, idxT: .03}}, {t: 'sin', r: .5, lvl: .1}]},
  fm_piano: {gain: .42, env: [.001, 3.2, 0, .35], filt: {type: 'lp', f: 6000, q: .5, key: .3, vel: .8}, drift: 1,
    oscs: [{t: 'fm3', p: {ratio: 1, ratio2: 3.005, idx: 2.2, idx2: 1, idxT: .4}}, {t: 'fm', lvl: .35, p: {ratio: 2, idx: 1, idx2: .1, idxT: .3}}]},
  wibrafon: {gain: .32, env: [.001, .1, 1, .9], filt: {type: 'lp', f: 7000, q: .5, key: .2, vel: .7}, drift: 1, trem: [5.2, .3],
    oscs: [{t: 'modal', p: {ratios: [1, 3.98, 9.88], amps: [1, .45, .2], taus: [4.5, 1.2, .35], tk: .4, norm: .6}}, {t: 'click', lvl: .12, p: {f: 4000, len: .003}}]},
  kieliszki: {gain: .3, env: [.35, 1, .85, 1], filt: {type: 'lp', f: 8000, q: .5, key: .2}, drift: 5, vib: [3.5, 7, .6],
    oscs: [{t: 'sin'}, {t: 'sin', r: 2.32, lvl: .25}, {t: 'sin', r: 4.25, lvl: .08}, {t: 'noise', r: 1, lvl: .05, p: {q: 60}}]},
  sax_tenor: {gain: .09, env: [.035, .3, .85, .12], fenv: [.02, .3, .55, .12], filt: {type: 'lp', f: 1400, q: .9, env: 1.1, key: .65, vel: .9}, drift: 3, sat: .35, glide: .045, vib: [5.2, 11, .3],
    oscs: [{t: 'pulse', p: {pw: .32, pwm: .04, pwmRate: .3}}, {t: 'saw', lvl: .45, det: 3}, {t: 'formant', lvl: .7, p: {vowel: 'o', scale: .95}}, {t: 'noise', r: 2.5, lvl: .14, p: {q: 3}}]},
  sax_tenor_krotkie: null, sax_baryton: null, sax_baryton_krotkie: null,
  trabka: {gain: .65, env: [.03, .25, .88, .1], fenv: [.035, .25, .55, .1], filt: {type: 'lp', f: 900, q: 1.1, env: 2.8, key: .7, vel: 1}, drift: 3, sat: .45, glide: .04, vib: [5.5, 9, .35],
    oscs: [{t: 'saw'}, {t: 'saw', det: 5, lvl: .6}, {t: 'pulse', lvl: .3, p: {pw: .45}}, {t: 'noise', r: 3, lvl: .05, p: {q: 4}}]},
  trabka_tlumik: {gain: .48, env: [.03, .25, .85, .1], filt: {type: 'bp', f: 1700, q: 2.6, key: .3, vel: .6}, drift: 3, sat: .3, glide: .04, vib: [5.4, 8, .34],
    oscs: [{t: 'saw'}, {t: 'formant', lvl: .9, p: {vowel: 'i', scale: 1.15}}, {t: 'noise', r: 3, lvl: .08, p: {q: 3}}]},
};
ENGINE_PATCH.sax_tenor_krotkie = {...ENGINE_PATCH.sax_tenor, env: [.02, .25, .6, .08], vib: null};
ENGINE_PATCH.sax_baryton = {...ENGINE_PATCH.sax_tenor, filt: {type: 'lp', f: 850, q: .9, env: 1.2, key: .6, vel: .9}, gain: .2, vib: [4.8, 10, .34],
  oscs: [{t: 'pulse', p: {pw: .3, pwm: .04, pwmRate: .25}}, {t: 'saw', lvl: .5, det: 3}, {t: 'formant', lvl: .7, p: {vowel: 'o', scale: .75}}, {t: 'noise', r: 2, lvl: .14, p: {q: 3}}]};
ENGINE_PATCH.sax_baryton_krotkie = {...ENGINE_PATCH.sax_baryton, env: [.02, .25, .6, .08], vib: null};
// which instruments the engine plays: 'auto' = on the phone, 'silnik' = always, 'sample' = never
const INSTR = {mode: 'auto'}; try { INSTR.mode = localStorage.getItem('jw_instr') || 'auto'; } catch (e){}
const useEngine = key => !!ENGINE_PATCH[key] && (INSTR.mode === 'silnik' || (INSTR.mode === 'auto' && PERF.lite));
let engineReady = null; const engineNodes = new Map(); let engineId = 1; const engineLast = {};
function loadEngine(){ if (!engineReady) engineReady = ctx.audioWorklet ? ctx.audioWorklet.addModule('js/wk/synth.js').then(() => true).catch(e => { console.warn(e); return false; }) : Promise.resolve(false); return engineReady; }
function engineNode(bus){
  let n = engineNodes.get(bus);
  if (!n){ n = new AudioWorkletNode(ctx, 'wielo', {numberOfInputs: 0, outputChannelCount: [2]}); n.connect(bus); n.port.postMessage({type: 'maxv', n: PERF.lite ? 10 : 16}); engineNodes.set(bus, n); }
  return n;
}
function engineOn(key, m, o = {}){
  if (!engineReady || !ctx.audioWorklet) return null;
  let node; try { node = engineNode(o.bus || bus); } catch (e){ return null; }
  const P = ENGINE_PATCH[key], t = Math.max(o.when ?? ctx.currentTime, ctx.currentTime), vel = clamp(o.vel ?? .6, 0, 1), f = mtof(m), chan = o.ch || key;
  const isMono = o.mono ?? MONO.has(key), prev = isMono ? engineLast[chan] : null, X = EXPR[key] || {}, feel = o.feel ?? .5;
  // legato: the same voice slides to the new note (no new attack), like one breath
  if (o.legato && prev && prev.alive && Math.abs(prev.m - m) <= 12){
    node.port.postMessage({type: 'bend', id: prev.id, f, t, glide: .03 + .008 * Math.abs(prev.m - m)});
    prev.m = m; glow(key, vel, t); return prev;
  }
  if (prev && prev.alive) prev.release(t + .02, .03);
  const id = engineId++, gain = (o.gain ?? 1) * .8;
  const vib = (o.vib === false || !X.vib) ? false : [X.vib[0] * rnd(.95, 1.05), X.vib[1] * (.55 + feel * .9), X.vib[2]];
  const msg = {type: 'on', id, t, f, vel, gain, patch: P, pan: 0};
  if (vib) msg.vib = vib; else if (P.vib) msg.vib = false;
  if (o.scoop){ msg.from = f * Math.pow(2, -o.scoop / 12); msg.glide = .05 + o.scoop * .03; }
  node.port.postMessage(msg); glow(key, vel, t);
  const v = {id, m, alive: true, amp: 1,
    release(at, tau){ if (!this.alive) return; this.alive = false; node.port.postMessage({type: 'off', id, t: Math.max(at ?? ctx.currentTime, t + .01), rel: tau ? tau * 3 : undefined}); },
    fall(at, semis, len){ node.port.postMessage({type: 'bend', id, f: f * Math.pow(2, -semis / 12), t: at, glide: len / 2.5}); this.release(at + len * .8, len * .25); },
    swell(at, to, len){ node.port.postMessage({type: 'swell', id, t: at, to: Math.max(.2, Math.min(1.5, to)), len}); } };
  if (isMono) engineLast[chan] = v;
  return v;
}
