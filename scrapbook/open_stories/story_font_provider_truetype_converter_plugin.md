# Story: FontProvider und TrueTypeFontConverter Plugin

Datum: 2026-05-15

## Kontext

VED-Text ist kein nativer Qt- oder GDI-Text. Textobjekte verwenden interne VED-Vektorfonts:

- `TDVecText` zeichnet `TDVecGlyph`.
- `TDVecGlyph` besteht aus `TDVecPolyCurve`-Konturen.
- `TDVecFont` enthaelt `TDVecCharacter` und deren Glyph-Geometrie.

Im alten Borland-VED wird ein TrueType-Systemfont in `TDFontManager::CreateVecFontFromSystemTT()` in dieses interne Format umgewandelt. Diese Funktion ist aber direkt an Windows/GDI gebunden:

- `windows.h`
- `HDC`, `LOGFONT`, `HFONT`
- `CreateCompatibleDC`, `CreateFontIndirect`, `SelectObject`
- `GetTextMetrics`
- `GetGlyphOutline(..., GGO_NATIVE, ...)`
- `TTPOLYGONHEADER`, `TTPOLYCURVE`, `TT_PRIM_LINE`, `TT_PRIM_QSPLINE`

Aus dem Borland-Resource-Ordner wurde ausserdem bestaetigt:

- `borland_ved/src/comp/resource/font/wps_default.vfn`
- eingebettet als `IDRES_VEC_DEFAULTFONT RCDATA "font\\wps_default.vfn"`
- die ersten `1024` Bytes sind Header/Reserve
- danach folgt ein serialisierter `TDVecFont`-Stream mit Markern wie `vfnt`, `vchr`, `vgly`, `plyc`

Damit haben wir zwei Quellen fuer Fonts:

- bestehende VED-Fontdateien (`.vfn`)
- bei Bedarf aus System-TrueType-Fonts konvertierte VED-Fonts

## Ziel

VED soll eine saubere, Qt-freie FontManager-/FontProvider-Architektur bekommen.

Der Default-Font `wps_default.vfn` soll in unsere App eingebunden sein und beim Start auch ohne Plugin verfuegbar werden. Er wird ueber einen eingebauten `TDFontProvider` bzw. `IVecFontProvider` an den `TDFontManager` geliefert und vom `TDFontManager` als Default-Font gehalten.

Der TrueType-Import soll nicht in `MainWindow` und nicht direkt im VED-Core passieren. Stattdessen wird er nach Einfuehrung des Plugin-Systems als erstes reales Plugin umgesetzt:

- `TrueTypeFontConverterPlugin`
- wird beim Start durch das Plugin-System geladen
- meldet verfuegbare Systemfonts an
- konvertiert Fonts nur bei Bedarf in `TDVecFont`
- registriert oder liefert diese Fonts ueber ein Core-Interface an den `TDFontManager`
- kann spaeter konvertierte Fonts als `.vfn` in einem Cache ablegen

Wichtig: Vor dieser Story wird zuerst der Rest der noch offenen `VOC`- und `VOM`-Operationen portiert, die nichts mit Text zu tun haben. Danach folgt das erste Plugin-System, danach dieses Plugin.

## Nicht-Ziele

- Keine TrueType-Konvertierung im `MainWindow`.
- Keine Qt-Abhaengigkeit im VED-Core.
- Kein direktes Portieren des Windows/GDI-Codes.
- Kein sofortiger kompletter Font-Editor.
- Keine Textoperationen als Voraussetzung fuer die restlichen nicht-Text-Operationen.
- Kein Plugin-Zwang fuer den Default-Font. `wps_default.vfn` muss ohne Plugin geladen werden koennen.

## Architekturvorschlag

### 1. Core-Fontmodell

Die VED-Fontdaten bleiben Teil des Core:

- `TDVecFont`
- `TDVecCharacter`
- `TDVecGlyph`
- Glyph-Geometrie als `TDVecPolyCurve`

