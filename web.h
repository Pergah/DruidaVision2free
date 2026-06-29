/*  web.h — Portal web local (WebServer :80).
 *  Sirve la pagina de medicion (PPFD + espectro + DLI + calibracion) y
 *  permite cargar WiFi/device_id desde el celular. HTML estatico en PROGMEM;
 *  los valores vivos los pide la pagina por fetch.                          */
#pragma once
#include <WebServer.h>
#include <stdio.h>
#include "sensor.h"   // g_last, g_factor, g_offset
#include "store.h"    // g_ssid, g_pass, g_deviceId, storeSave*
#include "net.h"      // netApply
#include "ui.h"       // uiRefreshCal

WebServer g_server(80);

static const char INDEX_HTML[] PROGMEM = R"HTML(<!DOCTYPE html><html lang=es><head>
<meta charset=UTF-8><meta name=viewport content="width=device-width,initial-scale=1">
<title>DruidaVision</title><style>
:root{--cy:#00ccff}
body{background:linear-gradient(135deg,#0a0f1e,#111927);color:#cfefff;font-family:'Courier New',monospace;margin:0;padding:14px}
.logo{font-size:2.3rem;font-weight:bold;text-align:center;color:var(--cy);text-shadow:0 0 8px var(--cy);margin:4px 0 14px}
.card{background:rgba(0,116,217,.12);border:1px solid rgba(0,204,255,.35);border-radius:14px;padding:14px;margin:12px auto;max-width:520px;box-shadow:0 0 12px rgba(0,204,255,.2)}
.ppfd{font-size:3.4rem;font-weight:bold;color:#fff;text-align:center;text-shadow:0 0 10px var(--cy)}
.unit{text-align:center;color:#7bdfff;margin-top:-4px}
svg{width:100%;height:120px;margin-top:8px}
button{background:#1a2433;border:2px solid var(--cy);border-radius:8px;color:var(--cy);font:inherit;font-weight:bold;padding:9px 16px;margin:6px 4px;cursor:pointer}
button:active{transform:scale(.97)}
input{font:inherit;background:rgba(0,0,0,.4);border:1px solid #0074D9;border-radius:8px;color:#0ff;padding:9px;width:100%;box-sizing:border-box;margin:5px 0}
.row{display:flex;gap:8px;align-items:center;justify-content:space-between}
.val{font-size:1.3rem;color:#fff;min-width:84px;text-align:center}
h3{color:var(--cy);margin:2px 0 8px;border-bottom:1px solid rgba(0,204,255,.3);padding-bottom:4px}
.small{font-size:.8rem;color:#8aa;text-align:center}
.res{font-size:1.1rem;color:#0ff;text-align:center;margin-top:8px}
</style></head><body>
<div class=logo>DRUIDA VISION</div>
<div class=card>
 <div class=ppfd id=ppfd>--</div><div class=unit>PPFD (umol/m2.s)</div>
 <svg viewBox="0 0 300 150" preserveAspectRatio=none><defs>
 <linearGradient id=g x1=0 x2=1><stop offset=0 stop-color=blue/><stop offset=.33 stop-color=cyan/><stop offset=.66 stop-color=yellow/><stop offset=1 stop-color=red/></linearGradient></defs>
 <path id=curve fill=url(#g)/></svg>
 <div style=text-align:center><button onclick=medir()>MEDIR</button>
 <button id=autoBtn onclick=toggleAuto()>AUTO: OFF</button></div>
 <div class=small id=spec>R -- G -- B --</div>
</div>
<div class=card><h3>DLI</h3>
 <div class=row><input id=horas type=number min=0 step=0.5 placeholder="Horas de luz">
 <button onclick=calcDLI()>CALCULAR</button></div>
 <div class=res id=dli>DLI: -</div>
</div>
<div class=card><h3>Calibracion</h3>
 <div class=row><span>Factor</span><button onclick="adj('factorDown')">-</button>
 <span class=val id=fac>-</span><button onclick="adj('factorUp')">+</button></div>
 <div class=row><span>Offset</span><button onclick="adj('offsetDown')">-</button>
 <span class=val id=off>-</span><button onclick="adj('offsetUp')">+</button></div>
 <div class=small>PPFD = cubica(CLEAR) * factor + offset</div>
</div>
<div class=card><h3>WiFi / Nube</h3>
 <input id=ssid placeholder=SSID>
 <input id=pass type=password placeholder=Clave>
 <input id=devid placeholder="device_id del bot (nube)">
 <button onclick=saveWifi()>GUARDAR Y CONECTAR</button>
 <div class=res id=wifiRes></div>
</div>
<script>
let last=0,auto=null;
function medir(){fetch('/medir').then(r=>r.text()).then(t=>{
 let L=t.trim().split('\n');
 let p=parseFloat((L[0]||'').split(':')[1])||0;
 let r=parseFloat((L[1]||'').split(':')[1])||0;
 let g=parseFloat((L[2]||'').split(':')[1])||0;
 let b=parseFloat((L[3]||'').split(':')[1])||0;
 last=p;document.getElementById('ppfd').innerText=Math.round(p);
 document.getElementById('spec').innerText='R '+r.toFixed(0)+'%  G '+g.toFixed(0)+'%  B '+b.toFixed(0)+'%';
 draw(r,g,b);});}
function draw(r,g,b){const W=300,H=150,s=30,cB=60,cG=150,cR=240;
 const m=Math.max(r,g,b,1),nr=r/m,ng=g/m,nb=b/m;
 const f=(x,u,a)=>a*Math.exp(-((x-u)*(x-u))/(2*s*s));let d='';
 for(let x=-30;x<=W+30;x++){let y=H-(f(x,cB,nb)+f(x,cG,ng)+f(x,cR,nr))*H;
 d+=(x===-30?'M'+x+','+H+' L':'L')+x+','+y.toFixed(1)+' ';}
 d+='L'+(W+30)+','+H+' Z';document.getElementById('curve').setAttribute('d',d);}
function toggleAuto(){const b=document.getElementById('autoBtn');
 if(auto){clearInterval(auto);auto=null;b.innerText='AUTO: OFF';}
 else{auto=setInterval(medir,1000);b.innerText='AUTO: ON';medir();}}
function calcDLI(){let h=parseFloat(document.getElementById('horas').value);
 if(isNaN(h)||h<=0){document.getElementById('dli').innerText='DLI: -';return;}
 document.getElementById('dli').innerText='DLI: '+(last*3600*h/1e6).toFixed(1)+' mol/m2.dia';}
function adj(t){fetch('/ajustar?tipo='+t).then(()=>{loadCal();medir();});}
function loadCal(){fetch('/valoresCalibracion').then(r=>r.json()).then(d=>{
 document.getElementById('fac').innerText=Number(d.factor).toFixed(3);
 document.getElementById('off').innerText=Number(d.offset).toFixed(1);
 if(!document.getElementById('ssid').value)document.getElementById('ssid').value=d.ssid||'';
 if(!document.getElementById('devid').value)document.getElementById('devid').value=d.devid||'';});}
function saveWifi(){let s=encodeURIComponent(ssid.value),p=encodeURIComponent(pass.value),d=encodeURIComponent(devid.value);
 fetch('/saveWifi?ssid='+s+'&pass='+p+'&devid='+d).then(r=>r.text()).then(()=>{
 document.getElementById('wifiRes').innerText='Guardado. Reconectando...';});}
loadCal();medir();
</script></body></html>)HTML";

static void webHandleRoot()  { g_server.send_P(200, "text/html", INDEX_HTML); }

static void webHandleMedir() {
  char buf[96];
  snprintf(buf, sizeof(buf), "PPFD: %.1f\nRed: %.1f\nGreen: %.1f\nBlue: %.1f",
           g_last.ppfd, g_last.pr, g_last.pg, g_last.pb);
  g_server.send(200, "text/plain", buf);
}

static void webHandleValores() {
  char buf[200];
  snprintf(buf, sizeof(buf),
    "{\"factor\":%.4f,\"offset\":%.2f,\"ssid\":\"%s\",\"devid\":\"%s\"}",
    g_factor, g_offset, g_ssid.c_str(), g_deviceId.c_str());
  g_server.send(200, "application/json", buf);
}

static void webHandleAjustar() {
  String t = g_server.arg("tipo");
  if      (t == "factorUp")   g_factor += 0.05f;
  else if (t == "factorDown") g_factor -= 0.05f;
  else if (t == "offsetUp")   g_offset += 10.0f;
  else if (t == "offsetDown") g_offset -= 10.0f;
  else if (t == "factorSet" && g_server.hasArg("v")) g_factor = g_server.arg("v").toFloat();
  else if (t == "offsetSet" && g_server.hasArg("v")) g_offset = g_server.arg("v").toFloat();
  if (g_factor < 0.0f) g_factor = 0.0f;
  storeSaveCalCustom();                    // ajuste manual via web -> calib 'custom' (no la pisa el OTA)
  uiRefreshCal();                          // mantiene en sync la pantalla tactil
  g_server.send(200, "text/plain", "OK");
}

static void webHandleSaveWifi() {
  if (g_server.hasArg("ssid"))  g_ssid     = g_server.arg("ssid");
  if (g_server.hasArg("pass"))  g_pass     = g_server.arg("pass");
  if (g_server.hasArg("devid")) g_deviceId = g_server.arg("devid");
  storeSaveNet();
  g_server.send(200, "text/plain", "OK");
  netApply();                              // reconecta con las nuevas credenciales
}

// true cuando el socket :80 esta ligado. NO se puede hacer g_server.begin() ni
// handleClient() con la radio apagada: sin red, la pila lwip/tcpip no esta
// inicializada y crear/atender el socket toma un mutex NULO -> assert
// (xQueueSemaphoreTake queue.c) -> crash en bucle. Por eso el portal solo vive
// mientras la radio esta encendida (ver webLoop, que lo (re)liga solo).
static bool g_webUp = false;

void webInit() {
  // Solo registra los handlers; el begin() lo hace webLoop cuando la radio este
  // arriba (con WiFi apagado por defecto, hacer begin() aca crasheaba el boot).
  g_server.on("/",                   webHandleRoot);
  g_server.on("/medir",              webHandleMedir);
  g_server.on("/valoresCalibracion", webHandleValores);
  g_server.on("/ajustar",            webHandleAjustar);
  g_server.on("/saveWifi",           webHandleSaveWifi);
}

void webLoop() {
  if (!netStackUp()) {        // radio apagada: lwip abajo, no tocar sockets (ver net.h)
    g_webUp = false;
    return;
  }
  if (!g_webUp) { g_server.begin(); g_webUp = true; }   // radio recien arriba -> (re)liga :80
  g_server.handleClient();
}

// Re-liga el socket :80 tras reactivar la radio (WIFI_OFF lo destruye). Lo llama
// net.h al encender la radio: forzamos que webLoop vuelva a hacer begin() sobre el
// socket nuevo (el anterior murio al apagar la radio).
void webOnWifiUp() { g_webUp = false; }
