# ESP32 Pro Controller

Manette Nintendo Switch personnalisée basée sur un ESP32 WROOM. Le firmware est écrit en C pour **ESP-IDF 4.4.x** et émule une Pro Controller via Bluetooth Classic HID.

## Fonctions disponibles

- 16 boutons sur MCP23017 par I²C et deux clics de joystick sur GPIO.
- Deux sticks analogiques sur ADC1, avec mesure automatique de leur position centrale au démarrage.
- Jusqu'à quatre profils de touches, permutation/inversion des axes et turbo, configurables depuis une interface Wi-Fi locale.
- Rapports HID Switch, LEDs joueur, LED HOME et moteur vibrant.
- Niveau batterie LiPo mesuré sur GPIO36 avec un pont diviseur 10 kΩ / 10 kΩ.
- Couleurs de la manette fournies à la Switch via l'émulation SPI.

Le gyroscope et le NFC/IR ne sont pas implémentés ; aucun échantillon IMU n’est envoyé.

## Prérequis et compilation

Le projet ESP-IDF se trouve dans [`Esp32-pro/`](/G:/code/esp32-switch-pro/Esp32-pro). Suivre la procédure détaillée : [`PROCEDURE.md`](/G:/code/esp32-switch-pro/Esp32-pro/PROCEDURE.md).

```bash
cd Esp32-pro
idf.py set-target esp32
idf.py build
idf.py -p COM3 flash monitor
```

Les paramètres matériels sont centralisés dans [`config.h`](/G:/code/esp32-switch-pro/Esp32-pro/main/config.h).

## Câblage principal

| Fonction | Broche ESP32 |
| --- | --- |
| MCP23017 SDA / SCL | GPIO21 / GPIO22 |
| Stick gauche X / Y | GPIO34 / GPIO35 |
| Stick droit X / Y | GPIO32 / GPIO33 |
| Batterie | GPIO36 |
| Clics L3 / R3 | GPIO25 / GPIO26 |
| Mode configuration, actif bas au démarrage | GPIO16 |
| LEDs joueur | GPIO27 / GPIO14 / GPIO12 / GPIO13 |
| LED HOME | GPIO15 |
| Transistor du moteur | GPIO4 |

Le MCP23017 utilise l'adresse `0x20` lorsque A0, A1 et A2 sont à la masse. Tous ses boutons sont actifs bas avec les pull-ups internes activés.

GPIO12 et GPIO15 sont des broches de strapping de l’ESP32. Le câblage des LED sur ces broches doit rester passif au reset : une LED ou son transistor ne doit pas imposer un niveau logique au démarrage. Le schéma KiCad et le diagramme HTML sont dans [`schematic/`](/G:/code/esp32-switch-pro/schematic).

## Batterie

Le diviseur est : `VBAT → 10 kΩ → GPIO36 → 10 kΩ → GND`. Le firmware calibre la conversion ADC puis multiplie la tension mesurée par deux. Les seuils correspondent à la tension réelle de la LiPo et sont définis dans `config.h` : 4,10 V, 3,85 V, 3,65 V et 3,45 V.

L’ADC de l’ESP32 varie d’une carte à l’autre. Vérifier la valeur au multimètre sur un prototype et ajuster ces seuils si nécessaire.

## Configuration des touches

Maintenir GPIO16 à GND pendant l’allumage. La manette crée alors le point d’accès `ProCon-Setup`, puis l’interface est accessible sur `http://192.168.4.1`.

Le mot de passe Wi-Fi est défini dans `config.h` et doit être modifié avant une utilisation hors d’un environnement maîtrisé. Le mode configuration n’exécute pas le Bluetooth.

## Appairage Switch

1. Démarrer normalement, sans maintenir GPIO16 à la masse.
2. Dans la Switch, ouvrir **Manettes → Changer le style/l’ordre**.
3. Choisir la Pro Controller détectée.

L’adresse de la Switch et l’état des profils sont enregistrés dans NVS.
