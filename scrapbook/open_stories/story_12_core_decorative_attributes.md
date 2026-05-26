# Story: Core Decorative Attributes fuer Vektorobjekte

Datum: 2026-05-15

## Kontext

VED-Objekte haben bereits teilweise dekorative Eigenschaften, z.B. Farbe. Weitere typische Darstellungsattribute fehlen noch oder sind nicht sauber isoliert:

- Farbe
- Linienstärke
- Strichart
- Schattierung/Fuellung
- Transparenz
- Marker/Pfeile
- ggf. weitere visuelle Attribute

Diese Attribute sind keine Plugin-Metadaten im engeren Sinn. Sie gehoeren zum VED-Kern, weil sie grundlegende Eigenschaften von Vektorobjekten sind und fuer Rendering, Export, Print und Bearbeitung relevant bleiben.

## Ziel

Dekorative Kernattribute in einer eigenen Core-Struktur sammeln und in der obersten polymorphen Objektbasis `TDVecObject` isolieren.

Die Attribute sollen:

- Qt-unabhaengig sein,
- von jeder GraphicEngine interpretiert werden koennen,
- optional in Views ein-/ausblendbar sein,
- spaeter persistiert werden,
- Grundlage fuer SVG/DXF/PDF/Print-Export sein.

## Architekturvorschlag

### Core Style-Strukturen

Neue Core-Dateien, z.B.:

- `src/ved_core/vecobjects/vec_style.h`
- `src/ved_core/vecobjects/vec_style.cpp`

Vorschlag:

```cpp
enum class TDVecLineStyle {
    Solid,
    Dash,
    Dot,
    DashDot,
    Custom
};

enum class TDVecFillStyle {
    None,
    Solid,
    Hatch,
    Gradient
};

struct TDVecStrokeStyle {
    TDRgbColor color = 0x00000000;
    double widthReal = 0.0;
    TDVecLineStyle lineStyle = TDVecLineStyle::Solid;
};

struct TDVecFillStyleData {
    TDVecFillStyle fillStyle = TDVecFillStyle::None;
    TDRgbColor color = 0x00FFFFFF;
    double opacity = 1.0;
};

struct TDVecShadowStyle {
    bool enabled = false;
    TDRgbColor color = 0x00808080;
    double offsetXReal = 0.0;
    double offsetYReal = 0.0;
    double blurReal = 0.0;
};

struct TDVecObjectStyle {
    TDVecStrokeStyle stroke;
    TDVecFillStyleData fill;
    TDVecShadowStyle shadow;
};
```

### Integration in TDVecObject

```cpp
class TDVecObject {
public:
    TDVecObjectStyle& Style();
    const TDVecObjectStyle& Style() const;

private:
    TDVecObjectStyle style_;
};
```

Bestehende Farbfelder sollten mittelfristig in diese Struktur ueberfuehrt werden.

## View-Schalter

Eine View kann dekorative Attribute bewusst ignorieren, z.B. fuer technische Bearbeitung:

```cpp
struct TDVecRenderOptions {
    bool showStrokeStyle = true;
    bool showFillStyle = true;
    bool showShadow = true;
    bool showDecorations = true;
};
```

Die GraphicEngine oder der View-Paint-Pfad bekommt RenderOptions.

## Abgrenzung zu Plugin-Metadaten

Kern-Dekoration:

- Farbe
- Linienstärke
- Strichart
- Fuellung
- Schatten
- Standard-Pfeile/Marker, falls als allgemeine Vektorattribute gewollt

Plugin-Metadaten:

- "dieses Objekt ist eine UML-Klasse"
- "diese Box ist eine Organisationseinheit"
- "diese Linie ist eine Reporting-Beziehung"

Grenzfall:

- Pfeile/Marker koennen sowohl Kernstil als auch Plugin-Semantik sein.
- Empfehlung:
  - allgemeine Pfeildarstellung als Core Style,
  - semantische Bedeutung der Pfeile als Plugin-Metadata.

## Auswirkungen

- Renderer muss Style aus `TDVecObject` beachten.
- Exporter muessen Style uebersetzen.
- Object Properties UI kann Style-Attribute bearbeiten.
- Persistenz muss Style speichern.

## Akzeptanzkriterien

- `TDVecObject` hat eine zentrale Style-Struktur.
- Bestehende Farbe wird in die Style-Struktur integriert oder sauber gespiegelt.
- Mindestens Farbe, Linienstärke und Strichart sind im Core modelliert.
- Qt zeichnet diese Attribute ueber die GraphicEngine.
- View kann dekorative Attribute optional ausblenden.
- Core bleibt Qt-frei.