Diese Klassen duerfen keine Qt-Typen kennen.

Moegliche Dateien:

- `src/ved_core/fontengine/vec_font.h`
- `src/ved_core/fontengine/vec_font.cpp`
- `src/ved_core/fontengine/vec_font_manager.h`
- `src/ved_core/fontengine/vec_font_manager.cpp`

Falls wir aus Gruenden der aktuellen Projektstruktur zuerst alles im gleichen Ordner halten, kann das temporaer anders liegen. Dauerhaft ist ein eigener Core-Bereich `fontengine` sauberer als `MainWindow` oder Qt-Code.

### 2. FontProvider Interface im Core

Der `TDFontManager` soll Fonts nicht selbst aus System-APIs erzeugen. Er fragt FontProvider.

Beispiel:

```cpp
struct TDVecFontDescriptor {
    std::string fontId;       // z.B. "TT:Arial" oder "VC:WPS Default"
    std::string displayName;  // z.B. "Arial"
    bool isVectorFont = false;
    bool isSystemTrueType = false;
};

class IVecFontProvider {
public:
    virtual ~IVecFontProvider() = default;

    virtual std::vector<TDVecFontDescriptor> AvailableFonts() const = 0;
    virtual std::unique_ptr<TDVecFont> LoadFont(const std::string& fontId) = 0;
};
```

Der Core kennt nur dieses Interface. Ob der Provider eine eingebundene `.vfn` liest, einen Systemfont konvertiert oder aus einem Cache laedt, bleibt Implementierungsdetail.

Das Interface ist nicht an Plugins gebunden. Im ersten Schritt kann jede normale Klasse dieses Interface implementieren. Spaeter koennen Plugins ebenfalls Provider registrieren.

### 3. FontManager als Registry und Lazy Loader

Der `TDFontManager` soll:

- Default-Font aus `.vfn` laden koennen.
- bei Instantiierung einen FontProvider bekommen, der den Default-Font liefern kann.
- FontProvider registrieren.
- Fonts erst bei Bedarf anfordern.
- geladene Fonts intern cachen.
- bei Dokument-Fonts weiterhin dokumentspezifische Fonts bevorzugen.

Beispiel:

```cpp
class TDFontManager {
public:
    explicit TDFontManager(IVecFontProvider& defaultProvider);

    void RegisterProvider(IVecFontProvider& provider);

    const TDVecFont* GetDefaultVecFont() const;
    TDVecFont* GetVecFont(const std::string& fontId, TDDocumentID docId);

private:
    std::vector<IVecFontProvider*> providers_;
    std::vector<std::unique_ptr<TDVecDocFont>> fonts_;
    std::unique_ptr<TDVecFont> defaultFont_;
};
```

Der Konstruktor bzw. die Initialisierung muss sicherstellen:

- `defaultProvider` wird sofort abgefragt.
- `defaultFont_` ist danach gesetzt.
- Wenn der Default-Font nicht geladen werden kann, muss dieser Fehler explizit sichtbar werden, statt spaeter still auf Null-Pointer zu laufen.
- Der FontManager ist danach speicherschonend: nur der Default-Font ist sofort geladen; weitere Fonts kommen lazy ueber Provider.

### 4. Built-in VFN Provider als erste sichere Basis

Vor dem TrueType-Plugin soll ein einfacher Built-in-VFN-Provider existieren. Dieser Provider ist kein Plugin.

Aufgaben:

- `wps_default.vfn` als App-Resource einbinden.
- `wps_default.vfn` beim Start ueber `IVecFontProvider` laden.
- 1024-Byte-Header ueberspringen.
- danach `TDVecFont` aus dem VED-Stream lesen.
- Font als Default-Font registrieren.

Das gibt uns Textdarstellung, ohne sofort Systemfont-Konvertierung und ohne Plugin-System loesen zu muessen.

Beispiel:

