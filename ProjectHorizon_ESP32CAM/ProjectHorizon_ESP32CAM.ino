#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "esp_camera.h"
#include "img_converters.h"

// ============================================================
// CONFIG - CHANGE THESE
// ============================================================
const char* ssid     = "";
const char* password = "";

// Static IP - must match your hotspot subnet
IPAddress local_IP(10, 141, 90, 184);
IPAddress gateway  (10, 141, 90, 1);
IPAddress subnet   (255, 255, 255, 0);

// ============================================================
// CAMERA PINS - AI Thinker ESP32-CAM
// ============================================================
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM   0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27
#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM     5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

WebServer server(80);

// ============================================================
// HTML PAGE
// ============================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>PROJECT HORIZON // DRONE-01</title>
<style>
* { margin:0; padding:0; box-sizing:border-box; }
body {
  background:#000;
  color:#00ff41;
  font-family:'Courier New',monospace;
  overflow:hidden;
  height:100vh; width:100vw;
  user-select:none;
}
body::after {
  content:'';
  position:fixed; top:0; left:0; right:0; bottom:0;
  background:repeating-linear-gradient(0deg,transparent,transparent 2px,rgba(0,0,0,0.12) 2px,rgba(0,0,0,0.12) 4px);
  pointer-events:none; z-index:1000;
}
.hud {
  display:grid;
  grid-template-columns:210px 1fr 210px;
  grid-template-rows:48px 1fr 72px;
  height:100vh; gap:3px; padding:3px;
}

