/**
 * @file web_page.h
 * @brief 内嵌配网页 (UTF-8 中文, 移动端单页, 无外部资源)
 *
 * 页面只负责收 SSID/密码 POST 给 /connect, 连网/存 NVS 都在固件侧。
 * 改文字/样式直接改这里重新编译。本文件必须保持 UTF-8 编码。
 */
#ifndef WIFI_PROV_WEB_PAGE_H
#define WIFI_PROV_WEB_PAGE_H

static const char PAGE_INDEX[] =
"<!DOCTYPE html><html lang=\"zh-CN\"><head><meta charset=\"utf-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,viewport-fit=cover\">"
"<title>WiFi 配网</title><style>"
"body{font-family:system-ui,sans-serif;background:#f4f5f7;margin:0;padding:16px;}"
"h2{margin:8px 0 16px;font-size:20px;}"
".card{background:#fff;border-radius:12px;padding:16px;margin-bottom:12px;"
"box-shadow:0 1px 3px rgba(0,0,0,.08);}"
"label{display:block;font-size:13px;color:#666;margin:10px 0 4px;}"
"input,select{width:100%;box-sizing:border-box;font-size:16px;padding:12px;"
"border:1px solid #d0d4da;border-radius:8px;background:#fff;}"
"button{width:100%;font-size:16px;padding:13px;border:0;border-radius:8px;"
"background:#2563eb;color:#fff;margin-top:14px;}"
"button.gray{background:#9ca3af;}"
"#list{list-style:none;padding:0;margin:8px 0 0;max-height:210px;overflow:auto;}"
"#list li{display:flex;justify-content:space-between;align-items:center;"
"padding:11px 12px;border-bottom:1px solid #eee;font-size:15px;}"
"#list li:active{background:#eef2ff;}"
".sig{color:#9ca3af;font-size:12px;}"
".hint{font-size:12px;color:#9ca3af;margin-top:10px;}"
"</style></head><body>"
"<h2>WiFi 配网</h2>"
"<div class=\"card\">"
"<button class=\"gray\" id=\"scanBtn\" type=\"button\">扫描附近 WiFi</button>"
"<ul id=\"list\"></ul></div>"
"<div class=\"card\">"
"<form method=\"POST\" action=\"/connect\" id=\"f\">"
"<label>WiFi 名称 (SSID)</label>"
"<input name=\"ssid\" id=\"ssid\" maxlength=\"32\" required placeholder=\"点击上方列表选择或手动输入\">"
"<label>密码</label>"
"<input name=\"pass\" id=\"pass\" type=\"password\" maxlength=\"63\" placeholder=\"开放网络可留空\">"
"<button type=\"submit\">连接</button>"
"</form>"
"<div class=\"hint\">提交后设备将重启并连接 WiFi, 请等待约 20 秒。若失败请重新上电再次配网。</div>"
"</div>"
"<script>"
"var scanBtn=document.getElementById('scanBtn');"
"function render(list){var ul=document.getElementById('list');ul.innerHTML='';"
"list.forEach(function(a){var li=document.createElement('li');"
"var b=document.createElement('span');b.textContent=a.ssid||'(隐藏网络)';"
"var s=document.createElement('span');s.className='sig';s.textContent=a.rssi+' dBm';"
"li.appendChild(b);li.appendChild(s);"
"li.onclick=function(){document.getElementById('ssid').value=a.ssid;};"
"ul.appendChild(li);});}"
"function scan(){scanBtn.disabled=true;scanBtn.textContent='扫描中...';"
"fetch('/scan').then(function(r){return r.json();}).then(render)"
".catch(function(){alert('扫描失败, 请重试');})"
".finally(function(){scanBtn.disabled=false;scanBtn.textContent='重新扫描';});}"
"scanBtn.onclick=scan;"
"document.getElementById('f').onsubmit=function(){"
"if(!document.getElementById('ssid').value){alert('请填写 WiFi 名称');return false;}"
"setTimeout(function(){document.body.innerHTML="
"'<h2>正在连接...</h2><div class=\"card\"><p>设备正在连接 WiFi 并重启。"
"请稍候约 20 秒; 若连接失败, 重新上电后可再次配网。</p></div>';},100);};"
"scan();"
"</script></body></html>";

/* /connect 成功应答页 (设备发完此页才重启, 手机一定看得到) */
static const char PAGE_CONNECT_OK[] =
"<!DOCTYPE html><html lang=\"zh-CN\"><head><meta charset=\"utf-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>连接中</title></head><body style=\"font-family:system-ui,sans-serif;padding:24px\">"
"<h2>已提交, 设备正在连接 WiFi</h2>"
"<p>设备将自动重启并连接路由器, 请等待约 20 秒。</p>"
"<p>若连接失败, 重新上电后可再次扫码配网。</p></body></html>";

/* /connect 参数错误应答页 */
static const char PAGE_CONNECT_ERR[] =
"<!DOCTYPE html><html lang=\"zh-CN\"><head><meta charset=\"utf-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>参数错误</title></head><body style=\"font-family:system-ui,sans-serif;padding:24px\">"
"<h2>参数错误</h2><p>SSID 或密码格式不正确。</p>"
"<p><a href=\"/\">返回重试</a></p></body></html>";

#endif /* WIFI_PROV_WEB_PAGE_H */
