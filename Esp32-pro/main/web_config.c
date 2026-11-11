/*
 * web_config.c — WiFi AP + serveur HTTP de configuration des boutons
 */
#include "web_config.h"
#include "btn_mapping.h"
#include "config.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "web";

static httpd_handle_t s_server = NULL;

static const char HTML_PAGE[] =
"<!DOCTYPE html><html lang='fr'><head>"
"<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Pro Controller — Configuration</title>"
"<style>"
"*{box-sizing:border-box}body{font-family:system-ui,sans-serif;max-width:720px;margin:0 auto;padding:16px;background:#1a1a2e;color:#eee}"
"h1{color:#e94560;margin-top:0}.card{background:#16213e;border-radius:8px;padding:16px;margin:12px 0}"
"label{display:block;margin:8px 0;font-size:14px}"
"select,input[type=text]{width:100%;padding:8px;border-radius:4px;border:1px solid #0f3460;background:#0f3460;color:#eee}"
"button{margin:4px 4px 4px 0;padding:8px 14px;border:none;border-radius:4px;background:#e94560;color:#fff;cursor:pointer}"
"button.sec{background:#533483}button.dng{background:#533483}"
".row{display:grid;grid-template-columns:1fr 1fr;gap:8px}"
".maprow{display:flex;align-items:center;gap:8px;flex-wrap:wrap;margin:6px 0}"
".maprow select{flex:1;min-width:120px}"
".turbo-lbl{font-size:12px;white-space:nowrap;display:flex;align-items:center;gap:4px}"
".live{display:flex;flex-wrap:wrap;gap:6px;margin-top:8px}"
".chip{padding:4px 8px;border-radius:4px;background:#0f3460;font-size:12px}"
".chip.on{background:#e94560}"
".msg{padding:8px;border-radius:4px;margin:8px 0;display:none}"
".msg.ok{display:block;background:#1b4332}.msg.err{display:block;background:#6a040f}"
"</style></head><body>"
"<h1>Pro Controller — Configuration</h1>"
  "<p>GPIO 16 actif — réseau WiFi <b>ProCon-Setup</b>. Le mot de passe est défini dans config.h.</p>"
