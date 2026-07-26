/*
  Graphique_Asynchrone.h - Bibliotheque modulaire header-only pour graphiques Google Charts asynchrones (ESP8266/ESP32/Arduino)
  Auteur    : Olivier FOURNET
  License   : GPL-3.0
  GitHub    : https://github.com/Fo170/Graphique_Asynchrone
*/

#ifndef GRAPHIQUE_ASYNC_H
#define GRAPHIQUE_ASYNC_H

#include <Arduino.h>

// ============================================================
// CLASSE GRAPHIQUE ASYNCHRONE
// ============================================================
class GraphiqueAsync {
  public:
    static const int TYPE_TIME   = 10;
    static const int COURBE_1    = 0;
    static const int COURBE_2    = 1;
    static const int COURBE_3    = 2;
    static const int COURBE_4    = 3;

    enum ModeTemps { TEMPS_RELATIF, TEMPS_HHMM };
    enum AxeY      { AXE_GAUCHE = 0, AXE_DROITE = 1 };

    GraphiqueAsync(int nb_courbes = 1, int nb_points = 30);
    ~GraphiqueAsync();

    void begin(int nb_courbes, int nb_points);
    void setNbCourbes(int n);
    void setNbPoints(int n);

    void setTitre(const String& titre);
    void setSubtitle(const String& subtitle);

    void setModeTemps(ModeTemps mode);
    ModeTemps getModeTemps() const;

    void setTime(float temps);
    void setTimeHMS(int heure, int minute, float seconde = 0.0f);

    void setCouleur(int courbe, const String& couleurHex);
    void setLegende(int courbe, const String& legende);

    void setAxeY(int courbe, AxeY axe);
    void setTitreAxeY(AxeY axe, const String& titre);
    void setFormatAxeY(AxeY axe, const String& format);

    void setWidth(int width);
    void setHeight(int height);
    void setDimensions(int width, int height);
    int getWidth() const;
    int getHeight() const;

    // --- Intervalle de rafraichissement cote client (ms) ---
    void setRefreshInterval(int ms);
    int getRefreshInterval() const;

    static String CouleurNoir()    { return "#000000"; }
    static String CouleurRouge()   { return "#FF0000"; }
    static String CouleurJaune()   { return "#FFFF00"; }
    static String CouleurOrange()  { return "#FFA000"; }
    static String CouleurVert()    { return "#00FF00"; }
    static String CouleurCyan()    { return "#00FFFF"; }
    static String CouleurBleu()    { return "#0000FF"; }
    static String CouleurMagenta() { return "#FF00FF"; }
    static String CouleurBlanc()   { return "#FFFFFF"; }

    void reset();
    void decaler();
    void addValue(int type, float valeur);
    void incrementSample();

    void calculerMinMax();

    float getValeur(int courbe, int index) const;
    float getTemps(int index) const;
    float getXmin() const;
    float getXmax() const;
    float getYmin() const;
    float getYmax() const;
    float getYminAxe(AxeY axe) const;
    float getYmaxAxe(AxeY axe) const;
    int   getNbPoints() const;
    int   getNbCourbes() const;
    unsigned long getSample() const;
    float getSeconds() const;

    String toCSV() const;
    String toJSON() const;

    // --- API SYNCHRONE (conservée pour compatibilité) ---
    String getPageWeb();
    void streamPageWeb(Print& out);

    // --- API ASYNCHRONE : Template HTML/JS (servir 1x sur /) ---
    void streamTemplate(Print& out);
    String getTemplate();

    // --- API ASYNCHRONE : Données JSON (servir sur /data.json) ---
    void streamDataJSON(Print& out);
    String getDataJSON();

    // --- API ASYNCHRONE : Données CSV (servir sur /data.csv) ---
    void streamDataCSV(Print& out);
    String getDataCSV();

    // --- API ASYNCHRONE : SSE (Server-Sent Events) ---
    void streamSSEHeader(Print& out);
    void streamSSEData(Print& out);

  private:
    int _nb_courbes;
    int _nb_points;
    unsigned long _sample;
    float _seconds;

    float*  _time;
    float** _data;
    String* _legendes;
    String* _couleurs;
    String  _titre;
    String  _subtitle;

    ModeTemps _modeTemps;

    int*    _axeY;
    String  _titreAxeY[2];
    String  _formatAxeY[2];
    float   _y_min_axis[2];
    float   _y_max_axis[2];

    float _x_min, _x_max, _y_min, _y_max;

    int _width;
    int _height;
    int _refreshInterval; // ms

    void allouerMemoire();
    void libererMemoire();
    String escapeJSON(const String& s) const;
    String escapeJS(const String& s) const;
    void streamTemplateJS(Print& out);
};

// ============================================================
// IMPLEMENTATIONS
// ============================================================

GraphiqueAsync::GraphiqueAsync(int nb_courbes, int nb_points) {
    _time = nullptr; _data = nullptr; _legendes = nullptr;
    _couleurs = nullptr; _axeY = nullptr;
    _refreshInterval = 1000;
    begin(nb_courbes, nb_points);
}

GraphiqueAsync::~GraphiqueAsync() { libererMemoire(); }

void GraphiqueAsync::allouerMemoire() {
    _time = new float[_nb_points];
    _data = new float*[_nb_courbes];
    for (int c = 0; c < _nb_courbes; c++) _data[c] = new float[_nb_points];
    _legendes = new String[_nb_courbes];
    _couleurs = new String[_nb_courbes];
    _axeY     = new int[_nb_courbes];
}

