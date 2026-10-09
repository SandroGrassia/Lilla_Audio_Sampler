<a id="lilla-user-guide"></a>

# Lilla Benutzerhandbuch

[English](User%20Guide.md) | [Italiano](Manuale%20Utente.md) | [Deutsch](Benutzerhandbuch.md) | [Français](Guide%20utilisateur.md)

Für **LILLA Audio Sampler 2026 | PCB2026_R1 | Firmware 7.0.2**

Ausgabe des Leitfadens: **8. Oktober 2026**

Druckversion auf Englisch: [User Guide als PDF](User%20Guide.pdf).

<img src="doc/assets/images/0.jpg" alt="LILLA-Startbildschirm mit Firmware-Version und Speicherinformationen" width="37%">

*Willkommensbildschirm. Die Fotos in diesem Handbuch zeigen ein funktionierendes Instrument; Patchnummern, Dateinamen und Werte sind Beispiele.*

LILLA vereint Sample-Wiedergabe, Line-Input-Aufnahme, Live-Sampling und MIDI-Looping in einem Instrument. Diese Anleitung führt Sie von Ihrem ersten spielbaren Patch über die Erstellung eigener Sounds bis hin zur Erhaltung einer kompletten Session.

Beginnen Sie mit **Erste Schritte**, wenn Sie sofort spielen möchten. Lesen Sie **Dateien, Sounds und Patches**, bevor Sie eine Bibliothek erstellen: Wenn Sie wissen, was bei jedem Speichervorgang erhalten bleibt, ist der Rest des Instruments einfacher zu verwenden.

**So lesen Sie dieses Handbuch:** Fettgedruckte Namen wie **Select** beziehen sich auf physische Bedienelemente; `UPPERCASE`-Text bezieht sich auf Bildschirmbeschriftungen oder Meldungen. **Tools > Setup** bedeutet, dass Sie den Tools-Selektor auf Setup einstellen und dann Tools drücken. Eine nummerierte Prozedur beschreibt die Reihenfolge der Vorgänge. Fotos veranschaulichen das Seitenlayout; Die beiliegenden Anweisungen beschreiben die Repository-Firmware, einschließlich der Änderungen, die nach der Aufnahme eines Fotos vorgenommen wurden.

Die Abläufe wurden mit der Firmware und den bereitgestellten Displayfotos abgeglichen. Eine vollständige Prüfung aller Schritte am physischen Instrument steht noch aus.

<a id="contents"></a>

## Inhalt