"<div id='msg' class='msg'></div>"
"<div class='card'><h2>Profil</h2>"
"<select id='profileSel'></select>"
"<button onclick='setActive()'>Activer</button>"
"<button class='sec' onclick='newProfile()'>Nouveau</button>"
"<button class='dng' onclick='delProfile()'>Supprimer</button>"
"<input type='text' id='profileName' placeholder='Nom du profil'>"
"<button class='sec' onclick='renameProfile()'>Renommer</button></div>"
"<div class='card'><h2>Mapping (physique → Switch)</h2>"
"<p style='font-size:13px;opacity:.8'>Par défaut A→A, B→B… Turbo = 2 impulsions/s si maintenu.</p>"
"<div id='mapping'></div>"
"<button onclick='saveMap()'>Sauvegarder touches</button></div>"
"<div class='card'><h2>Joysticks</h2><div class='row'>"
"<label><input type='checkbox' id='swap_l'> Gauche : échanger X/Y</label>"
"<label><input type='checkbox' id='inv_lx'> Gauche : inverser X</label>"
"<label><input type='checkbox' id='inv_ly'> Gauche : inverser Y</label>"
"<label><input type='checkbox' id='swap_r'> Droit : échanger X/Y</label>"
"<label><input type='checkbox' id='inv_rx'> Droit : inverser X</label>"
"<label><input type='checkbox' id='inv_ry'> Droit : inverser Y</label>"
"</div><button onclick='saveSticks()'>Sauvegarder joysticks</button></div>"
"<div class='card'><h2>Test en direct</h2><div id='live' class='live'></div></div>"
"<div class='card'><button class='dng' onclick='resetAll()'>Tout remettre par défaut</button></div>"
"<script>"
"let S={profiles:[],active:0,btnLabels:[]};"
"const F={swap_l:1,inv_lx:2,inv_ly:4,swap_r:8,inv_rx:16,inv_ry:32};"
"async function api(m,u,b){const o={method:m,headers:{'Content-Type':'application/json'}};"
"if(b)o.body=JSON.stringify(b);return(await fetch(u,o)).json();}"
"function showMsg(t,ok){const e=document.getElementById('msg');e.textContent=t;e.className='msg '+(ok?'ok':'err');}"
"function profIdx(){return +document.getElementById('profileSel').value;}"
"function buildLogicOptions(sel,v){sel.innerHTML='';"
"S.btnLabels.forEach((l,i)=>{const o=document.createElement('option');o.value=i;o.textContent=l;"
"if(i===v)o.selected=true;sel.appendChild(o);});"
"const n=document.createElement('option');n.value=255;n.textContent='— Aucun —';"
"if(v===255)n.selected=true;sel.appendChild(n);}"
"function renderMapping(){const d=document.getElementById('mapping');d.innerHTML='';"
"const p=S.profiles[profIdx()];if(!p)return;"
"p.mapping.forEach((row,i)=>{const wrap=document.createElement('div');wrap.className='maprow';"
"const lab=document.createElement('span');lab.textContent=row.phys+' →';lab.style.minWidth='90px';"
"const sel=document.createElement('select');sel.dataset.phys=i;buildLogicOptions(sel,row.logic);"
"const tl=document.createElement('label');tl.className='turbo-lbl';"
"const cb=document.createElement('input');cb.type='checkbox';cb.className='turbo';cb.dataset.phys=i;"
"cb.checked=!!row.turbo;tl.appendChild(cb);tl.appendChild(document.createTextNode('Turbo'));"
"wrap.appendChild(lab);wrap.appendChild(sel);wrap.appendChild(tl);d.appendChild(wrap);});"
"const st=p.sticks;document.getElementById('swap_l').checked=!!(st.flags&F.swap_l);"
"document.getElementById('inv_lx').checked=!!(st.flags&F.inv_lx);"
"document.getElementById('inv_ly').checked=!!(st.flags&F.inv_ly);"
"document.getElementById('swap_r').checked=!!(st.flags&F.swap_r);"
"document.getElementById('inv_rx').checked=!!(st.flags&F.inv_rx);"
"document.getElementById('inv_ry').checked=!!(st.flags&F.inv_ry);}"
"function renderProfiles(){const s=document.getElementById('profileSel');s.innerHTML='';"
"S.profiles.forEach((p,i)=>{const o=document.createElement('option');o.value=i;o.textContent=p.name;"
"if(i===S.active)o.selected=true;s.appendChild(o);});"
"document.getElementById('profileName').value=S.profiles[S.active].name;}"
"async function refresh(){S=await api('GET','/api/state');renderProfiles();renderMapping();}"
"async function setActive(){await api('POST','/api/active',{idx:profIdx()});showMsg('Profil activé',true);refresh();}"
"async function newProfile(){const n=prompt('Nom:');if(!n)return;"
"const r=await api('POST','/api/profile/create',{name:n});showMsg(r.ok?'Créé':r.error,false);refresh();}"
"async function delProfile(){if(!confirm('Supprimer ?'))return;"
"const r=await api('POST','/api/profile/delete',{idx:profIdx()});showMsg(r.ok?'Supprimé':r.error,false);refresh();}"
"async function renameProfile(){const n=document.getElementById('profileName').value;"
"const r=await api('POST','/api/profile/rename',{idx:profIdx(),name:n});showMsg(r.ok?'OK':r.error,false);refresh();}"
"async function saveMap(){const p=profIdx();let mask=0;"
"document.querySelectorAll('#mapping .turbo').forEach(cb=>{if(cb.checked)mask|=1<<+cb.dataset.phys;});"
"for(const sel of document.querySelectorAll('#mapping select'))"
"await api('POST','/api/map',{profile:p,phys:+sel.dataset.phys,logic:+sel.value});"
"await api('POST','/api/turbo',{profile:p,mask});"
"showMsg('Touches sauvegardées',true);refresh();}"
"async function saveSticks(){const f=(document.getElementById('swap_l').checked?F.swap_l:0)"
"|(document.getElementById('inv_lx').checked?F.inv_lx:0)|(document.getElementById('inv_ly').checked?F.inv_ly:0)"
"|(document.getElementById('swap_r').checked?F.swap_r:0)|(document.getElementById('inv_rx').checked?F.inv_rx:0)"
"|(document.getElementById('inv_ry').checked?F.inv_ry:0);"
"await api('POST','/api/sticks',{profile:profIdx(),flags:f});showMsg('Joysticks sauvegardés',true);}"
"async function resetAll(){if(!confirm('Remettre mapping et profils par défaut ?'))return;"
"await api('POST','/api/reset',{});showMsg('Réinitialisé',true);refresh();}"
"async function pollLive(){try{const j=await api('GET','/api/live');const d=document.getElementById('live');d.innerHTML='';"
"j.pressed.forEach(n=>{const c=document.createElement('span');c.className='chip on';c.textContent=n;d.appendChild(c);});"
"if(!j.pressed.length){const c=document.createElement('span');c.className='chip';c.textContent='(aucune)';d.appendChild(c);}"
"}catch(e){}setTimeout(pollLive,150);}"
"document.getElementById('profileSel').onchange=renderMapping;"
"refresh();pollLive();"
"</script></body></html>";

static int recv_body(httpd_req_t *req, char *buf, size_t max) {
    int total = 0;
    while (total < (int)max - 1) {
        int r = httpd_req_recv(req, buf + total, max - 1 - total);
        if (r <= 0) break;
        total += r;
        if (total >= req->content_len) break;
    }
    buf[total] = '\0';
    return total;
}