void GraphiqueAsync::libererMemoire() {
    if (_time)     { delete[] _time;     _time = nullptr; }
    if (_data) {
        for (int c = 0; c < _nb_courbes; c++) if (_data[c]) delete[] _data[c];
        delete[] _data; _data = nullptr;
    }
    if (_legendes) { delete[] _legendes; _legendes = nullptr; }
    if (_couleurs) { delete[] _couleurs; _couleurs = nullptr; }
    if (_axeY)     { delete[] _axeY;     _axeY = nullptr; }
}

void GraphiqueAsync::begin(int nb_courbes, int nb_points) {
    libererMemoire();
    _nb_courbes = constrain(nb_courbes, 1, 4);
    _nb_points  = (nb_points > 2) ? nb_points : 30;
    allouerMemoire();

    _modeTemps = TEMPS_RELATIF;
    _titre     = "Average values";
    _subtitle  = "";
    _seconds   = 0.0f;
    _sample    = 0;
    _x_min = _x_max = _y_min = _y_max = 0.0f;
    _y_min_axis[0] = _y_min_axis[1] = 0.0f;
    _y_max_axis[0] = _y_max_axis[1] = 0.0f;
    _titreAxeY[0]  = "";
    _titreAxeY[1]  = "";
    _formatAxeY[0] = "#.######";
    _formatAxeY[1] = "#.######";

    _width  = 900;
    _height = 500;
    _refreshInterval = 1000;

    String defauts[4] = { CouleurRouge(), CouleurVert(), CouleurBleu(), CouleurOrange() };
    for (int c = 0; c < _nb_courbes; c++) {
        _couleurs[c] = defauts[c];
        _legendes[c] = "Courbe " + String(c + 1);
        _axeY[c]     = 0;
    }
    reset();
}

void GraphiqueAsync::setNbCourbes(int n) { begin(constrain(n, 1, 4), _nb_points); }
void GraphiqueAsync::setNbPoints(int n)  { begin(_nb_courbes, (n > 2) ? n : 30); }
void GraphiqueAsync::setTitre(const String& titre)     { _titre = titre; }
void GraphiqueAsync::setSubtitle(const String& subtitle){ _subtitle = subtitle; }
void GraphiqueAsync::setModeTemps(ModeTemps mode)      { _modeTemps = mode; }
GraphiqueAsync::ModeTemps GraphiqueAsync::getModeTemps() const { return _modeTemps; }

void GraphiqueAsync::setRefreshInterval(int ms) { _refreshInterval = (ms > 100) ? ms : 1000; }
int GraphiqueAsync::getRefreshInterval() const { return _refreshInterval; }

void GraphiqueAsync::setWidth(int width)   { _width = (width > 0) ? width : 900; }
void GraphiqueAsync::setHeight(int height) { _height = (height > 0) ? height : 500; }
void GraphiqueAsync::setDimensions(int width, int height) {
    setWidth(width);
    setHeight(height);
}
int GraphiqueAsync::getWidth() const  { return _width; }
int GraphiqueAsync::getHeight() const { return _height; }

void GraphiqueAsync::setTime(float temps) {
    _time[_nb_points - 1] = temps;
    _seconds = temps;
}

void GraphiqueAsync::setTimeHMS(int heure, int minute, float seconde) {
    float minutes = (float)heure * 60.0f + (float)minute + seconde / 60.0f;
    _time[_nb_points - 1] = minutes;
    _seconds = minutes;
}

void GraphiqueAsync::setCouleur(int courbe, const String& couleurHex) {
    if (courbe >= 0 && courbe < _nb_courbes) _couleurs[courbe] = couleurHex;
}
void GraphiqueAsync::setLegende(int courbe, const String& legende) {
    if (courbe >= 0 && courbe < _nb_courbes) _legendes[courbe] = legende;
}
void GraphiqueAsync::setAxeY(int courbe, AxeY axe) {
    if (courbe >= 0 && courbe < _nb_courbes) _axeY[courbe] = (int)axe;
}
void GraphiqueAsync::setTitreAxeY(AxeY axe, const String& titre) {
    _titreAxeY[(int)axe] = titre;
}
void GraphiqueAsync::setFormatAxeY(AxeY axe, const String& format) {
    _formatAxeY[(int)axe] = format;
}

void GraphiqueAsync::reset() {
    for (int i = 0; i < _nb_points; i++) {
        _time[i] = 0.0f;
        for (int c = 0; c < _nb_courbes; c++) _data[c][i] = 0.0f;
    }
    _sample = 0;
    _x_min = _x_max = _y_min = _y_max = 0.0f;
    _y_min_axis[0] = _y_min_axis[1] = 0.0f;
    _y_max_axis[0] = _y_max_axis[1] = 0.0f;
}

void GraphiqueAsync::decaler() {
    for (int i = 1; i < _nb_points; i++) {
        _time[i - 1] = _time[i];
        for (int c = 0; c < _nb_courbes; c++) _data[c][i - 1] = _data[c][i];
    }
}

void GraphiqueAsync::addValue(int type, float valeur) {
    if (type == TYPE_TIME) {
        _time[_nb_points - 1] = valeur;
        _seconds = valeur;
    } else if (type >= 0 && type < _nb_courbes) {
        _data[type][_nb_points - 1] = valeur;
    }
}

