# AGENTS.md — Graphique_Asynchrone

Single-header Arduino library (`Graphique_Asynchrone.h`) for streaming Google Charts to ESP8266/ESP32 web clients. GPL-3.0.

## Architecture

- **Header-only**: just `#include <Graphique_Asynchrone.h>`. No `.cpp`, no build config, no tests.
- **Class `GraphiqueAsync`**: stores a ring buffer of `_nb_points` per curve. Supports 1–4 curves.
- **Two API modes** coexist:
  - **Sync (legacy)**: `getPageWeb()` / `streamPageWeb()` — embeds data in HTML, wasteful.
  - **Async (preferred)**: `streamTemplate()` → serve once on `/`; `streamDataJSON()` → serve on `/data.json` for client polling.
- Browser fetches `/data.json` every `_refreshInterval` ms (default 1000, min 200 on ESP8266).
- Requires client internet access for Google Charts CDN (`www.gstatic.com`).

## Endpoints

| Route | Handler | Notes |
|-------|---------|-------|
| `/` | `streamTemplate(out)` | Serve once. Injects static config + JS polling code. |
| `/data.json` | `streamDataJSON(out)` | Polled by client JS every N ms. |
| `/data.csv` | `streamDataCSV(out)` | Downloadable export. |

Also supports SSE: `streamSSEHeader(out)` → `streamSSEData(out)`.

## Critical gotchas

- **Always use `stream*()` methods** (zero-copy via `Print&`). `getTemplate()` and `getDataJSON()` are **stubs** that return placeholder strings — do not use.
- **Call order in `loop()`**: `decaler()` → `setTime()` → `addValue()` → `incrementSample()`. `decaler()` shifts the ring buffer left; then the last slot is written.
- **`streamDataJSON()` calls `calculerMinMax()` internally** — do NOT call it manually before streaming.
- **`setRefreshInterval()` clamps to min 100ms** (constructor default 1000ms). README says 200ms min on ESP8266.
- **`setNbCourbes()` / `setNbPoints()` call `begin()`** which reallocates memory and resets all data.
- Dual Y-axis via `setAxeY(courbe, AXE_GAUCHE/AXE_DROITE)` — per-curve.

## Style

- French naming in API (`couleur`, `legende`, `titre`, `courbe`, `decaler`) — respect it.
- String formatting: `String(val, N)` with N=3 for time, N=6 for data values.
- `TYPE_TIME` constant (value 10) used with `addValue()` for setting time values.