static esp_err_t send_json(httpd_req_t *req, cJSON *root) {
    char *str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!str) return ESP_FAIL;
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_sendstr(req, str);
    free(str);
    return err;
}

static cJSON *build_state_json(void) {
    const BtnMappingStore *st = mapping_store();
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "active", st->active_profile);

    cJSON *labels = cJSON_CreateArray();
    for (int i = 0; i < MAP_BTN_COUNT; i++)
        cJSON_AddItemToArray(labels, cJSON_CreateString(mapping_btn_name(i)));
    cJSON_AddItemToObject(root, "btnLabels", labels);

    cJSON *profiles = cJSON_CreateArray();
    for (int p = 0; p < st->profile_count; p++) {
        cJSON *prof = cJSON_CreateObject();
        cJSON_AddStringToObject(prof, "name", st->names[p]);

        cJSON *map = cJSON_CreateArray();
        for (int i = 0; i < MAP_BTN_COUNT; i++) {
            cJSON *row = cJSON_CreateObject();
            cJSON_AddStringToObject(row, "phys", mapping_btn_name(i));
            cJSON_AddNumberToObject(row, "logic", st->profiles[p].target[i]);
            cJSON_AddBoolToObject(row, "turbo",
                (st->profiles[p].turbo_mask >> i) & 1);
            cJSON_AddItemToArray(map, row);
        }
        cJSON_AddItemToObject(prof, "mapping", map);

        cJSON *sticks = cJSON_CreateObject();
        cJSON_AddNumberToObject(sticks, "flags", st->profiles[p].flags);
        cJSON_AddItemToObject(prof, "sticks", sticks);

        cJSON_AddItemToArray(profiles, prof);
    }
    cJSON_AddItemToObject(root, "profiles", profiles);
    return root;
}

static esp_err_t h_get_root(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, HTML_PAGE, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t h_get_state(httpd_req_t *req) {
    return send_json(req, build_state_json());
}

static esp_err_t h_get_live(httpd_req_t *req) {
    uint16_t mcp;
    bool l3, r3;
    btn_mapping_read_phys(&mcp, &l3, &r3);

    cJSON *root = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < MAP_BTN_COUNT; i++) {
        if (mapping_phys_pressed(i, mcp, l3, r3))
            cJSON_AddItemToArray(arr, cJSON_CreateString(mapping_btn_name(i)));
    }
    cJSON_AddItemToObject(root, "pressed", arr);
    return send_json(req, root);
}

static esp_err_t h_post_active(httpd_req_t *req) {
    char buf[64];
    recv_body(req, buf, sizeof(buf));
    cJSON *j = cJSON_Parse(buf);
    cJSON *root = cJSON_CreateObject();
    bool ok = j && cJSON_IsNumber(cJSON_GetObjectItem(j, "idx")) &&
              mapping_set_active((uint8_t)cJSON_GetObjectItem(j, "idx")->valueint);
    cJSON_AddBoolToObject(root, "ok", ok);
    if (j) cJSON_Delete(j);
    return send_json(req, root);
}

static esp_err_t h_post_create(httpd_req_t *req) {
    char buf[128];
    recv_body(req, buf, sizeof(buf));
    cJSON *j = cJSON_Parse(buf);
    cJSON *name = j ? cJSON_GetObjectItem(j, "name") : NULL;
    cJSON *root = cJSON_CreateObject();
    bool ok = name && cJSON_IsString(name) && mapping_create_profile(name->valuestring);
    cJSON_AddBoolToObject(root, "ok", ok);
    if (j) cJSON_Delete(j);
    return send_json(req, root);
}

static esp_err_t h_post_delete(httpd_req_t *req) {
    char buf[64];
    recv_body(req, buf, sizeof(buf));
    cJSON *j = cJSON_Parse(buf);
    cJSON *root = cJSON_CreateObject();
    bool ok = j && cJSON_IsNumber(cJSON_GetObjectItem(j, "idx")) &&
              mapping_delete_profile((uint8_t)cJSON_GetObjectItem(j, "idx")->valueint);
    cJSON_AddBoolToObject(root, "ok", ok);
    if (j) cJSON_Delete(j);
    return send_json(req, root);
}

static esp_err_t h_post_rename(httpd_req_t *req) {
    char buf[128];
    recv_body(req, buf, sizeof(buf));
    cJSON *j = cJSON_Parse(buf);
    cJSON *name = j ? cJSON_GetObjectItem(j, "name") : NULL;
    cJSON *idx  = j ? cJSON_GetObjectItem(j, "idx") : NULL;
    cJSON *root = cJSON_CreateObject();
    bool ok = name && cJSON_IsString(name) && idx && cJSON_IsNumber(idx) &&
              mapping_rename_profile((uint8_t)idx->valueint, name->valuestring);
    cJSON_AddBoolToObject(root, "ok", ok);
    if (j) cJSON_Delete(j);
    return send_json(req, root);
}