- [Erste Schritte](#getting-started)
- [Anschlüsse und Tasten](#io-connections-and-buttons)
- [Bedienelemente und Navigation](#controls-and-navigation)
- [Dateien, Sounds und Patches](#files-sounds-and-patches)
- [Performance](#performance)
- [Einen Sound bearbeiten](#editing-a-sound)
- [Audio importieren](#importing-audio)
- [Mit Sampler aufnehmen](#recording-with-sampler)
- [Live Sampler](#live-sampler)
- [MIDI Loop](#midi-loop)
- [Mixer, Delay und Filter](#mixer-delay-and-filters)
- [Setup und MIDI-Steuerung](#setup-and-midi-controls)
- [Sicherung und Wiederherstellung](#backup-and-restore)
- [Die Firmware unter Windows aktualisieren](#updating-the-firmware-on-windows)
- [Die Firmware auf dem Mac aktualisieren](#updating-the-firmware-on-mac)
- [Fehlerbehebung](#troubleshooting)
- [Praktische Projekte](#practical-projects)
- [Kurzreferenz](#quick-reference)
- [Glossar](#glossary)

**Beliebte Arbeitsabläufe:** [Einen Tastatur-Split erstellen](#build-a-keyboard-split) | [Einen Sound stimmen und Auto Tune verwenden](#tune-a-sound-and-use-auto-tune) | [Eine Live-Schleife in einen Patch übernehmen](#capture-a-live-loop-into-a-patch) | [Ein vollständiges Archiv planen](#plan-a-complete-archive)

<a id="getting-started"></a>

## Erste Schritte

LILLA ist ein polyphoner, multitimbraler Hardware-Sampler mit bis zu 16 Wiedergabestimmen. Ein Performance-Patch kann bis zu acht Sounds mit einzelnen MIDI-Kanälen und Tastaturbereichen enthalten. Sie können importiertes Audio abspielen, den Line-Eingang aufzeichnen, mit einem temporären Live-Puffer arbeiten und vierspurige MIDI-Loops aufnehmen.

<a id="choose-the-right-mode"></a>

### Den passenden Modus wählen

| Sie wollen... | Wählen | Womit Sie arbeiten |
| --- | --- | --- |
| Spielen Sie ein Keyboard-Split-, Layer-Instrument oder multitimbrales Setup | **Performance** | Ein Patch mit bis zu acht Sound-Slots. |
| Nehmen Sie einen Take auf, behalten Sie ihn im Aufnahmespeicher oder exportieren Sie einen WAV | **Sampler** | Eine Mono- oder Stereo-Audioaufnahme in Flash. |
| Erkunden Sie eingehende Audiodaten und erfassen Sie ein ausgewähltes Fragment | **Live Sampler** | Ein temporärer kreisförmiger Audiopuffer. |
| Nehmen Sie Phrasen auf, die auf Ihrem MIDI-Controller abgespielt werden, und spielen Sie sie ab | **MIDI Loop** | Vier Spuren von MIDI-Ereignissen mit dem aktuellen Patch. |

Verwenden Sie Sampler für einen Take, den Sie anhören, konvertieren oder exportieren möchten. Verwenden Sie Live Sampler, wenn Sie den interessanten Sound in einem laufenden Audiostream finden möchten.

<a id="connect-and-play"></a>

### Anschließen und spielen

1. Schließen Sie einen MIDI-Controller an den MIDI-Eingang von LILLA an.
2. Verbinden Sie den Stereo-Line-Ausgang mit Ihrem Mixer, Verstärker oder Audio-Interface. Beginnen Sie mit niedrigen Hörpegeln.
3. Schalten Sie den LILLA ein und warten Sie, bis der Startvorgang abgeschlossen ist.
4. Stellen Sie den Modes-Selektor auf **Performance** ein.
5. Drehen Sie **Select**, um die Patch-Nummer hervorzuheben, und drehen Sie dann **Value**, um einen vorhandenen Patch auszuwählen.
6. Stellen Sie Ihren Controller auf den angezeigten MIDI-Kanal für einen Sound im Patch ein.
7. Spielen Sie Noten im `FROM K`- und `TO K`-Bereich dieses Sounds.
8. Erhöhen Sie **Line Out Vol** schrittweise.

**Was Sie sehen sollten:** Auf der Seite Performance werden eine Patch-Nummer und die aktiven Soundzeilen angezeigt. Der MIDI-Kanal und der Tastaturbereich in jeder Reihe bestimmen, welche Noten diesen Klang auslösen können. Ein Acht-Slot-Patch muss nicht jeden Slot verwenden.

**Was Sie hören sollten:** ein Ton, wenn Sie eine Note innerhalb des Bereichs einer aktiven Zeile auf dem zugewiesenen MIDI-Kanal spielen. Wenn Sie nichts hören, beginnen Sie mit dem Kanal und Bereich, bevor Sie das Sample oder seine Hüllkurve ändern.

<a id="your-first-edit"></a>

### Die erste Bearbeitung

1. Drücken Sie **S1**, wenn Sound 1 aktiv ist, oder drücken Sie die Taste eines anderen aktiven Sounds.
2. Drehen Sie Select, um `GAIN` hervorzuheben, und drehen Sie dann Value ein wenig.
3. Spielen Sie ein paar Noten und hören Sie sich die Veränderung an.
4. Wählen Sie `RETURN` und drücken Sie Select.
5. Wählen Sie in Performance `SAVE` aus, wenn es verfügbar ist.

Dies leitet den normalen Bearbeitungszyklus ein: **Öffnen Sie einen Sound → passen Sie ihn an → kehren Sie zu Performance zurück → speichern Sie den Patch**. Das Zurückkehren von einer Seite behält Ihre Arbeitsänderungen bei, ersetzt jedoch nicht den Schritt „Speichern“.

<a id="finish-a-session"></a>

### Eine Sitzung beenden

Speichern Sie vor dem Ausschalten bearbeitete Patches, speichern Sie alle MIDI-Loops, die Sie behalten möchten, und schließen Sie alle ausstehenden Live Sampler-Capture-Speicherungen ab. Warten Sie, bis die Schreib- oder Exportvorgänge abgeschlossen sind. Der Live-Puffer selbst überlebt das Ausschalten nicht.

Wenn kein passendes Audio geladen ist, befolgen Sie [Audio importieren](#importing-audio). Beim Importieren wird die Flash-Audiobibliothek ersetzt und vorhandene Aufnahmen gelöscht. Sichern Sie daher zunächst Ihre Arbeit.

<a id="io-connections-and-buttons"></a>

## Anschlüsse und Tasten

| Verbindung oder Taste | Stecker | Beschreibung |
| --- | --- | --- |
| Line in | 3,5-mm-Klinkenstecker | Stereo-Line-Eingang / dynamischer Stereo-Mikrofoneingang. |
| Line out | 3,5-mm-Klinkenstecker | Stereo-Line-Ausgang, 3,1 Vpp. |
| Phones line | 3,5-mm-Klinkenstecker | Hauptkopfhörerausgang. |
| Phones pre-listen | 3,5-mm-Klinkenstecker | Kopfhörerausgang zum Vorhören. |
| MIDI IN | 3,5-mm-Klinkenstecker | MIDI-Eingang. |
| MIDI OUT | 3,5-mm-Klinkenstecker | MIDI-Ausgabe. |
| Tor IN | 3,5-mm-Klinkenstecker | Gate-Eingang, +5 V. |
| Tor OUT | 3,5-mm-Klinkenstecker | Gate-Ausgang, +5 V. |
| USB-C | USB-C | +5 V DC Stromversorgung und Programmierung. |
| Firmware_upload mode-Taste | Taste | Wechseln Sie in den Firmware-Upload-Modus. |
| On/off-Taste | Taste | Schalten Sie das Instrument ein oder aus. |

**Zukünftige Entwicklung:** MIDI OUT, Gate IN und Gate OUT sind physisch verfügbar und über Klassen zugänglich, die bereits in der Firmware-Codebasis enthalten sind. Derzeit nutzen keine benutzerorientierten Funktionen diese Verbindungen. Sie stehen für zukünftige Entwicklungen zur Verfügung.

<a id="controls-and-navigation"></a>

## Bedienelemente und Navigation

<img src="doc/assets/images/top.jpg" alt="LILLA-Oberseite mit Display, Encodern, Moduswählern und Tontasten" width="100%">

*Übersicht über das obere Bedienfeld: physische Bedienelemente und ihre Positionen.*

Der weiße Auswahlrahmen identifiziert das Feld oder den Befehl, der auf die Navigationssteuerelemente reagiert. Auf den mitgelieferten Fotos sind Beschriftungen im Allgemeinen in Cyan, bearbeitbare Werte und Befehle in Gelb und Seitenüberschriften in Rot gehalten.

**Das Drehen und Drücken eines Encoders sind separate Aktionen.** Durch Drehen von Value wird beispielsweise ein hervorgehobener Parameter bearbeitet, während durch Drücken von Value dieser Parameter auf bestimmten Seiten zurückgesetzt oder umgeschaltet werden kann.

Dieselben Steuerelemente führen je nach aktiver Seite unterschiedliche Aufgaben aus. Folgen Sie dem hervorgehobenen Feld und den aktuell auf dem Bildschirm angezeigten Optionen.

| Kontrolle | Hauptverwendung |
| --- | --- |
| Modes-Selektor | Wählen Sie Sampler, Live Sampler, Performance oder MIDI Loop. |
| Tools-Selektor | Wählen Sie Mixer, Delay, Setup oder Test. |
| Tools-Taste | Öffnen Sie das ausgewählte Werkzeug. Das Symbol Tools LED zeigt den Werkzeugzugriff an. |
| Auswählen, drehen | Verschieben Sie die Hervorhebung zwischen Feldern und Menüelementen. |
| Auswählen, drücken | Führen Sie einen Menübefehl aus, bestätigen Sie eine Auswahl oder betreten/verlassen Sie eine Gruppe von Feldern. |
| Value, umdrehen | Ändern Sie den markierten Parameter. |
| S1-S8 | Aktive Sounds in Performance öffnen; S1 öffnet eine Mono- oder Aufnahme auf dem linken Kanal und S2 den rechten Kanal in Sampler; Capture in Sound-Slots in Live Sampler. |
| From / To | Die Sample-Grenzen in Sound Edit und den Live-Wiedergabebereich in Live Sampler einstellen. |
| Step | Bearbeitungsschritte ändern; seine Push-Funktion hängt von der Seite ab. |
| Line Out Vol | Passen Sie die Lautstärke der Patch-Wiedergabe auf leistungsbezogenen Seiten an. |
| Pre Listen Vol | Passen Sie den Vorhörpegel an. |
| Resolution / Downsampling | Ändern Sie den Wiedergabecharakter durch Bitreduzierung und Sample-Wiederholung. |
| Cutoff | Passen Sie den gemeinsamen Tiefpassfilter an; drücken, um die maximale Abschaltgrenze wiederherzustellen. |
| Tuning Tone | Aktivieren Sie die Tuning-Referenz; Drehen Sie, um den Pegel anzupassen, wenn die Funktion aktiviert ist. |
| Schleife / Tempo | Wählen und steuern Sie MIDI-Loops und deren Wiedergabe-Timing. |
| Track 1-4 / Rec 1-4 | Steuern Sie einzelne MIDI-loop-Spuren und deren Aufnahme. |

Um ein Werkzeug zu öffnen, stellen Sie den Tools-Wähler auf die gewünschte Position und drücken Sie **Tools**. Test öffnet den Monitor MIDI. Verwenden Sie die Schaltfläche Tools, um von einem Werkzeug zurückzukehren, sofern es unterstützt wird, oder wählen Sie den erforderlichen Betriebsmodus aus.

Drehen Sie in Bestätigungsdialogen **Select**, um eine Option auszuwählen, und drücken Sie zur Bestätigung darauf. Lesen Sie den Dialog, bevor Sie bestätigen: Speichern, Verwerfen, Löschen und Wiederherstellen haben unterschiedliche Konsequenzen.

Menüs sind dynamisch. Ein Befehl kann ausgeblendet werden, wenn der aktuelle Status dies nicht zulässt, beispielsweise wenn keine Aufzeichnung zum Exportieren vorhanden ist. Ein gespeicherter, unveränderter Patch zeigt möglicherweise weniger Befehle an als ein Patch mit ausstehenden Änderungen.

<a id="three-navigation-patterns"></a>

### Drei Navigationsarten

**Menübefehle:** Drehen Sie Select, bis der Rahmen den Befehl umgibt, und drücken Sie dann Select. Wenn eine Bestätigung erscheint, wählen Sie die gewünschte Antwort aus und drücken Sie erneut Select.

**Parameterbearbeitung:** Drehen Sie Select, bis der Parameter hervorgehoben ist, und drehen Sie dann Value. Hören Sie zu, während Sie sich anpassen. viele Veränderungen sind sofort hörbar.

**Tabellen mit Sounds oder Quellen:** Wählen Sie zuerst die Zeile oder Quelle aus und drücken Sie dann Select, um die bearbeitbaren Felder aufzurufen. Drücken Sie erneut Select, um die Gruppe zu verlassen, sofern die Seite dieses Bedienmuster unterstützt.

<a id="useful-shortcuts"></a>

### Nützliche Kurzbefehle

| Wo | Aktion | Ergebnis |
| --- | --- | --- |
| Performance | Drücken Sie eine aktive S1-S8-Taste | Öffnen Sie den Editor dieses Sounds. |
| Sound Edit von Performance | Drücken Sie die gleiche Tontaste erneut | Öffnen Sie die Seite VCF. |
| Sound Edit von Sampler | Drücken Sie S1 oder S2 für Stereo | Wählen Sie den Aufnahmekanal; Mit der gleichen Schaltfläche bleibt dieser Kanal im Sound Edit. |
| Sound Edit, `PITCH` ausgewählt | Drücken Sie Value | Setzen Sie die Tonhöhenanpassung zurück. |
| Sound Edit, `PITCH` ausgewählt | Drücken Sie Select | Führen Sie Auto Tune für die ausgewählte Audioregion aus. |
| Sound Edit | Drücken Sie From | Verschieben Sie den Anfang der Region an den Anfang der Quelle. |
| Sound Edit | Drücken Sie To | Schalten Sie das Grenz-/Slice-Bearbeitungsverhalten um. |
| Sound Edit | Drücken Sie Step | Wählen Sie den bereichsbezogenen Trimmschritt aus. |
| Performance oder Sound Edit, `PAN` ausgewählt | Drücken Sie Value | Zentrieren Sie die Pfanne. |
| Live Sampler | Drücken Sie Step | Schalten Sie das Verhalten der Startpunktsperre um. |
| Allgemeine Wiedergabesteuerungen | Drücken Sie Cutoff | Stellen Sie den maximalen gemeinsamen Tiefpass-Cutoff wieder her. |
| Allgemeine Wiedergabesteuerungen | Drücken Sie Line Out Vol | Aktive Spieler stoppen; Deutliches Verzögerungsfeedback auch außerhalb des Direct Sampler-Verzögerungskontexts. |

Verwenden Sie die letzte Tastenkombination, wenn Sie das Erklingen von Noten schnell stoppen müssen. Bei MIDI Loop stoppt es auch die Titelwiedergabe.

<a id="files-sounds-and-patches"></a>

## Dateien, Sounds und Patches

| Begriff | Bedeutung |
| --- | --- |
| Audiodatei | Das für die Wiedergabe verwendete Quell-Sample. Importierte Audiodaten werden in Flash in Mono-RAW-Audio konvertiert. |
| Aufnahme | Vom Sampler in seinem Flash-Aufnahmebereich aufgenommenes Audio; Es kann Mono oder Stereo sein. |
| Klang | Eine Quelldatei plus Wiedergabeeinstellungen wie Trimmen, Tonhöhe, Hüllkurve, Panorama und Verstärkung. |
| Instrumenten-/Sound-Slot | Eine von bis zu acht Positionen in einem Patch, mit MIDI-Zuordnung, Grundton, Tastaturbereich und Filtereinstellungen. |
| Patch | Die Gruppe von Sounds und Einstellungen, die für eine Aufführung verwendet werden. |
| MIDI-Schleife | Aufgezeichnete MIDI-Events, angeordnet in vier Spuren. Die von diesen Veranstaltungen abgespielten Audiobeispiele sind nicht enthalten. |
| Live-Puffer | Temporäres Audio in PSRAM für Live Sampler gespeichert. |

Die Firmware bietet Speicherplatz für bis zu 200 Patches und 800 Soundaufzeichnungen. Dabei handelt es sich um Speicherkapazitäten; Das Instrument verfügt über bis zu 16 gleichzeitige Wiedergabestimmen. Die verfügbare Polyphonie hängt auch von der Wiedergabeauslastung ab.

Flash enthält die importierte Audiobibliothek und Sampler-Aufnahmen. PSRAM bietet Live-Audio. Die microSD-Karte wird für Import, WAV Export, Backups und MIDI-loop-Dateien verwendet.

<a id="follow-the-sound-from-source-to-keyboard"></a>

### Von der Audioquelle zur Tastatur

Ein typisches Setup besteht aus drei Ebenen:

**Audiodatei → Klangeinstellungen → Instrument in einem Patch**

Beispielsweise wird `piano.wav` als `piano.raw` importiert. Ein Sound wählt diese Quelle aus und definiert deren Trimmung, Hüllkurve und Stimmung. Ein Instrumenten-Slot weist ihm einen Grundton, einen MIDI-Kanal und einen spielbaren Tastaturbereich zu.

Durch Ändern der Trimmung wird angepasst, welcher Teil der Quelle wiedergegeben wird. Die ursprüngliche Quelldatei wird nicht ausgeschnitten. Wenn Sie ein Instrument fallen lassen, wird sein Platz im Patch entfernt; Das Quellaudio wird dadurch nicht gelöscht.

Eine Bildschirmbezeichnung wie `SOUND 1` bedeutet den ersten Slot des aktuellen Patches. Es ist nicht dasselbe wie Audiodatei 1, Aufnahme 1 oder Patch 1.

<a id="what-survives-power-off"></a>

### Was bleibt nach dem Ausschalten erhalten?

| Material | Wie man es behält |
| --- | --- |
| Bearbeitet einen Patch und seine Sounds | Speichern von Performance. |
| Eine abgeschlossene Sampler-Aufnahme | Beenden Sie die Aufnahme mit `STOP`; Verwenden Sie Backup oder WAV-Export für eine externe Kopie. |
| Sampler zeichnet A/B Grenzen auf | Wird beim Bearbeiten automatisch gespeichert; mit der Aufnahme für die Tastaturwiedergabe und den RAW/WAV-Export abgerufen. Andere Bearbeitungen der Aufnahmewiedergabe bleiben in der aktuellen Sitzung erhalten. |
| Eine mit `MAKE_RAW` konvertierte Sampler-Aufnahme | Behalten Sie die generierte Flash-Quelle und speichern Sie den Patch, der sie verwendet. |
| Audio befindet sich noch im Live Sampler-Puffer | Erfassen Sie die gewünschte Region in einem Sound-Slot und speichern Sie den resultierenden Patch. |
| Ein neu aufgenommener Live Sampler-Sound | Speichern Sie den Patch, damit das ausstehende Audio in Flash geschrieben wird. |
| Eine MIDI-Schleife | Speichern Sie es auf microSD von MIDI Loop. |
| Eine tragbare Kopie Ihrer Arbeit | Behalten Sie die Konfigurations-, Audioaufzeichnungs-, Quellbibliotheks- und MIDI-loop-Dateien bei, wie unter Sichern und Wiederherstellen beschrieben. |

<a id="patch-numbers-and-the-temporary-session"></a>

### Patchnummern und temporärer Arbeitsbereich

Normale Patches verwenden **IDs 0-199**. **Patch 200** ist der temporäre Sampling-Arbeitsbereich; Es handelt sich nicht um einen extra normalen Patch-Slot.

Die erste erfolgreiche Live Sampler-Soundaufnahme erstellt einen normalen Patch mit dem **ersten verfügbaren ID in 0-199**. Spätere Aufnahmen können die verbleibenden Plätze füllen. Gehen Sie zu Performance, um diesen Patch zu überprüfen und zu speichern.

<a id="file-names-matter"></a>

### Dateinamen beachten

Der Sound-Header zeigt nur die ersten acht Zeichen des Quellbasisnamens an und lässt `.raw` weg. Daher können zwei Dateien mit ähnlichen Namen in diesem kleinen Feld gleich aussehen. Wählen Sie kurze, markante Anfänge wie `BassDry` und `BassFX`.

LILLA behält die Identität einer Datei bei, wenn das Audio fehlt. Durch erneutes Importieren desselben Basisnamens können Sounds, die darauf verweisen, erneut verbunden werden. Durch das Umbenennen einer Datei entsteht eine andere Identität; Behandeln Sie Bibliotheksnamen als Teil Ihres Projekts.

Die Dateinamentabelle unterstützt 260 RAW-Dateiidentitäten, einschließlich der Ersatzquelle, erzeugter Dateien und gespeicherter Verweise auf fehlende Dateien. Freier Audiospeicher und freie Dateiidentitäten sind getrennte Ressourcen.

<a id="performance"></a>

## Performance

<img src="doc/assets/images/1.jpg" alt="Performance-Seite mit einer Tastaturbelegung mit sieben Sounds" width="37%">

*Ein Beispiel-Patch, verteilt auf sieben Sound-Slots. Jede Reihe hat ihren eigenen Grundton, Bereich und Verstärkung.*

Performance ist die Hauptseite zum Zusammenstellen und Speichern eines spielbaren Instruments. Lesen Sie es von oben nach unten: Patch und Lautstärke oben, allgemeine Klangcharakterregler in der Mitte, dann die aktiven Instrumentenreihen.

<a id="choose-a-patch"></a>

### Einen Patch auswählen

Markieren Sie die Patch-Nummer mit **Select** und drehen Sie **Value**, um nach vorhandenen Patches zu suchen.

Wenn der aktuelle Patch Änderungen enthält, fragt LILLA, was vor dem Wechsel zu tun ist. Sie können im aktuellen Patch bleiben, seine Änderungen verwerfen oder sie speichern und fortfahren. Nutzen Sie die angezeigten Auswahlmöglichkeiten, anstatt wegzuwechseln, ohne die Eingabeaufforderung zu überprüfen.

<a id="map-sounds-to-your-controller"></a>

### Sounds dem Controller zuordnen

1. Markieren Sie eine Tonzeile.
2. Drücken Sie **Select**, um die Felder aufzurufen.
3. Drehen Sie **Select**, um zwischen den Feldern zu wechseln.
4. Drehen Sie **Value**, um das ausgewählte Feld zu bearbeiten.
5. Drücken Sie **Select**, um die Felder der Zeile zu verlassen.

| Feld | Zweck |
| --- | --- |
| `SOUND` | Sound-Slot im Patch, nummeriert von 1-8. |
| `LOCK` | Schützt den Sound vor bestimmten Performance-Reglern, darunter Pitch Bend, Resolution und Downsampling. Beeinflusst auch die Verarbeitung losgelassener Noten. |
| `P` | Wiedergabepriorität: Gibt dem Ton Priorität bei der Stimmenzuordnung. Es garantiert keine unbegrenzte Stimmenanzahl. |
| `MIDI` | Empfang des MIDI-Kanals, angezeigt als 1-16. |
| `ROOT K` | Referenztaste, die für die Tonhöhenzuordnung des Sounds verwendet wird. |
| `FROM K` / `TO K` | Inklusive Tastaturbereich, der diesen Sound auslöst. |
| `PAN` | Stereoposition; Drücken Sie Value in diesem Feld, um es zu zentrieren. |
| `GAIN` | Schallpegel. |

Legen Sie für einen Keyboard-Split zwei Sounds mit unterschiedlichen Tastenbereichen auf denselben MIDI-Kanal. Geben Sie für eine Ebene überlappende Bereiche an. Weisen Sie für die multitimbrale Wiedergabe verschiedene MIDI-Kanäle zu.

<a id="build-a-keyboard-split"></a>

### Einen Tastatur-Split erstellen

Bei einer Aufteilung spielt ein Teil der Tastatur einen Bassklang und ein anderer Teil ein Pad, Klavier oder Lead.

1. Beginnen Sie mit einem Patch, der zwei aktive Sounds enthält. Öffnen Sie bei Bedarf einen vorhandenen Sound und fügen Sie mit `CLONE` einen Slot hinzu.
2. Wählen Sie für jeden Sound die passende Quelle und Hüllkurve.
3. Geben Sie in Performance beiden Steckplätzen den gleichen MIDI-Kanal.
4. Stellen Sie den `TO K` von Sound 1 auf die letzte Taste der unteren Zone.
5. Stellen Sie den `FROM K` von Sound 2 auf die nächsthöhere Taste ein.
6. Test die beiden Noten auf beiden Seiten des Split-Punkts.
7. Gleichen Sie die Verstärkungen aus und speichern Sie den Patch.

Die Bereiche sind inklusive. Wenn Sound 1 auf derselben Taste endet, auf der Sound 2 beginnt, löst diese Taste beide Sounds aus.

<a id="build-a-layer-or-multitimbral-setup"></a>

### Layer oder multitimbrales Setup erstellen

Weisen Sie für eine **Ebene** zwei oder mehr Slots denselben MIDI-Kanal und überlappende Bereiche zu. Beginnen Sie mit niedrigeren Einzelverstärkungen und erhöhen Sie diese dann, während Sie sich den kombinierten Klang anhören.

Für ein **multitimbrales Setup** weisen Sie verschiedene Kanäle verschiedenen Slots zu. Ein Sequenzer oder Controller kann dann jeden Teil separat ansprechen. Ein MIDI-Kanal wählt aus, welches Instrument reagiert; Die Steckplatznummer legt diesen Kanal nicht automatisch fest.

Jede ausgelöste Ebene verwendet Wiedergabestimmen. Ein vierstimmiger Akkord mit zwei Ebenen kann acht Stimmen erfordern, bevor die Ausklänge gezählt werden.

<a id="set-the-root-key"></a>

### Die Grundtaste festlegen

Die Grundtaste ist die Tastaturreferenz für das Sample. Auf dieser Taste wird es mit der eigenen Tonhöheneinstellung des Sounds abgespielt; höhere und tiefere Noten transponieren die Quelle relativ zu dieser Referenz.

Stellen Sie für ein Tonhöhen-Sample den Grundton auf die durch die Aufnahme dargestellte Note ein. Für einen Einzeltastenanschlag stellen Sie `FROM K` und `TO K` auf die gleiche Triggernote ein und wählen einen Grundton, der der gewünschten Wiedergabetonhöhe entspricht.

Oktavbezeichnungen hängen von Setups `FIRST OCTAVE` ab. Wenn Sie die Einstellungen mit einem anderen Gerät vergleichen, vergleichen Sie die tatsächliche Tonart MIDI sowie den angezeigten Oktavnamen.

<a id="save-or-duplicate-a-patch"></a>

### Einen Patch speichern oder duplizieren

Verwenden Sie die im Menü Performance angezeigten Befehle:

- `SAVE`: Speichern Sie das aktuelle Patch und die bearbeiteten Sounds.
- `CLONE`: Erstellen Sie eine Kopie in einem anderen verfügbaren Patch-Slot.
- `SAVE_AS_NEW`: Speichern Sie das bearbeitete Ergebnis als neuen Patch.
- `EXIT`: Verwerfen Sie die aktuellen Änderungen über den Exit-Workflow Performance.
- `DROP`: Löschen Sie den Patch nach der Löschbestätigung.

Der Befehl `RETURN` eines Sounds behält Änderungen in der aktuellen Sitzung bei. Speichern Sie den Patch, um diese Änderungen dauerhaft zu machen.

Verwenden Sie `CLONE`, wenn Sie möchten, dass aus einem vorhandenen Patch ein zweiter Patch entwickelt wird. Verwenden Sie `SAVE_AS_NEW`, wenn Sie einen Patch bearbeitet haben und das Ergebnis unter einem anderen verfügbaren Patch ID beibehalten möchten. Lesen Sie das angezeigte Ziel und die Bestätigung, bevor Sie fortfahren.

**Bevor Sie in einen Sampling-Workflow wechseln, speichern Sie den Patch, den Sie bearbeitet haben.** Live Sampler kann den vorherigen Performance-Patch im Speicher behalten, aber das Erstellen eines neuen erfassten Patches erfordert, dass der vorherige Patch keine ausstehenden Änderungen aufweist.

<a id="editing-a-sound"></a>

## Einen Sound bearbeiten

<img src="doc/assets/images/2.jpg" alt="Sound Edit-Seite mit Hüllkurve, Wiedergabemodus und Beispielwellenform" width="37%">

*Die Wellenform gehört zur ausgewählten Quelle. `FROM`, `TO` und `TOT` beschreiben die spielbare Region; `TRIM STEP` steuert die Bearbeitungsschrittweite.*

Drücken Sie in Performance **S1-S8** für einen aktiven Ton, um Sound Edit zu öffnen. Bei einem nicht verwendeten Steckplatz wird `SOUND ... IS NOT USED` angezeigt. Durch Drücken wird kein neuer Ton erzeugt.

<a id="choose-and-trim-the-source"></a>

### Die Quelle auswählen und zuschneiden

1. Markieren Sie das Dateifeld mit **Select**.
2. Drehen Sie **Value**, um eine verfügbare Quelle auszuwählen.
3. Spielen Sie den Sound von Ihrem MIDI-Controller ab.
4. Drehen Sie **From** und **To**, um den Wiedergabebereich anzupassen.
5. Drehen Sie **Step**, um eine geeignete Trimmschrittweite auszuwählen: Verwenden Sie größere Schritte, um den Bereich zu finden, und dann kleinere Schritte, um ihn zu verfeinern.

Beim Ändern der Quelldatei wird der Trimmbereich auf die volle Länge der neuen Datei zurückgesetzt und die Tonhöhenanpassung zurückgesetzt. Überprüfen Sie die Grenzen erneut, nachdem Sie Dateien geändert haben.

<a id="work-from-a-rough-cut-to-a-precise-region"></a>

### Vom groben Zuschnitt zum präzisen Bereich

Beginnen Sie mit einem großen Trimmschritt, um lange Pausen zu entfernen oder eine Phrase zu lokalisieren. Reduzieren Sie den Schritt in der Nähe des gewünschten Angriffs- und Endpunkts. Spielen Sie das Sample wiederholt ab, während Sie es verfeinern: Die Wellenform hilft dabei, ein Ereignis zu lokalisieren, aber durch Zuhören erfahren Sie, ob Sie dessen Attack entfernt haben oder einen unerwünschten Ausklang hinterlassen haben.

Durch Drücken von **From** kehrt der Start zum Quellanfang zurück. Durch Drücken von **To** wird zwischen der Bearbeitung eines Endpunkts und der Arbeit mit einem Slice umgeschaltet. Beim Slice-Verhalten kann durch Verschieben von From die Region verschoben werden, während ihre Länge erhalten bleibt. Achten Sie beim Verschieben auf die Regionsanzeige.

Ein kurzer Bereich ist für eine sich wiederholende Textur nützlich. Ein längerer Bereich kann den natürlichen Anschlag und Ausklang eines Instruments bewahren. Wenn Sie die Quelle später ändern, wiederholen Sie den Zuschneidevorgang, da durch die Änderung der Quelle die Grenzen zurückgesetzt werden.

<a id="shape-playback"></a>

### Die Wiedergabe gestalten

Verwenden Sie Select, um einen Parameter hervorzuheben, und Value, um ihn zu ändern:

| Parameter | Zweck |
| --- | --- |
| Pitch | Das Sample stimmen. |
| Gain / Pan | Pegel und Stereoposition einstellen. |
| Attack | Legen Sie fest, wie schnell der Ton seinen Anfangspegel erreicht. |
| Decay | Stellen Sie den Übergang auf den Sustain-Pegel ein. |
| Sustain | Stellen Sie den Pegel der gehaltenen Hüllkurve ein. |
| Release | Stellen Sie den Fade nach der Veröffentlichung ein. |
| Play mode | Wählen Sie die Wiedergaberichtung und das One-Shot- oder Looping-Verhalten. |
| Noclick | Passen Sie die Glättung der Schleifengrenzen an, sofern verfügbar. |

| Wiedergabemodus | Wie die Region gelesen wird | Typische Verwendung |
| --- | --- | --- |
| Once FWD | Einmal von Anfang bis Ende. | Hits, Spoken Words und natürliche Zerfälle. |
| Once REV | Einmal vom Ende zum Anfang. | Umgekehrte Stöße und Schwellungen. |
| Loop FWD | Wird in Vorwärtsrichtung wiederholt. | Anhaltende Töne und sich wiederholende Phrasen. |
| Loop FWD/REV | Wechselt die Richtung und beginnt vorwärts. | Texturen, die sich an den Grenzen umkehren. |
| Loop REV/FWD | Wechselt die Richtung und beginnt rückwärts. | Eine umgekehrt beginnende Variante einer Wechselschleife. |
| Loop REV | Wird in umgekehrter Richtung wiederholt. | Umgekehrte, sich wiederholende Texturen. |

Das Verhalten der Hüllkurve und der Notenfreigabe wirkt sich immer noch auf das aus, was Sie hören. Ein Loop-Bereich kann mit der Hüllkurve ausgeblendet werden; Eine einmalige Quelle hat immer noch ein endliches Ende.

**Ein einfacher Ausgangspunkt für die Hüllkurve:** Verwenden Sie einen kurzen Anschlag für die Percussion, einen langsameren Anschlag für ein Pad und eine ausreichend lange Freigabe, um ein abruptes Ende zu vermeiden. Erhöhen Sie bei einem Loop das Sustain, um den wiederholten Bereich deutlich zu hören, bevor Sie Decay und Release formen.

Für eine saubere Schleife verfeinern Sie die Start- und Endpunkte, bevor Sie Noclick erhöhen. Der verfügbare Noclick-Bereich hängt von der ausgewählten Region ab.

<a id="tune-a-sound-and-use-auto-tune"></a>

### Einen Sound stimmen und Auto Tune verwenden

Der Wert `PITCH` zeigt das Wiedergabeverhältnis an: **1.000** entspricht der unveränderten Geschwindigkeit der Quelle. Eine höhere Tonhöhe beschleunigt auch die normale Sample-Wiedergabe; eine niedrigere Tonhöhe verlangsamt sie.

1. Wählen Sie `PITCH`.
2. Drehen Sie **Value**, um die Einstellung nach Gehör vorzunehmen, oder drücken Sie **Value**, um die Einstellung zurückzusetzen.
3. Drücken Sie **Select**, um **Auto Tune** in der ausgewählten Region auszuführen.
4. Hören Sie sich das Ergebnis beim Grundton an und vergleichen Sie es mit Ihren anderen Instrumenten.
5. Speichern Sie den Patch, wenn Sie die Anpassung beibehalten möchten.

Auto Tune ist sowohl für die One-Shot- als auch für die Loop-Wiedergabe verfügbar. Es analysiert die stärkste Frequenzkomponente und passt sie an die nächstgelegene chromatische Note an. Ein stark harmonischer, rauschender Anschlag oder ein Sample ohne Tonhöhe entspricht möglicherweise nicht dem musikalischen Grundton, den Sie erwartet haben. Um ein klareres Ergebnis zu erzielen, wählen Sie einen Abschnitt mit stabiler Tonhöhe und überprüfen Sie das Ergebnis nach Gehör.

Wenn ein `AUTO-TUNE`-Fehler auftritt, lesen Sie die Ursache: Eine ungültige Region, eine nicht verfügbare Quelle, kein messbares Signal oder ein ausgelasteter Audiovorgang erfordern eine andere Reaktion. Auto Tune ersetzt nicht die Auswahl des richtigen Grundtons und Tastaturbereichs.

Die Anzeige `MAX PITCH` beschreibt die verfügbare Wiedergabeobergrenze für den aktuellen Quellpfad. Es kann sich mit der Quellenzwischenspeicherung und der Wiedergabevorbereitung ändern; Gehen Sie nicht davon aus, dass jede Quelle die gleiche Umsetzungsobergrenze hat.

<a id="return-clone-or-remove-a-sound"></a>

### Zurückkehren, klonen oder einen Sound entfernen

Die folgenden Befehle gelten für Sounds, die über Performance geöffnet werden. Für eine Aufzeichnung, die über Sampler geöffnet wird, ist der einzige Menübefehl `RETURN`; siehe [Eine Aufnahme vor Konvertierung oder Export bearbeiten](#edit-a-recording-before-conversion-or-export).

- `RETURN` behält die Änderungen bei und kehrt zur vorherigen Leistungsseite zurück.
- `CLONE` kopiert das Instrument in einen freien Slot im aktuellen Patch. Passen Sie den Tonumfang, den Grundton oder die Quelle der Kopie nach Bedarf an.
- `DROP` entfernt das Instrument aus dem aktuellen Patch.

Speichern Sie den Patch, nachdem Sie fertig sind. Das Entfernen eines Sound-Slots unterscheidet sich vom Löschen seiner Quellaudiodatei.

<a id="importing-audio"></a>

## Audio importieren

> **Der Import ersetzt die Audiobibliothek.** Durch die Bestätigung des Imports werden die vorherigen Flash-Audiodateien und Sampler-Aufnahmen gelöscht. Erstellen Sie ein Backup und bewahren Sie Ihr Quellaudio auf Ihrem Computer auf, bevor Sie fortfahren.

<img src="doc/assets/images/12.jpg" alt="Audio-Importseite mit Quelldateien, Flash-Kapazität und der Löschwarnung" width="37%">

*Überprüfen Sie vor dem Import sowohl den Quellbericht als auch die Zielkapazität. Auf diesem Foto steht 35 Sekunden; Das aktuelle Firmware-Limit liegt bei 3 MiB dekodiertem Mono PCM, etwa 35,7 Sekunden.*

<a id="prepare-the-microsd-card"></a>

### Die microSD-Karte vorbereiten

Erstellen Sie `/LILLA_AUDIO` im Stammverzeichnis der Karte und legen Sie Ihre Audiodateien direkt darin ab.

| Format | Einfuhrbestimmungen |
| --- | --- |
| `.raw` | Headerloses Mono, signiertes 16-Bit-Little-Endian PCM bei 44,1 kHz. |
| `.wav` | Unkomprimiertes PCM, 16-Bit, 44,1 kHz, Mono oder Stereo. |
| `.aif` / `.aiff` | Unkomprimiertes AIFF, 16-Bit, 44,1 kHz, Mono oder Stereo. |
| `.mp3` | Mono oder Stereo mit Standard-MP3-Raten von 8 bis 48 kHz; konvertiert auf 44,1 kHz. |

Stereoimporte werden durch Mittelwertbildung in Mono umgewandelt. Um eine Stereoquelle als zwei unabhängig spielbare Dateien zu erhalten, bereiten Sie vor dem Import getrennte Dateien für links und rechts mit unterschiedlichen Namen vor.

Importiertes Audio wird als `<basename>.raw` gespeichert. Verwenden Sie unterschiedliche Basisnamen: `piano.wav` und `piano.mp3` zielen beide auf `piano.raw` und werden als Duplikate behandelt.

Ein einfaches Kartenlayout ist:

```text
microSD-Wurzelverzeichnis/
  LILLA_AUDIO/
    BassDry.wav
    Bell.aiff
    DrumLoop.mp3
    Texture.raw
```

Legen Sie die Dateien direkt in diesem Ordner ab. Halten Sie ihre Basisnamen eindeutig und nicht länger als 31 Byte. Einfache ASCII-Namen sind eine einfache Möglichkeit, diese Grenze einzuhalten. Vermeiden Sie Namen, die für die Aufzeichnung von Daten reserviert sind, z. B. `P12.raw`.

Ein komprimiertes MP3 kann auf Ihrem Computer klein sein, nach der Konvertierung jedoch viel größer. Ermitteln Sie die Flash-Anforderungen anhand des Importberichts, der das dekodierte Audio berücksichtigt.

Jede importierte Datei ist auf 3 MiB dekodierter Mono-PCM, etwa 35,7 Sekunden, begrenzt. Längere Dateien werden abgeschnitten. Die Dekodierung und Ratenkonvertierung von MP3 kann länger dauern als der Import von PCM.

<a id="import-the-files"></a>

### Die Dateien importieren

1. Legen Sie die vorbereitete microSD-Karte ein.
2. Öffnen Sie **Tools > Setup**.
3. Wählen Sie `IMPORT AUDIO FILES FROM /LILLA_AUDIO` und drücken Sie Select.
4. Überprüfen Sie den Importbericht und die letzte Löschwarnung.
5. Bestätigen Sie dies erst, wenn Sie bereit sind, die aktuelle Bibliothek und die aktuellen Aufnahmen zu ersetzen.
6. Warten Sie, bis der Kopiervorgang, die Speicherkonfiguration und der Neustart abgeschlossen sind.
7. Öffnen Sie Sound Edit und wählen Sie die importierte Quelle aus, die Sie verwenden möchten.

Hören Sie sich nach dem Import einige Quellen in Sound Edit an, bevor Sie einen gesamten Patch neu erstellen. Bestätigen Sie den Angriff, den Endpunkt und die Tonhöhe, insbesondere bei einer langen oder konvertierten Quelle.

**Für ein Stereoinstrument:** Exportieren Sie links und rechts als separate Monodateien, importieren Sie beide, weisen Sie sie zwei Sound-Slots mit passenden Grundtonarten und Bereichen zu und schwenken Sie sie dann nach links und rechts. Der Standard-Stereodateiimport selbst erzeugt eine Monoquelle.

Der Importbildschirm meldet ungültige Dateien, Duplikate und Probleme mit der Flash-Kapazität. Wenn für das Sampling nicht mehr genügend Speicherplatz vorhanden ist, bereiten Sie eine kleinere Bibliothek oder kürzere Dateien vor.

<a id="recording-with-sampler"></a>

## Mit Sampler aufnehmen

Sampler zeichnet die Line-Eingabe in Flash auf und ermöglicht Ihnen das Anhören, Konvertieren oder Exportieren des Ergebnisses.

<img src="doc/assets/images/6.jpg" alt="Sampler in PAUSE+REC mit linken und rechten Eingangsanzeigen" width="37%">

*Mit den Eingangsanzeigen können Sie die Aufnahmeverstärkung vor Beginn einstellen. Freie Aufnahmezeit und freier Speicherplatz für Audiodateien werden separat angezeigt.*

<a id="understand-the-recording-stages"></a>

### Die Aufnahmeschritte verstehen

**Überwachen, Aufzeichnen, Stoppen, Vorhören, dann Konvertieren oder Exportieren.**

Mit der Überwachung in `PAUSE+REC` können Sie die Quelle und die Pegel vorbereiten. `MONO_REC` oder `STEREO_REC` beginnt mit der Aufnahme. `STOP` beendet es. Anschließend erstellt `MAKE_RAW` eine Quelle für einen Patch, während `EXPORT_WAV_TO_SD` eine Datei zur Verwendung außerhalb von LILLA erstellt.

Beim Starten eines Takes wird Audio 20 ms lang aufgezeichnet, bevor die nächste Steueraktion akzeptiert wird, sodass die anfängliche Einblendung abgeschlossen werden kann. Nach dieser kurzen Pause wird ein sofortiger Stopp ausgeführt und der Take bleibt erhalten. Stoppen oder Verlassen Sampler wartet, bis die Aufnahme beendet ist, bevor sie gespeichert wird.

Eine Flash-Aufnahme kann über die Tastatur abgespielt und vor der Konvertierung bearbeitet werden. Durch eine erfolgreiche RAW-Konvertierung oder einen WAV-Export wird die Quellaufzeichnung entfernt und deren Aufzeichnungsspeicherplatz freigegeben. Erstellen Sie zunächst ein Backup, wenn Sie die vollständige Originalaufnahme beibehalten möchten.

<a id="make-a-recording"></a>

### Eine Aufnahme erstellen

1. Schließen Sie Ihre Audioquelle an den Stereo-Line-Eingang an.
2. Stellen Sie den Modes-Selektor auf **Sampler** ein.
3. Wählen Sie `PAUSE+REC`, um den Eingang vor der Aufnahme zu überwachen.
4. Passen Sie die angezeigte Line-Eingangsverstärkung mit Value an, wenn das Verstärkungsfeld ausgewählt ist. Beobachten Sie beide Pegelanzeigen und vermeiden Sie anhaltende rote Spitzen.
5. Wählen Sie `MONO_REC` oder `STEREO_REC`.
6. Starten Sie Ihre Quelle und sehen Sie sich die verstrichene Zeit und den verfügbaren Aufnahmespeicher an.
7. Wählen Sie `STOP` aus, wenn Sie fertig sind.
8. Wählen Sie die Aufnahme aus, die Sie anhören möchten, und passen Sie deren Wiedergabelautstärke nach Bedarf an.

**Stellen Sie die Verstärkung anhand des lautesten Teils der Quelle ein.** Ein Pegel, der während einer ruhigen Passage angenehm aussieht, kann bei Akzenten übersteuern. Proben Sie diesen lauten Abschnitt in `PAUSE+REC` und nehmen Sie dann die Aufnahme auf. Eine nachträgliche Reduzierung der Wiedergabelautstärke kann die am Eingang aufgezeichnete Verzerrung nicht rückgängig machen.

Die Speicheranzeige unterscheidet den verfügbaren Speicherplatz für Aufnahmen vom verfügbaren Speicherplatz für RAW-Dateien. Eine Aufnahme kann auch dann passen, wenn nicht genügend Platz für die Konvertierung in RAW vorhanden ist.

<a id="edit-a-recording-before-conversion-or-export"></a>

### Eine Aufnahme vor Konvertierung oder Export bearbeiten

1. Beenden Sie die Aufnahme mit `STOP` oder wählen Sie eine bereits abgeschlossene Aufnahme in Sampler aus.
2. Drücken Sie **S1** für eine Monoaufnahme. Für Stereo drücken Sie **S1** für den linken Kanal oder **S2** für den rechten Kanal.
3. Spielen Sie in Sound Edit die Aufnahme über Ihr MIDI-Keyboard ab, während Sie die Wiedergabeeinstellungen anpassen.
4. Stellen Sie mit **From** und **To** den ersten und letzten Sample-Wert ein, die als **A/B**-Grenzen angezeigt werden. Verwenden Sie Step, um den Bereich genauer einzustellen.
5. Passen Sie Tonhöhe, Verstärkung, Pan, MIDI-Kanal, Attack-Kurve, ADSR, Wiedergabemodus oder Noclick nach Bedarf an.
6. Wählen Sie `RETURN` und drücken Sie Select, um zu **SAMPLER** zurückzukehren.

Die Quelle bleibt die ausgewählte Aufnahme in Flash; Der Ton wird beim Zuschneiden oder Bearbeiten nicht neu geschrieben und die Quellenauswahl kann in diesem Editor nicht geändert werden. Die Tastaturwiedergabe verwendet die ausgewählte Region und bleibt während der Bearbeitung verfügbar.

Bei einer Stereoaufnahme wird jeder bearbeitete Wiedergabeparameter automatisch auf den anderen Kanal übertragen: A/B, Stimmung und Auto Tune, Gain, Pan, MIDI-Kanal, Attack-Kurve, ADSR, Wiedergabemodus und Noclick. Änderungen auf einem beliebigen Kanal wirken auf beide. Die ursprünglichen Pan-Positionen links/rechts bleiben erhalten, bis Sie Pan bearbeiten. Eine Änderung oder Zentrierung von Pan setzt beide Kanäle auf denselben Wert.

Der Sampler-Editor bietet nur `RETURN`, ohne separate Auswahl zum Speichern oder Verwerfen. A/B-Grenzen werden automatisch gespeichert und für die spätere Wiedergabe und den Export beibehalten, auch nach Auswahl einer anderen Aufnahme oder Neustart von LILLA. Auch beim Verlassen des Editors bleiben die Grenzen erhalten. Andere Wiedergabeparameter bleiben in der aktuellen Bearbeitungssitzung aktiv, werden jedoch nicht als dauerhafte Aufnahmeeinstellungen gespeichert.

RAW-Konvertierung und WAV-Export verwenden den gespeicherten A/B-Bereich einschließlich beider Grenzwerte. Stereoexporte nutzen für beide Kanäle denselben Anfang und dasselbe Ende, sodass ihre Dauer übereinstimmt. Diese Vorgänge kopieren den gewählten Audiobereich; Tonhöhe, Hüllkurve, Gain, Pan und andere Wiedergabeeffekte des Editors werden nicht in das exportierte Audio eingerechnet. Um die gesamte Aufnahme zu exportieren, setzen Sie A/B vor dem Export auf die Grenzen der vollständigen Aufnahme zurück.

<a id="make-a-playable-raw-file"></a>

### Eine spielbare RAW-Datei erstellen

1. Wählen Sie die Aufnahme aus, bearbeiten Sie bei Bedarf ihren A/B-Bereich mit S1/S2 und wählen Sie anschließend `RETURN`.
2. Wählen Sie `MAKE_RAW` und bestätigen Sie die Konvertierung.
3. Wählen Sie die verfügbare Ausgabe: `MAKE_MONO`, `MAKE_LEFT`, `MAKE_RIGHT` oder `MAKE_BOTH`.
4. Warten Sie, bis die Konvertierung abgeschlossen ist.
5. Öffnen Sie Sound Edit und wählen Sie die generierte RAW-Quelle aus.
6. Speichern Sie den Patch, der ihn verwendet.

| Konvertierungsauswahl | Ergebnis |
| --- | --- |
| `MAKE_MONO` | Erstellen Sie aus der Aufnahme eine Monoquelle. |
| `MAKE_LEFT` | Erstellen Sie eine Quelle aus dem linken Kanal der Stereoaufnahme. |
| `MAKE_RIGHT` | Erstellen Sie eine Quelle aus ihrem rechten Kanal. |
| `MAKE_BOTH` | Erstellen Sie separate linke und rechte Quellen. |

Die verfügbaren Optionen hängen von der ausgewählten Aufnahme ab. Bei Stereoaufnahmen können links und rechts zu separaten RAW-Quellen werden. `CANCEL` verlässt die Konvertierungsoptionen. Wenn LILLA meldet, dass keine RAW-Datei erstellt werden kann, überprüfen Sie den freien RAW-Speicher und die verfügbaren Dateinamen.

Bei der Konvertierung wird die gespeicherte A/B-Region kopiert. Nachdem alle angeforderten RAW-Dateien erfolgreich erstellt wurden, wird die ursprüngliche Sampler-Aufzeichnung gelöscht und ihr Speicherplatz freigegeben. Bei einer fehlgeschlagenen Konvertierung bleibt die Aufzeichnung erhalten. Sichern Sie den Original-Take vor der Konvertierung, wenn Sie ihn behalten möchten.

<a id="export-a-wav-file"></a>

### Eine WAV-Datei exportieren

Legen Sie eine microSD-Karte ein, wählen Sie eine Aufnahme aus und wählen Sie `EXPORT_WAV_TO_SD`. Der exportierte WAV enthält seinen gespeicherten A/B-Bereich, behält das Mono- oder Stereo-Layout der Aufnahme bei und verwendet 16-Bit PCM bei 44,1 kHz.

Dateien werden in `/LILLAWAV_EXPORT` geschrieben, mit Namen wie `0M.wav` für Mono oder `0S.wav` für Stereo. Warten Sie auf die Erfolgsmeldung, bevor Sie die Karte entfernen.

Nach einem erfolgreichen WAV-Export löscht LILLA die ursprüngliche Sampler-Aufnahme und gibt ihren Aufnahmeplatz und ihre Flash-Pakete frei. Schlägt der Export fehl, bleibt die Aufnahme erhalten. Wenn Sie die Aufnahme in LILLA behalten und zusätzlich eine externe Kopie benötigen, verwenden Sie stattdessen eine Sicherung.

Sampler unterstützt bis zu 30 Aufnahmen. Wenn jeder Steckplatz belegt ist, wird `PAUSE+REC` ausgeblendet und Sie werden durch einen vorübergehenden Hinweis aufgefordert, eine Aufnahme zu löschen oder zu exportieren, bevor Sie eine weitere Aufnahme aufnehmen.

Der WAV-Export ist nützlich, wenn Sie einen Take auf einem Computer bearbeiten, eine Aufnahme teilen oder eine Audiokopie unabhängig von der LILLA-Konfiguration behalten möchten. Dieser Befehl exportiert Sampler-Aufzeichnungen; Es handelt sich nicht um einen allgemeinen Exportbefehl für jede RAW-Quelle in Flash.

`CANCEL_RECORDING` löscht die ausgewählte Aufnahme. Exportieren Sie zunächst alles, was Sie behalten möchten.

<a id="live-sampler"></a>

## Live Sampler

<img src="doc/assets/images/4.jpg" alt="Live Sampler vor der Aufnahme, mit Wiedergabe- und Puffersteuerung" width="37%">

*Die Ansicht „Leerer Puffer“ zeigt Kapazität, Eingangsverstärkung, Wiedergabemodus, Rückmeldung und Startpunktsteuerung.*

Live Sampler zeichnet in einem zirkulären PSRAM-Puffer auf. Es bietet etwa 40 Sekunden in Mono oder 20 Sekunden in Stereo. Während die Aufnahme fortgesetzt wird, ersetzt neues Audio den älteren Pufferinhalt.

Der Live-Puffer ist temporär und geht beim Ausschalten verloren. Verwenden Sie den folgenden Capture-to-Patch-Workflow, um eine ausgewählte Schleife beizubehalten.

<a id="record-and-explore"></a>

### Aufnehmen und erkunden

1. Stellen Sie den Modes-Selektor auf **Live Sampler** ein.
2. Wählen Sie vor der Aufnahme `MONO/STEREO`. Durch Ändern dieser Einstellung wird der Puffer gelöscht.
3. Wählen Sie `CAPTURE` aus, um mit der Aufzeichnung der Eingabe im Live-Puffer zu beginnen.
4. Wählen Sie `STOP`, um die Aufnahme einzufrieren und mit dem aufgenommenen Audio zu arbeiten.
5. Wählen Sie einen Wiedergabemodus und spielen Sie über Ihren MIDI-Controller.
6. Drehen Sie From, um den Anfang der Region anzupassen, und To, um ihre Länge anzupassen.
7. Drehen Sie Step, um die Bearbeitungsschrittweite zu ändern.

<a id="read-and-navigate-the-live-waveform"></a>

### Die Live-Wellenform lesen und navigieren

<img src="doc/assets/images/5.jpg" alt="Live Sampler mit einer aufgezeichneten Wellenform und einem ausgewählten Loop" width="37%">

*Hier beträgt das Anzeigefenster 1,3 Sekunden, während die ausgewählte Schleife 0,46 Sekunden beträgt. Das Zoomen der Ansicht und das Ändern der Schleifenlänge sind separate Vorgänge.*

| Feld | Bedeutung |
| --- | --- |
| `BUFFER` | Gesamte Live-Aufnahmekapazität für das ausgewählte Mono-/Stereo-Layout. |
| `LINE IN GAIN` | Verstärkung, die auf das eingehende Aufnahmesignal angewendet wird. |
| `PLAY MODE` | Richtung und Schleifenverhalten. Für die Live-Aufnahme in einen Ton ist ein Loop-Modus erforderlich. |
| `FEEDBACK` | Menge des vorherigen Materials, das während der Live-Aufnahme zurückgekoppelt wurde. |
| `WINDOW` | In der Wellenformansicht sichtbare Audiozeit. |
| `START POINT` | Startpositionsverhalten und seine angezeigte Position oder Beziehung zur Aufzeichnung. |
| `LOOP` | Dauer des ausgewählten Wiederholungsbereichs. |
| `STEP` | Inkrement, das zum Verschieben der Regionssteuerelemente verwendet wird. |

Ein kleinerer Window hilft bei der Untersuchung eines Transienten oder einer kurzen Schleife. Das ausgewählte Audio wird nicht automatisch gekürzt. Verwenden Sie **To**, um die Schleifenlänge anzupassen, und **From**, um den Anfang zu verschieben.

Beginnen Sie mit einem bescheidenen Feedback und passen Sie es beim Zuhören an. Durch Rückkopplung verändert sich das aufgenommene Material. Vergleichen Sie daher das Ergebnis, bevor Sie es weiter steigern.

Durch Drücken von Step wird das Verhalten der Live-Startpunktsperre umgeschaltet. **FIXED** enthält einen Speicherort im Ringpuffer. Das entsperrte Verhalten folgt der Aufnahmeposition und die Seite kann je nach Offset **SYNC**, **BEHIND** oder eine andere relative Position anzeigen.

Stoppen Sie bei Ihrer ersten Aufnahme die Aufnahme und arbeiten Sie an einem festen Bereich. Sobald Sie mit dem Auffinden und Zuschneiden von Material vertraut sind, experimentieren Sie während der Aufnahme mit einem beweglichen Startpunkt. Beim Wickeln des Puffers wird altes Material ersetzt.

`ERASE` löscht den aufgezeichneten Puffer. Durch den Wechsel von Mono/Stereo wird es ebenfalls zurückgesetzt. Wählen Sie daher das Layout aus, bevor Sie Material aufnehmen, das Sie behalten möchten.

<a id="continuous-fwd-playback-while-recording"></a>

### Kontinuierliche FWD-Wiedergabe während der Aufnahme

Während Live Sampler aufzeichnet, werden gehaltene Noten in **FWD** um den Ringpuffer herum fortgesetzt, anstatt nach einer Pufferlänge anzuhalten. Dies gilt für **SYNC**, relative Startpositionen und **FIXED**, in Mono und Stereo. Tiefere Noten können daher länger als 40 Sekunden in Mono aktiv bleiben (80 Sekunden bei halber Geschwindigkeit). Note-Off und die Klanghüllkurve steuern weiterhin die Stimme.

Während des ersten Füllvorgangs werden noch nicht aufgenommene Audiosignale immer noch ausgeblendet und die Stimme stoppt. Sobald die Aufnahme stoppt, nimmt der FWD ab der aktuellen Wiedergabeposition sein normales One-Shot-Verhalten wieder auf. Der Rückwärts- und Schleifenmodus bleibt unverändert.

<a id="stereo-recording-compressor"></a>

### Kompressor für die Stereoaufnahme

Drehen Sie auf der Seite Live Sampler **Select**, um den gelben Wert `ON`/`OFF` neben `COMPRESSOR` hervorzuheben, und drücken Sie dann **Select**, um ihn umzuschalten. Das Steuerelement befindet sich in der Zeile `FEEDBACK`, ausgerichtet an `LOOP`. Beim Einschalten startet es **aus**; Die Einstellung bleibt während der aktuellen Sitzung erhalten, wird jedoch nicht in einem Patch gespeichert. Alle Soundtasten, **S1-S8**, bleiben für die Aufnahme in die entsprechenden Patch-Slots verfügbar.

Der Kompressor verarbeitet die Summe aus Line-Eingang und Feedback, bevor er in den Live-Puffer schreibt. Die Audioverarbeitung läuft nur, während Live Sampler aufzeichnet, auch wenn die Seite Mixer oder Delay geöffnet ist; Andernfalls entleert der Block seine Eingänge, ohne Ausgangsblöcke zuzuweisen. Die ON/OFF-Einstellung bleibt erhalten und der Lookahead-Verlauf wird gelöscht, wenn die Aufzeichnung fortgesetzt wird. Links und rechts haben die gleiche Verstärkungsreduzierung, auch bei der Aufnahme einer Monomischung. Die Reduzierung der Verstärkung beginnt bei etwa -6 dBFS und begrenzt die Sample-Spitzen auf etwa -1 dBFS, sobald sie vollständig aktiviert ist. Es repariert kein Clipping, das bereits am Eingang oder anderswo im Rückkopplungspfad aufgetreten ist, und verarbeitet kein bereits aufgezeichnetes Material.

Ein 128-Sample-Lookahead verlängert den Aufnahmepfad um etwa 2,9 ms, auch wenn der Kompressor ausgeschaltet ist. Beim Umschalten erfolgt ein allmählicher 10-ms-Übergang zwischen gleich verzögerten Signalen, sodass die Aufzeichnungszeitleiste nicht springt. Die Verstärkungswiederherstellung dauert ungefähr 100 ms pro Zeitkonstante. Starke Komprimierung kann dennoch den Klang und das Rückkopplungsverhalten verändern. Während des Bypasses oder des Übergangs zum/vom Bypass ist kein vollständiger Spitzenschutz gewährleistet.

<a id="capture-a-live-loop-into-a-patch"></a>

### Eine Live-Schleife in einen Patch übernehmen

1. Speichern Sie alle ausstehenden Änderungen am Performance-Patch, bevor Sie mit diesem Ablauf beginnen.
2. Nehmen Sie Live-Audio auf und wählen Sie dann `STOP` aus.
3. Wählen Sie einen **Loop**-Wiedergabemodus und verfeinern Sie die Region.
4. Drücken Sie die gewünschte Slot-Taste **S1-S8**.
5. Wenn Sie einen belegten Erfassungssteckplatz ersetzen, antworten Sie mit `REPLACE CAPTURE?`, bevor Sie fortfahren.
6. Wenn Sie dazu aufgefordert werden, spielen Sie eine MIDI-Taste, um den Grundton des aufgenommenen Sounds festzulegen, oder wählen Sie „Abbrechen“.
7. Erfassen Sie bei Bedarf weitere Regionen in anderen Slots.
8. Wechseln Sie zu Performance, überprüfen Sie den neu erstellten Patch und wählen Sie `SAVE`.
9. Warten Sie, bis die ausstehenden Audio-Schreibvorgänge abgeschlossen sind, bevor Sie das Gerät ausschalten.

Die erste Übernahme verwendet die erste freie normale Patch-ID im Bereich **0-199**. Patch **200** bleibt der temporäre Sampling-Arbeitsbereich.

Jeder aufgenommene Ton wird zunächst auf der für die Grundton-Eingabeaufforderung verwendeten Taste abgespielt: Die unteren und oberen Tastengrenzen sind auf dieselbe Taste eingestellt. Erweitern Sie den Tastaturbereich in Performance, wenn Sie es melodisch spielen möchten.

**Stereo-Slot-Zuweisung:** Wählen Sie einen Slot mit dem folgenden freien Slot aus, z. B. S1 mit freiem S2, um links und rechts getrennt zu erfassen. Die beiden Sounds werden nach links und rechts verschoben. Durch Ersetzen eines vorhandenen Erfassungspaars kann dieses Paar wiederverwendet werden. Wenn kein zweiter Slot verfügbar ist, einschließlich einer neuen Aufnahme auf S8, wird die ausgewählte Region als Mono-Mix der beiden Kanäle in einem Slot erfasst.

Der ausgewählte Bereich muss in den Capture-Cache passen; außerdem müssen freie Patch-, Sound- und Dateiressourcen verfügbar sein. Der Live-Puffer kann länger sein als ein einzelner übernommener Sound. Verkürzen Sie die ausgewählte Schleife, wenn sie die Übernahmegrenze überschreitet.

Aufgenommenes Audio verbleibt zunächst in PSRAM. Beim Speichern des Patches werden die ausstehenden Audioaufnahmen als RAW-Dateien in Flash geschrieben. Warten Sie, bis der Speichervorgang abgeschlossen ist. Ein fehlgeschlagener Speichervorgang muss vor dem Ausschalten erneut versucht werden.

<a id="if-the-previous-patch-has-unsaved-edits"></a>

### Wenn der vorherige Patch ungespeicherte Änderungen enthält

Die Nachricht:

> OPEN PERFORMANCE<br>
> AND SAVE THE PREVIOUS PATCH

bezieht sich auf den Performance-Patch, den Sie vor dem Wechsel zu Live Sampler verwendet haben.

1. Stoppen Sie die Live-Aufzeichnung, wenn sie noch läuft.
2. Kehren Sie zu Performance zurück und schließen Sie die Exit-Bestätigung ab.
3. Speichern Sie den vorherigen Patch.
4. Zurück zu Live Sampler.
5. Wählen Sie die gewünschte Region aus und drücken Sie erneut die Sound-Slot-Taste.

Sie werden nicht aufgefordert, Patch 200 zu speichern. Die Warnung schützt Änderungen am vorherigen normalen Patch, bevor ein neuer erfasster Patch erstellt wird.

<a id="capture-messages"></a>

### Meldungen zur Übernahme von Live-Audio

| Nachricht | Nächste Aktion |
| --- | --- |
| `NO RECORDED AUDIO` | Zeichnen Sie einige Eingaben auf, bevor Sie sie abspielen oder aufzeichnen. |
| `STOP REC AND SELECT LOOP MODE` | Stoppen Sie die Aufnahme und wählen Sie einen Loop-Wiedergabemodus. |
| `LOOP TOO LONG FOR CACHE` | Verkürzen Sie den ausgewählten Bereich. |
| `NO FREE PATCH` | Geben Sie einen normalen Patch-Slot frei, nachdem Sie alles Notwendige erhalten haben. |
| `NO FREE SOUND / CACHE / FILE` | Speichern Sie ausstehende Arbeiten und überprüfen Sie verfügbare Sound-, Audio-Cache- und Dateiressourcen. |
| `CAPTURE CACHE UNAVAILABLE` | Der erforderliche Audiospeicher konnte nicht erworben werden; Bewahren Sie ausstehende Arbeiten auf, bevor Sie es erneut versuchen. |
| `SAVE BUSY - TRY AGAIN` | Lassen Sie die Audioaktivität ruhen und versuchen Sie es erneut mit dem Speichern. |
| `RAW SAVE FAILED - RETRY` | Versuchen Sie erneut zu speichern und lassen Sie das Instrument eingeschaltet, während die Audioaufnahme noch aussteht. |

<a id="midi-loop"></a>

## MIDI Loop

<img src="doc/assets/images/8.jpg" alt="MIDI Loop-Seite mit vier Spuren, Pegel, Verschiebung und Transposition" width="37%">

*Die vier Spalten sind MIDI-Spuren. Die unteren Soundanzeigen zeigen die Aktivität an, die mit den Sounds des Patches verbunden ist.*

MIDI Loop zeichnet MIDI-Ereignisse in vier Spuren auf und spielt sie über den aktuellen Patch ab. Track 1 ist der Master-Track und legt die Loop-Dauer fest.

<a id="record-your-first-loop"></a>

### Die erste Schleife aufnehmen

1. Legen Sie eine microSD-Karte ein, um Loops zu speichern.
2. Wählen Sie einen Patch und prüfen Sie, ob Ihr Controller die beabsichtigten Sounds wiedergibt.
3. Stellen Sie den Modes-Selektor auf **MIDI Loop** ein.
4. Drücken Sie **Rec 1**, spielen Sie Ihre Phrase ab und drücken Sie erneut Rec 1, um die Aufnahme zu beenden.
5. Hören Sie sich den sich wiederholenden Master-Track an.
6. Drücken Sie Rec 2, Rec 3 oder Rec 4, um eine weitere Spur aufzunehmen. Drücken Sie dieselbe Aufnahmetaste erneut, um den Vorgang abzuschließen.
7. Verwenden Sie das Menü, um die Schleife zu speichern.

Die Aufnahme in eine belegte Spur ersetzt deren Ereignisse. **Durch die erneute Aufnahme von Track 1 werden auch die anderen Spuren gelöscht**, da dadurch ein neuer Master-Loop erstellt wird. Speichern Sie einen vorhandenen Loop, bevor Sie dessen Master-Spur ersetzen.

Eine ohne Ereignisse scharfgeschaltete Aufnahme wird nach ca. 20 Sekunden abgebrochen.

<a id="add-parts-without-replacing-the-master"></a>

### Weitere Stimmen hinzufügen, ohne die Masterspur zu ersetzen

Nehmen Sie zuerst auf Track 1 die Stimme auf, die die Phrasenlänge vorgibt. Sobald sie korrekt wiederholt wird, ergänzen Sie eine zweite Stimme auf Track 2 und fahren dann mit den Spuren 3 und 4 fort. Verwenden Sie einen anderen MIDI-Kanal am Controller, wenn die neue Stimme einen anderen Sound in einem multitimbralen Patch ansprechen soll.

Die aufgezeichneten Ereignisse lösen die aktuellen Sounds des Patches aus. Das Ändern einer Quelle, eines Tastaturbereichs oder einer MIDI-Zuweisung kann daher den Klang einer vorhandenen Schleife ändern. Bewahren Sie das Patch und die Audiobibliothek neben der Schleife auf, wenn Sie das Arrangement später reproduzieren möchten.

<a id="play-and-edit"></a>

### Abspielen und bearbeiten

- Drehen Sie **Loop**, um gespeicherte Loops zu durchsuchen.
- Drücken Sie Loop, um die Spurgruppe zu stoppen oder neu zu starten.
- Drücken Sie einen **Track**-Encoder, um diesen einzelnen Track zu stoppen oder zu starten.
- Drehen Sie **Tempo**, um das Wiedergabe-Timing zu ändern; Drücken Sie darauf, um die Timing-Einstellung zurückzusetzen.
- Verwenden Sie Select, um die Spurparameterzeile auszuwählen, und drehen Sie dann jeden Spur-Encoder, um den Pegel, die Tonhöhe oder die Zeitverschiebung dieser Spur zu ändern.

<a id="understand-the-track-controls"></a>

### Die Spurregler verstehen

| Reihe | Was es verändert | Ausgangspunkt |
| --- | --- | --- |
| `LEVEL` | Der Wiedergabepegel dieser Spur. | 1,0 für ein nicht angepasstes Niveau. |
| `SHIFT` | Der Timing-Offset des Tracks innerhalb der Schleife. | 0,00 Sekunden ohne Verschiebung. |
| `TRANSP` | Die Notentransposition des Tracks. | 0 Tasten für die Originalnoten. |

Mit einem kleinen Shift-Wert können Sie eine Stimme gegenüber den anderen Spuren vorziehen oder verzögern. Hören Sie sowohl am Schleifenübergang als auch in der Mitte der Phrase. Probieren Sie mit der Transposition eine andere Tonhöhe aus und setzen Sie sie anschließend zum Vergleich mit dem Original auf null zurück.

Durch Drehen von Tempo ändert sich das Timing der MIDI-Sequenz. Die Wellenform eines Samples wird nicht neu geschrieben und eine aufgezeichnete Audiophrase wird nicht automatisch zeitlich gestreckt.

<a id="save-loops"></a>

### Schleifen speichern

Verwenden Sie `SAVE`, um eine gespeicherte Schleife zu aktualisieren, und `SAVE_AS_NEW`, um eine andere Version beizubehalten. `NEW` startet eine neue Schleife; `DELETE` entfernt eine gespeicherte Schleife.

Loop-Dateien werden in `/LILLALOOP` gespeichert. Bewahren Sie eine Kopie dieses Ordners auf, wenn Sie Ihre Arbeit archivieren. Loop-Dateien enthalten MIDI-Daten. Behalten Sie daher auch den erforderlichen Patch und das Quellaudio bei.

<a id="mixer-delay-and-filters"></a>

## Mixer, Delay und Filter

<a id="mixer"></a>

### Mixer

<img src="doc/assets/images/7.jpg" alt="Mixer-Seite mit Tonquellen, Line-Eingang und separaten Ausgangsrouten" width="37%">

*Die hervorgehobene Spalte ist die ausgewählte Quelle. LINEOUT und MONITOR sind separate Routen.*

Öffnen Sie **Tools > Mixer**, um die Quellenverstärkung oder Stummschaltung, das Panorama und die Weiterleitung an die Line- und Monitorausgänge anzupassen. Drehen Sie Select, um eine Quellspalte auszuwählen, und drücken Sie dann darauf, um die Felder einzugeben. Drehen Sie Select, um ein Feld auszuwählen, und Value, um seine Einstellung zu ändern. Drücken Sie Select, um zur Quellenauswahl zurückzukehren.

Nutzen Sie die getrennten Leitungs- und Monitorwege, um zu entscheiden, was Ihr Publikum hört und was Sie während der Überwachung hören. Wenn eine Quelle still ist, überprüfen Sie deren Stummschaltung/Verstärkung und Ausgaberoute sowie die Patch-Lautstärke.

<a id="delay"></a>

### Delay

<img src="doc/assets/images/9.jpg" alt="Delay-Seite mit Sound-Routing, Feedback, Zeit und Stereomodulation" width="37%">

*Die Zeile ROUTING wählt aus, welche Sound-Slots die Verzögerung speisen. Bei den Beispielwerten handelt es sich nicht um empfohlene Standardwerte.*

Öffnen Sie **Tools > Delay**, um Feedback, Verzögerungszeit, das Links-/Rechts-Zeitverhältnis, Modulationsquelle, Modulationsfrequenz, Modulationstiefe und Links-/Rechts-Modulationsphase anzupassen.

Beginnen Sie mit einem geringen Feedback und steigern Sie es dann beim Zuhören. Überprüfen Sie das Delay-Routing des Instruments, wenn Sie kein verzögertes Signal hören. Speichern Sie den Patch, um seine Verzögerungseinstellungen beizubehalten.

Für einen ersten Delay-Sound leiten Sie ein Instrument weiter, verwenden einen moderaten Feedback-Pegel und wählen eine deutlich hörbare Delay-Zeit. Spielen Sie kurze Noten mit Lücken dazwischen, damit Sie die Wiederholungen hören können. Passen Sie dann den Zeitunterschied links/rechts für die Stereotrennung an.

Die Modulation variiert die Verzögerung über die Zeit. Führen Sie die Tiefe schrittweise ein und passen Sie dann die Geschwindigkeit und die linke/rechte Phase an, während Sie zuhören. Höheres Feedback führt zu einer Anhäufung von Wiederholungen. Reduzieren Sie es daher, wenn das verzögerte Signal den trockenen Klang überlagert.

<a id="filters-and-sound-character"></a>

### Filter und Klangcharakter

<img src="doc/assets/images/3.jpg" alt="Seite „Instrument VCF“ mit Tiefpassfilterung und LFO-Modulation" width="37%">

*Der einzelne VCF formt ein Instrument. Darüber bleibt der gemeinsame Cutoff LPF sichtbar.*

Um von Performance auf VCF zuzugreifen, drücken Sie die Taste des aktiven Sounds, um Sound Edit zu öffnen, und drücken Sie dann dieselbe Soundtaste erneut. Verwenden Sie Select, um einen Filterparameter hervorzuheben, und Value, um ihn zu bearbeiten.

| Filtertyp | Hörbarer Effekt |
| --- | --- |
| Lowpass | Reduziert Frequenzen oberhalb der Grenzfrequenz; Nützlich zum Abdunkeln einer hellen Quelle. |
| Highpass | Reduziert tiefe Frequenzen; Nützlich zum Ausdünnen eines Klangs oder zum Entfernen von Gewichten im Tieftonbereich. |
| Bandpass | Betont einen Bereich zwischen niedrigen und hohen Frequenzen. |
| Notch | Entfernt ein Frequenzband. |
| None | Deaktiviert den Instrumentenfilter. |

Resonanz betont die Reaktion des Filters um seine charakteristische Frequenz. Beginnen Sie mit einer moderaten Einstellung und hören Sie dann zu, während Sie den Cutoff verschieben. Die Modulationsquelle und -tiefe bestimmen, ob und wie sich diese Einstellung im Laufe der Zeit verändert; Das Frequenz-/Zeitfeld folgt der gewählten Modulationsart.



Die gemeinsame Steuerung **Cutoff** verändert den Tiefpassfilter. Drücken Sie darauf, um die maximale Abschaltung wiederherzustellen. **Resolution** und **Downsampling** fügen digitale Farbgebung hinzu.

Die Instrumentenseite VCF bietet Einstellungen für Filtertyp, Cutoff, Resonanz und Modulation. Passen Sie diese an, während Sie den ausgewählten Sound abspielen, damit Sie hören können, wie sie mit seinem Sample und seiner Hüllkurve interagieren.

Ein gesperrtes Instrument ist vor ausgewählten Leistungsänderungen geschützt. Überprüfen Sie `LOCK`, wenn Pitch Bend, Auflösung oder Downsampling scheinbar keinen Einfluss auf diesen Klang haben.

<a id="setup-and-midi-controls"></a>

## Setup und MIDI-Steuerung

<img src="doc/assets/images/10.jpg" alt="Setup-Seite mit Tuning-Konventionen, MIDI-Zuweisung und Speichervorgängen" width="37%">

*Setup kombiniert globale Wiedergabeeinstellungen mit Audiobibliothek- und Sicherungsvorgängen.*

Öffnen Sie **Tools > Setup** für:

- `KEY STEP`: Tonhöhenzuordnungsschritte von einem Halbton, einem halben Halbton, einem Viertel oder einem Achtel eines Halbtons.
- `FIRST OCTAVE`: die Oktavzahlenkonvention, die für die Notenanzeige verwendet wird.
- `CONTROL CHANGE ASSIGNMENT`: MIDI CC Zuweisungen für die Verstärkung von Sound 1–8 und den Tiefpass-Cutoff.
- Audioimport, Sicherung, Wiederherstellung und Zurücksetzen auf Werkseinstellungen.

<a id="pitch-steps-and-note-names"></a>

### Tonhöhenschritte und Notennamen

Wenn `KEY STEP` einen Halbton hat, verwenden benachbarte MIDI-Noten den normalen chromatischen Tonhöhenabstand. Kleinere Schritte verteilen ein kleineres Tonhöhenintervall über jeden Tastaturschritt und ermöglichen so das Spielen im Halb-, Viertel- oder Achtelhalbton. Kehren Sie bei der Überprüfung einer herkömmlichen Tastaturbelegung zu einem Halbton zurück.

`FIRST OCTAVE` ändert die in der Anzeige verwendete Oktavnummerierung. Es hilft dabei, die Namenskonvention Ihres Controllers anzupassen. Es sollte nicht als Ersatz für die Einstellung des Grundtons des Instruments verwendet werden.

<a id="assign-a-controller-knob"></a>

### Einen Controller-Regler zuweisen

<img src="doc/assets/images/11.jpg" alt="Control Change Assignment-Seite für acht Klangverstärkungen und LPF Cutoff" width="37%">

*Jedes Ziel kann eine CC-Zuweisung haben. Ein Bindestrich bedeutet keine Zuordnung.*

1. Öffnen Sie `CONTROL CHANGE ASSIGNMENT`.
2. Wählen Sie das Verstärkungsziel oder die LPF-Grenze.
3. Stellen Sie die gewünschte CC-Nummer ein.
4. Stellen Sie den Knopf oder Schieberegler Ihres MIDI-Controllers so ein, dass dieser CC übertragen wird.
5. Für eine Klangverstärkung verwenden Sie den diesem Klang zugewiesenen MIDI-Kanal.
6. Kehren Sie zurück und testen Sie die Steuerung während des Spielens.

Verwenden Sie einen Bindestrich, um einem Ziel keine Zuweisung zu geben. Überprüfen Sie sowohl die Controller-Nummer als auch den Sendekanal, wenn das Bewegen der externen Steuerung keine Auswirkung hat.

<a id="check-incoming-midi"></a>

### Eingehende MIDI-Daten prüfen

<img src="doc/assets/images/15.jpg" alt="MIDI-Monitor, der eine NoteOn-Nachricht, einen Kanal, eine Note und eine Anschlagstärke anzeigt" width="37%">

*Dieses Beispiel bestätigt den Empfang eines NoteOn auf Kanal 1. Der Notenname folgt der aktuellen Konvention für die Oktavanzeige.*

Öffnen Sie **Tools > Test**, um eingehende MIDI-Nachrichtentypen zu überwachen. Dies ist nützlich, wenn Sie überprüfen möchten, ob der Controller Noten, Pitchbend, Aftertouch oder Steueränderungen sendet.

Wenn der Monitor Noten empfängt, der Performance aber stumm ist, funktioniert die Verbindung: Überprüfen Sie als Nächstes die Kanalzuordnung, die Tastenbereiche und das Audio-Routing. Wenn keine Meldung angezeigt wird, überprüfen Sie den Controller-Ausgang, das Kabel und die ausgewählte Verbindung, bevor Sie den Patch bearbeiten.

<a id="backup-and-restore"></a>

## Sicherung und Wiederherstellung

<a id="plan-a-complete-archive"></a>

### Ein vollständiges Archiv planen

Eine Konfigurationssicherung ist ein Teil der Sitzungserhaltung. Bewahren Sie die passende Audiobibliothek und die MIDI-loop-Dateien auf, damit die gespeicherten Einstellungen über das benötigte Material verfügen.

| Artikel | Nummerierte Konfigurations-/Aufzeichnungssicherung | Zusätzliche Aktion |
| --- | --- | --- |
| Gespeicherte Patches, Sounds und Konfiguration | Im Lieferumfang enthalten. | Speichern Sie aktuelle Änderungen vor dem Sichern. |
| Zuordnungen von Dateinamen | Im Lieferumfang enthalten. | Behalten Sie die entsprechenden Audio-Basisnamen bei. |
| Sampler Audioaufnahme | Im Lieferumfang enthalten. | Der WAV-Export ist auch für den Computerzugriff nützlich. |
| Sampler zeichnet A/B Grenzen auf | In aktuellen Backups enthalten. | Ältere Backups ohne Trimmmetadaten stellen den gesamten Aufnahmebereich wieder her. |
| Importierte RAW-Bibliothek | Nicht im Lieferumfang enthalten. | Bewahren Sie die ursprüngliche Importbibliothek separat auf. |
| RAW-Dateien, die aus Aufzeichnungen oder Live-Aufnahmen generiert wurden | Nicht als vollständiges RAW-library-Archiv enthalten. | Behalten Sie eine unabhängige, wiederherstellbare Audiokopie; Das alleinige Speichern auf Flash ist kein externes Backup. |
| MIDI-loop-Verzeichnis | Nicht im Lieferumfang enthalten. | Kopieren Sie `/LILLALOOP` von der Karte. |
| Temporärer Live-Puffer | Nicht im Lieferumfang enthalten. | Erfassen und speichern Sie nützliches Material vor dem Ausschalten. |

Für eine Live Sampler-Erfassung bietet der Befehl Sampler WAV-export dieser Firmware keinen allgemeinen RAW-library-Export. Gehen Sie nicht davon aus, dass ein nummeriertes Backup allein jede erfasste RAW-Quelle wiederherstellen kann, nachdem diese Quelle gelöscht wurde.

<a id="create-a-backup"></a>

### Eine Sicherung erstellen

1. Speichern Sie Ihre aktuellen Patch-Änderungen.
2. Legen Sie eine microSD-Karte mit ausreichend freiem Speicherplatz ein.
3. Öffnen Sie Tools > Setup.
4. Wählen Sie `NEW NUMBERED BACKUP IN /LILLABACKUP` und bestätigen Sie.
5. Warten Sie auf die Erfolgsmeldung der Sicherung.
6. Kopieren Sie den Sicherungsordner zur sicheren Aufbewahrung auf Ihren Computer.

Sicherungen werden in nummerierten Verzeichnissen wie `/LILLABACKUP/000001` erstellt. Sie enthalten die Konfiguration und die Sampler-Audioaufzeichnung.

Bewahren Sie Ihre importierte Quellbibliothek separat auf. Der Sicherungsvorgang kopiert nicht die gesamte importierte RAW-Bibliothek, den temporären Live-Puffer oder das MIDI-loop-Verzeichnis. Kopieren Sie `/LILLALOOP` separat und behalten Sie Ihre Importdateien. Speichern Sie Live-Aufnahmen in einem Patch, bevor Sie ihre Einstellungen archivieren, und berücksichtigen Sie die oben beschriebene RAW-audio-Einschränkung.

<a id="restore-a-backup"></a>

### Eine Sicherung wiederherstellen

<img src="doc/assets/images/13.jpg" alt="Bestätigungswarnung für die Wiederherstellung, dass Patches, Sounds und Aufnahmen ersetzt werden" width="37%">

*Wählen Sie YES erst, nachdem Sie das beabsichtigte Backup im Backup-Root der Karte vorbereitet haben.*

1. Wählen Sie auf Ihrem Computer das nummerierte Backup aus, das Sie wiederherstellen möchten.
2. Kopieren Sie **den Inhalt** dieses Verzeichnisses nach `/LILLABACKUP` auf der Karte und behalten Sie dabei die Konfiguration und die Aufzeichnungsdateien bei.
3. Überprüfen Sie, ob `/LILLABACKUP/LILLA_CONFIG.fram` vorhanden ist. Es reicht beispielsweise nicht aus, es nur in `000001` zu belassen.
4. Legen Sie die Karte ein und öffnen Sie Tools > Setup.
5. Wählen Sie `RESTORE CONFIG + AUDIO FROM /LILLABACKUP ROOT` und bestätigen Sie.
6. Warten Sie, bis die Wiederherstellung und ein etwaiger angeforderter Neustart abgeschlossen sind.
7. Überprüfen Sie die wiederhergestellten Patches und Aufzeichnungen und stellen Sie sicher, dass die importierten Quelldateien verfügbar sind.

Der Wiederherstellungsspeicherort sollte wie folgt aussehen:

```text
microSD-Wurzelverzeichnis/
  LILLABACKUP/
    LILLA_CONFIG.fram
    [zugehörige Aufnahme-Audiodateien der gewählten Sicherung]
```

Die Zeile in Klammern oben ist eine Beschreibung und kein zu erstellender Dateiname. Kopieren Sie die eigentlichen Aufnahmedateien zusammen mit der Konfiguration; Mischen Sie keine Dateien aus unterschiedlich nummerierten Backups.

Die Wiederherstellung ersetzt die Konfiguration und stellt die Audioaufzeichnung wieder her. Sichern Sie den aktuellen Zustand, bevor Sie einen anderen wiederherstellen. Behalten Sie die Originalsicherung bei: Ungültiger oder fehlender Aufnahmeton kann die Wiederherstellung einer Aufnahme verhindern.

<a id="factory-reset"></a>

### Werkseinstellungen wiederherstellen

<img src="doc/assets/images/14.jpg" alt="Bestätigung zum Zurücksetzen auf die Werkseinstellungen auf der Seite Setup" width="37%">

*Das Zurücksetzen auf die Werkseinstellungen ist ein destruktiver Konfigurationsvorgang und keine Möglichkeit, eine Bearbeitungsseite zu verlassen.*

`FACTORY RESET` löscht Patches, Sounds und Aufnahmen. Erstellen Sie vor der Bestätigung ein Backup. Warten Sie, bis der Reset und Neustart abgeschlossen ist.

<a id="updating-the-firmware-on-windows"></a>

## Die Firmware unter Windows aktualisieren

Die Firmware ist das Programm, das LILLA steuert. Zum Übertragen einer kompilierten Firmware unter Windows 10 oder 11 verwenden Sie **Teensy Loader (`teensy.exe`)**. Das Programm läuft eigenständig: Laden Sie es herunter und starten Sie es ohne Installation. **Teensyduino** ist die Arduino-Erweiterung für die Entwicklung; zum Übertragen der mitgelieferten HEX-Datei benötigen Sie weder diese Erweiterung noch Arduino IDE oder PlatformIO. Die [PJRC-Downloadseite](https://www.pjrc.com/teensy/td_download.html) erläutert den Unterschied zwischen Entwicklungswerkzeugen und dem eigenständigen Loader.

<a id="what-you-need"></a>

### Voraussetzungen

- Ihr LILLA-Instrument, das einen Teensy 4.1 verwendet.
- Ein Windows 10- oder 11-Computer.
- Ein zum Computer passendes **Datenkabel** USB und zum USB-C-Anschluss von LILLA. Ein reines Ladekabel kann keine Firmware übertragen.
- Die Firmware-Datei LILLA, zum Beispiel `Lilla_v7_0_2.hex`.
- Teensy Loader, heruntergeladen von PJRC.

<a id="download-the-firmware"></a>

### Die Firmware herunterladen

1. Öffnen Sie den [Hauptzweig des LILLA GitHub-Repositorys](https://github.com/SandroGrassia/Lilla_Audio_Sampler/tree/main). Vergewissern Sie sich, dass in der Zweigauswahl **main** angezeigt wird.
2. Öffnen Sie in der Dateiliste der obersten Ebene des Projekts die veröffentlichte `.hex`-Datei für Ihr Instrument, zum Beispiel `Lilla_v7_0_2.hex`. Dies ist die kompilierte Firmware; **Code > ZIP herunterladen** lädt stattdessen die Projektquellen herunter.
3. Klicken Sie auf der Dateiseite HEX auf **Rohdatei herunterladen**. Laden Sie die Firmware nur von **main** herunter. Der Zweig **develop** enthält laufende Arbeiten und kann fehlerhafte oder ungetestete Builds enthalten.
4. Speichern Sie die Datei in einem Ordner, den Sie leicht finden können, z. B. `Downloads/LILLA`. Stellen Sie sicher, dass der Name auf `.hex` und nicht auf `.html` oder `.txt` endet.

Die Versionsnummern in `Lilla_v7_0_2.hex` identifizieren die Firmware-Version und -Revision. Behalten Sie die heruntergeladene Kopie, wenn Sie genau diesen Build behalten möchten. Wenn auf **main** keine HEX-Datei verfügbar ist, warten Sie, bis der Betreuer sie veröffentlicht. Ersetzen Sie keine Datei aus **develop**.

<a id="download-teensy-loader"></a>

### Teensy Loader herunterladen

Öffnen Sie die [offizielle PJRC-Seite für den Windows-Loader](https://www.pjrc.com/teensy/loader_win10.html) und klicken Sie auf **Teensy Loader Program**. Speichern Sie `teensy.exe` und öffnen Sie die Datei per Doppelklick. Das kleine Teensy-Loader-Fenster sollte erscheinen. Sie können das Programm im selben Ordner wie die HEX-Datei aufbewahren. Laut den [PJRC-Anweisungen zur ersten Verwendung](https://www.pjrc.com/teensy/first_use.html) nutzt der Programmiermodus die integrierten USB-Treiber von Windows; ein zusätzlicher Programmiertreiber ist nicht erforderlich.

<a id="prepare-lilla"></a>

### LILLA vorbereiten

Speichern Sie Ihren aktuellen Patch und alle MIDI-Loops und schließen Sie ausstehende Aufnahme- oder Exportvorgänge ab. Verwenden Sie [Sicherung und Wiederherstellung](#backup-and-restore), um Ihre Arbeit zu sichern, bevor Sie die Firmware ändern. Überprüfen Sie alle mit der neuen Version gelieferten Kompatibilitäts- oder Migrationsanweisungen.

Senken Sie den Pegel Ihres Verstärkers oder Mischpults und verbinden Sie dann den USB-C-Anschluss des LILLA mit dem Datenkabel mit dem Computer. Halten Sie die Stromversorgung und den USB während der gesamten Programmierung angeschlossen.

<a id="upload-and-restart"></a>

### Übertragen und neu starten

1. Lassen Sie in Teensy Loader den **Automatikmodus** für dieses manuelle Verfahren ausgeschaltet.
2. Wählen Sie **Datei > HEX-Datei öffnen** und wählen Sie die heruntergeladene `Lilla_v7_0_2.hex` aus. Bestätigen Sie den im Loader angezeigten Dateinamen.
3. Drücken Sie kurz die **Firmware_upload mode**-Taste des LILLA und lassen Sie sie wieder los. Dies ist die Programmiertaste, nicht die On/off-Taste. Das aktuelle Programm des Instruments stoppt und der Lader sollte den Teensy erkennen.
4. Wählen Sie **Operationen > Programm**. Warten Sie, bis **Download abgeschlossen** ist, bevor Sie die Verbindung trennen.
5. Wählen Sie **Operationen > Neustart**. LILLA sollte neu starten.
6. Überprüfen Sie die Firmware-Version auf dem Begrüßungsbildschirm von LILLA. Auf dem Begrüßungsbildschirm sollte Version 7.0.2 angezeigt werden. Laden Sie einen bekannten Patch und prüfen Sie die Wiedergabe bei niedriger Hörlautstärke.

Die oben beschriebenen manuellen Schritte folgen den [PJRC-Anweisungen für den Windows-Loader](https://www.pjrc.com/teensy/loader_win10.html). Ein Firmware-Update programmiert den internen Programmspeicher des Teensy; der Audioimport von der SD-Karte ist ein eigener Vorgang.

<a id="if-the-upload-does-not-start"></a>

### Wenn die Übertragung nicht startet

| Symptom | Was zu überprüfen ist |
| --- | --- |
| Der Loader erkennt LILLA nicht | Drücken Sie kurz die Taste Firmware_upload mode, nachdem Sie USB angeschlossen haben. Versuchen Sie es mit einem bekannten Datenkabel und einem anderen Computer-USB-Anschluss. |
| Die HEX-Datei kann nicht geöffnet werden | Laden Sie die Rohdatei `.hex` erneut herunter. Stellen Sie sicher, dass Sie die GitHub-Webseite oder ein Quellarchiv nicht gespeichert haben. |
| Die Programmierung ist abgeschlossen, aber LILLA startet nicht | Wählen Sie „Vorgänge“ > „Neustart nach Abschluss des Downloads“. Stellen Sie ggf. die Verbindung wieder her und wiederholen Sie den Vorgang mit der richtigen LILLA-Firmware. |
| Der Begrüßungsbildschirm zeigt die alte Version | Überprüfen Sie, welche HEX-Datei im Loader geöffnet ist, und wiederholen Sie das Programm, gefolgt von einem Neustart. |

<a id="updating-the-firmware-on-mac"></a>

## Die Firmware auf dem Mac aktualisieren

Verwenden Sie unter macOS die eigenständige Anwendung **Teensy Loader** und die veröffentlichte Datei LILLA `.hex`. Arduino IDE, PlatformIO und das Entwicklungs-Add-on Teensyduino sind zum Hochladen einer kompilierten Firmware nicht erforderlich. PJRC listet den Standalone-Loader auf seiner [Downloadseite](https://www.pjrc.com/teensy/td_download.html).

<a id="what-you-need-on-mac"></a>

### Voraussetzungen auf dem Mac

- Ihr LILLA-Instrument, das einen Teensy 4.1 verwendet.
- Ein Mac, der mit dem aktuellen Teensy Loader-Download kompatibel ist.
- Ein USB **Datenkabel** passend zum Mac und zum USB-C-Anschluss von LILLA. Wenn ein Adapter erforderlich ist, muss dieser USB-Daten unterstützen.
- Die veröffentlichte LILLA HEX-Datei und die macOS Teensy Loader-Anwendung.

<a id="download-the-firmware-on-mac"></a>

### Die Firmware auf dem Mac herunterladen

1. Öffnen Sie in Ihrem Browser den [Hauptzweig des LILLA GitHub-Repositorys](https://github.com/SandroGrassia/Lilla_Audio_Sampler/tree/main). Vergewissern Sie sich, dass in der Zweigauswahl **main** angezeigt wird.
2. Öffnen Sie die veröffentlichte `.hex`-Datei in der Dateiliste der obersten Ebene des Projekts, zum Beispiel `Lilla_v7_0_2.hex`.
3. Klicken Sie auf **Rohdatei herunterladen** und speichern Sie sie in einem geeigneten Ordner, z. B. `Downloads/LILLA`.
4. Vergewissern Sie sich im Finder, dass die heruntergeladene Datei auf `.hex` endet. Eine GitHub-Webseite oder das Quellarchiv **Code > ZIP herunterladen** kann nicht als Firmware geladen werden.

Verwenden Sie nur Firmware von **main**. Dateien auf **Entwicklung** sind in Arbeit und möglicherweise fehlerhaft oder ungetestet. Wenn main keine HEX-Datei hat, warten Sie, bis der Betreuer sie veröffentlicht. Behalten Sie die heruntergeladene Kopie, um genau diesen Build beizubehalten.

<a id="download-and-open-teensy-loader-on-mac"></a>

### Teensy Loader auf dem Mac herunterladen und öffnen

1. Öffnen Sie die [offizielle PJRC Mac-Loader-Seite](https://www.pjrc.com/teensy/loader_mac.html) und laden Sie **Teensy Loader Disk Image** herunter.
2. Öffnen Sie im Finder das heruntergeladene `.dmg`. Es enthält die Anwendung Teensy Loader.
3. Kopieren Sie die Anwendung zur bequemen Wiederverwendung in **Anwendungen** und öffnen Sie sie dann. Bestätigen Sie **Öffnen**, wenn macOS nach der heruntergeladenen Anwendung fragt.

Wenn macOS eine nicht verifizierte Anwendung blockiert, prüfen Sie zunächst, ob sie aus dem offiziellen PJRC-Download stammt. Versuchen Sie, sie zu öffnen, und verwenden Sie danach, falls verfügbar, **Apple-Menü > Systemeinstellungen > Datenschutz & Sicherheit > Dennoch öffnen**. Bestätigen Sie mit **Öffnen**. Folgen Sie den [Apple-Anweisungen zum Öffnen heruntergeladener Apps](https://support.apple.com/en-us/102445); deaktivieren Sie die macOS-Sicherheit nicht global. Meldet der Loader ein nicht unterstütztes System, beziehen Sie eine kompatible Version von PJRC.

<a id="prepare-lilla-on-mac"></a>

### LILLA am Mac vorbereiten

Speichern Sie Ihre Patch- und MIDI-Loops, beenden Sie ausstehende Aufzeichnungs- oder Exportvorgänge und erstellen Sie ein Backup mit [Sicherung und Wiederherstellung](#backup-and-restore). Lesen Sie alle Kompatibilitäts- oder Migrationsanweisungen, die der Firmware beiliegen.

Senken Sie den Hörpegel. Verbinden Sie den USB-C-Anschluss des LILLA mit dem Datenkabel mit dem Mac und halten Sie die Stromversorgung und den USB während der gesamten Programmierung verbunden. Erlauben Sie die Verbindung des USB-Zubehörs, wenn Ihr Mac um Erlaubnis bittet.

<a id="upload-and-restart-on-mac"></a>

### Am Mac übertragen und neu starten

1. Lassen Sie den **Automatikmodus** des Teensy Loader ausgeschaltet.
2. Wählen Sie **Datei > HEX-Datei öffnen** und wählen Sie die heruntergeladene LILLA HEX-Datei aus.
3. Drücken Sie kurz die **Firmware_upload mode**-Taste von LILLA und nicht die On/off-Taste. Der Lader sollte den Teensy erkennen.
4. Wählen Sie **Operationen > Programm** und warten Sie, bis **Download abgeschlossen** ist.
5. Wählen Sie **Operations > Reboot**, um LILLA neu zu starten.
6. Überprüfen Sie die Version auf dem Begrüßungsbildschirm und testen Sie einen bekannten Patch bei niedriger Hörlautstärke. Für diese Version sollte auf dem Begrüßungsbildschirm Version 7.0.2 angezeigt werden.

Diese Steuerelemente werden in den [PJRC Mac-Loader-Anweisungen](https://www.pjrc.com/teensy/loader_mac.html) beschrieben.

<a id="if-the-mac-cannot-upload"></a>

### Wenn die Übertragung am Mac nicht funktioniert

| Symptom | Was zu überprüfen ist |
| --- | --- |
| Teensy Loader lässt sich nicht öffnen | Überprüfen Sie die Downloadquelle, die macOS-Berechtigungsaufforderung und die Systemanforderungen des Loaders. |
| LILLA wird nicht erkannt | Schließen Sie USB erneut an, lassen Sie das Zubehör zu, wenn Sie dazu aufgefordert werden, und drücken Sie kurz Firmware_upload mode. Probieren Sie ein anderes Datenkabel, einen anderen Anschluss oder einen anderen Adapter aus. |
| Der HEX kann nicht geöffnet werden | Laden Sie das rohe HEX erneut von der Hauptdatei herunter und überprüfen Sie seine Erweiterung im Finder. |
| Die Programmierung ist abgeschlossen, aber LILLA startet nicht neu | Wählen Sie „Vorgänge“ > „Neustart nach Abschluss des Downloads“. |

<a id="troubleshooting"></a>

## Fehlerbehebung

<a id="diagnose-silence-in-a-useful-order"></a>

### Bei fehlendem Ton systematisch prüfen

1. **MIDI:** Zeigt Tools > Test eingehende Notizen an?
2. **Mapping:** Ist diesem Kanal und Notenbereich ein aktiver Sound zugewiesen?
3. **Quelle:** Ist die erwartete Audioquelle vorhanden und ist die Region gültig?
4. **Hüllkurve und Pegel:** Sind Gain und Sustain ausreichend und ist der Attack ungewöhnlich lang?
5. **Routing:** Ist die Stummschaltung der Quelle aufgehoben und sie an den von Ihnen verwendeten Ausgang weitergeleitet?
6. **Ausgabe:** Sind Patch-Lautstärke, externer Mixer-Pegel und die physische Verbindung korrekt?

Ändern Sie jeweils eine Sache und wiederholen Sie den Test mit derselben Notiz. Dies erleichtert die Identifizierung der tatsächlichen Ursache.

| Problem | Was zu überprüfen ist |
| --- | --- |
| Kein Ton vom Controller | MIDI-Verbindung, Instrumenten-MIDI-Kanal, Tastaturbereich, Quellenverfügbarkeit, Verstärkung, Patch-Lautstärke und Mixer-Ausgangsrouting. |
| Ein Slot wird nicht zur Bearbeitung geöffnet | Der Steckplatz ist möglicherweise unbenutzt. Bearbeiten Sie einen aktiven Slot oder klonen Sie ein vorhandenes Instrument in ein freies. |
| Das Sample klingt zu hoch oder zu tief | Grundton, Tonhöhe, Controller-Pitch-Bend und Setup-Tastenschritt. |
| Audioclips oder Verzerrungen | Reduzieren Sie die Verstärkung des Aufnahmeeingangs oder der Wiedergabe. Überprüfen Sie die Mixer-Pegel, die Auflösung und das Downsampling. |
| Klickt auf eine Schleifengrenze | Verfeinern Sie From/To und passen Sie Noclick an, sofern verfügbar. |
| Es werden weniger Noten gespielt als erwartet | LILLA hat bis zu 16 Stimmen; Ebenen verbrauchen mehrere Stimmen und eine lautere Wiedergabe kann die Verfügbarkeit verringern. Überprüfen Sie die Priorität und Anordnungsdichte. |
| Der SD-Import kann keine Dateien finden | Überprüfen Sie die Karte und den genauen Ordner `/LILLA_AUDIO` im Stammverzeichnis. |
| Der Import lehnt einen WAV oder AIFF ab | Verwenden Sie unkomprimiertes 16-Bit PCM bei 44,1 kHz mit einem oder zwei Kanälen. |
| Importieren Sie Berichte mit doppelten Dateien | Geben Sie jeder Quelle einen eindeutigen Basisnamen, auch für Dateien in unterschiedlichen Formaten. |
| Eine importiertes Sample endet vorzeitig | Überprüfen Sie das Importlimit von ca. 35,7 Sekunden. |
| Die RAW-Konvertierung schlägt fehl | Überprüfen Sie den freien RAW-Dateispeicher und die verfügbaren Dateinamen. |
| WAV-Export schlägt fehl | Überprüfen Sie, ob die SD-Karte vorhanden und beschreibbar ist und über freien Speicherplatz verfügt. |
| Live-Audio verschwindet | Der Live-Puffer ist temporär. Übernehmen Sie die gewünschte Schleife in einen Patch und speichern Sie ihn vor dem Ausschalten. |
| Live Sampler fordert dazu auf, Performance zu öffnen und zu speichern | Der vorherige normale Patch enthält nicht gespeicherte Änderungen. Kehren Sie zu Performance zurück, speichern Sie es und versuchen Sie dann erneut, es zu erfassen. |
| Die Live-Aufnahme wird abgelehnt | Stoppen Sie die Aufnahme, wählen Sie den Loop-Modus, kürzen Sie den Bereich bei Bedarf und speichern Sie vorhandene Patch-Änderungen. |
| Spuren verschwinden nach der Neuaufnahme Track 1 | Track 1 erstellt eine neue Schleife und löscht die anderen Spuren. |
| Die Wiederherstellung kann das Backup nicht finden | Platzieren Sie `LILLA_CONFIG.fram` und die zugehörigen Aufnahmedateien direkt in `/LILLABACKUP`. |
| Der wiederhergestellte Patch kann seine Quelle nicht abspielen | Stellen Sie die erforderliche Quellbibliothek wieder her oder importieren Sie sie erneut. Die Konfigurationssicherung umfasst nicht alle importierten Audiodaten. |

<a id="practical-projects"></a>

## Praktische Projekte

<a id="make-a-playable-instrument-from-a-recorded-note"></a>

### Aus einer aufgenommenen Note ein spielbares Instrument erstellen

**Sie benötigen:** eine an den Line-Eingang angeschlossene Quelle und einen MIDI-Controller.

1. Verwenden Sie in Sampler `PAUSE+REC`, um den Eingangspegel festzulegen.
2. Nehmen Sie eine saubere, anhaltende Note in Mono auf, einschließlich ihres Einschwingens und Abklingens.
3. Stoppen Sie, drücken Sie S1, um über die Tastatur vorzuhören, trimmen Sie A/B und wählen Sie `RETURN` aus. Erstellen Sie jetzt ein Backup, wenn Sie die vollständige Originalaufnahme behalten möchten.
4. Verwenden Sie `MAKE_RAW`, um eine abspielbare Quelle aus der ausgewählten Region zu erstellen; Bei erfolgreicher Konvertierung wird die Aufnahme entfernt.
5. Klonen Sie in Performance einen Patch, den Sie als Ausgangspunkt verwenden können.
6. Öffnen Sie einen aktiven Sound und wählen Sie die neue Quelle.
7. Schneiden Sie unerwünschte Stille ab, wählen Sie einen Wiedergabemodus und passen Sie die Hüllkurve an.
8. Stellen Sie den Grundton so ein, dass er mit der aufgenommenen Note übereinstimmt, und legen Sie dann den Tastaturbereich fest.
9. Spielen Sie oberhalb und unterhalb der Grundtaste, um das Ergebnis zu prüfen.
10. Speichern Sie den Patch. Bewahren Sie die vor der Konvertierung erstellte Sicherung auf, wenn Sie die Originalaufnahme benötigen. Ein konvertierter Take ist für den Sampler WAV-Export nicht mehr verfügbar.

**Versuchen Sie es als Nächstes:** Klonen Sie den Sound in einen anderen Slot, wählen Sie einen anderen Bereich derselben Quelle aus und weisen Sie den beiden Slots separate Tastaturzonen zu.

<a id="turn-live-audio-into-a-small-playable-kit"></a>

### Aus Live-Audio ein kleines spielbares Kit erstellen

**Sie benötigen:** mehrere kurze Ereignisse im Live Sampler-Puffer und einen zuvor gespeicherten Performance-Patch.

1. Zeichnen Sie die Quelle in Live Sampler auf und stoppen Sie dann.
2. Wählen Sie den Schleifenmodus und finden Sie das erste nützliche Ereignis mit From, To und Window.
3. Drücken Sie S1 und spielen Sie die gewünschte Auslösetaste, wenn Sie dazu aufgefordert werden.
4. Verschieben Sie die Region auf ein zweites Ereignis.
5. Drücken Sie einen anderen freien Steckplatz und wählen Sie eine andere Auslösetaste.
6. Wiederholen Sie diesen Vorgang für die übrigen Ereignisse und lassen Sie Slot-Paare für Stereoaufnahmen zu.
7. Wechseln Sie zu Performance und überprüfen Sie die Trigger-Tasten-Zuweisungen.
8. Öffnen Sie jeden Sound, um seinen endgültigen One-Shot- oder Loop-Modus und seine Hüllkurve auszuwählen.
9. Speichern Sie den Patch und warten Sie, bis sein Audio in Flash geschrieben wird.

**Ergebnis:** ein normaler Patch, dessen Slots verschiedene erfasste Regionen enthalten. Der ursprüngliche Live-Puffer bleibt ein temporärer Arbeitsbereich.

<a id="build-a-layered-texture-and-animate-it"></a>

### Eine geschichtete Klangtextur erstellen und modulieren

1. Beginnen Sie mit einem gespeicherten Patch und klonen Sie einen Sound in einen freien Slot.
2. Geben Sie beiden Slots den gleichen MIDI-Kanal und überlappende Tastenbereiche.
3. Verwenden Sie unterschiedliche Trimmbereiche oder Tonhöhenanpassungen für die beiden Ebenen.
4. Verringern Sie die einzelnen Gain-Werte, bevor Sie die Kombination anhören.
5. Öffnen Sie VCF einer Ebene und fügen Sie eine langsame Modulation mit mäßiger Tiefe hinzu.
6. Leiten Sie eine oder beide Ebenen an Delay und fügen Sie eine kleine Menge Feedback hinzu.
7. Spielen Sie gehaltene Noten und hören Sie sich die Ausklingtöne an.
8. Speichern Sie den Patch, wenn die Balance funktioniert.

**Versuchen Sie es als nächstes:** Nehmen Sie eine kurze Phrase auf MIDI Loop Track 1 auf, fügen Sie einen Kontrastteil auf Track 2 hinzu und experimentieren Sie mit einem kleinen Timing Shift auf der zweiten Spur.

<a id="quick-reference"></a>

## Kurzreferenz

| Aufgabe | Ausgangspunkt |
| --- | --- |
| Spielen Sie einen vorhandenen Patch ab | Performance > Patchfeld > Value. |
| Bearbeiten Sie einen Ton | Performance > S1-S8 für einen aktiven Steckplatz. |
| Fügen Sie ein weiteres Instrument mit einem vorhandenen Sound hinzu | Sound Edit > CLONE, und bearbeiten Sie dann den neuen Steckplatz. |
| Behalten Sie die Tonänderungen nach dem Ausschalten bei | Zurück zu Performance > SAVE. |
| Computer-Audio importieren | SD `/LILLA_AUDIO` > Tools > Setup > importieren. |
| Line-Eingabe aufzeichnen | Sampler > PAUSE+REC > MONO_REC oder STEREO_REC > STOP. |
| Bearbeiten Sie eine Aufnahme, während Sie sie über die Tastatur abspielen | Sampler > abgeschlossene Aufnahme > S1 (Mono/links) oder S2 (Stereo rechts) > Sound Edit > RETURN. |
| Zeichnen Sie weiterhin Trimmpunkte für die Wiedergabe und den Export auf | Passen Sie A/B in Sampler Sound Edit an; automatisch gespeichert, mit verknüpften Stereokanälen. |
| Verwandeln Sie eine Aufnahme in eine Quelle | Sampler > MAKE_RAW. |
| Exportieren Sie eine Aufnahme | Sampler > EXPORT_WAV_TO_SD > SD `/LILLAWAV_EXPORT`. |
| Nehmen Sie vorübergehend Live-Audio auf | Live Sampler > CAPTURE > STOP. |
| Behalten Sie eine Live-Schleife bei | Live-Aufnahme stoppen > Loop-Modus > S1-S8 > Grundton festlegen > Patch speichern. |
| Zeichnen Sie eine MIDI-Schleife auf | MIDI Loop > Rec 1 > abspielen > Rec 1 erneut. |
| Konfiguration und Aufzeichnungen sichern | Tools > Setup > neues nummeriertes Backup. |
| Stellen Sie ein nummeriertes Backup wieder her | Kopieren Sie den Inhalt nach SD `/LILLABACKUP` > Tools > Setup > Wiederherstellen. |

<a id="glossary"></a>

## Glossar

| Begriff | In diesem Ratgeber |
| --- | --- |
| ADSR | Attack, Decay, Sustain und Release: die Phasen, die den Pegel eines Klangs im Laufe der Zeit formen. |
| Basisname | Ein Dateiname ohne Erweiterung, z. B. `BassDry` in `BassDry.wav`. |
| Erfassen | Nehmen Sie im Menü Live Sampler in den Live-Puffer auf. Kopieren Sie mit S1-S8 eine ausgewählte Region in einen Patch-Sound. |
| Rundpuffer | Aufnahmespeicher, der älteres Material umschließt und überschreibt. |
| Blitz | Permanenter Audiospeicher im LILLA. |
| FRAM | Permanenter Speicher für Konfiguration und Patch-/Sound-Metadaten. |
| LFO | Niederfrequenzoszillator, der zur zeitlichen Variation eines Parameters verwendet wird. |
| LPF | Tiefpassfilter, der höhere Frequenzen reduziert. |
| MIDI CC | Eine MIDI Control Change-Nachricht, die zur Steuerung eines zugewiesenen Parameters verwendet wird. |
| Mono | Ein Audiokanal. |
| Noclick | Grenzglättung wird verwendet, um Diskontinuitäten in geeigneten Schleifenbereichen zu reduzieren. |
| PCM | Unkomprimierte digitale Beispieldaten. |
| Polyfonie | Die Anzahl der gleichzeitig wiedergegebenen Stimmen; Layer und Release Tails nutzen ebenfalls Stimmen. |
| PSRAM | Funktionierender Audiospeicher; Der Inhalt geht beim Ausschalten verloren. |
| RAW | Audiobeispieldaten ohne WAV- oder AIFF-Containerheader. |
| Root-Schlüssel | Die Referenztaste MIDI für die Tonhöhenzuordnung eines Sounds. |
| Stereo | Zwei Audiokanäle, links und rechts. |
| VCF | Der Instrumentenfilter mit Cutoff-, Resonanz- und Modulationsreglern. |

---

*Lilla Benutzerhandbuch - Deutsche Ausgabe - 8. Oktober 2026*

*Anleitungsbilder: [doc/assets/images](doc/assets/images/). Behalten Sie diesen relativen Ordnerpfad bei, wenn Sie die illustrierte Anleitung freigeben.*
