# ArdQua – Automatisches Bewässerungssystem

ArdQua ist ein kompaktes, batteriebetriebenes Bewässerungssystem für Zimmerpflanzen. Das Gerät misst die Feuchtigkeit im Pflanzensubstrat und aktiviert bei Trockenheit automatisch eine Pumpe, welche eine definierte Menge Wasser über einen Schlauch direkt ins Substrat fördert. Das gesamte System ist als eigenständiges Modul konzipiert: neben die Pflanze stellen, Batterie einsetzen, Wasser auffüllen.

---

## Funktionen
- Kontinuierliche Messung der Bodenfeuchtigkeit  
- Automatische Bewässerung bei Unterschreiten eines Schwellwerts  
- Definierte Wassermenge pro Pumpen-Zyklus  
- Geschlossenes Gehäuse mit integriertem Tank  
- Batteriebetrieb  
- Geringer Pflegeaufwand für Benutzerinnen und Benutzer

---

## Projektziel
Ein lauffähiges MVP (Minimum Viable Product), das:
- zuverlässige Feuchtigkeitsmessungen ermöglicht,
- die Pumpe präzise steuert,
- Energieeffizienz berücksichtigt,
- ein funktionales Gesamtsystem demonstriert.

---

## Hardware
- Mikrocontroller (Arduino-basiert)
- Bodenfeuchtigkeitssensor
- Mini-Wasserpumpe
- MOSFET/Transistor zur Pumpensteuerung
- Schläuche
- Batterieeinheit
- Gehäuse mit Wasserbehälter

---

## Software / Logik
- Sensorabfrage in Intervallen  
- Glättung der Messwerte  
- Schwellwert-Logik mit Zeitfenster  
- Aktivierung der Pumpe über MOSFET  
- Sicherheitsmechanismen:
  - maximale Pumpdauer  
  - Mindestabstand zwischen Bewässerungen  

---

## MVP-Fokus
- Stabile Sensorwerte  
- Wiederholbare Bewässerung  
- Robuste Elektronik  
- Energieverbrauch optimieren  
- Einfache Nutzung ohne App oder Netzwerk

---

## Gehäuse
- Kompaktes Modul, das neben der Pflanze steht  
- Integrierter Tank (z. B. PET- oder Kunststoffbehälter)  
- Fach für Batterie  
- Interne Kabelführung und Schlauchdurchführung  

---

## Zukunftsideen
- Zigbee-Integration für Home Assistant  
- App-Anbindung  
- Füllstandserkennung  
- Mehrere Pflanzen anschliessbar  
- 3D-gedrucktes Snap-Fit-Gehäuse  


---

## Lizenz
Projektarbeit im Modul *Hardwarenahe Programmierung* (ZHAW).  
Nutzung nach Absprache.

