<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Авиагоризонт — Marine Dive ROV</title>
<style>
  :root{
    --size: 380px;
    --px-per-deg: 3.2;
  }
  *{box-sizing:border-box}
  body{
    margin:0;min-height:100vh;display:grid;place-items:center;
    background:#0b2038;color:#e9f1ff;font-family:system-ui,Segoe UI,Roboto,Arial,sans-serif;
    padding:20px
  }
  .wrap{display:grid;gap:16px;justify-items:center}
  .panel{
    background:rgba(255,255,255,0.06);
    border:1px solid rgba(255,255,255,0.12);
    border-radius:14px;
    padding:12px 16px;
    box-shadow:0 10px 24px rgba(0,0,0,0.35)
  }
  .horizon{
    width:var(--size);height:var(--size);
    border-radius:50%;position:relative;overflow:hidden;
    background:#111;
    box-shadow:
      inset 0 0 0 2px rgba(255,255,255,0.18),
      inset 0 0 24px rgba(0,0,0,0.6),
      0 10px 30px rgba(0,0,0,0.45);
  }
  .sphere{
    position:absolute;inset:-240% -80%;
    background:
      linear-gradient(to bottom, #4aa3ff 0 50%, #7b4d1f 50% 100%),
      repeating-linear-gradient(to bottom, rgba(255,255,255,0.10) 0 1px, transparent 1px 12px);
    filter:saturate(1.05);
  }
  .bezel{position:absolute;inset:0;border-radius:50%;pointer-events:none;
    box-shadow:inset 0 0 0 2px rgba(255,255,255,.25), inset 0 0 40px rgba(0,0,0,.6)}
  .roll-pointer{position:absolute;inset:0;pointer-events:none}
  .roll-pointer::before{
    content:"";position:absolute;left:50%;top:6px;transform:translateX(-50%);
    width:0;height:0;border-left:7px solid transparent;border-right:7px solid transparent;
    border-bottom:10px solid #fff;opacity:.95
  }
  .center-line{
    position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);
    width:140px;height:2px;background:#fff;border-radius:1px;box-shadow:0 0 6px rgba(255,255,255,.6)
  }
  .compass{position:absolute;inset:0;pointer-events:none;transform-origin:50% 50%;}
  .roll-scale{position:absolute;inset:0;border-radius:50%;}
  .roll-tick{
    position:absolute;left:50%;top:50%;
    width:2px;height:12px;background:#fff;border-radius:2px;opacity:.9;
    transform-origin:50% 50%;
  }
  .roll-tick.small{height:8px;opacity:.75}
  .roll-label{
    position:absolute;left:50%;top:50%;
    transform-origin:50% 50%;
    font-size:12px;opacity:.95;white-space:nowrap;
    background:rgba(0,0,0,.35);padding:1px 4px;border-radius:6px
  }
  .pitch-ladder{position:absolute;inset:0;pointer-events:none}
  .ladder-line{
    position:absolute;left:50%;
    width:110px;height:2px;background:#fff;opacity:.95;border-radius:1px
  }
  .ladder-line .text{
    position:absolute;top:-12px;font-size:12px;background:rgba(0,0,0,.4);
    padding:1px 3px;border-radius:4px
  }
  .ladder-line .text.left{left:-58px}
  .ladder-line .text.right{right:58px}
  .readouts{display:grid;grid-auto-flow:column;gap:14px;align-items:center;font-size:14px}
  .readouts b{font-size:16px}
  .ok{color:#7CFF7C}.warn{color:#FFD46A}
  .btn{display:inline-block;padding:6px 10px;border-radius:8px;background:rgba(255,255,255,.08);
       color:#cfe6ff;text-decoration:none;border:1px solid rgba(255,255,255,.12)}
  .btn:hover{background:rgba(255,255,255,.14)}
</style>
</head>
<body>
  <div class="wrap">
    <div class="panel">
      <div class="horizon" aria-label="Авиагоризонт">
        <!-- Динамический диск -->
        <div id="sphere" class="sphere">
          <div id="pitchLadder" class="pitch-ladder"></div>
        </div>

        <!-- КОМПАС (шкала по ободу) -->
        <div id="compass" class="compass">
          <div id="rollScale" class="roll-scale"></div>
        </div>

        <div class="bezel"></div>
        <div class="roll-pointer"></div>
        <div class="center-line"></div>
      </div>
    </div>

    <div class="panel readouts">
      <div>Roll:  <b id="rollVal">0.0</b>°</div>
      <div>Pitch: <b id="pitchVal">0.0</b>°</div>
      <div>Yaw:   <b id="yawVal">0.0</b>°</div>
      <div>IMU:   <b id="imuStatus" class="warn">—</b></div>
      <a href="/" class="btn">← Панель</a>
      <button id="zeroBtn" class="btn" type="button">Обнулить</button>
    </div>
  </div>

<script>
const PX_PER_DEG=parseFloat(getComputedStyle(document.documentElement).getPropertyValue('--px-per-deg'))||3.2;
const SMOOTH=0.18;

let sRoll=0,sPitch=0,sYaw=0;
let zeroRoll=0,zeroPitch=0;

const sphere=document.getElementById('sphere');
const pitchLadder=document.getElementById('pitchLadder');
const rollScale=document.getElementById('rollScale');
const compass=document.getElementById('compass');
const rollEl=document.getElementById('rollVal');
const pitchEl=document.getElementById('pitchVal');
const yawEl=document.getElementById('yawVal');
const imuStat=document.getElementById('imuStatus');
const zeroBtn=document.getElementById('zeroBtn');

zeroBtn.addEventListener('click',()=>{zeroRoll=sRoll;zeroPitch=sPitch;});

// Рисуем шкалу по кругу с цифрами и N/E/S/W
(function renderRollScale(){
  const size=parseFloat(getComputedStyle(rollScale).width);
  const rTick=size/2-12;
  const rText=rTick+22;
  for(let d=0; d<360; d+=10){
    const isMajor=(d%30===0);

    const tick=document.createElement('div');
    tick.className='roll-tick'+(isMajor?'':' small');
    tick.style.transform=`translate(-50%,-50%) rotate(${d}deg) translateY(${-rTick}px)`;
    rollScale.appendChild(tick);

    if(isMajor){
      const lbl=document.createElement('div');
      lbl.className='roll-label';
      let text=d.toString();
      if(d===0) text="N";
      else if(d===90) text="E";
      else if(d===180) text="S";
      else if(d===270) text="W";
      lbl.textContent=text;
      lbl.style.transform=`translate(-50%,-50%) rotate(${d}deg) translateY(${-rText}px) rotate(${-d}deg)`;
      rollScale.appendChild(lbl);
    }
  }
})();

// Лесенка тангажа
(function renderPitchLadder(){
  for(let d=-60;d<=60;d+=10){
    if(d===0) continue;
    const line=document.createElement('div');
    line.className='ladder-line';
    line.style.left='50%';
    const y=-d*PX_PER_DEG;
    line.style.top=`calc(50% + ${y}px)`;
    const tL=document.createElement('span');
    tL.className='text left'; tL.textContent=Math.abs(d);
    const tR=document.createElement('span');
    tR.className='text right'; tR.textContent=Math.abs(d);
    line.appendChild(tL); line.appendChild(tR);
    pitchLadder.appendChild(line);
  }
})();

function norm360(a){a=a%360;if(a<0)a+=360;return a;}

async function fetchIMU(){
  try{
    const r=await fetch('/imu',{cache:'no-store'});
    if(!r.ok) throw new Error('HTTP '+r.status);
    const data=await r.json();

    const roll_raw=Number(data.pitch??0);   // roll ← pitch
    const pitch_raw=Number(data.roll??0);   // pitch ← roll
    const yaw_raw=Number(data.yaw??0);
    const ts=Number(data.ts??0);

    sRoll = sRoll+SMOOTH*(roll_raw-sRoll);
    sPitch= sPitch+SMOOTH*(pitch_raw-sPitch);
    sYaw  = sYaw+SMOOTH*(yaw_raw-sYaw);

    const dispRoll=sRoll-zeroRoll;
    const dispPitch=sPitch-zeroPitch;

    const translateY=dispPitch*PX_PER_DEG;
    const rotateDeg=-dispRoll;
    sphere.style.transform=`translateY(${translateY}px) rotate(${rotateDeg}deg)`;

    const yawDeg=norm360(sYaw);
    // Шкала крутится по yaw (в ту же сторону, что и поворот)
    compass.style.transform=`rotate(${yawDeg}deg)`;

    rollEl.textContent=dispRoll.toFixed(1);
    pitchEl.textContent=dispPitch.toFixed(1);
    yawEl.textContent=yawDeg.toFixed(1);

    const fresh=(Date.now()/1000-ts)<1.0;
    imuStat.textContent=fresh?'OK':'STALE';
    imuStat.className=fresh?'ok':'warn';
  }catch(e){
    imuStat.textContent='NO DATA';
    imuStat.className='warn';
  }
}

setInterval(fetchIMU,50);
fetchIMU();
</script>
</body>
</html>
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Авиагоризонт — Marine Dive ROV</title>
<style>
  :root{
    --size: 380px;
    --px-per-deg: 3.2;
  }
  *{box-sizing:border-box}
  body{
    margin:0;min-height:100vh;display:grid;place-items:center;
    background:#0b2038;color:#e9f1ff;font-family:system-ui,Segoe UI,Roboto,Arial,sans-serif;
    padding:20px
  }
  .wrap{display:grid;gap:16px;justify-items:center}
  .panel{
    background:rgba(255,255,255,0.06);
    border:1px solid rgba(255,255,255,0.12);
    border-radius:14px;
    padding:12px 16px;
    box-shadow:0 10px 24px rgba(0,0,0,0.35)
  }
  .horizon{
    width:var(--size);height:var(--size);
    border-radius:50%;position:relative;overflow:hidden;
    background:#111;
    box-shadow:
      inset 0 0 0 2px rgba(255,255,255,0.18),
      inset 0 0 24px rgba(0,0,0,0.6),
      0 10px 30px rgba(0,0,0,0.45);
  }
  .sphere{
    position:absolute;inset:-240% -80%;
    background:
      linear-gradient(to bottom, #4aa3ff 0 50%, #7b4d1f 50% 100%),
      repeating-linear-gradient(to bottom, rgba(255,255,255,0.10) 0 1px, transparent 1px 12px);
    filter:saturate(1.05);
  }
  .bezel{position:absolute;inset:0;border-radius:50%;pointer-events:none;
    box-shadow:inset 0 0 0 2px rgba(255,255,255,.25), inset 0 0 40px rgba(0,0,0,.6)}
  .roll-pointer{position:absolute;inset:0;pointer-events:none}
  .roll-pointer::before{
    content:"";position:absolute;left:50%;top:6px;transform:translateX(-50%);
    width:0;height:0;border-left:7px solid transparent;border-right:7px solid transparent;
    border-bottom:10px solid #fff;opacity:.95
  }
  .center-line{
    position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);
    width:140px;height:2px;background:#fff;border-radius:1px;box-shadow:0 0 6px rgba(255,255,255,.6)
  }
  .compass{position:absolute;inset:0;pointer-events:none;transform-origin:50% 50%;}
  .roll-scale{position:absolute;inset:0;border-radius:50%;}
  .roll-tick{
    position:absolute;left:50%;top:50%;
    width:2px;height:12px;background:#fff;border-radius:2px;opacity:.9;
    transform-origin:50% 50%;
  }
  .roll-tick.small{height:8px;opacity:.75}
  .roll-label{
    position:absolute;left:50%;top:50%;
    transform-origin:50% 50%;
    font-size:12px;opacity:.95;white-space:nowrap;
    background:rgba(0,0,0,.35);padding:1px 4px;border-radius:6px
  }
  .pitch-ladder{position:absolute;inset:0;pointer-events:none}
  .ladder-line{
    position:absolute;left:50%;
    width:110px;height:2px;background:#fff;opacity:.95;border-radius:1px
  }
  .ladder-line .text{
    position:absolute;top:-12px;font-size:12px;background:rgba(0,0,0,.4);
    padding:1px 3px;border-radius:4px
  }
  .ladder-line .text.left{left:-58px}
  .ladder-line .text.right{right:58px}
  .readouts{display:grid;grid-auto-flow:column;gap:14px;align-items:center;font-size:14px}
  .readouts b{font-size:16px}
  .ok{color:#7CFF7C}.warn{color:#FFD46A}
  .btn{display:inline-block;padding:6px 10px;border-radius:8px;background:rgba(255,255,255,.08);
       color:#cfe6ff;text-decoration:none;border:1px solid rgba(255,255,255,.12)}
  .btn:hover{background:rgba(255,255,255,.14)}
</style>
</head>
<body>
  <div class="wrap">
    <div class="panel">
      <div class="horizon" aria-label="Авиагоризонт">
        <!-- Динамический диск -->
        <div id="sphere" class="sphere">
          <div id="pitchLadder" class="pitch-ladder"></div>
        </div>

        <!-- КОМПАС (шкала по ободу) -->
        <div id="compass" class="compass">
          <div id="rollScale" class="roll-scale"></div>
        </div>

        <div class="bezel"></div>
        <div class="roll-pointer"></div>
        <div class="center-line"></div>
      </div>
    </div>

    <div class="panel readouts">
      <div>Roll:  <b id="rollVal">0.0</b>°</div>
      <div>Pitch: <b id="pitchVal">0.0</b>°</div>
      <div>Yaw:   <b id="yawVal">0.0</b>°</div>
      <div>IMU:   <b id="imuStatus" class="warn">—</b></div>
      <a href="/" class="btn">← Панель</a>
      <button id="zeroBtn" class="btn" type="button">Обнулить</button>
    </div>
  </div>

<script>
const PX_PER_DEG=parseFloat(getComputedStyle(document.documentElement).getPropertyValue('--px-per-deg'))||3.2;
const SMOOTH=0.18;

let sRoll=0,sPitch=0,sYaw=0;
let zeroRoll=0,zeroPitch=0;

const sphere=document.getElementById('sphere');
const pitchLadder=document.getElementById('pitchLadder');
const rollScale=document.getElementById('rollScale');
const compass=document.getElementById('compass');
const rollEl=document.getElementById('rollVal');
const pitchEl=document.getElementById('pitchVal');
const yawEl=document.getElementById('yawVal');
const imuStat=document.getElementById('imuStatus');
const zeroBtn=document.getElementById('zeroBtn');

zeroBtn.addEventListener('click',()=>{zeroRoll=sRoll;zeroPitch=sPitch;});

// Рисуем шкалу по кругу с цифрами и N/E/S/W
(function renderRollScale(){
  const size=parseFloat(getComputedStyle(rollScale).width);
  const rTick=size/2-12;
  const rText=rTick+22;
  for(let d=0; d<360; d+=10){
    const isMajor=(d%30===0);

    const tick=document.createElement('div');
    tick.className='roll-tick'+(isMajor?'':' small');
    tick.style.transform=`translate(-50%,-50%) rotate(${d}deg) translateY(${-rTick}px)`;
    rollScale.appendChild(tick);

    if(isMajor){
      const lbl=document.createElement('div');
      lbl.className='roll-label';
      let text=d.toString();
      if(d===0) text="N";
      else if(d===90) text="E";
      else if(d===180) text="S";
      else if(d===270) text="W";
      lbl.textContent=text;
      lbl.style.transform=`translate(-50%,-50%) rotate(${d}deg) translateY(${-rText}px) rotate(${-d}deg)`;
      rollScale.appendChild(lbl);
    }
  }
})();

// Лесенка тангажа
(function renderPitchLadder(){
  for(let d=-60;d<=60;d+=10){
    if(d===0) continue;
    const line=document.createElement('div');
    line.className='ladder-line';
    line.style.left='50%';
    const y=-d*PX_PER_DEG;
    line.style.top=`calc(50% + ${y}px)`;
    const tL=document.createElement('span');
    tL.className='text left'; tL.textContent=Math.abs(d);
    const tR=document.createElement('span');
    tR.className='text right'; tR.textContent=Math.abs(d);
    line.appendChild(tL); line.appendChild(tR);
    pitchLadder.appendChild(line);
  }
})();

function norm360(a){a=a%360;if(a<0)a+=360;return a;}

async function fetchIMU(){
  try{
    const r=await fetch('/imu',{cache:'no-store'});
    if(!r.ok) throw new Error('HTTP '+r.status);
    const data=await r.json();

    const roll_raw=Number(data.pitch??0);   // roll ← pitch
    const pitch_raw=Number(data.roll??0);   // pitch ← roll
    const yaw_raw=Number(data.yaw??0);
    const ts=Number(data.ts??0);

    sRoll = sRoll+SMOOTH*(roll_raw-sRoll);
    sPitch= sPitch+SMOOTH*(pitch_raw-sPitch);
    sYaw  = sYaw+SMOOTH*(yaw_raw-sYaw);

    const dispRoll=sRoll-zeroRoll;
    const dispPitch=sPitch-zeroPitch;

    const translateY=dispPitch*PX_PER_DEG;
    const rotateDeg=-dispRoll;
    sphere.style.transform=`translateY(${translateY}px) rotate(${rotateDeg}deg)`;

    const yawDeg=norm360(sYaw);
    // Шкала крутится по yaw (в ту же сторону, что и поворот)
    compass.style.transform=`rotate(${yawDeg}deg)`;

    rollEl.textContent=dispRoll.toFixed(1);
    pitchEl.textContent=dispPitch.toFixed(1);
    yawEl.textContent=yawDeg.toFixed(1);

    const fresh=(Date.now()/1000-ts)<1.0;
    imuStat.textContent=fresh?'OK':'STALE';
    imuStat.className=fresh?'ok':'warn';
  }catch(e){
    imuStat.textContent='NO DATA';
    imuStat.className='warn';
  }
}

setInterval(fetchIMU,50);
fetchIMU();
</script>
</body>
</html>
