# Graphique_Asynchrone

Version asynchrone de la bibliothèque **Graphique_Synchrone** pour Arduino (ESP8266 / ESP32).

Cette version sépare le **template HTML/JS** (servi une seule fois au navigateur) des **données JSON** (mises à jour en continu via `fetch`). Cela réduit drastiquement la consommation mémoire et la charge réseau, et offre une expérience temps réel fluide sans rechargement de page.

Auteur : Olivier FOURNET | Licence : GPL-3.0

---

## Principe

| Version | Fonctionnement | Inconvénient principal |
|---------|---------------|------------------------|
| **Synchrone** | Le serveur génère l'HTML+JS+données à chaque requête. | String massive, fragmentation mémoire, clignotement à l'affichage. |
| **Asynchrone** | Le serveur envoie le template HTML une seule fois, puis le navigateur récupère les données via `/data.json` toutes les N ms. | Nécessite 2 endpoints (template + données). |

---

## Installation

### Arduino IDE
Placez `Graphique_Asynchrone.h` dans le dossier `libraries` de votre sketchbook, puis incluez-la :

```cpp
#include <Graphique_Asynchrone.h>
```

### PlatformIO
Ajoutez la dépendance dans `platformio.ini` :

```ini
lib_deps =
    https://github.com/Fo170/Graphique_Asynchrone.git@^1.0.0
```

---

## API Asynchrone

### `streamTemplate(Print& out)`
Génère la page HTML complète avec le JavaScript embarqué. **À servir une seule fois** sur la route `/` (ou `/index.html`). Le navigateur va ensuite faire du polling automatique sur `data.json`.

### `streamDataJSON(Print& out)`
Génère uniquement le JSON des données courantes. **À servir sur `/data.json`**.

### `streamDataCSV(Print& out)`
Génère un export CSV à la volée. **À servir sur `/data.csv`**.

### `streamSSEHeader(Print& out)` + `streamSSEData(Print& out)`
Pour du **Server-Sent Events** (push serveur → navigateur sans polling). Nécessite une connexion keep-alive.

### `setRefreshInterval(int ms)`
Définit l'intervalle de rafraîchissement côté client (défaut : 1000 ms).

---

## Exemple complet (ESP8266 + serveur web standard)

```cpp
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Graphique_Asynchrone.h>

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP8266WebServer server(80);
GraphiqueAsync g(2, 60);

void handleRoot() {
    server.sendHeader("Content-Type", "text/html; charset=utf-8");
    // On stream directement dans le client (zero-copy)
    g.streamTemplate(server.client());
}

void handleDataJSON() {
    server.sendHeader("Content-Type", "application/json");
    g.streamDataJSON(server.client());
}

void handleDataCSV() {
    server.sendHeader("Content-Type", "text/csv");
    server.sendHeader("Content-Disposition", "attachment; filename=data.csv");
    g.streamDataCSV(server.client());
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }
    Serial.println(WiFi.localIP());

    g.setDimensions(1200, 600);
    g.setTitre("Station Meteo Asynchrone");
    g.setRefreshInterval(800); // rafraichissement toutes les 800ms

    g.setLegende(GraphiqueAsync::COURBE_1, "Temperature (C)");
    g.setCouleur(GraphiqueAsync::COURBE_1, GraphiqueAsync::CouleurRouge());
    g.setAxeY(GraphiqueAsync::COURBE_1, GraphiqueAsync::AXE_GAUCHE);

    g.setLegende(GraphiqueAsync::COURBE_2, "Humidite (%)");
    g.setCouleur(GraphiqueAsync::COURBE_2, GraphiqueAsync::CouleurBleu());
    g.setAxeY(GraphiqueAsync::COURBE_2, GraphiqueAsync::AXE_DROITE);

    server.on("/", handleRoot);
    server.on("/data.json", handleDataJSON);
    server.on("/data.csv", handleDataCSV);
    server.begin();
}

void loop() {
    server.handleClient();

    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();

        float temp = 20.0 + 5.0 * sin(millis() / 10000.0);
        float hum  = 50.0 + 10.0 * cos(millis() / 15000.0);

        g.decaler();
        g.setTime(millis() / 1000.0);
        g.addValue(GraphiqueAsync::COURBE_1, temp);
        g.addValue(GraphiqueAsync::COURBE_2, hum);
        g.incrementSample();
    }
}
```

---

## Routes

| Route | Handler | Description |
|-------|---------|-------------|
| `/` | `streamTemplate()` | Page principale avec graphique auto-rafraîchissant |
| `/data.json` | `streamDataJSON()` | Données brutes JSON (polling) |
| `/data.csv` | `streamDataCSV()` | Export CSV téléchargeable |

---

## Avantages de l'asynchrone

- **Mémoire** : le template HTML est streamé directement vers le client sans allocation de `String` massive.
- **Réseau** : seuls quelques ko de JSON transitent à chaque cycle au lieu de toute la page HTML.
- **UX** : pas de rechargement de page, pas de clignotement Google Charts, animation fluide.
- **Multi-client** : plusieurs navigateurs peuvent observer le même flux JSON sans surcharger l'ESP.

---

## Compatibilité

- Toute l'API de la version synchrone est conservée (`getPageWeb()`, `streamPageWeb()`, `toJSON()`, `toCSV()`, etc.).
- Vous pouvez migrer progressivement : gardez `getPageWeb()` pour un endpoint legacy et ajoutez les endpoints asynchrones en parallèle.

---

## Remarques

- Le navigateur doit avoir accès à Internet pour charger le CDN Google Charts (`www.gstatic.com`).
- L'intervalle de rafraîchissement est configurable via `setRefreshInterval()`. Ne descendez pas en dessous de 200 ms sur un ESP8266 pour préserver la stabilité du serveur web.
- Le template génère du HTML5 valide avec un CSS minimal intégré.