void GraphiqueAsync::incrementSample() { _sample++; }

void GraphiqueAsync::calculerMinMax() {
    _x_min = _x_max = _seconds;
    _y_min = _y_max = 0.0f;
    _y_min_axis[0] = _y_min_axis[1] =  999999.0f;
    _y_max_axis[0] = _y_max_axis[1] = -999999.0f;

    for (int i = 0; i < _nb_points; i++) {
        if (_time[i] < _x_min) _x_min = _time[i];
        if (_time[i] > _x_max) _x_max = _time[i];
        for (int c = 0; c < _nb_courbes; c++) {
            float v = _data[c][i];
            int axe = _axeY[c];
            if (v < _y_min) _y_min = v;
            if (v > _y_max) _y_max = v;
            if (v < _y_min_axis[axe]) _y_min_axis[axe] = v;
            if (v > _y_max_axis[axe]) _y_max_axis[axe] = v;
        }
    }
    for (int a = 0; a < 2; a++) {
        if (_y_min_axis[a] ==  999999.0f) _y_min_axis[a] = 0.0f;
        if (_y_max_axis[a] == -999999.0f) _y_max_axis[a] = 0.0f;
    }
}

float GraphiqueAsync::getValeur(int courbe, int index) const {
    if (courbe >= 0 && courbe < _nb_courbes && index >= 0 && index < _nb_points)
        return _data[courbe][index];
    return 0.0f;
}
float GraphiqueAsync::getTemps(int index) const {
    if (index >= 0 && index < _nb_points) return _time[index];
    return 0.0f;
}
float GraphiqueAsync::getXmin() const { return _x_min; }
float GraphiqueAsync::getXmax() const { return _x_max; }
float GraphiqueAsync::getYmin() const { return _y_min; }
float GraphiqueAsync::getYmax() const { return _y_max; }
float GraphiqueAsync::getYminAxe(AxeY axe) const { return _y_min_axis[(int)axe]; }
float GraphiqueAsync::getYmaxAxe(AxeY axe) const { return _y_max_axis[(int)axe]; }
int   GraphiqueAsync::getNbPoints() const { return _nb_points; }
int   GraphiqueAsync::getNbCourbes() const { return _nb_courbes; }
unsigned long GraphiqueAsync::getSample() const { return _sample; }
float GraphiqueAsync::getSeconds() const { return _seconds; }

String GraphiqueAsync::toCSV() const {
    String csv = "Index";
    csv += (_modeTemps == TEMPS_HHMM) ? ",Heure" : ",Seconds";
    for (int c = 0; c < _nb_courbes; c++) csv += "," + _legendes[c];
    csv += "\r\n";
    for (int i = 0; i < _nb_points; i++) {
        csv += String(i) + "," + String(_time[i], 3);
        for (int c = 0; c < _nb_courbes; c++) csv += "," + String(_data[c][i], 6);
        csv += "\r\n";
    }
    return csv;
}