```cpp
class TDBuiltinVfnFontProvider : public IVecFontProvider {
public:
    std::vector<TDVecFontDescriptor> AvailableFonts() const override;
    std::unique_ptr<TDVecFont> LoadFont(const std::string& fontId) override;
};
```

Dieser Provider kann zuerst nur einen Font kennen:

```text
VC:WPS Default
```

Spaeter kann derselbe Mechanismus auch fuer weitere mitgelieferte `.vfn`-Fonts genutzt werden.

### 5. TrueTypeFontConverterPlugin

Dieses Plugin ist die erste echte Anwendung des Plugin-Systems.

Aufgaben beim Start:

- Plugin wird durch Qt-Plugin-Layer geladen.
- Plugin registriert einen Core-`IVecFontProvider`.
- Provider liefert eine Liste verfuegbarer Systemfonts.
- Provider konvertiert erst dann, wenn `TDFontManager::GetVecFont("TT:...")` den Font wirklich braucht.

Die Qt-Huelle darf Qt verwenden:

- `QPluginLoader`
- `QObject`
- spaeter UI fuer Font-Auswahl

Der Core-Provider-Teil darf nur Core-Interfaces beruehren. Wenn die Konvertierung Qt-APIs nutzt, bleibt diese Implementierung ausserhalb von `ved_core`.

Moegliche Qt-Konvertierungsbasis:

- `QRawFont`
- `QPainterPath`
- Pfadelemente in VED-`TDVecPolyCurve` umwandeln

Wichtig: Diese Umwandlung muss VED-treu validiert werden, weil Borland `GetGlyphOutline` native TrueType-Konturen mit Linien und Quadratic-Splines liefert.

### 6. Font Cache

Nach der Portierung kann das Plugin konvertierte Fonts als `.vfn` speichern.

Moegliche Cache-Orte:

- neben der User-Config
- in einem dedizierten VED-Cache-Verzeichnis
- projekt-/dokumentnah, falls Fonts dokumentbezogen eingebettet werden sollen

Ziele:

- Systemfont nur einmal konvertieren.
- Beim naechsten Start direkt `.vfn` laden.
- Cache anhand Fontname, Fontdatei, Style, Groesse/Scale und Font-Version invalidieren.

Beispiel:

```text
config/
  ved/
    font-cache/
      TT_Arial_Regular_<hash>.vfn
      TT_TimesNewRoman_Bold_<hash>.vfn
```

## Reihenfolge

1. Restliche nicht-Text-`VOC`- und `VOM`-Operationen portieren.
2. Core-Fontmodell und `TDFontManager` portieren.
3. `IVecFontProvider` einfuehren.
4. Built-in-Provider fuer eingebundenes `wps_default.vfn` implementieren.
5. FontManager so initialisieren, dass der Default-Font immer ueber diesen Provider bereitsteht.
6. Basis-Plugin-System einfuehren.
7. `TrueTypeFontConverterPlugin` als ersten echten Plugin-Provider implementieren.
8. Lazy Conversion und FontManager-Cache aktivieren.
9. Persistenten `.vfn`-Cache fuer konvertierte Fonts ergaenzen.

## Akzeptanzkriterien

- `MainWindow` enthaelt keine TrueType-Konvertierungslogik.
- `ved_core` enthaelt keine Qt-Includes fuer Systemfont-Konvertierung.
- `wps_default.vfn` ist in die App eingebunden.
- `TDFontManager` bekommt bei Initialisierung einen `IVecFontProvider`.
- Der Built-in-Provider stellt `wps_default.vfn` auch ohne Plugin bereit.
- `TDFontManager` hat nach der Initialisierung immer einen Default-Font.
- `TDFontManager` kann Provider registrieren und Fonts lazy anfordern.
- `TrueTypeFontConverterPlugin` kann eine Fontliste bereitstellen.
- Ein Systemfont wird erst bei Bedarf in `TDVecFont` konvertiert.
- Konvertierte Fonts koennen spaeter als `.vfn` gecacht werden.
