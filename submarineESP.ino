<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1.0"/>
<title>Marine Dive ROV - Control Panel</title>
<style>
  :root{ --size: 280px; --px-per-deg: 2.2; }
  *{margin:0;padding:0;box-sizing:border-box}
  body{ font-family: system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif; color:#fff;
        min-height:100vh; background:#0c1e36 url("/static/bg.png") center/cover fixed no-repeat; }
  .container{ max-width:1200px;margin:0 auto;padding:20px; display:grid;gap:20px;grid-template-columns: 1fr 1fr; }
  .header{ grid-column:1/-1;background:rgba(0,0,0,.35); border:1px solid rgba(255,255,255,.15);border-radius:12px; padding:16px;text-align:center }
  .header h1{font-size:26px;margin-bottom:6px}
  .card{ background:rgba(0,0,0,.30);border:1px solid rgba(255,255,255,.14); border-radius:12px;padding:16px }
  h2{color:#8fd0ff;font-size:18px;margin-bottom:12px}
  .video-feed{width:100%;aspect-ratio:4/3;display:block;border-radius:8px;background:#000}
  .telemetry{display:grid;grid-template-columns: 1fr auto;gap:16px;align-items:start}
  .telemetry-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
  .telemetry-item{ background:rgba(255,255,255,.08);border:1px solid rgba(255,255,255,.12); border-radius:8px;padding:10px;text-align:center }
  .telemetry-item .label{opacity:.8;font-size:13px;margin-bottom:6px}
  .control-panel{display:grid;grid-template-columns: repeat(4, 1fr); gap:15px}
  .control-group{ background:rgba(255,255,255,.08);border:1px solid rgba(255,255,255,.12); border-radius:8px;padding:12px;text-align:center }
  .slider{width:100%}
  .value-display{margin-top:8px;opacity:.9}
  .button{background:#4CAF50;border:none;padding:10px 12px;border-radius:8px;color:#fff;cursor:pointer;margin-top:8px}
  /* horizon styles omitted for brevity (оставил как у тебя) */
  .horizon{width:var(--size);height:var(--size);border-radius:50%;position:relative;overflow:hidden;background:#111;box-shadow: inset 0 0 0 2px rgba(255,255,255,0.18), inset 0 0 24px rgba(0,0,0,0.6)}
  .sphere{position:absolute;inset:-240% -80%;background:linear-gradient(to bottom,#4aa3ff 0 50%,#7b4d1f 50% 100%),repeating-linear-gradient(to bottom,rgba(255,255,255,0.10) 0 1px,transparent 1px 12px);filter:saturate(1.05)}
  .bezel{position:absolute;inset:0;border-radius:50%;pointer-events:none;box-shadow:inset 0 0 0 2px rgba(255,255,255,.25), inset 0 0 40px rgba(0,0,0,.6)}
  .roll-pointer{position:absolute;inset:0;pointer-events:none}
  .roll-pointer::before{content:"";position:absolute;left:50%;top:6px;transform:translateX(-50%);width:0;height:0;border-left:7px solid transparent;border-right:7px solid transparent;border-bottom:10px solid #fff;opacity:.95}
  .center-line{position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);width:90px;height:2px;background:#fff;border-radius:1px}
  .compass{position:absolute;inset:0;pointer-events:none;transform-origin:50% 50%;}
  .roll-scale{position:absolute;inset:0;border-radius:50%;}
  .roll-tick{position:absolute;left:50%;top:50%;transform-origin:50% 50%;width:2px;height:9px;background:#fff;border-radius:2px;opacity:.9}
  .roll-tick.small{height:6px;opacity:.75}
  .roll-label{position:absolute;left:50%;top:50%;transform-origin:50% 50%;font-size:10px;opacity:.95;white-space:nowrap;background:rgba(0,0,0,.35);padding:1px 4px;border-radius:6px}
  .pitch-ladder{position:absolute;inset:0;pointer-events:none}
  .ladder-line{position:absolute;left:50%;width:60px;height:2px;background:#fff;opacity:.95;border-radius:1px}
  .ladder-line .text{position:absolute;top:-10px;left:-40px;font-size:10px;background:rgba(0,0,0,.4);padding:0 3px;border-radius:4px}
  @media (max-width: 980px){ .container{grid-template-columns:1fr} .telemetry{grid-template-columns: 1fr} .horizon{margin:0 auto} .control-panel{grid-template-columns: repeat(2, 1fr)} }
  .gamepad-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;text-align:left;font-size:14px}
  .muted{opacity:.75}
</style>
</head>
<body>
  <div class="container">
    <div class="header card">
      <h1>Marine Dive ROV — Control Panel</h1>
      <p>Статус: <span id="connection-status">Подключено</span></p>
    </div>

    <div class="card">
      <h2>Видеопоток</h2>
      <img src="/video_feed" class="video-feed" alt="Live Camera Feed">
    </div>

    <div class="card">
      <h2>Телеметрия</h2>
      <!-- оставлено без изменений -->
      <div class="telemetry">
        <div class="telemetry-grid">
          <div class="telemetry-item"><div class="label">Глубина</div><div class="value" id="depth-value">0.0 м</div></div>
          <div class="telemetry-item"><div class="label">Температура</div><div class="value" id="temp-value">20.5 °C</div></div>
          <div class="telemetry-item"><div class="label">Напряжение</div><div class="value" id="voltage-value">12.3 В</div></div>
          <div class="telemetry-item"><div class="label">Ток</div><div class="value" id="current-value">1.2 А</div></div>
          <div class="telemetry-item"><div class="label">Курс (Yaw)</div><div class="value" id="yaw-value">0.0°</div></div>
          <div class="telemetry-item"><div class="label">IMU</div><div class="value" id="imuStatus">—</div></div>
        </div>
        <div>
          <!-- авиагоризонт — как раньше -->
          <div class="horizon" aria-label="Авиагоризонт">
            <div id="sphere" class="sphere"><div id="pitchLadder" class="pitch-ladder"></div></div>
            <div id="compass" class="compass"><div id="rollScale" class="roll-scale"></div></div>
            <div class="bezel"></div><div class="roll-pointer"></div><div class="center-line"></div>
          </div>
          <div style="text-align:center;opacity:.9;margin-top:8px">
            Roll: <span id="rollVal">0.0</span>° &nbsp; | &nbsp; Pitch: <span id="pitchVal">0.0</span>°
          </div>
          <div style="text-align:center;margin-top:8px"><button id="zeroBtn" class="button" type="button">Обнулить горизонт</button></div>
        </div>
      </div>
    </div>

    <div class="card" style="grid-column:1/-1">
      <h2>Управление</h2>
      <div class="control-panel">
        <div class="control-group">
          <h3>Мотор</h3>
          <input type="range" min="-255" max="255" value="0" class="slider" id="motor-slider" oninput="updateMotor(this.value)">
          <div class="value-display">Мощность: <span id="motor-value">0</span></div>
        </div>

        <div class="control-group">
          <h3>Помпа 1</h3>
          <input type="range" min="-255" max="255" value="0" class="slider" id="pump1-slider" oninput="updatePump1(this.value)">
          <div class="value-display">Мощность: <span id="pump1-value">0</span></div>
        </div>

        <div class="control-group">
          <h3>Помпа 2</h3>
          <input type="range" min="-255" max="255" value="0" class="slider" id="pump2-slider" oninput="updatePump2(this.value)">
          <div class="value-display">Мощность: <span id="pump2-value">0</span></div>
        </div>

        <div class="control-group">
          <h3>Освещение</h3>
          <button class="button" id="btnLight" type="button" onclick="toggleLight()">Свет: <span id="light-status">OFF</span></button>
        </div>

        <!-- новые -->
        <div class="control-group">
          <h3>Помпа 3</h3>
          <input type="range" min="-255" max="255" value="0" class="slider" id="pump3-slider" oninput="updatePump3(this.value)">
          <div class="value-display">Мощность: <span id="pump3-value">0</span></div>
        </div>

        <div class="control-group">
          <h3>Помпа 4</h3>
          <input type="range" min="-255" max="255" value="0" class="slider" id="pump4-slider" oninput="updatePump4(this.value)">
          <div class="value-display">Мощность: <span id="pump4-value">0</span></div>
        </div>

        <!-- Геймпад (как было) -->
        <div class="control-group" style="grid-column:1/-1">
          <h3>Геймпад</h3>
          <div class="gamepad-grid">
            <div>
              <div>Состояние: <span id="gp-state" class="muted">не подключен</span></div>
              <div class="muted" id="gp-name"></div>
              <div style="margin-top:8px"><label><input type="checkbox" id="gp-enable"> Включить управление геймпадом</label></div>
              <div style="margin-top:8px"><label>Мёртвая зона: <input id="gp-deadzone" type="range" min="0" max="0.3" step="0.01" value="0.08" style="width:160px"></label></div>
            </div>
            <div class="muted">
              <b>Раскладка (по умолчанию):</b><br/>
              LS ↑/↓ — тяга (M); LS ←/→ — руль (R)<br/>
              RS ↑/↓ — помпа 2 (P2); LT/RT — помпа 1 (P1)<br/>
              A — свет. (P3/P4 слайдерами)
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>

<script>
/* ===== Управление (слайдеры/кнопка) ===== */
let lightState=false;
function sendControl(payload){
  return fetch('/control',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)}).catch(()=>{});
}
function updateMotor(v){ v=parseInt(v); document.getElementById('motor-value').textContent=v; sendControl({M:v}); }
function updatePump1(v){ v=parseInt(v); document.getElementById('pump1-value').textContent=v; sendControl({P1:v}); }
function updatePump2(v){ v=parseInt(v); document.getElementById('pump2-value').textContent=v; sendControl({P2:v}); }
function updatePump3(v){ v=parseInt(v); document.getElementById('pump3-value').textContent=v; sendControl({P3:v}); }
function updatePump4(v){ v=parseInt(v); document.getElementById('pump4-value').textContent=v; sendControl({P4:v}); }
function toggleLight(){
  lightState=!lightState;
  document.getElementById('light-status').textContent=lightState?'ON':'OFF';
  sendControl({L: lightState ? 1 : 0});
  lastSent.L = lightState ? 1 : 0;
}

/* ===== Телеметрия ===== */
function updateTelemetry(){
  fetch('/telemetry').then(r=>r.json()).then(d=>{
    document.getElementById('depth-value').textContent   = d.depth.toFixed(1)+' м';
    document.getElementById('temp-value').textContent    = d.temperature.toFixed(1)+' °C';
    document.getElementById('voltage-value').textContent = d.voltage.toFixed(1)+' В';
    document.getElementById('current-value').textContent = d.current.toFixed(1)+' А';
  }).catch(()=>{});
}
setInterval(updateTelemetry,2000); updateTelemetry();

/* ===== Авиагоризонт (как было) ===== */
const PX_PER_DEG=2.2, SMOOTH=0.18;
let sRoll=0,sPitch=0,sYaw=0,zeroRoll=0,zeroPitch=0;
const sphere=document.getElementById('sphere'), compass=document.getElementById('compass');
const rollScale=document.getElementById('rollScale'), pitchLadder=document.getElementById('pitchLadder');
const rollEl=document.getElementById('rollVal'), pitchEl=document.getElementById('pitchVal'), yawOut=document.getElementById('yaw-value');
const imuStat=document.getElementById('imuStatus'), zeroBtn=document.getElementById('zeroBtn');
zeroBtn.addEventListener('click',()=>{zeroRoll=sRoll;zeroPitch=sPitch;});
(function(){const size=parseFloat(getComputedStyle(rollScale).width);const rTick=size/2-10;const rText=rTick+18;for(let d=0;d<360;d+=10){const isMajor=(d%30===0);const t=document.createElement('div');t.className='roll-tick'+(isMajor?'':' small');t.style.transform=`translate(-50%,-50%) rotate(${d}deg) translateY(${-rTick}px)`;rollScale.appendChild(t);if(isMajor){const l=document.createElement('div');l.className='roll-label';let text=d.toString();if(d===0)text='N';else if(d===90)text='E';else if(d===180)text='S';else if(d===270)text='W';l.textContent=text;l.style.transform=`translate(-50%,-50%) rotate(${d}deg) translateY(${-rText}px) rotate(${-d}deg)`;rollScale.appendChild(l);}}})();
(function(){for(let d=-60;d<=60;d+=10){if(d===0)continue;const line=document.createElement('div');line.className='ladder-line';const y=-d*PX_PER_DEG;line.style.top=`calc(50% + ${y}px)`;const tL=document.createElement('span');tL.className='text';tL.textContent=Math.abs(d);line.appendChild(tL);pitchLadder.appendChild(line);}})();
function norm360(a){a=a%360;if(a<0)a+=360;return a;}
async function fetchIMU(){try{const r=await fetch('/imu',{cache:'no-store'});if(!r.ok)throw 0;const d=await r.json();const rr=Number(d.pitch||0), pr=Number(d.roll||0), yr=Number(d.yaw||0), ts=Number(d.ts||0);sRoll=sRoll+SMOOTH*(rr-sRoll);sPitch=sPitch+SMOOTH*(pr-sPitch);sYaw=sYaw+SMOOTH*(yr-sYaw);const dispR=sRoll-zeroRoll, dispP=sPitch-zeroPitch;const trY=Math.max(-60,Math.min(60,dispP))*PX_PER_DEG; sphere.style.transform=`translateY(${trY}px) rotate(${-dispR}deg)`; const yaw=norm360(sYaw); compass.style.transform=`rotate(${yaw}deg)`; rollEl.textContent=dispR.toFixed(1); pitchEl.textContent=dispP.toFixed(1); yawOut.textContent=yaw.toFixed(1)+'°'; const fresh=(Date.now()/1000-ts)<1.0; imuStat.textContent=fresh?'OK':'STALE'; imuStat.style.color=fresh?'#7CFF7C':'#FFD46A';}catch(e){imuStat.textContent='NO DATA';imuStat.style.color='#FFD46A';}}
setInterval(fetchIMU,50); fetchIMU();

/* ===== Gamepad (P3/P4 не маппим — управляются слайдерами) ===== */
const gpStateEl=document.getElementById('gp-state'), gpNameEl=document.getElementById('gp-name');
const gpEnable=document.getElementById('gp-enable'), gpDeadzone=document.getElementById('gp-deadzone');
let gpIndex=null, gpLightLatched=false, lastSent={M:0,P1:0,P2:0,P3:0,P4:0,R:0,L:0}, lastSendTs=0;
const SEND_HZ=20, HEARTBEAT_MS=700, INCLUDE_L_IN_HEARTBEAT=true;
window.addEventListener('gamepadconnected',e=>{gpIndex=e.gamepad.index;gpStateEl.textContent='подключен';gpNameEl.textContent=e.gamepad.id||'';});
window.addEventListener('gamepaddisconnected',()=>{gpIndex=null;gpStateEl.textContent='не подключен';gpNameEl.textContent='';});
function clamp(n,min,max){return Math.min(max,Math.max(min,n));}
function applyDeadzoneExpo(v,dz,expo=0.2){const s=Math.sign(v);v=Math.abs(v);if(v<dz)return 0;const t=(v-dz)/(1-dz);return s*t**(1+expo);}
function getPads(){const p=navigator.getGamepads?navigator.getGamepads():[];return p||[];}
function getTriggerValue(gp,axisIdx,btnIdx){let v=0;if(gp.axes&&gp.axes.length>axisIdx&&gp.axes[axisIdx]!=null){v=(gp.axes[axisIdx]+1)/2;}else if(gp.buttons&&gp.buttons.length>btnIdx&&gp.buttons[btnIdx]){v=gp.buttons[btnIdx].value||(gp.buttons[btnIdx].pressed?1:0);}return clamp(v,0,1);}
function tick(){const now=performance.now(); if(gpIndex!=null && gpEnable.checked){const pads=getPads();const gp=pads[gpIndex]; if(gp){const dz=parseFloat(gpDeadzone.value||'0.08'); const axLX=applyDeadzoneExpo(gp.axes[0]||0,dz,0.25); const axLY=applyDeadzoneExpo(gp.axes[1]||0,dz,0.15); const axRY=applyDeadzoneExpo(gp.axes[3]||0,dz,0.20); const lt=getTriggerValue(gp,2,6), rt=getTriggerValue(gp,5,7);
  const M=Math.round(clamp(-axLY,-1,1)*255), R=Math.round(clamp(axLX,-1,1)*100), P2=Math.round(clamp(-axRY,-1,1)*255), P1=Math.round(clamp(rt-lt,-1,1)*255);
  const aPressed=gp.buttons&&gp.buttons[0]&&gp.buttons[0].pressed;
  if(aPressed&&!gpLightLatched){lightState=!lightState;document.getElementById('light-status').textContent=lightState?'ON':'OFF';sendControl({L: lightState?1:0});gpLightLatched=true;lastSent.L=lightState?1:0;} else if(!aPressed){gpLightLatched=false;}
  const changed=(Math.abs(M-lastSent.M)>1)||(Math.abs(P1-lastSent.P1)>1)||(Math.abs(P2-lastSent.P2)>1)||(Math.abs(R-lastSent.R)>1);
  const due=(now-lastSendTs)>=(1000/SEND_HZ), hb=(now-lastSendTs)>=HEARTBEAT_MS;
  if((changed&&due)||hb){const payload={M,P1,P2,R}; if(INCLUDE_L_IN_HEARTBEAT||hb) payload.L=lightState?1:0; sendControl(payload); lastSent={...lastSent,M,P1,P2,R,L:lightState?1:0}; lastSendTs=now;
    document.getElementById('motor-slider').value=M;document.getElementById('motor-value').textContent=M;
    document.getElementById('pump1-slider').value=P1;document.getElementById('pump1-value').textContent=P1;
    document.getElementById('pump2-slider').value=P2;document.getElementById('pump2-value').textContent=P2;
  }}} requestAnimationFrame(tick);}
gpStateEl.textContent=(getPads().some(p=>p))?'подключен':'не подключен'; requestAnimationFrame(tick);
</script>
</body>
</html>
