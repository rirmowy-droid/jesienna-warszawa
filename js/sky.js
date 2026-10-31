// Night-time Warsaw background: rain, street lamps that glow with the notes, falling leaves.
'use strict';
// ======================= night sky: rain, lamps, leaves =======================
const cv=$('sky'), g2=cv.getContext('2d'); let W=0,H=0, glowLvl=0, glowCol=[217,149,63];
const drops=[], leaves=[];
function resize(){ const dpr=1; W=cv.width=innerWidth*dpr; H=cv.height=innerHeight*dpr; }
addEventListener('resize', resize); resize();
const GLOW = {chor:[150,160,210], rhodes:[217,149,63], sax:[230,170,90], trabka:[230,170,90], kontrabas:[160,90,60], syn:[110,140,200], wibrafon:[180,200,230], dzwon:[200,210,240], pianino:[230,210,180]};
function glow(key, vel, t){ const k=Object.keys(GLOW).find(p=>key.startsWith(p)); const d=Math.max(0,(t-(ctx?ctx.currentTime:0))*1000);
  setTimeout(()=>{ glowLvl=Math.min(1, glowLvl+vel*.35); if(k) glowCol=GLOW[k]; }, d); }
function frame(){
  const noc = M.noc, rain = .2 + M.ruch*.8;
  g2.fillStyle=`rgba(${Math.round(lerp(26,10,noc))},${Math.round(lerp(22,10,noc))},${Math.round(lerp(34,16,noc))},1)`; g2.fillRect(0,0,W,H);
  // street lamps
  const lamps=[.18,.5,.82];
  lamps.forEach((x,i)=>{ const lx=x*W, ly=H*.62, r=H*(.18+glowLvl*.12)*(1+.08*Math.sin(Date.now()/900+i));
    const gr=g2.createRadialGradient(lx,ly,0,lx,ly,r); gr.addColorStop(0,`rgba(${glowCol.join(',')},${.22+glowLvl*.25})`); gr.addColorStop(1,'rgba(0,0,0,0)');
    g2.fillStyle=gr; g2.beginPath(); g2.arc(lx,ly,r,0,7); g2.fill();
    g2.fillStyle='rgba(20,18,24,.9)'; g2.fillRect(lx-2,ly,4,H-ly); g2.fillStyle=`rgba(${glowCol.join(',')},.9)`; g2.beginPath(); g2.arc(lx,ly,5,0,7); g2.fill(); });
  // skyline
  g2.fillStyle='rgba(8,8,12,.95)'; let x=0; let seed=7; while(x<W){ seed=(seed*9301+49297)%233280; const w=W*(.04+seed/233280*.08), h=H*(.12+((seed*7)%100)/100*.22); g2.fillRect(x,H-h,w,h); x+=w; }
  // rain
  while (drops.length < 140*rain*(PERF.lite?.35:1)) drops.push({x:Math.random()*W, y:Math.random()*H, v:rnd(.6,1.2)});
  g2.strokeStyle='rgba(180,190,220,.18)'; g2.lineWidth=1; g2.beginPath();
  for (let i=drops.length-1;i>=0;i--){ const d=drops[i]; d.y+=H*.012*d.v*(.6+M.ruch); d.x-=W*.0015; g2.moveTo(d.x,d.y); g2.lineTo(d.x+W*.003,d.y-H*.02);
    if (d.y>H){ if (drops.length>140*rain*(PERF.lite?.35:1)) drops.splice(i,1); else { d.y=-10; d.x=Math.random()*W*1.1; } } }
  g2.stroke();
  // leaves
  if (leaves.length<10 && chance(.02)) leaves.push({x:Math.random()*W, y:-20, r:Math.random()*6, vx:rnd(-.6,.6), vy:rnd(.6,1.2), c:pick(['#a8482c','#cfae5c','#d9953f','#7a3522'])});
  for (let i=leaves.length-1;i>=0;i--){ const l=leaves[i]; l.x+=l.vx*W*.002+Math.sin(Date.now()/700+i)*.6; l.y+=l.vy*H*.0015; l.r+=.03;
    g2.save(); g2.translate(l.x,l.y); g2.rotate(l.r); g2.fillStyle=l.c; g2.globalAlpha=.7; g2.beginPath(); g2.ellipse(0,0,7,3.5,0,0,7); g2.fill(); g2.restore();
    if (l.y>H+20) leaves.splice(i,1); }
  glowLvl*=.93;
}
let lastF=0; function loop(ts){ if (ts-lastF > (PERF.lite ? 100 : 40)){ lastF=ts; frame(); } requestAnimationFrame(loop); }  // ~25 fps is enough and leaves room for audio
requestAnimationFrame(loop);
