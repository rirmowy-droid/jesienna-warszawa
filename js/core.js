// Small shared helpers.
'use strict';
const $ = id => document.getElementById(id);
const clamp = (x,a,b)=>Math.max(a,Math.min(b,x));
const rnd = (a,b)=>a+Math.random()*(b-a);
const pick = a => a[Math.floor(Math.random()*a.length)];
const chance = p => Math.random() < p;
const lerp = (a,b,t)=>a+(b-a)*t;
const NAMES = ['C','C♯','D','E♭','E','F','F♯','G','A♭','A','B♭','H'];
const LAYERS = ['p','mf','f'];
const LAYER_DB = {p:-8, mf:-4, f:0};