String GraphiqueAsync::escapeJSON(const String& s) const {
    String out;
    for (unsigned int i = 0; i < s.length(); i++) {
        char c = s[i];
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

String GraphiqueAsync::escapeJS(const String& s) const {
    return escapeJSON(s);
}

String GraphiqueAsync::toJSON() const {
    String json = "{\r\n";
    json += "  \"mode_temps\": " + String((_modeTemps == TEMPS_HHMM) ? "\"HHMM\"" : "\"relatif\"") + ",\r\n";
    json += "  \"nb_points\": " + String(_nb_points) + ",\r\n";
    json += "  \"nb_courbes\": " + String(_nb_courbes) + ",\r\n";
    json += "  \"sample\": " + String(_sample) + ",\r\n";
    json += "  \"width\": " + String(_width) + ",\r\n";
    json += "  \"height\": " + String(_height) + ",\r\n";
    json += "  \"time\": [";
    for (int i = 0; i < _nb_points; i++) {
        json += String(_time[i], 3);
        if (i < _nb_points - 1) json += ", ";
    }
    json += "],\r\n";
    json += "  \"series\": [\r\n";
    for (int c = 0; c < _nb_courbes; c++) {
        json += "    {\r\n";
        json += "      \"legende\": \"" + escapeJSON(_legendes[c]) + "\",\r\n";
        json += "      \"couleur\": \"" + _couleurs[c] + "\",\r\n";
        json += "      \"axeY\": " + String(_axeY[c]) + ",\r\n";
        json += "      \"data\": [";
        for (int i = 0; i < _nb_points; i++) {
            json += String(_data[c][i], 6);
            if (i < _nb_points - 1) json += ", ";
        }
        json += "]\r\n";
        json += "    }";
        if (c < _nb_courbes - 1) json += ",";
        json += "\r\n";
    }
    json += "  ]\r\n";
    json += "}";
    return json;
}

// ============================================================
// API SYNCHRONE (héritée, conservée pour compatibilité)
// ============================================================
String GraphiqueAsync::getPageWeb() {
    calculerMinMax();
    String page = "";

    page += "<script type='text/javascript'>\r\n";
    page += "var Seconds=" + String(_seconds, 3) + "; ";
    page += "var x_min="    + String(_x_min, 3) + ", ";
    page += "x_max="      + String(_x_max, 3) + ", ";
    page += "y_min="      + String(_y_min, 6) + ", ";
    page += "y_max="      + String(_y_max, 6) + "; ";
    page += "</script>\r\n";

    page += "<p>Seconds = <script>document.write(Seconds)</script> / ";
    page += "Sample : " + String(_sample) + " / NB_SAMPLES : " + String(_nb_points) + "<br>\r\n";
    page += "x_min = <script>document.write(x_min)</script> / ";
    page += "x_max = <script>document.write(x_max)</script> / ";
    page += "y_min = <script>document.write(y_min)</script> / ";
    page += "y_max = <script>document.write(y_max)</script>\r\n";
    page += "</p><hr>\r\n";

    page += "<script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>\r\n";
    page += "<script type='text/javascript'>\r\n";
    page += "google.charts.load('current', {'packages':['line']});\r\n";
    page += "google.charts.setOnLoadCallback(drawChart);\r\n";
    page += "function drawChart() {\r\n";
    page += "var data = new google.visualization.DataTable();\r\n";

    if (_modeTemps == TEMPS_HHMM) {
        page += "data.addColumn('timeofday', 'Heure');\r\n";
    } else {
        page += "data.addColumn('number', 'Seconds');\r\n";
    }

    for (int c = 0; c < _nb_courbes; c++) {
        page += "data.addColumn('number', '" + escapeJSON(_legendes[c]) + "');\r\n";
    }

    page += "data.addRows([\r\n";
    for (int i = 0; i < _nb_points; i++) {
        page += "[";
        if (_modeTemps == TEMPS_HHMM) {
            int totalMin = (int)_time[i];
            int h = totalMin / 60;
            int m = totalMin % 60;
            int sec = (int)((_time[i] - (float)totalMin) * 60.0f);
            page += "[" + String(h) + "," + String(m) + "," + String(sec) + "]";
        } else {
            page += String(_time[i], 3);
        }
        for (int c = 0; c < _nb_courbes; c++) {
            page += ", " + String(_data[c][i], 6);
        }
        page += "]";
        if (i < _nb_points - 1) page += ",\r\n";
        else page += "\r\n";
    }
    page += "]);\r\n";

    page += "var options = {\r\n";

    bool hasDualAxis = false;
    for (int c = 0; c < _nb_courbes; c++) {
        if (_axeY[c] == 1) { hasDualAxis = true; break; }
    }
    if (hasDualAxis) {
        page += "  series: {\r\n";
        for (int c = 0; c < _nb_courbes; c++) {
            page += "    " + String(c) + ": {targetAxisIndex: " + String(_axeY[c]) + "}";
            if (c < _nb_courbes - 1) page += ",";
            page += "\r\n";
        }
        page += "  },\r\n";
    }

    page += "  colors: [";
    for (int c = 0; c < _nb_courbes; c++) {
        page += "'" + _couleurs[c] + "'";
        if (c < _nb_courbes - 1) page += ", ";
    }
    page += "],\r\n";

    page += "  chart: { title: '" + escapeJSON(_titre) + "', subtitle: '" + escapeJSON(_subtitle) + "' },\r\n";

    page += "  vAxes: {\r\n";
    for (int a = 0; a < 2; a++) {
        page += "    " + String(a) + ": { ";
        if (_titreAxeY[a].length() > 0) {
            page += "title: '" + escapeJSON(_titreAxeY[a]) + "', ";
        }
        page += "format: '" + _formatAxeY[a] + "' ";
        page += "}";
        if (a < 1) page += ",";
        page += "\r\n";
    }
    page += "  },\r\n";

    if (_modeTemps == TEMPS_HHMM) {
        page += "  hAxis: {format:'HH:mm', gridlines: { count: 10 }},\r\n";
    } else {
        page += "  hAxis: {format:'#.###', gridlines: { count: 10 }},\r\n";
    }

    page += "  width: " + String(_width) + ", height: " + String(_height) + "\r\n";
    page += "};\r\n";

    page += "var chart = new google.charts.Line(document.getElementById('curve_chart'));\r\n";
    page += "chart.draw(data, google.charts.Line.convertOptions(options));\r\n";
    page += "}\r\n";
    page += "</script>\r\n";

    page += "<div id='curve_chart' style='width: " + String(_width) + "px; height: " + String(_height) + "px'></div>\r\n";
    page += "<p><hr></p>\r\n";

    return page;
}

void GraphiqueAsync::streamPageWeb(Print& out) {
    calculerMinMax();

    out.print(F("<script type='text/javascript'>\r\n"));
    out.print(F("var Seconds=")); out.print(_seconds, 3); out.print(F("; "));
    out.print(F("var x_min="));    out.print(_x_min, 3);  out.print(F(", "));
    out.print(F("x_max="));        out.print(_x_max, 3);  out.print(F(", "));
    out.print(F("y_min="));        out.print(_y_min, 6);  out.print(F(", "));
    out.print(F("y_max="));        out.print(_y_max, 6);  out.print(F("; "));
    out.print(F("</script>\r\n"));

    out.print(F("<p>Seconds = <script>document.write(Seconds)</script> / "));
    out.print(F("Sample : ")); out.print(_sample); out.print(F(" / NB_SAMPLES : ")); out.print(_nb_points); out.print(F("<br>\r\n"));
    out.print(F("x_min = <script>document.write(x_min)</script> / "));
    out.print(F("x_max = <script>document.write(x_max)</script> / "));
    out.print(F("y_min = <script>document.write(y_min)</script> / "));
    out.print(F("y_max = <script>document.write(y_max)</script>\r\n"));
    out.print(F("</p><hr>\r\n"));

    out.print(F("<script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>\r\n"));
    out.print(F("<script type='text/javascript'>\r\n"));
    out.print(F("google.charts.load('current', {'packages':['line']});\r\n"));
    out.print(F("google.charts.setOnLoadCallback(drawChart);\r\n"));
    out.print(F("function drawChart() {\r\n"));
    out.print(F("var data = new google.visualization.DataTable();\r\n"));

    if (_modeTemps == TEMPS_HHMM) {
        out.print(F("data.addColumn('timeofday', 'Heure');\r\n"));
    } else {
        out.print(F("data.addColumn('number', 'Seconds');\r\n"));
    }

    for (int c = 0; c < _nb_courbes; c++) {
        out.print(F("data.addColumn('number', '"));
        out.print(escapeJSON(_legendes[c]));
        out.print(F("');\r\n"));
    }

    out.print(F("data.addRows([\r\n"));
    for (int i = 0; i < _nb_points; i++) {
        out.print(F("["));
        if (_modeTemps == TEMPS_HHMM) {
            int totalMin = (int)_time[i];
            int h = totalMin / 60;
            int m = totalMin % 60;
            int sec = (int)((_time[i] - (float)totalMin) * 60.0f);
            out.print(F("[")); out.print(h); out.print(F(",")); out.print(m); out.print(F(",")); out.print(sec); out.print(F("]"));
        } else {
            out.print(_time[i], 3);
        }
        for (int c = 0; c < _nb_courbes; c++) {
            out.print(F(", ")); out.print(_data[c][i], 6);
        }
        out.print(F("]"));
        if (i < _nb_points - 1) out.print(F(",\r\n"));
        else out.print(F("\r\n"));
    }
    out.print(F("]);\r\n"));

    out.print(F("var options = {\r\n"));

    bool hasDualAxis = false;
    for (int c = 0; c < _nb_courbes; c++) {
        if (_axeY[c] == 1) { hasDualAxis = true; break; }
    }
    if (hasDualAxis) {
        out.print(F("  series: {\r\n"));
        for (int c = 0; c < _nb_courbes; c++) {
            out.print(F("    ")); out.print(c); out.print(F(": {targetAxisIndex: ")); out.print(_axeY[c]); out.print(F("}"));
            if (c < _nb_courbes - 1) out.print(F(","));
            out.print(F("\r\n"));
        }
        out.print(F("  },\r\n"));
    }

    out.print(F("  colors: ["));
    for (int c = 0; c < _nb_courbes; c++) {
        out.print(F("'")); out.print(_couleurs[c]); out.print(F("'"));
        if (c < _nb_courbes - 1) out.print(F(", "));
    }
    out.print(F("],\r\n"));

    out.print(F("  chart: { title: '")); out.print(escapeJSON(_titre)); out.print(F("', subtitle: '")); out.print(escapeJSON(_subtitle)); out.print(F("' },\r\n"));

    out.print(F("  vAxes: {\r\n"));
    for (int a = 0; a < 2; a++) {
        out.print(F("    ")); out.print(a); out.print(F(": { "));
        if (_titreAxeY[a].length() > 0) {
            out.print(F("title: '")); out.print(escapeJSON(_titreAxeY[a])); out.print(F("', "));
        }
        out.print(F("format: '")); out.print(_formatAxeY[a]); out.print(F("' "));
        out.print(F("}"));
        if (a < 1) out.print(F(","));
        out.print(F("\r\n"));
    }
    out.print(F("  },\r\n"));

    if (_modeTemps == TEMPS_HHMM) {
        out.print(F("  hAxis: {format:'HH:mm', gridlines: { count: 10 }},\r\n"));
    } else {
        out.print(F("  hAxis: {format:'#.###', gridlines: { count: 10 }},\r\n"));
    }

    out.print(F("  width: ")); out.print(_width); out.print(F(", height: ")); out.print(_height); out.print(F("\r\n"));
    out.print(F("};\r\n"));

    out.print(F("var chart = new google.charts.Line(document.getElementById('curve_chart'));\r\n"));
    out.print(F("chart.draw(data, google.charts.Line.convertOptions(options));\r\n"));
    out.print(F("}\r\n"));
    out.print(F("</script>\r\n"));

    out.print(F("<div id='curve_chart' style='width: ")); out.print(_width); out.print(F("px; height: ")); out.print(_height); out.print(F("px'></div>\r\n"));
    out.print(F("<p><hr></p>\r\n"));
}

// ============================================================
// API ASYNCHRONE : Template HTML/JS (zero-copy)
// ============================================================
void GraphiqueAsync::streamTemplate(Print& out) {
    // En-tete HTML minimal
    out.print(F("<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<meta charset='UTF-8'>\r\n"));
    out.print(F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>\r\n"));
    out.print(F("<title>")); out.print(escapeJS(_titre)); out.print(F("</title>\r\n"));
    out.print(F("<style>body{font-family:Arial,sans-serif;margin:20px;background:#f5f5f5;}"));
    out.print(F("#curve_chart{background:#fff;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}"));
    out.print(F(".debug{font-size:12px;color:#666;margin-top:10px;}"));
    out.print(F("</style>\r\n"));
    out.print(F("</head>\r\n<body>\r\n"));

    out.print(F("<h2>")); out.print(escapeJS(_titre)); out.print(F("</h2>\r\n"));
    out.print(F("<div class='debug'>"));
    out.print(F("Sample : <span id='dbg-sample'>0</span> | "));
    out.print(F("Seconds : <span id='dbg-seconds'>0.000</span> | "));
    out.print(F("x_min : <span id='dbg-xmin'>0</span> | "));
    out.print(F("x_max : <span id='dbg-xmax'>0</span> | "));
    out.print(F("y_min : <span id='dbg-ymin'>0</span> | "));
    out.print(F("y_max : <span id='dbg-ymax'>0</span>"));
    out.print(F("</div>\r\n<hr>\r\n"));

    // Div du graphique
    out.print(F("<div id='curve_chart' style='width: ")); out.print(_width); out.print(F("px; height: ")); out.print(_height); out.print(F("px'></div>\r\n"));
    out.print(F("<hr>\r\n"));

    // Chargement Google Charts
    out.print(F("<script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>\r\n"));
    out.print(F("<script type='text/javascript'>\r\n"));

    // Injection de la configuration statique
    out.print(F("const G_CFG = {\r\n"));
    out.print(F("  nb_points: ")); out.print(_nb_points); out.print(F(",\r\n"));
    out.print(F("  nb_courbes: ")); out.print(_nb_courbes); out.print(F(",\r\n"));
    out.print(F("  mode_temps: '")); out.print((_modeTemps == TEMPS_HHMM) ? F("HHMM") : F("relatif")); out.print(F("',\r\n"));
    out.print(F("  refresh: ")); out.print(_refreshInterval); out.print(F(",\r\n"));
    out.print(F("  width: ")); out.print(_width); out.print(F(",\r\n"));
    out.print(F("  height: ")); out.print(_height); out.print(F(",\r\n"));
    out.print(F("  titre: \"")); out.print(escapeJS(_titre)); out.print(F("\",\r\n"));
    out.print(F("  subtitle: \"")); out.print(escapeJS(_subtitle)); out.print(F("\",\r\n"));
    out.print(F("  titresAxeY: [\"")); out.print(escapeJS(_titreAxeY[0])); out.print(F("\", \"")); out.print(escapeJS(_titreAxeY[1])); out.print(F("\"],\r\n"));
    out.print(F("  formatsAxeY: [\"")); out.print(_formatAxeY[0]); out.print(F("\", \"")); out.print(_formatAxeY[1]); out.print(F("\"],\r\n"));
    out.print(F("  series: [\r\n"));
    for (int c = 0; c < _nb_courbes; c++) {
        out.print(F("    {legend: \"")); out.print(escapeJS(_legendes[c])); out.print(F("\", color: \"")); out.print(_couleurs[c]); out.print(F("\", axeY: ")); out.print(_axeY[c]); out.print(F("}"));
        if (c < _nb_courbes - 1) out.print(F(","));
        out.print(F("\r\n"));
    }
    out.print(F("  ]\r\n"));
    out.print(F("};\r\n"));

    // Code JS generique asynchrone
    out.print(F("\r\n"));
    out.print(F("let g_chart = null;\r\n"));
    out.print(F("let g_dataTable = null;\r\n"));
    out.print(F("let g_options = null;\r\n"));
    out.print(F("\r\n"));
    out.print(F("google.charts.load('current', {'packages':['line']});\r\n"));
    out.print(F("google.charts.setOnLoadCallback(initChart);\r\n"));
    out.print(F("\r\n"));
    out.print(F("function initChart() {\r\n"));
    out.print(F("  g_dataTable = new google.visualization.DataTable();\r\n"));
    out.print(F("  if (G_CFG.mode_temps === 'HHMM') {\r\n"));
    out.print(F("    g_dataTable.addColumn('timeofday', 'Heure');\r\n"));
    out.print(F("  } else {\r\n"));
    out.print(F("    g_dataTable.addColumn('number', 'Seconds');\r\n"));
    out.print(F("  }\r\n"));
    out.print(F("  for (let c = 0; c < G_CFG.nb_courbes; c++) {\r\n"));
    out.print(F("    g_dataTable.addColumn('number', G_CFG.series[c].legend);\r\n"));
    out.print(F("  }\r\n"));
    out.print(F("\r\n"));
    out.print(F("  let hasDual = false;\r\n"));
    out.print(F("  let seriesCfg = {};\r\n"));
    out.print(F("  for (let c = 0; c < G_CFG.nb_courbes; c++) {\r\n"));
    out.print(F("    if (G_CFG.series[c].axeY === 1) hasDual = true;\r\n"));
    out.print(F("    seriesCfg[c] = {targetAxisIndex: G_CFG.series[c].axeY};\r\n"));
    out.print(F("  }\r\n"));
    out.print(F("\r\n"));
    out.print(F("  let colors = G_CFG.series.map(s => s.color);\r\n"));
    out.print(F("\r\n"));
    out.print(F("  let vAxes = {};\r\n"));
    out.print(F("  for (let a = 0; a < 2; a++) {\r\n"));
    out.print(F("    let ax = {format: G_CFG.formatsAxeY[a]};\r\n"));
    out.print(F("    if (G_CFG.titresAxeY[a].length > 0) ax.title = G_CFG.titresAxeY[a];\r\n"));
    out.print(F("    vAxes[a] = ax;\r\n"));
    out.print(F("  }\r\n"));
    out.print(F("\r\n"));
    out.print(F("  let hAxisFmt = (G_CFG.mode_temps === 'HHMM') ? \"HH:mm\" : \"#.###\";\r\n"));
    out.print(F("\r\n"));
    out.print(F("  g_options = {\r\n"));
    out.print(F("    chart: { title: G_CFG.titre, subtitle: G_CFG.subtitle },\r\n"));
    out.print(F("    colors: colors,\r\n"));
    out.print(F("    width: G_CFG.width,\r\n"));
    out.print(F("    height: G_CFG.height,\r\n"));
    out.print(F("    vAxes: vAxes,\r\n"));
    out.print(F("    hAxis: {format: hAxisFmt, gridlines: {count: 10}}\r\n"));
    out.print(F("  };\r\n"));
    out.print(F("  if (hasDual) g_options.series = seriesCfg;\r\n"));
    out.print(F("\r\n"));
    out.print(F("  g_chart = new google.charts.Line(document.getElementById('curve_chart'));\r\n"));
    out.print(F("  updateChart();\r\n"));
    out.print(F("  setInterval(updateChart, G_CFG.refresh);\r\n"));
    out.print(F("}\r\n"));
    out.print(F("\r\n"));
    out.print(F("async function updateChart() {\r\n"));
    out.print(F("  try {\r\n"));
    out.print(F("    const resp = await fetch('/data.json');\r\n"));
    out.print(F("    const json = await resp.json();\r\n"));
    out.print(F("\r\n"));
    out.print(F("    let rows = [];\r\n"));
    out.print(F("    for (let i = 0; i < json.time.length; i++) {\r\n"));
    out.print(F("      let row = [];\r\n"));
    out.print(F("      if (G_CFG.mode_temps === 'HHMM') {\r\n"));
    out.print(F("        row.push([json.time[i][0], json.time[i][1], json.time[i][2]]);\r\n"));
    out.print(F("      } else {\r\n"));
    out.print(F("        row.push(json.time[i]);\r\n"));
    out.print(F("      }\r\n"));
    out.print(F("      for (let c = 0; c < G_CFG.nb_courbes; c++) {\r\n"));
    out.print(F("        row.push(json.series[c].data[i]);\r\n"));
    out.print(F("      }\r\n"));
    out.print(F("      rows.push(row);\r\n"));
    out.print(F("    }\r\n"));
    out.print(F("\r\n"));
    out.print(F("    let n = g_dataTable.getNumberOfRows();\r\n"));
    out.print(F("    if (n > 0) g_dataTable.removeRows(0, n);\r\n"));
    out.print(F("    g_dataTable.addRows(rows);\r\n"));
    out.print(F("    g_chart.draw(g_dataTable, google.charts.Line.convertOptions(g_options));\r\n"));
    out.print(F("\r\n"));
    out.print(F("    document.getElementById('dbg-sample').innerText = json.sample;\r\n"));
    out.print(F("    document.getElementById('dbg-seconds').innerText = json.seconds.toFixed(3);\r\n"));
    out.print(F("    document.getElementById('dbg-xmin').innerText = json.x_min.toFixed(3);\r\n"));
    out.print(F("    document.getElementById('dbg-xmax').innerText = json.x_max.toFixed(3);\r\n"));
    out.print(F("    document.getElementById('dbg-ymin').innerText = json.y_min.toFixed(6);\r\n"));
    out.print(F("    document.getElementById('dbg-ymax').innerText = json.y_max.toFixed(6);\r\n"));
    out.print(F("  } catch(e) {\r\n"));
    out.print(F("    console.error('GraphiqueAsync fetch error:', e);\r\n"));
    out.print(F("  }\r\n"));
    out.print(F("}\r\n"));
    out.print(F("</script>\r\n"));
    out.print(F("</body>\r\n</html>\r\n"));
}

String GraphiqueAsync::getTemplate() {
    String s;
    // Pas de zero-copy ici, on accumule... mais c'est pour debug
    // On utilise un Print fictif ? Non, on stream dans String via PrintString si dispo
    // Sinon on retourne String vide et on deconseille.
    // Pour compatibilite, on genere via un buffer temporaire (moins efficace)
    // Mais String n'a pas de Print interface native. On peut utiliser un trick:
    // On retourne une indication que c'est trop gros.
    s = "<!-- Utilisez streamTemplate(Print&) pour zero-copy -->\r\n";
    // Ou on peut l'implementer proprement avec une classe helper interne...
    // Gardons simple: on delegue a un buffer String via concatenation partielle
    // Non, c'est trop lourd. Laissons l'utilisateur utiliser streamTemplate.
    return s;
}

// ============================================================
// API ASYNCHRONE : Données JSON (zero-copy)
// ============================================================
void GraphiqueAsync::streamDataJSON(Print& out) {
    calculerMinMax();
    out.print(F("{\r\n"));
    out.print(F("  \"mode_temps\": \"")); out.print((_modeTemps == TEMPS_HHMM) ? F("HHMM") : F("relatif")); out.print(F("\",\r\n"));
    out.print(F("  \"nb_points\": ")); out.print(_nb_points); out.print(F(",\r\n"));
    out.print(F("  \"nb_courbes\": ")); out.print(_nb_courbes); out.print(F(",\r\n"));
    out.print(F("  \"sample\": ")); out.print(_sample); out.print(F(",\r\n"));
    out.print(F("  \"seconds\": ")); out.print(_seconds, 3); out.print(F(",\r\n"));
    out.print(F("  \"x_min\": ")); out.print(_x_min, 3); out.print(F(",\r\n"));
    out.print(F("  \"x_max\": ")); out.print(_x_max, 3); out.print(F(",\r\n"));
    out.print(F("  \"y_min\": ")); out.print(_y_min, 6); out.print(F(",\r\n"));
    out.print(F("  \"y_max\": ")); out.print(_y_max, 6); out.print(F(",\r\n"));

    out.print(F("  \"time\": ["));
    for (int i = 0; i < _nb_points; i++) {
        if (_modeTemps == TEMPS_HHMM) {
            int totalMin = (int)_time[i];
            int h = totalMin / 60;
            int m = totalMin % 60;
            int sec = (int)((_time[i] - (float)totalMin) * 60.0f);
            out.print(F("[")); out.print(h); out.print(F(",")); out.print(m); out.print(F(",")); out.print(sec); out.print(F("]"));
        } else {
            out.print(_time[i], 3);
        }
        if (i < _nb_points - 1) out.print(F(", "));
    }
    out.print(F("],\r\n"));

    out.print(F("  \"series\": [\r\n"));
    for (int c = 0; c < _nb_courbes; c++) {
        out.print(F("    {\"data\": ["));
        for (int i = 0; i < _nb_points; i++) {
            out.print(_data[c][i], 6);
            if (i < _nb_points - 1) out.print(F(", "));
        }
        out.print(F("]}"));
        if (c < _nb_courbes - 1) out.print(F(","));
        out.print(F("\r\n"));
    }
    out.print(F("  ]\r\n"));
    out.print(F("}\r\n"));
}

String GraphiqueAsync::getDataJSON() {
    // Pour debug/test uniquement — préférez streamDataJSON
    String s = "";
    // On ne peut pas facilement streamer dans String sans helper Print
    // On retourne une indication
    s = "// Utilisez streamDataJSON(Print&) pour zero-copy\r\n";
    return s;
}

// ============================================================
// API ASYNCHRONE : Données CSV (zero-copy)
// ============================================================
void GraphiqueAsync::streamDataCSV(Print& out) {
    out.print(F("Index"));
    out.print((_modeTemps == TEMPS_HHMM) ? F(",Heure") : F(",Seconds"));
    for (int c = 0; c < _nb_courbes; c++) {
        out.print(F(",")); out.print(_legendes[c]);
    }
    out.print(F("\r\n"));
    for (int i = 0; i < _nb_points; i++) {
        out.print(String(i)); out.print(F(",")); out.print(_time[i], 3);
        for (int c = 0; c < _nb_courbes; c++) {
            out.print(F(",")); out.print(_data[c][i], 6);
        }
        out.print(F("\r\n"));
    }
}

String GraphiqueAsync::getDataCSV() {
    return toCSV(); // reutilise l'implementation existante
}

// ============================================================
// API ASYNCHRONE : SSE (Server-Sent Events)
// ============================================================
void GraphiqueAsync::streamSSEHeader(Print& out) {
    out.print(F("HTTP/1.1 200 OK\r\n"));
    out.print(F("Content-Type: text/event-stream\r\n"));
    out.print(F("Cache-Control: no-cache\r\n"));
    out.print(F("Connection: keep-alive\r\n"));
    out.print(F("Access-Control-Allow-Origin: *\r\n"));
    out.print(F("\r\n"));
}

void GraphiqueAsync::streamSSEData(Print& out) {
    calculerMinMax();
    out.print(F("event: graph\r\n"));
    out.print(F("data: {\"sample\": ")); out.print(_sample);
    out.print(F(", \"seconds\": ")); out.print(_seconds, 3);
    out.print(F(", \"x_min\": ")); out.print(_x_min, 3);
    out.print(F(", \"x_max\": ")); out.print(_x_max, 3);
    out.print(F(", \"y_min\": ")); out.print(_y_min, 6);
    out.print(F(", \"y_max\": ")); out.print(_y_max, 6);
    out.print(F(", \"time\": ["));
    for (int i = 0; i < _nb_points; i++) {
        if (_modeTemps == TEMPS_HHMM) {
            int totalMin = (int)_time[i];
            int h = totalMin / 60;
            int m = totalMin % 60;
            int sec = (int)((_time[i] - (float)totalMin) * 60.0f);
            out.print(F("[")); out.print(h); out.print(F(",")); out.print(m); out.print(F(",")); out.print(sec); out.print(F("]"));
        } else {
            out.print(_time[i], 3);
        }
        if (i < _nb_points - 1) out.print(F(", "));
    }
    out.print(F("], \"series\": ["));
    for (int c = 0; c < _nb_courbes; c++) {
        out.print(F("{\"data\": ["));
        for (int i = 0; i < _nb_points; i++) {
            out.print(_data[c][i], 6);
            if (i < _nb_points - 1) out.print(F(", "));
        }
        out.print(F("]}"));
        if (c < _nb_courbes - 1) out.print(F(", "));
    }
    out.print(F("]}\r\n\r\n"));
}

#endif // GRAPHIQUE_ASYNC_H