static esp_err_t h_post_map(httpd_req_t *req) {
    char buf[96];
    recv_body(req, buf, sizeof(buf));
    cJSON *j = cJSON_Parse(buf);
    cJSON *prof  = j ? cJSON_GetObjectItem(j, "profile") : NULL;
    cJSON *phys  = j ? cJSON_GetObjectItem(j, "phys") : NULL;
    cJSON *logic = j ? cJSON_GetObjectItem(j, "logic") : NULL;
    cJSON *root = cJSON_CreateObject();
    bool ok = prof && phys && logic &&
              cJSON_IsNumber(prof) && cJSON_IsNumber(phys) && cJSON_IsNumber(logic) &&
              mapping_set_target((uint8_t)prof->valueint, (uint8_t)phys->valueint,
                                 (uint8_t)logic->valueint);
    cJSON_AddBoolToObject(root, "ok", ok);
    if (j) cJSON_Delete(j);
    return send_json(req, root);
}

static esp_err_t h_post_sticks(httpd_req_t *req) {
    char buf[96];
    recv_body(req, buf, sizeof(buf));
    cJSON *j = cJSON_Parse(buf);
    cJSON *prof  = j ? cJSON_GetObjectItem(j, "profile") : NULL;
    cJSON *flags = j ? cJSON_GetObjectItem(j, "flags") : NULL;
    cJSON *root = cJSON_CreateObject();
    bool ok = prof && flags && cJSON_IsNumber(prof) && cJSON_IsNumber(flags) &&
              mapping_set_stick_flags((uint8_t)prof->valueint, (uint8_t)flags->valueint);
    cJSON_AddBoolToObject(root, "ok", ok);
    if (j) cJSON_Delete(j);
    return send_json(req, root);
}

static esp_err_t h_post_turbo(httpd_req_t *req) {
    char buf[96];
    recv_body(req, buf, sizeof(buf));
    cJSON *j = cJSON_Parse(buf);
    cJSON *prof = j ? cJSON_GetObjectItem(j, "profile") : NULL;
    cJSON *mask = j ? cJSON_GetObjectItem(j, "mask") : NULL;
    cJSON *root = cJSON_CreateObject();
    bool ok = prof && mask && cJSON_IsNumber(prof) && cJSON_IsNumber(mask) &&
              mapping_set_turbo_mask((uint8_t)prof->valueint, (uint32_t)mask->valueint);
    cJSON_AddBoolToObject(root, "ok", ok);
    if (j) cJSON_Delete(j);
    return send_json(req, root);
}

static esp_err_t h_post_reset(httpd_req_t *req) {
    mapping_reset_defaults();
    mapping_save();
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", true);
    return send_json(req, root);
}

static httpd_handle_t start_httpd(void) {
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.max_uri_handlers = 15;
    cfg.stack_size = 8192;

    if (httpd_start(&s_server, &cfg) != ESP_OK)
        return NULL;

    httpd_uri_t routes[] = {
        { .uri = "/",                   .method = HTTP_GET,  .handler = h_get_root },
        { .uri = "/api/state",          .method = HTTP_GET,  .handler = h_get_state },
        { .uri = "/api/live",           .method = HTTP_GET,  .handler = h_get_live },
        { .uri = "/api/active",         .method = HTTP_POST, .handler = h_post_active },
        { .uri = "/api/profile/create", .method = HTTP_POST, .handler = h_post_create },
        { .uri = "/api/profile/delete", .method = HTTP_POST, .handler = h_post_delete },
        { .uri = "/api/profile/rename", .method = HTTP_POST, .handler = h_post_rename },
        { .uri = "/api/map",            .method = HTTP_POST, .handler = h_post_map },
        { .uri = "/api/sticks",         .method = HTTP_POST, .handler = h_post_sticks },
        { .uri = "/api/turbo",          .method = HTTP_POST, .handler = h_post_turbo },
        { .uri = "/api/reset",          .method = HTTP_POST, .handler = h_post_reset },
    };
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++)
        httpd_register_uri_handler(s_server, &routes[i]);

    return s_server;
}

static void wifi_ap_start(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wcfg));

    wifi_config_t ap = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = 0,
            .channel = WIFI_AP_CHANNEL,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)ap.ap.password, WIFI_AP_PASS, sizeof(ap.ap.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP démarré — SSID: %s  →  http://192.168.4.1", WIFI_AP_SSID);
}

void web_config_run(void) {
    wifi_ap_start();
    if (!start_httpd()) {
        ESP_LOGE(TAG, "Échec démarrage serveur HTTP");
        return;
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
