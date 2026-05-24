#include "wifi_portal.h"
#include "wifi_config.h"
#include <WiFi.h>
#include <WebServer.h>

static WebServer server(80);
static bool running = false;
static String apSSID;
static String apPassword = "12345678";

static const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Gatinho-Pet Setup</title>
  <style>
    body { font-family: -apple-system, sans-serif; max-width: 400px; margin: 30px auto; padding: 20px; background: #f5f5f5; }
    .card { background: white; padding: 24px; border-radius: 12px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }
    h1 { color: #ff7700; margin-top: 0; }
    label { display: block; margin: 12px 0 4px; font-weight: 600; color: #555; }
    input { width: 100%; padding: 12px; font-size: 16px; border: 2px solid #ddd; border-radius: 8px; box-sizing: border-box; }
    input:focus { outline: none; border-color: #ff7700; }
    button { width: 100%; padding: 14px; font-size: 16px; background: #ff7700; color: white; border: 0; border-radius: 8px; margin-top: 20px; cursor: pointer; font-weight: 600; }
    button:hover { background: #e66600; }
  </style>
</head>
<body>
  <div class="card">
    <h1>&#x1F431; Gatinho-Pet</h1>
    <p>Configure a rede WiFi para conectar o seu gatinho.</p>
    <form method="POST" action="/save">
      <label for="ssid">Nome da rede (SSID):</label>
      <input id="ssid" name="ssid" required maxlength="32" autocomplete="off">
      <label for="password">Senha:</label>
      <input id="password" name="password" type="password" maxlength="64" autocomplete="off">
      <button type="submit">Salvar e Reiniciar</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

static const char PAGE_OK[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Salvo!</title>
  <style>
    body { font-family: -apple-system, sans-serif; max-width: 400px; margin: 60px auto; padding: 20px; text-align: center; }
    h1 { color: #ff7700; }
  </style>
</head>
<body>
  <h1>&#x2705; Salvo!</h1>
  <p>Reiniciando o gatinho...</p>
</body>
</html>
)rawliteral";

static void handleRoot() {
    server.send_P(200, "text/html", PAGE_HTML);
}

static void handleSave() {
    String ssid = server.arg("ssid");
    String pwd  = server.arg("password");

    if (ssid.length() == 0) {
        server.send(400, "text/plain", "SSID vazio");
        return;
    }

    wifiConfigSave(ssid, pwd);
    server.send_P(200, "text/html", PAGE_OK);
    Serial.printf("[portal] Credenciais salvas, reiniciando em 3s...\n");
    delay(3000);
    ESP.restart();
}

static String buildAPSSID() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char suffix[5];
    snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);
    return "Gatinho-Setup-" + String(suffix);
}

void portalStart() {
    if (running) return;

    apSSID = buildAPSSID();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());

    Serial.printf("[portal] AP iniciado: %s / %s\n", apSSID.c_str(), apPassword.c_str());
    Serial.printf("[portal] IP: %s\n", WiFi.softAPIP().toString().c_str());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();
    running = true;
}

void portalStop() {
    if (!running) return;
    server.stop();
    WiFi.softAPdisconnect(true);
    running = false;
    Serial.println("[portal] Parado");
}

bool portalIsRunning() {
    return running;
}

void portalLoop() {
    if (running) server.handleClient();
}

String portalGetAPSSID() {
    return apSSID;
}

String portalGetAPPassword() {
    return apPassword;
}

String portalGetAPIP() {
    return WiFi.softAPIP().toString();
}