/* --- TOP BAR --- */
.top-bar {
  grid-column:1/-1;
  display:flex; align-items:center; justify-content:space-between;
  border:1px solid #00ff41;
  padding:0 12px;
  background:rgba(0,255,65,0.04);
  font-size:11px;
}
.top-title {
  font-size:15px; font-weight:bold; letter-spacing:4px;
  text-shadow:0 0 12px #00ff41;
}
.top-right { display:flex; gap:16px; font-size:10px; }
.blink { animation:blink 1s step-end infinite; }
@keyframes blink { 50%{opacity:0} }
.red  { color:#ff3333; }
.yel  { color:#ffff00; }
.whi  { color:#ffffff; }
.grn  { color:#00ff41; }
.dim  { color:rgba(0,255,65,0.5); }

/* --- PANELS --- */
.left-panel, .right-panel {
  display:flex; flex-direction:column; gap:3px;
}
.box {
  border:1px solid rgba(0,255,65,0.5);
  background:rgba(0,20,0,0.6);
  padding:7px;
}
.box-title {
  font-size:8px; letter-spacing:3px;
  color:#00ff41; text-shadow:0 0 6px #00ff41;
  border-bottom:1px solid rgba(0,255,65,0.3);
  padding-bottom:4px; margin-bottom:6px;
}
.row {
  display:flex; justify-content:space-between;
  font-size:9px; padding:2px 0;
  border-bottom:1px solid rgba(0,255,65,0.1);
}
.row .v { color:#fff; font-weight:bold; }

/* --- RADAR --- */
#radar { display:block; margin:0 auto; }
.radar-info {
  display:flex; justify-content:space-between;
  font-size:8px; margin-top:4px;
}

/* --- SIGNAL BARS --- */
.sig-bars { display:flex; gap:2px; align-items:flex-end; height:18px; }
.sig-bar { width:7px; background:#00ff41; transition:height 0.4s; }

/* --- VIDEO CENTER --- */
.video-center {
  position:relative;
  border:1px solid #00ff41;
  background:#000; overflow:hidden;
}
.video-center img { width:100%; height:100%; object-fit:cover; }

/* Scan line */
.scan { position:absolute; left:0; right:0; height:2px;
  background:linear-gradient(90deg,transparent,rgba(0,255,65,0.6),transparent);
  animation:scandown 3s linear infinite; pointer-events:none; }
@keyframes scandown { 0%{top:0} 100%{top:100%} }

/* Crosshair */
.xhair { position:absolute; top:50%; left:50%; transform:translate(-50%,-50%);
  width:110px; height:110px; pointer-events:none; }
.c { position:absolute; width:18px; height:18px; border-color:#ff2222; border-style:solid; }
.c.tl{top:0;left:0;border-width:2px 0 0 2px}
.c.tr{top:0;right:0;border-width:2px 2px 0 0}
.c.bl{bottom:0;left:0;border-width:0 0 2px 2px}
.c.br{bottom:0;right:0;border-width:0 2px 2px 0}
.ch { position:absolute; top:50%; left:50%; transform:translate(-50%,-50%); }
.chh { width:28px; height:1px; background:rgba(255,30,30,0.8); }
.chv { width:1px; height:28px; background:rgba(255,30,30,0.8); }
.dot { width:4px; height:4px; border-radius:50%; background:#ff2222;
  animation:pdot 2s ease-in-out infinite; }
@keyframes pdot { 0%,100%{box-shadow:0 0 3px #ff2222} 50%{box-shadow:0 0 14px #ff2222} }

/* Threat box */
.tbox {
  position:absolute; border:1px solid #ff2222;
  pointer-events:none; opacity:0; transition:opacity 0.3s;
}
.tbox::before { content:'TGT'; position:absolute; top:-14px; left:0;
  font-size:8px; color:#ff2222; }
.tbox::after { content:''; position:absolute; top:50%; left:50%;
  transform:translate(-50%,-50%);
  width:6px; height:6px; background:#ff2222; border-radius:50%;
  animation:pdot 0.5s ease-in-out infinite; }

/* Lock text */
.lock-txt {
  position:absolute; color:#ff2222; font-size:10px; font-weight:bold;
  letter-spacing:2px; opacity:0; pointer-events:none;
  text-shadow:0 0 10px #ff2222;
  animation:lockblink 0.4s step-end infinite;
}
@keyframes lockblink { 50%{opacity:0} }

/* Video overlays */
.vov { position:absolute; font-size:9px; color:#00ff41;
  text-shadow:0 0 5px #00ff41; pointer-events:none; line-height:1.6; }
.vov.tl { top:8px; left:10px; }
.vov.tr { top:8px; right:10px; text-align:right; }
.vov.bl { bottom:8px; left:10px; }
.vov.br { bottom:8px; right:10px; text-align:right; }

/* Compass */
.compass {
  position:absolute; top:6px; left:50%; transform:translateX(-50%);
  font-size:9px; letter-spacing:4px; pointer-events:none;
  background:rgba(0,0,0,0.5); padding:2px 8px;
  border:1px solid rgba(0,255,65,0.4);
}
.compass span { color:#fff; }

/* Altitude mini-graph */
#altc { width:100%; height:36px; display:block; }

/* --- WEAPON PANEL --- */
.wpn-row { display:flex; justify-content:space-between; font-size:9px; padding:2px 0; }
.armed-txt  { color:#ff3333; }
.safe-txt   { color:#00ff41; }
.stby-txt   { color:#ffff00; }

/* NUKE BUTTON */
.nuke {
  width:100%; padding:10px 0; margin-top:8px;
  background:#5a0000; color:#ff2222;
  border:2px solid #ff2222;
  font-family:'Courier New',monospace;
  font-size:10px; font-weight:bold; letter-spacing:3px;
  cursor:pointer; text-transform:uppercase;
  animation:nukepulse 1.8s ease-in-out infinite;
  transition:all 0.1s;
}
.nuke:hover { background:#aa0000; box-shadow:0 0 25px rgba(255,0,0,0.7); transform:scale(1.02); }
.nuke:active { transform:scale(0.97); background:#ff0000; color:#fff; }
@keyframes nukepulse {
  0%,100%{box-shadow:0 0 8px rgba(255,0,0,0.4)}
  50%{box-shadow:0 0 22px rgba(255,0,0,0.85)}
}

/* --- BOTTOM BAR --- */
.bot-bar {
  grid-column:1/-1;
  display:flex; align-items:center; justify-content:space-between;
  border:1px solid #00ff41;
  padding:0 12px;
  background:rgba(0,255,65,0.04);
  font-size:9px;
}
.ticker-wrap { overflow:hidden; flex:1; margin:0 16px; }
.ticker { display:inline-block; white-space:nowrap;
  animation:tick 28s linear infinite; color:rgba(0,255,65,0.65); font-size:8px; }
@keyframes tick { 0%{transform:translateX(120%)} 100%{transform:translateX(-100%)} }

/* Warning flash overlay */
#wflash {
  position:fixed; top:0; left:0; right:0; bottom:0;
  background:rgba(255,0,0,0.12); pointer-events:none;
  opacity:0; z-index:500; transition:opacity 0.2s;
}

/* Confetti canvas */
#ccanvas {
  position:fixed; top:0; left:0; width:100%; height:100%;
  pointer-events:none; z-index:9999; display:none;
}

/* Log box */
#logbox { font-size:8px; color:rgba(0,255,65,0.7); line-height:1.6; margin-top:6px; }
</style>
</head>
<body>
<div id="wflash"></div>
<canvas id="ccanvas"></canvas>

<div class="hud">

  <div class="top-bar">
    <div class="top-title">&#11041; PROJECT HORIZON // DRONE-01 // TEJ4M-2026</div>
    <div class="top-right">
      <span>SYS: <span class="whi" id="sysstat">NOMINAL</span></span>
      <span>LINK: <span class="grn">ENCRYPTED</span></span>
      <span>MODE: <span class="yel">SURVEILLANCE</span></span>
      <span class="red blink">&#9679; REC</span>
      <span class="whi" id="toptime"></span>
    </div>
  </div>

  <div class="left-panel">

    <div class="box">
      <div class="box-title">&#9656; TACTICAL RADAR</div>
      <canvas id="radar" width="186" height="148"></canvas>
      <div class="radar-info">
        <span>RANGE: <span class="whi">500m</span></span>
        <span>THREATS: <span class="red" id="tcount">0</span></span>
        <span>IFF: <span class="grn">ON</span></span>
      </div>
    </div>

    <div class="box">
      <div class="box-title">&#9656; COMM LINK</div>
      <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:5px;">
        <span style="font-size:9px;">NRF24 UPLINK</span>
        <div class="sig-bars" id="sigbars">
          <div class="sig-bar" style="height:4px"></div>
          <div class="sig-bar" style="height:8px"></div>
          <div class="sig-bar" style="height:12px"></div>
          <div class="sig-bar" style="height:16px"></div>
          <div class="sig-bar" style="height:18px"></div>
        </div>
      </div>
      <div class="row"><span>FREQ</span><span class="v">2.4 GHz</span></div>
      <div class="row"><span>LATENCY</span><span class="v" id="lat">4ms</span></div>
      <div class="row"><span>PACKETS</span><span class="v" id="pkts">0</span></div>
      <div class="row"><span>RANGE</span><span class="v">2000m</span></div>
      <div class="row"><span>ENCRYPT</span><span class="v">AES-256</span></div>
    </div>

    <div class="box">
      <div class="box-title">&#9656; GPS / NAVIGATION</div>
      <div class="row"><span>LAT</span><span class="v" id="glat">45.3924&deg;N</span></div>
      <div class="row"><span>LON</span><span class="v" id="glon">75.7202&deg;W</span></div>
      <div class="row"><span>ELEV</span><span class="v" id="galt">--m</span></div>
      <div class="row"><span>SATS</span><span class="v">12 / LOCK</span></div>
      <div class="row"><span>ACCURACY</span><span class="v">&plusmn;1.8m</span></div>
    </div>

    <div class="box" style="flex:1">
      <div class="box-title">&#9656; ALTITUDE PROFILE</div>
      <canvas id="altc"></canvas>
    </div>

  </div>

  <div class="video-center">
    <img src="/stream" id="stream">
    <div class="scan"></div>

    <div class="xhair">
      <div class="c tl"></div><div class="c tr"></div>
      <div class="c bl"></div><div class="c br"></div>
      <div class="ch chh"></div>
      <div class="ch chv"></div>
      <div class="ch dot"></div>
    </div>

    <div class="tbox" id="tbox" style="width:75px;height:50px;"></div>
    <div class="lock-txt" id="locktxt" style="opacity:0;">&#9889; LOCK ACQUIRED &#9889;</div>

    <div class="compass" id="compass">N &nbsp; NE &nbsp; <span>E</span> &nbsp; SE &nbsp; S</div>

    <div class="vov tl">
      DRONE-01 // LIVE<br>
      <span id="valt">ALT: --m</span><br>
      <span id="vspd">SPD: --km/h</span>
    </div>
    <div class="vov tr">
      <span id="vhdg">HDG: --&deg;</span><br>
      <span id="vbat">BAT: --%</span><br>
      <span id="vmtime">T+ 00:00:00</span>
    </div>
    <div class="vov bl">
      <span id="vtemp">TEMP: --&deg;C</span><br>
      <span id="vwind">WIND: --km/h</span>
    </div>
    <div class="vov br">
      <span id="vvolt">--V</span><br>
      <span id="vcurr">--A</span>
    </div>
  </div>

  <div class="right-panel">

    <div class="box">
      <div class="box-title">&#9656; FLIGHT TELEMETRY</div>
      <div class="row"><span>ALTITUDE</span><span class="v" id="talt">--m</span></div>
      <div class="row"><span>SPEED</span><span class="v" id="tspd">--km/h</span></div>
      <div class="row"><span>HEADING</span><span class="v" id="thdg">--&deg;</span></div>
      <div class="row"><span>PITCH</span><span class="v" id="tpitch">--&deg;</span></div>
      <div class="row"><span>ROLL</span><span class="v" id="troll">--&deg;</span></div>
      <div class="row"><span>YAW RATE</span><span class="v" id="tyaw">--&deg;/s</span></div>
      <div class="row"><span>G-FORCE</span><span class="v" id="tg">--g</span></div>
    </div>

    <div class="box">
      <div class="box-title">&#9656; POWER SYSTEMS</div>
      <div class="row"><span>BATTERY</span><span class="v" id="tbat">--%</span></div>
      <div class="row"><span>VOLTAGE</span><span class="v" id="tvolt">--V</span></div>
      <div class="row"><span>CURRENT</span><span class="v" id="tcurr">--A</span></div>
      <div class="row"><span>CONSUMED</span><span class="v" id="tmah">--mAh</span></div>
      <div class="row"><span>EST FLIGHT</span><span class="v" id="teft">--min</span></div>
    </div>

    <div class="box">
      <div class="box-title">&#9656; WEAPONS SYSTEMS</div>
      <div class="wpn-row"><span>PAYLOAD</span><span class="stby-txt">STANDBY</span></div>
      <div class="wpn-row"><span>TARGETING</span><span class="armed-txt blink">ARMED</span></div>
      <div class="wpn-row"><span>COUNTERMSR</span><span class="safe-txt">READY</span></div>
      <div class="wpn-row"><span>WARHEAD</span><span class="armed-txt blink">ARMED</span></div>
      <div class="wpn-row"><span>SAFETY</span><span class="safe-txt" id="safetystat">ENGAGED</span></div>
      <button class="nuke" onclick="launchNuke()" id="nukebtn">&#9762; LAUNCH NUKE &#9762;</button>
    </div>

    <div class="box" style="flex:1">
      <div class="box-title">&#9656; SYSTEM STATUS</div>
      <div class="row"><span>FLIGHT CTRL</span><span class="safe-txt">NOMINAL</span></div>
      <div class="row"><span>IMU / GYRO</span><span class="safe-txt">CALIBRATED</span></div>
      <div class="row"><span>ESC x4</span><span class="safe-txt">ONLINE</span></div>
      <div class="row"><span>MOTORS</span><span class="safe-txt">NOMINAL</span></div>
      <div class="row"><span>CAM FEED</span><span class="safe-txt">LIVE</span></div>
      <div class="row"><span>MCU TEMP</span><span class="v" id="mtemp">--&deg;C</span></div>
      <div id="logbox">
        &gt; SYSTEM BOOT COMPLETE<br>
        &gt; NRF24 UPLINK ACTIVE<br>
        &gt; CAMERA FEED LIVE<br>
      </div>
    </div>

  </div>

  <div class="bot-bar">
    <span>OPR-1: HOSSAMELDIEN Z. &nbsp;|&nbsp; OPR-2: ZABARAH A.</span>
    <div class="ticker-wrap">
      <div class="ticker">
        &#9656; ALL SYSTEMS NOMINAL &nbsp;&nbsp;&nbsp;
        &#9656; NRF24 UPLINK 2.4GHz ACTIVE &nbsp;&nbsp;&nbsp;
        &#9656; GPS LOCK ACQUIRED &mdash; 12 SATELLITES &nbsp;&nbsp;&nbsp;
        &#9656; SURVEILLANCE MODE ACTIVE &nbsp;&nbsp;&nbsp;
        &#9656; BETAFLIGHT FC ONLINE &nbsp;&nbsp;&nbsp;
        &#9656; PROJECT HORIZON // TEJ4M CAPSTONE 2026 &nbsp;&nbsp;&nbsp;
        &#9656; EARL OF MARCH SECONDARY SCHOOL &nbsp;&nbsp;&nbsp;
        &#9656; AES-256 ENCRYPTED DATALINK ACTIVE &nbsp;&nbsp;&nbsp;
        &#9656; CUSTOM PCB CONTROLLER ONLINE &nbsp;&nbsp;&nbsp;
        &#9656; WIFI FPV STREAM ACTIVE &nbsp;&nbsp;&nbsp;
      </div>
    </div>
    <span style="text-align:right;">
      CLASSIFIED // TS-SCI<br>
      <span class="whi" id="bottime"></span>
    </span>
  </div>

</div>

<script>
// ==============================
// STATE
// ==============================
const start = Date.now();
let alt=52, spd=31, hdg=237, bat=87, volt=11.8, curr=14.5;
let pitch=1.8, roll=-1.2, yaw=0.4, gforce=1.02;
let temp=39, wind=13, mah=380;
let pkts=21000, nukeCount=0;
let altHist=[], threatActive=false;
let radarAngle=0, blips=[];

function r(a,b){ return Math.random()*(b-a)+a; }
function ri(a,b){ return Math.floor(r(a,b)); }
function clamp(v,a,b){ return Math.min(b,Math.max(a,v)); }

// ==============================
// TELEMETRY UPDATE
// ==============================
function updateTelem() {
  alt   = clamp(alt + r(-2,2), 5, 200);
  spd   = clamp(spd + r(-1,1), 0, 80);
  hdg   = (hdg + r(-1,1) + 360) % 360;
  bat   = clamp(bat - 0.008, 0, 100);
  volt  = clamp(volt - 0.001, 9, 12.6);
  curr  = r(10,22);
  pitch += r(-0.4,0.4);
  roll  += r(-0.4,0.4);
  yaw    = r(-6,6);
  gforce = 1 + r(-0.05,0.08);
  temp   = r(35,45);
  wind   = r(8,18);
  mah   += r(0.05,0.15);
  pkts  += ri(190,240);

  altHist.push(alt);
  if(altHist.length > 120) altHist.shift();

  let as = alt.toFixed(0)+'m';
  let ss = spd.toFixed(0)+' km/h';
  let hs = hdg.toFixed(0)+'\u00B0';
  let bs = bat.toFixed(0)+'%';

  set('talt',  as);       set('valt',  'ALT: '+as);
  set('tspd',  ss);       set('vspd',  'SPD: '+ss);
  set('thdg',  hs);       set('vhdg',  'HDG: '+hs);
  set('tpitch',pitch.toFixed(1)+'\u00B0');
  set('troll', roll.toFixed(1)+'\u00B0');
  set('tyaw',  yaw.toFixed(1)+'\u00B0/s');
  set('tg',    gforce.toFixed(2)+'g');
  set('tbat',  bs);       set('vbat',  'BAT: '+bs);
  set('tvolt', volt.toFixed(1)+'V'); set('vvolt', volt.toFixed(1)+'V');
  set('tcurr', curr.toFixed(1)+'A'); set('vcurr', curr.toFixed(1)+'A');
  set('tmah',  mah.toFixed(0)+'mAh');
  let eft = ((bat/100)*3000/curr/60).toFixed(0);
  set('teft',  eft+' min');
  set('mtemp', temp.toFixed(0)+'\u00B0C');
  set('vtemp', 'TEMP: '+temp.toFixed(0)+'\u00B0C');
  set('vwind', 'WIND: '+wind.toFixed(0)+'km/h');
  set('lat',   ri(3,8)+'ms');
  set('pkts',  pkts.toLocaleString());
  set('glat',  (45.3924+r(-0.0001,0.0001)).toFixed(4)+'\u00B0N');
  set('glon',  (75.7202+r(-0.0001,0.0001)).toFixed(4)+'\u00B0W');
  set('galt',  as);

  updateCompass(hdg);
  drawAlt();
  animateSigBars();
}

function set(id, val) {
  let el = document.getElementById(id);
  if(el) el.textContent = val;
}

// ==============================
// CLOCK
// ==============================
function updateClock() {
  let n = new Date();
  let ts = n.toTimeString().slice(0,8)+' UTC';
  set('toptime', ts); set('bottime', ts);
  let e = Math.floor((Date.now()-start)/1000);
  let h=String(Math.floor(e/3600)).padStart(2,'0');
  let m=String(Math.floor((e%3600)/60)).padStart(2,'0');
  let s=String(e%60).padStart(2,'0');
  set('vmtime', 'T+ '+h+':'+m+':'+s);
}

// ==============================
// COMPASS
// ==============================
function updateCompass(deg) {
  const pts = ['N','NE','E','SE','S','SW','W','NW'];
  let ci = Math.round(deg/45)%8;
  let out = '';
  for(let i=-4;i<=4;i++){
    let d = pts[(ci+i+800)%8];
    out += (i===0 ? '<span>'+d+'</span>' : d) + ' &nbsp; ';
  }
  let el = document.getElementById('compass');
  if(el) el.innerHTML = out;
}

// ==============================
// ALTITUDE GRAPH
// ==============================
function drawAlt() {
  let c = document.getElementById('altc');
  if(!c) return;
  c.width = c.offsetWidth || 180;
  let ctx = c.getContext('2d');
  ctx.clearRect(0,0,c.width,c.height);
  if(altHist.length < 2) return;
  let mn = Math.min(...altHist)-5, mx = Math.max(...altHist)+5;
  let w=c.width, h=c.height;
  ctx.strokeStyle='#00ff41'; ctx.lineWidth=1.5;
  ctx.shadowColor='#00ff41'; ctx.shadowBlur=4;
  ctx.beginPath();
  altHist.forEach((v,i)=>{
    let x=(i/(altHist.length-1))*w;
    let y=h-((v-mn)/(mx-mn))*h;
    i===0?ctx.moveTo(x,y):ctx.lineTo(x,y);
  });
  ctx.stroke();
  ctx.lineTo(w,h); ctx.lineTo(0,h); ctx.closePath();
  ctx.fillStyle='rgba(0,255,65,0.07)'; ctx.fill();
  ctx.shadowBlur=0;
}

// ==============================
// SIGNAL BARS
// ==============================
function animateSigBars() {
  let bars = document.querySelectorAll('.sig-bar');
  let heights = [4,8,12,16,18];
  bars.forEach((b,i)=>{
    let jitter = ri(-2,2);
    b.style.height = Math.max(2, heights[i]+jitter)+'px';
  });
}

// ==============================
// RADAR
// ==============================
function drawRadar() {
  let c = document.getElementById('radar');
  if(!c) return;
  let ctx = c.getContext('2d');
  let W=c.width, H=c.height, cx=W/2, cy=H/2, R=Math.min(W,H)/2-4;

  ctx.clearRect(0,0,W,H);

  ctx.fillStyle='rgba(0,15,0,0.9)';
  ctx.beginPath(); ctx.arc(cx,cy,R,0,Math.PI*2); ctx.fill();

  for(let i=1;i<=4;i++){
    ctx.beginPath(); ctx.arc(cx,cy,R*i/4,0,Math.PI*2);
    ctx.strokeStyle='rgba(0,255,65,0.15)'; ctx.lineWidth=0.5; ctx.stroke();
  }
  for(let i=0;i<8;i++){
    let a=i*Math.PI/4;
    ctx.beginPath();
    ctx.moveTo(cx,cy);
    ctx.lineTo(cx+Math.cos(a)*R, cy+Math.sin(a)*R);
    ctx.strokeStyle='rgba(0,255,65,0.1)'; ctx.lineWidth=0.5; ctx.stroke();
  }

  for(let i=0;i<40;i++){
    let a=radarAngle-(i*0.04);
    ctx.beginPath();
    ctx.moveTo(cx,cy);
    ctx.arc(cx,cy,R,a,a+0.04);
    ctx.closePath();
    ctx.fillStyle='rgba(0,255,65,'+(0.12*(1-i/40))+')';
    ctx.fill();
  }

  ctx.beginPath();
  ctx.moveTo(cx,cy);
  ctx.lineTo(cx+Math.cos(radarAngle)*R, cy+Math.sin(radarAngle)*R);
  ctx.strokeStyle='rgba(0,255,65,0.9)'; ctx.lineWidth=1.5;
  ctx.shadowColor='#00ff41'; ctx.shadowBlur=8;
  ctx.stroke(); ctx.shadowBlur=0;

  ctx.beginPath(); ctx.arc(cx,cy,3,0,Math.PI*2);
  ctx.fillStyle='#00ff41'; ctx.fill();

  blips = blips.filter(b=>b.age<b.maxAge);
  blips.forEach(b=>{
    let bx=cx+Math.cos(b.ang)*b.d*R;
    let by=cy+Math.sin(b.ang)*b.d*R;
    let fade=1-(b.age/b.maxAge);
    ctx.beginPath(); ctx.arc(bx,by,3,0,Math.PI*2);
    ctx.fillStyle='rgba(255,60,60,'+fade+')';
    ctx.shadowColor='#ff3333'; ctx.shadowBlur=8;
    ctx.fill(); ctx.shadowBlur=0;
    ctx.beginPath(); ctx.arc(bx,by,6,0,Math.PI*2);
    ctx.strokeStyle='rgba(255,60,60,'+(fade*0.4)+')'; ctx.lineWidth=0.5; ctx.stroke();
    b.age++;
  });

  if(blips.length < 5 && Math.random()<0.015) {
    blips.push({ang:r(0,Math.PI*2), d:r(0.1,0.85), age:0, maxAge:ri(180,400)});
  }

  ctx.beginPath(); ctx.arc(cx,cy,R,0,Math.PI*2);
  ctx.strokeStyle='rgba(0,255,65,0.6)'; ctx.lineWidth=1; ctx.stroke();

  ctx.fillStyle='rgba(0,255,65,0.5)'; ctx.font='8px Courier New';
  ctx.textAlign='center';
  ctx.fillText('N',cx,cy-R+10);
  ctx.fillText('S',cx,cy+R-3);
  ctx.textAlign='left';
  ctx.fillText('E',cx+R-12,cy+3);
  ctx.textAlign='right';
  ctx.fillText('W',cx-R+12,cy+3);

  set('tcount', blips.length.toString());

  radarAngle += 0.025;
}

// ==============================
// THREAT DETECTION
// ==============================
function checkThreat() {
  if(Math.random()<0.08 && !threatActive) {
    threatActive = true;
    let vc = document.querySelector('.video-center');
    if(!vc) return;
    let vw=vc.offsetWidth, vh=vc.offsetHeight;
    let bx=r(vw*0.1,vw*0.65), by=r(vh*0.1,vh*0.55);
    let box=document.getElementById('tbox');
    let ltx=document.getElementById('locktxt');
    box.style.left=bx+'px'; box.style.top=by+'px'; box.style.opacity='1';

    setTimeout(()=>{
      ltx.style.opacity='1';
      ltx.style.left=(bx+40)+'px'; ltx.style.top=(by+55)+'px';
      setTimeout(()=>{
        box.style.opacity='0'; ltx.style.opacity='0';
        setTimeout(()=>{ threatActive=false; }, 500);
      },2000);
    },1200);
  }
}

// ==============================
// NUKE + CONFETTI
// ==============================
function launchNuke() {
  nukeCount++;

  let fl=document.getElementById('wflash');
  fl.style.opacity='0.7';
  setTimeout(()=>fl.style.opacity='0',400);

  let log=document.getElementById('logbox');
  log.innerHTML='&gt; &#9888; AUTH CODE ACCEPTED<br>&gt; WARHEAD ARMED<br>&gt; TARGET ACQUIRED<br>&gt; <span style="color:#ff2222">NUCLEAR LAUNCH DETECTED x'+nukeCount+'</span><br>';

  let btn=document.getElementById('nukebtn');
  btn.textContent='\u2622 NUKE LAUNCHED \u00D7 '+nukeCount+' \u2622';
  btn.style.background='#ff0000'; btn.style.color='#fff';
  setTimeout(()=>{
    btn.style.background='#5a0000'; btn.style.color='#ff2222';
    btn.textContent='\u2622 LAUNCH NUKE \u2622';
  }, 2500);

  document.getElementById('safetystat').textContent='DISENGAGED';
  document.getElementById('safetystat').className='armed-txt blink';

  launchConfetti();
}

function launchConfetti() {
  let c=document.getElementById('ccanvas');
  c.style.display='block';
  c.width=window.innerWidth; c.height=window.innerHeight;
  let ctx=c.getContext('2d');
  let cols=['#ff0000','#00ff41','#ffff00','#ff8800','#ff66ff','#00ffff','#ffffff','#ff4444','#44ff44'];
  let pieces=[];

  for(let b=0;b<4;b++) {
    for(let i=0;i<100;i++) {
      pieces.push({
        x: c.width/2 + r(-150,150),
        y: r(-20, 30),
        w: r(5,14), h: r(8,18),
        col: cols[ri(0,cols.length)],
        vx: r(-18,18),
        vy: r(1,10)+b,
        rot: r(0,360), rotv: r(-10,10),
        alpha: 1,
        shape: Math.random()>0.4?'rect':'circle'
      });
    }
  }

  function anim() {
    ctx.clearRect(0,0,c.width,c.height);
    let alive=false;
    pieces.forEach(p=>{
      if(p.alpha<=0) return;
      alive=true;
      p.x+=p.vx; p.y+=p.vy;
      p.vy+=0.25; p.vx*=0.985;
      p.rot+=p.rotv;
      if(p.y>c.height*0.65) p.alpha-=0.025;
      ctx.save();
      ctx.globalAlpha=Math.max(0,p.alpha);
      ctx.translate(p.x,p.y);
      ctx.rotate(p.rot*Math.PI/180);
      ctx.fillStyle=p.col;
      ctx.shadowColor=p.col; ctx.shadowBlur=4;
      if(p.shape==='circle'){
        ctx.beginPath(); ctx.arc(0,0,p.w/2,0,Math.PI*2); ctx.fill();
      } else {
        ctx.fillRect(-p.w/2,-p.h/2,p.w,p.h);
      }
      ctx.restore();
    });
    if(alive) requestAnimationFrame(anim);
    else { ctx.clearRect(0,0,c.width,c.height); c.style.display='none'; }
  }
  anim();
}

// ==============================
// START LOOPS
// ==============================
setInterval(updateTelem, 500);
setInterval(updateClock, 1000);
setInterval(drawRadar, 40);
setInterval(checkThreat, 2500);

updateTelem(); updateClock(); drawRadar();
</script>
</body>
</html>
)rawliteral";

// ============================================================
// STREAM HANDLER
// RGB565 -> software JPEG conversion for RHYX M21-45
// which has no hardware JPEG encoder
// ============================================================
void handleStream() {
  WiFiClient client = server.client();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Cache-Control: no-cache");
  client.println("Connection: keep-alive");
  client.println();

  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      delay(10);
      continue;
    }

    // Software convert RGB565 to JPEG
    // RHYX M21-45 has no hardware JPEG encoder so we must do this in firmware
    uint8_t *jpg_buf = NULL;
    size_t   jpg_len = 0;
    bool converted = fmt2jpg(fb->buf, fb->len, fb->width, fb->height,
                             PIXFORMAT_RGB565, 80, &jpg_buf, &jpg_len);
    esp_camera_fb_return(fb);

    if (!converted || jpg_buf == NULL || jpg_len == 0) {
      // Conversion failed, skip this frame entirely
      // Do NOT send partial/corrupt data to the client
      if (jpg_buf) free(jpg_buf);
      delay(10);
      continue;
    }

    // Send clean JPEG frame
    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", jpg_len);
    client.write(jpg_buf, jpg_len);
    client.println();
    free(jpg_buf);

    delay(100); // ~10fps - increase to 150 for ~7fps if WiFi is struggling
                // decrease to 66 for ~15fps if you want smoother video
  }
}

void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== PROJECT HORIZON - CAMERA SYSTEM ===");

  camera_config_t cfg;
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer   = LEDC_TIMER_0;
  cfg.pin_d0 = Y2_GPIO_NUM; cfg.pin_d1 = Y3_GPIO_NUM;
  cfg.pin_d2 = Y4_GPIO_NUM; cfg.pin_d3 = Y5_GPIO_NUM;
  cfg.pin_d4 = Y6_GPIO_NUM; cfg.pin_d5 = Y7_GPIO_NUM;
  cfg.pin_d6 = Y8_GPIO_NUM; cfg.pin_d7 = Y9_GPIO_NUM;
  cfg.pin_xclk     = XCLK_GPIO_NUM;
  cfg.pin_pclk     = PCLK_GPIO_NUM;
  cfg.pin_vsync    = VSYNC_GPIO_NUM;
  cfg.pin_href     = HREF_GPIO_NUM;
  cfg.pin_sscb_sda = SIOD_GPIO_NUM;
  cfg.pin_sscb_scl = SIOC_GPIO_NUM;
  cfg.pin_pwdn     = PWDN_GPIO_NUM;
  cfg.pin_reset    = RESET_GPIO_NUM;

  // RHYX M21-45 specific settings:
  // Lower clock improves stability for this camera module
  cfg.xclk_freq_hz = 10000000;       // 10MHz - more stable than 20MHz on RHYX
  cfg.pixel_format = PIXFORMAT_RGB565; // Only format RHYX supports
  cfg.frame_size   = FRAMESIZE_QVGA;  // 320x240 - best FPS for software JPEG conversion
  cfg.jpeg_quality = 12;              // Not used directly but keep in config
  cfg.fb_count     = 1;               // 1 buffer - RHYX is slower, 2 can cause issues
  cfg.fb_location  = CAMERA_FB_IN_DRAM; // Force DRAM, more reliable than PSRAM for RHYX

  if (psramFound()) {
    Serial.println("PSRAM: found (using DRAM anyway for RHYX stability)");
  }

  esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("Camera FAILED: 0x%x\n", err);
    Serial.println("Check: is this definitely a RHYX M21-45 / GC2145 module?");
    return;
  }
  Serial.println("Camera: OK (RHYX M21-45 / RGB565 mode, software JPEG conversion)");

  // RHYX M21-45 outputs little-endian RGB565 but the ESP32 software JPEG
  // encoder assumes big-endian by default. Without this fix the image comes
  // out with swapped color channels (green tint / broken colors).
  jpgSetRgb565BE(false);

  // WiFi
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);
  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  if (MDNS.begin("drone")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://drone.local");
  } else {
    Serial.println("mDNS: failed (use IP address instead)");
  }

  server.on("/", handleRoot);
  server.on("/stream", handleStream);
  server.begin();
  Serial.println("Server: ready on port 80");
  Serial.println("Open: http://drone.local or http://10.141.90.184");
  Serial.println("======================================");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  server.handleClient();

  static unsigned long last = 0;
  if (millis() - last > 3000) {
    Serial.printf("IP: %s  |  http://drone.local\n",
                  WiFi.localIP().toString().c_str());
    last = millis();
  }
}
