
# ARDQUA – Automatisches Bewässerungssystem mit Arduino

ARDQUA ist ein Arduino-basiertes, autonom arbeitendes Bewässerungssystem für Zimmerpflanzen.  
Es misst die Bodenfeuchtigkeit und steuert eine Wasserpumpe automatisch.

Das Projekt wurde im Rahmen des Moduls **Hardwarenahe Programmierung** im **CAS Computer Science 1 (ZHAW)** umgesetzt.

---

## Funktionen

- Kapazitiver Bodenfeuchtigkeitssensor  
- Drei Bewässerungsprofile: **trocken / mittel / feucht** (Auswahl über 3-stufigen Schalter)  
- Automatische Ansteuerung einer Wasserpumpe über ein Relais  
- Cooldown-Zeit zur Vermeidung von Fehlmessungen und Überbewässerung  
- TFT-Display mit Anzeige von:
  - aktuellem Bodenfeuchtigkeitswert  
  - aktivem Bewässerungsprofil  
  - grafischem Verlauf der Bodenfeuchtigkeit  
- Wechsel zwischen Text- und Graph-Anzeige per Tastendruck  

---

## Hardware 

- Arduino Nano  
- Kapazitiver Bodenfeuchtigkeitssensor  
- TFT-Display (SPI)  
- Relais und Wasserpumpe  
- DPDT-Schalter (3-stufig)  
- Druckknopf  
- Externes Wasserreservoir  

Die vollständige Pin-Belegung ist in der Projektdokumentation beschrieben.

---

## Software

- Arduino-Sketch mit eigener Klasse `Ardqua`  
- Verwendete Libraries:
  - `Adafruit_ST7735`
  - `Adafruit_GFX`
  - `ThreadController`
  - `Thread`
  - `SPI`
  

---

## Kalibrierung

Der Bodenfeuchtigkeitssensor wird über zwei Referenzmessungen kalibriert:
- Messung in Luft (trocken)
- Messung in Wasser (nass)

Die Bewässerungsgrenzwerte sowie die Pumpdauer sind statisch definiert und profilabhängig.

---

## Projektstatus

- MVP vollständig umgesetzt  
- Bewässerungsprofile, Display-Modi und Automatisierung integriert  
- Weitere Optimierungen (z. B. Energiesparmodus, Datenspeicherung) möglich

---
## Lizenz
Projektarbeit im Modul *Hardwarenahe Programmierung* (ZHAW).  
Nutzung nach Absprache.
