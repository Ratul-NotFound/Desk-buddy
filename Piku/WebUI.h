#pragma once
#include <Arduino.h>

const char WEBUI_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>PIKU AI — Desk Companion</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
body{background:#111;color:#eee;font-family:system-ui,-apple-system,sans-serif;padding:10px;min-height:100vh}
:root{--a:#00d4aa;--b:#111;--c:#1a1a1a;--d:#2a2a2a;--e:#888}
.wrap{max-width:480px;margin:0 auto;padding-bottom:24px}
h1{font-size:18px;color:var(--a);letter-spacing:.5px;text-align:center;padding:10px 0 6px}
.pill{display:inline-block;font-size:11px;padding:3px 10px;border-radius:20px;border:1px solid var(--e);color:var(--e)}
.pill.on{border-color:var(--a);color:var(--a)}
.status-row{display:flex;justify-content:space-between;align-items:center;background:var(--c);border-radius:10px;padding:10px 12px;margin-bottom:10px;font-size:13px}
.tabs{display:flex;gap:4px;margin-bottom:10px}
.tab{flex:1;background:var(--c);border:none;color:var(--e);padding:8px 4px;font-size:12px;border-radius:8px;cursor:pointer}
.tab.active{background:var(--a);color:#111;font-weight:700}
.pane{display:none}.pane.active{display:block}
.card{background:var(--c);border-radius:10px;padding:12px;margin-bottom:10px}
.label{font-size:11px;color:var(--e);margin-bottom:4px}
.vbar{background:var(--d);border-radius:4px;height:7px;margin-top:4px;overflow:hidden}
.vfill{height:100%;background:var(--a);transition:width .4s}
.vrow{display:flex;justify-content:space-between;font-size:12px;margin-bottom:6px}
.chat{height:150px;overflow-y:auto;background:var(--d);border-radius:8px;padding:8px;margin-bottom:8px;font-size:12px;display:flex;flex-direction:column;gap:6px}
.bu{padding:7px 10px;border-radius:10px;max-width:85%;line-height:1.4}
.bu.u{background:#004433;align-self:flex-end}
.bu.a{background:#1a1a2e;align-self:flex-start}
.row{display:flex;gap:6px}
input,textarea,select{flex:1;background:var(--d);border:1px solid #333;color:#eee;padding:9px 10px;border-radius:8px;font-size:12px;outline:none;width:100%}
input:focus,textarea:focus,select:focus{border-color:var(--a)}
.btn{background:var(--d);border:1px solid #333;color:#eee;padding:9px 10px;border-radius:8px;font-size:12px;cursor:pointer;display:flex;flex-direction:column;align-items:center;gap:3px;text-align:center}
.btn:active{background:var(--a);color:#111;border-color:var(--a)}
.btn.p{background:var(--a);color:#111;border:none;font-weight:700}
.btn.s{background:#4a1540;border-color:#993388;color:#ee88cc}
.g4{display:grid;grid-template-columns:repeat(4,1fr);gap:6px}
.g3{display:grid;grid-template-columns:repeat(3,1fr);gap:6px}
.slider{width:100%;accent-color:var(--a);margin:8px 0}
.toggle{display:flex;justify-content:space-between;align-items:center;font-size:12px;padding:6px 0}
.sw{position:relative;display:inline-block;width:36px;height:20px}
.sw input{opacity:0;width:0;height:0}
.sw span{position:absolute;cursor:pointer;inset:0;background:#333;border-radius:20px;transition:.3s}
.sw span:before{position:absolute;content:"";width:14px;height:14px;left:3px;top:3px;background:#eee;border-radius:50%;transition:.3s}
.sw input:checked+span{background:var(--a)}
.sw input:checked+span:before{transform:translateX(16px)}
.ai-resp{font-size:12px;color:#ccc;background:var(--d);border-radius:8px;padding:10px;min-height:40px;margin-bottom:8px;line-height:1.5}
.chips{display:flex;gap:4px;flex-wrap:wrap;margin-top:6px}
.chip{font-size:10px;padding:4px 8px;background:var(--d);border:1px solid #333;border-radius:16px;cursor:pointer;white-space:nowrap}
.chip:active{background:var(--a);color:#111;border-color:var(--a)}
.emo-ico{font-size:18px}
#onboarding-box{display:none;background:#00382e;border:1px solid var(--a);border-radius:10px;padding:12px;margin-bottom:10px}
</style>
</head>
<body>
<div class="wrap">
<h1>🤖 PIKU AI</h1>

<div id="onboarding-box">
  <div style="font-weight:700;color:var(--a);margin-bottom:4px">👋 Welcome! Meet your new AI buddy.</div>
  <div style="font-size:12px;color:#ccc;margin-bottom:8px">What should Piku call you?</div>
  <div class="row">
    <input type="text" id="onboard-inp" placeholder="Enter your name...">
    <button class="btn p" onclick="submitOnboarding()" style="padding:9px 14px">Set Name</button>
  </div>
</div>

<div class="status-row">
  <div>
    <span id="cpill" class="pill">AP 192.168.4.1</span>
    <div id="owner-name" style="font-size:11px;color:#888;margin-top:4px"></div>
  </div>
  <div style="text-align:right;font-size:12px">
    <div id="clk" style="color:var(--a);font-weight:700">--:--</div>
    <div id="wth" style="color:#888;font-size:11px">--°C</div>
  </div>
</div>

<div class="tabs">
  <button class="tab active" onclick="tab(0)">Status</button>
  <button class="tab" onclick="tab(1)">Chat</button>
  <button class="tab" onclick="tab(2)">Controls</button>
  <button class="tab" onclick="tab(3)">Settings</button>
</div>

<!-- STATUS TAB -->
<div class="pane active" id="p0">
  <div class="card">
    <div class="label">VITALS & METABOLISM</div>
    <div class="vrow"><span>❤️ Affection</span><span id="sa">85%</span></div>
    <div class="vbar"><div class="vfill" id="ba" style="width:85%"></div></div>
    <div class="vrow" style="margin-top:8px"><span>⚡ Energy</span><span id="se">100%</span></div>
    <div class="vbar"><div class="vfill" id="be" style="width:100%;background:#fbbf24"></div></div>
    <div class="vrow" style="margin-top:8px"><span>🍕 Hunger</span><span id="sh">90%</span></div>
    <div class="vbar"><div class="vfill" id="bh" style="width:90%;background:#f97316"></div></div>
  </div>
  <div class="card">
    <div class="label">LAST AI RESPONSE</div>
    <div class="ai-resp" id="last-resp">Hi! I am Piku. Ask me anything!</div>
  </div>
  <div class="card">
    <div class="label">SPONTANEOUS TALK INTERVAL</div>
    <select id="auto-talk-sel" onchange="setAutoTalk(this.value)" style="margin-top:6px">
      <option value="0">Off (Silent unless spoken to)</option>
      <option value="5">Every 5 minutes</option>
      <option value="10" selected>Every 10 minutes</option>
      <option value="20">Every 20 minutes</option>
      <option value="30">Every 30 minutes</option>
    </select>
  </div>
</div>

<!-- CHAT TAB -->
<div class="pane" id="p1">
  <div class="card">
    <div class="label" style="display:flex;justify-content:space-between">
      <span>CHAT WITH PIKU</span>
      <span id="ai-badge" style="font-size:11px;color:#888">● Ready</span>
    </div>
    <div class="chat" id="clog">
      <div class="bu a">🤖 Hi! I'm Piku! Ask me anything or tap a quick question below.</div>
    </div>
    <div class="row">
      <input type="text" id="ainp" placeholder="Ask Piku..." onkeydown="if(event.key==='Enter')sendAI()">
      <button class="btn" id="mic-btn" onclick="toggleMic()" style="min-width:40px;padding:9px">🎙️</button>
      <button class="btn p" onclick="sendAI()" style="padding:9px 14px">Ask</button>
    </div>
    <div class="chips">
      <div class="chip" onclick="chip('Who are you?')">🤖 Who are you?</div>
      <div class="chip" onclick="chip('What is the weather like?')">🌤️ Weather?</div>
      <div class="chip" onclick="chip('Tell me a short witty joke!')">😂 Joke</div>
      <div class="chip" onclick="chip('How are you feeling right now?')">❤️ Feeling?</div>
      <div class="chip" onclick="chip('Share a fun fact with me!')">💡 Fun Fact</div>
    </div>
    <div class="toggle" style="margin-top:8px;border-top:1px solid #222;padding-top:8px">
      <span style="font-size:11px;color:#888">🔊 Speak replies in browser</span>
      <label class="sw"><input type="checkbox" id="tts-tog" checked><span></span></label>
    </div>
  </div>
</div>

<!-- CONTROLS TAB -->
<div class="pane" id="p2">
  <div class="card">
    <div class="label" style="display:flex;justify-content:space-between">
      <span>VOLUME</span><span id="vol-lbl">80%</span>
    </div>
    <input type="range" min="0" max="100" value="80" class="slider" id="vol-sl" oninput="setVol(this.value)">
    <div class="g4" style="margin-top:4px">
      <button class="btn" onclick="setVol(0)">🔇</button>
      <button class="btn" onclick="setVol(40)">40%</button>
      <button class="btn" onclick="setVol(80)">80%</button>
      <button class="btn" onclick="setVol(100)">📢</button>
    </div>
  </div>
  <div class="card">
    <div class="label">EXPRESSIONS & EMOTIONS</div>
    <div class="g4" style="margin-top:6px">
      <button class="btn" onclick="cmd('hello')"><div class="emo-ico">👋</div><div>Hello</div></button>
      <button class="btn" onclick="cmd('love')"><div class="emo-ico">❤️</div><div>Love</div></button>
      <button class="btn" onclick="cmd('party')"><div class="emo-ico">🎉</div><div>Party</div></button>
      <button class="btn" onclick="cmd('shades')"><div class="emo-ico">😎</div><div>Cool</div></button>
      <button class="btn" onclick="cmd('cat')"><div class="emo-ico">🐱</div><div>Cat</div></button>
      <button class="btn" onclick="cmd('kiss')"><div class="emo-ico">😘</div><div>Kiss</div></button>
      <button class="btn" onclick="cmd('fire')"><div class="emo-ico">🔥</div><div>Fire</div></button>
      <button class="btn" onclick="cmd('matrix')"><div class="emo-ico">💻</div><div>Hacker</div></button>
      <button class="btn" onclick="cmd('pacman')"><div class="emo-ico">👾</div><div>Pacman</div></button>
      <button class="btn" onclick="cmd('money')"><div class="emo-ico">💰</div><div>Money</div></button>
      <button class="btn" onclick="cmd('dizzy')"><div class="emo-ico">😵</div><div>Dizzy</div></button>
      <button class="btn" onclick="cmd('sad')"><div class="emo-ico">😢</div><div>Sad</div></button>
      <button class="btn" onclick="cmd('uhoh')"><div class="emo-ico">⚠️</div><div>UhOh</div></button>
      <button class="btn" onclick="cmd('tada')"><div class="emo-ico">✨</div><div>Tada</div></button>
      <button class="btn" onclick="cmd('clock')"><div class="emo-ico">🕒</div><div>Clock</div></button>
      <button class="btn" onclick="cmd('weather')"><div class="emo-ico">🌤️</div><div>Weather</div></button>
      <button class="btn" onclick="cmd('study')"><div class="emo-ico">👓</div><div>Study</div></button>
      <button class="btn" onclick="cmd('sleep')"><div class="emo-ico">💤</div><div>Sleep</div></button>
    </div>
  </div>
  <div class="card">
    <div class="label">MINI-GAMES & SPECIAL MODES</div>
    <div class="g3" style="margin-top:6px">
      <button class="btn p" onclick="cmd('flap_start')"><div class="emo-ico">🐤</div><div>Flappy</div></button>
      <button class="btn" onclick="cmd('flap_jump')"><div class="emo-ico">⬆️</div><div>Jump</div></button>
      <button class="btn" onclick="cmd('rps')"><div class="emo-ico">✂️</div><div>RPS</div></button>
      <button class="btn" onclick="cmd('8ball')"><div class="emo-ico">🎱</div><div>8-Ball</div></button>
      <button class="btn" onclick="cmd('snack')"><div class="emo-ico">🍕</div><div>Feed</div></button>
      <button class="btn s" onclick="cmd('sentry')"><div class="emo-ico">🚨</div><div>Sentry</div></button>
    </div>
  </div>
  <div class="card">
    <div class="label" style="display:flex;justify-content:space-between"><span>HEAD SERVO MOTION</span><span id="srv-lbl">90°</span></div>
    <input type="range" min="40" max="140" value="90" class="slider" id="srv-sl" oninput="steer(this.value)">
    <div class="g3" style="margin-top:4px">
      <button class="btn" onclick="steer(60)">◀ Left</button>
      <button class="btn" onclick="steer(90)">Center</button>
      <button class="btn" onclick="steer(120)">Right ▶</button>
    </div>
    <div class="g3" style="margin-top:6px">
      <button class="btn" onclick="cmd('nod')">Nod</button>
      <button class="btn" onclick="cmd('shake')">Shake</button>
      <button class="btn" onclick="cmd('wiggle')">Wiggle</button>
    </div>
  </div>
  <div class="card">
    <div class="label">OLED BILLBOARD</div>
    <div class="row" style="margin-top:6px">
      <input type="text" id="bill-inp" placeholder="Message to scroll on OLED...">
      <button class="btn p" onclick="sendBill()" style="padding:9px 14px">Send</button>
    </div>
  </div>
</div>

<!-- SETTINGS TAB -->
<div class="pane" id="p3">
  <div class="card">
    <div class="label">OWNER PROFILE & MEMORY</div>
    <div style="font-size:12px;color:#888;margin:6px 0">Current Owner: <span id="own-name-lbl" style="color:var(--a);font-weight:700">--</span></div>
    <div class="row" style="margin-bottom:8px">
      <input type="text" id="name-inp" placeholder="Update your name...">
      <button class="btn p" onclick="saveName()" style="padding:9px 14px">Save</button>
    </div>
    <button class="btn" onclick="resetOwner()" style="font-size:11px;padding:6px 12px;width:100%">🗑️ Reset Profile & Clear Memory</button>
  </div>
  <div class="card">
    <div class="label">GEMINI API KEYS <span id="key-badge" style="color:#888;font-size:11px">(0 active)</span></div>
    <textarea id="key-inp" rows="3" placeholder="AIzaSy...&#10;AIzaSy... (one per line or comma-separated)" style="margin-top:6px"></textarea>
    <button class="btn p" onclick="saveKeys()" style="width:100%;margin-top:8px">Save Key Pool</button>
  </div>
  <div class="card">
    <div class="label">SOUND SENSOR (CLAP)</div>
    <div class="toggle">
      <span style="color:#888;font-size:12px">Enable double-clap DJ mode</span>
      <label class="sw"><input type="checkbox" id="mic-tog" onchange="setMic(this.checked)"><span></span></label>
    </div>
    <div style="font-size:10px;color:#555;margin-top:4px">Keep off in noisy environments.</div>
  </div>
  <div class="card">
    <div class="label">TIMEZONE & LOCATION</div>
    <select id="tz-sel" style="margin:6px 0">
      <option value="-8">UTC-8 (California)</option>
      <option value="-5">UTC-5 (New York)</option>
      <option value="0">UTC+0 (London)</option>
      <option value="1">UTC+1 (Paris)</option>
      <option value="5.5">UTC+5:30 (India)</option>
      <option value="6" selected>UTC+6 (Dhaka)</option>
      <option value="8">UTC+8 (Singapore)</option>
      <option value="9">UTC+9 (Tokyo)</option>
    </select>
    <div class="row" style="margin-bottom:8px">
      <input type="text" id="lat-inp" value="23.8103" placeholder="Latitude">
      <input type="text" id="lon-inp" value="90.4125" placeholder="Longitude">
    </div>
    <button class="btn p" onclick="saveLoc()" style="width:100%">Save & Resync</button>
  </div>
  <div class="card">
    <div class="label">WIFI ROUTER</div>
    <button class="btn" onclick="scanWifi()" style="width:100%;margin-bottom:8px">🔍 Scan Networks</button>
    <select id="wifi-sel" style="margin-bottom:8px"><option value="">-- Scan first --</option></select>
    <input type="password" id="wifi-pass" placeholder="WiFi Password" style="margin-bottom:8px">
    <button class="btn p" onclick="saveWifi()" style="width:100%">Connect & Save</button>
  </div>
</div>
</div>

<script>
function tab(i){
  document.querySelectorAll('.tab').forEach((t,j)=>t.classList.toggle('active',i===j));
  document.querySelectorAll('.pane').forEach((p,j)=>p.classList.toggle('active',i===j));
}
function cmd(c){fetch('/api?cmd='+c);}
function steer(a){
  document.getElementById('srv-sl').value=a;
  document.getElementById('srv-lbl').textContent=a+'°';
  fetch('/api?steer='+a);
}
function setVol(v){
  document.getElementById('vol-sl').value=v;
  document.getElementById('vol-lbl').textContent=v+'%';
  fetch('/api?vol='+v);
}
function setMic(e){fetch('/api?mic_en='+(e?1:0));}
function setAutoTalk(m){fetch('/api?autotalk='+m);}

function speakBrowser(t){
  if(!('speechSynthesis' in window)) return;
  if(!document.getElementById('tts-tog').checked) return;
  speechSynthesis.cancel();
  let clean = t.replace(/\[[A-Za-z0-9_:]+\]/g,'').replace(/\[REMEMBER:.*?\]/g,'').trim();
  if(!clean) return;
  const u=new SpeechSynthesisUtterance(clean);
  u.lang='en-US';
  u.rate=1.02;
  u.pitch=1.25;
  const voices=speechSynthesis.getVoices();
  const v=voices.find(x=>x.lang.startsWith('en')&&(x.name.includes('Natural')||x.name.includes('Female')||x.name.includes('Google')||x.name.includes('Zira')||x.name.includes('Samantha')));
  if(v) u.voice=v;
  speechSynthesis.speak(u);
}
if('speechSynthesis' in window){speechSynthesis.onvoiceschanged=()=>speechSynthesis.getVoices();}

function addChat(text,type){
  const c=document.getElementById('clog');
  const d=document.createElement('div');
  d.className='bu '+(type==='user'?'u':'a');
  d.textContent=(type==='user'?'👤 ':'')+text;
  c.appendChild(d);c.scrollTop=c.scrollHeight;
  return d;
}
function chip(q){document.getElementById('ainp').value=q;sendAI();}
function sendAI(){
  const inp=document.getElementById('ainp');
  const q=inp.value.trim();if(!q)return;
  addChat(q,'user');inp.value='';
  const thinking=addChat('🤖 Thinking...','ai');
  document.getElementById('ai-badge').textContent='● Thinking...';
  fetch('/api/gemini/ask',{method:'POST',headers:{'Content-Type':'text/plain'},body:q})
  .then(r=>r.text()).then(ans=>{
    thinking.textContent='🤖 '+ans;
    document.getElementById('ai-badge').textContent='● Ready';
    document.getElementById('last-resp').textContent=ans;
    document.getElementById('clog').scrollTop=99999;
    speakBrowser(ans);
  }).catch(()=>{
    thinking.textContent='⚠️ Could not reach Piku.';
    document.getElementById('ai-badge').textContent='● Offline';
  });
}

let recog=null;
function toggleMic(){
  if(!('webkitSpeechRecognition' in window)&&!('SpeechRecognition' in window)){
    alert('Voice input requires Chrome or Edge.');return;
  }
  const btn=document.getElementById('mic-btn');
  if(!recog){
    const SR=window.SpeechRecognition||window.webkitSpeechRecognition;
    recog=new SR();recog.continuous=false;recog.interimResults=false;
    recog.onstart=()=>btn.textContent='🔴';
    recog.onresult=e=>{document.getElementById('ainp').value=e.results[0][0].transcript;btn.textContent='🎙️';sendAI();};
    recog.onerror=recog.onend=()=>btn.textContent='🎙️';
  }
  recog.start();
}

function submitOnboarding(){
  const n=document.getElementById('onboard-inp').value.trim();
  if(!n){alert('Please enter your name!');return;}
  fetch('/api/owner/name',{method:'POST',body:n}).then(()=>{
    document.getElementById('onboarding-box').style.display='none';
    alert('Nice to meet you, '+n+'!');
  });
}

function saveName(){
  const n=document.getElementById('name-inp').value.trim();
  if(!n){alert('Please enter your name!');return;}
  fetch('/api/owner/name',{method:'POST',body:n}).then(()=>alert('Owner name updated!'));
}

function scanWifi(){
  const sel=document.getElementById('wifi-sel');
  sel.innerHTML='<option>Scanning...</option>';
  fetch('/api/wifi/scan',{method:'POST'}).then(r=>r.json()).then(nets=>{
    sel.innerHTML='';
    if(!nets.length){sel.innerHTML='<option>No networks found</option>';return;}
    nets.forEach(n=>{const o=document.createElement('option');o.value=n.ssid;o.textContent=n.ssid+' ('+n.rssi+'dBm)';sel.appendChild(o);});
  }).catch(()=>sel.innerHTML='<option>Scan failed</option>');
}
function saveWifi(){
  const s=document.getElementById('wifi-sel').value;
  const p=document.getElementById('wifi-pass').value;
  if(!s){alert('Select a network first!');return;}
  fetch('/api/wifi/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:s,pass:p})})
  .then(()=>alert('Connecting to '+s+'...'));
}
function saveKeys(){
  const k=document.getElementById('key-inp').value.trim();
  if(!k){alert('Enter at least one key!');return;}
  fetch('/api/gemini/key',{method:'POST',body:k}).then(()=>alert('Key pool saved!'));
}
function saveLoc(){
  const tz=document.getElementById('tz-sel').value;
  const lat=document.getElementById('lat-inp').value;
  const lon=document.getElementById('lon-inp').value;
  fetch('/api/settings/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({tz:parseFloat(tz),lat:parseFloat(lat),lon:parseFloat(lon)})})
  .then(()=>{alert('Saved! Resyncing...');fetch('/api/sync',{method:'POST'});});
}
function sendBill(){
  const m=document.getElementById('bill-inp').value.trim();
  if(!m)return;
  fetch('/api/billboard',{method:'POST',body:m}).then(()=>alert('Sent!'));
}
function resetOwner(){
  if(confirm('Clear owner profile and memory?'))
    fetch('/api/owner/reset',{method:'POST'}).then(()=>alert('Profile reset! Reboot Piku.'));
}

function poll(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    // Onboarding box
    if(!d.onboarding_done && (!d.owner_name || d.owner_name==='Friend')){
      document.getElementById('onboarding-box').style.display='block';
    } else {
      document.getElementById('onboarding-box').style.display='none';
    }
    // Vitals
    ['aff','eng','hng'].forEach((k,i)=>{
      const vals=[d.affection,d.energy,d.hunger];
      document.getElementById('s'+k[0]).textContent=vals[i]+'%';
      document.getElementById('b'+k[0]).style.width=vals[i]+'%';
    });
    // Sliders (skip if focused)
    const vs=document.getElementById('vol-sl');
    if(document.activeElement!==vs){vs.value=d.volume;document.getElementById('vol-lbl').textContent=(d.is_muted?'Muted':d.volume+'%');}
    const ss=document.getElementById('srv-sl');
    if(document.activeElement!==ss){ss.value=d.servo_angle;document.getElementById('srv-lbl').textContent=d.servo_angle+'°';}
    // Clock + weather
    document.getElementById('clk').textContent=d.time_str;
    document.getElementById('wth').textContent=d.temp_c+'°C '+d.weather;
    // Connection pill
    const pill=document.getElementById('cpill');
    if(d.sta_connected){pill.textContent='🟢 '+d.sta_ip;pill.className='pill on';}
    else{pill.textContent='AP 192.168.4.1';pill.className='pill';}
    // Mic toggle
    const mt=document.getElementById('mic-tog');if(document.activeElement!==mt)mt.checked=d.mic_enabled;
    // Key badge
    document.getElementById('key-badge').textContent='('+d.key_count+' active)';
    // Owner name
    document.getElementById('own-name-lbl').textContent=d.owner_name||'--';
    document.getElementById('owner-name').textContent=d.owner_name?('Hi, '+d.owner_name+'!'):'';
    // Auto-talk sel
    const at=document.getElementById('auto-talk-sel');
    if(document.activeElement!==at&&d.auto_talk_min!==undefined)at.value=d.auto_talk_min;
    // Last AI resp
    if(d.last_reply&&d.last_reply.length>2)document.getElementById('last-resp').textContent=d.last_reply;
  }).catch(()=>{
    document.getElementById('cpill').textContent='🔴 Disconnected';
    document.getElementById('cpill').className='pill';
  });
}
setInterval(poll,2000);poll();
</script>
</body>
</html>
)rawliteral";
