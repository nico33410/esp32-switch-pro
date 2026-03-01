# ESP32 Pro Controller

Manette Nintendo Switch custom basée sur un ESP32, communiquant en BLE et se faisant passer pour une Pro Controller officielle.

> Développé avec l'assistance de Claude (Anthropic)

---

## Fonctionnalités

- 16 boutons via MCP23017 (D-Pad complet, ABXY, L/R/ZL/ZR, −/+, Home, Capture)
- 2 joysticks analogiques 10kΩ avec calibration automatique au démarrage
- Module joystick universel — orientable dans 4 positions grâce aux boutons de configuration
- Clics joystick gauche/droit sur GPIO directs
- Gyroscope simulé (immobile)
- Mesure batterie LiPo
- Vibration motorisée
- LEDs joueur (x4) + LED HOME avec contrôle PWM
- Couleurs de manette personnalisables (asymétriques gauche/droite)
- Connexion BLE — appairage natif Switch sans driver

---

## Composants

| Composant | Description |
|-----------|-------------|
| **ESP32 WROOM Dev Kit** | Microcontrôleur principal, BLE, ADC |
| **Module MCP23017** | Expandeur I/O 16 bits I²C — résistances pull-up 4.7kΩ intégrées |
| **LiPo Rider Plus** | Gestion batterie LiPo — charge USB + boost 5V |
| **Boutons arcade** | Boutons principaux (ABXY, L/R/ZL/ZR, −/+, Home, Capture, D-Pad) |
| **Joysticks analogiques 10kΩ** | 2× module joystick avec axe X/Y potentiomètre 10kΩ |
| **Boutons à bascule** | 4× pour configuration orientation joysticks (swap X/Y, inversion sens) |
| **Gros bouton power** | Bouton de mise sous tension |
| **Interrupteur à bascule 2 voies** | Interrupteur principal alimentation |
| **LEDs 3-6V** | LEDs avec résistance intégrée — 4× joueur + 1× HOME |
| **Transistor NPN (2N2222)** | Pilotage moteur vibrant |
| **Diode 1N4007** | Protection flyback moteur vibrant |
| **Résistances 10kΩ × 2** | Diviseur de tension mesure batterie |

> 📷 *Photos des composants à ajouter*

---

## Schéma de câblage

### MCP23017 — 16 boutons via I²C

| Pin MCP | Define | Bouton |
|---------|--------|--------|
| GPA0 | `MCP_BTN_A` | A |
| GPA1 | `MCP_BTN_B` | B |
| GPA2 | `MCP_BTN_X` | X |
| GPA3 | `MCP_BTN_Y` | Y |
| GPA4 | `MCP_BTN_L` | L |
| GPA5 | `MCP_BTN_R` | R |
| GPA6 | `MCP_BTN_ZL` | ZL |
| GPA7 | `MCP_BTN_ZR` | ZR |
| GPB0 | `MCP_BTN_MINUS` | − |
| GPB1 | `MCP_BTN_PLUS` | + |
| GPB2 | `MCP_DPAD_LEFT` | ◀ D-Pad Left |
| GPB3 | `MCP_DPAD_RIGHT` | ▶ D-Pad Right |
| GPB4 | `MCP_BTN_HOME` | 🏠 Home |
| GPB5 | `MCP_BTN_CAPTURE` | ⬤ Capture |
| GPB6 | `MCP_DPAD_UP` | ▲ D-Pad Up |
| GPB7 | `MCP_DPAD_DOWN` | ▼ D-Pad Down |

Alimentation MCP23017 :
- VDD (pin 9) → 3.3V
- VSS (pin 10) → GND
- RESET (pin 18) → 3.3V
- A0/A1/A2 (pin 15-16-17) → GND → adresse `0x20`

### GPIO ESP32

| GPIO | Define | Fonction | Type |
|------|--------|----------|------|
| GPIO 21 | `I2C_SDA` | I²C SDA → MCP23017 pin 13 | I2C |
| GPIO 22 | `I2C_SCL` | I²C SCL → MCP23017 pin 12 | I2C |
| GPIO 25 | `LSTICK_BTN_PIN` | Clic joystick gauche | INPUT_PULLUP |
| GPIO 26 | `RSTICK_BTN_PIN` | Clic joystick droit | INPUT_PULLUP |
| GPIO 16 | `BTN_LEFT_SWAP_PIN` | Config swap X/Y gauche | INPUT_PULLUP |
| GPIO 17 | `BTN_LEFT_INVERT_PIN` | Config invert gauche | INPUT_PULLUP |
| GPIO 18 | `BTN_RIGHT_SWAP_PIN` | Config swap X/Y droit | INPUT_PULLUP |
| GPIO 19 | `BTN_RIGHT_INVERT_PIN` | Config invert droit | INPUT_PULLUP |
| GPIO 34 | `STICK_LX_PIN` | Joystick gauche X | ADC1_CH6 |
| GPIO 35 | `STICK_LY_PIN` | Joystick gauche Y | ADC1_CH7 |
| GPIO 32 | `STICK_RX_PIN` | Joystick droit X | ADC1_CH4 |
| GPIO 33 | `STICK_RY_PIN` | Joystick droit Y | ADC1_CH5 |
| GPIO 27 | `LED_1` | LED joueur 1 | OUTPUT |
| GPIO 14 | `LED_2` | LED joueur 2 | OUTPUT |
| GPIO 12 | `LED_3` | LED joueur 3 | OUTPUT |
| GPIO 13 | `LED_4` | LED joueur 4 | OUTPUT |
| GPIO 15 | `LED_HOME_PIN` | LED HOME | PWM |
| GPIO 4 | `VIBRATION_PIN` | Moteur vibrant | PWM |
| GPIO 36 | `BATTERY_PIN` | Mesure batterie | ADC1_CH0 |

> ⚠️ GPIO 34/35/36 : input only, pas de pull-up interne  
> ⚠️ GPIO 25/26 : utilisés en digital uniquement — ADC2 incompatible avec BLE

### Diviseur de tension batterie

```
VBAT (+4.2V)
    │
   R1 10kΩ
    │
    ├──── GPIO 36
    │
   R2 10kΩ
    │
   GND
```

### Moteur vibrant

```
GPIO 4 ── R 1kΩ ── Base NPN (2N2222)
                   Collecteur ── Moteur ── VCC
                   Émetteur ── GND
                   Diode 1N4007 en parallèle sur le moteur
```

---

## Module joystick universel

Chaque joystick est monté dans un boîtier orientable dans 4 positions. Les boutons à bascule permettent de corriger l'orientation sans modifier le code :

| Invert | Swap | Orientation |
|--------|------|-------------|
| ❌ | ❌ | 0° — normal |
| ✅ | ❌ | 180° — retourné |
| ❌ | ✅ | 90° |
| ✅ | ✅ | 270° |

---

## Configuration

Toute la configuration matérielle est centralisée dans `config.h` :

- Mapping des pins GPIO et MCP23017
- Couleurs de la manette (RGB, asymétriques gauche/droite)
- Deadzone et filtre passe-bas des joysticks
- Intervalle de vérification batterie
- Activation/désactivation des logs debug

### Couleurs

Plusieurs presets sont disponibles en commentaire dans `config.h` :

- Pro Controller gris (défaut Nintendo)
- Splatoon 2 (vert néon / rose)
- Joy-Con néon (bleu / rouge)
- Pokémon Scarlet/Violet
- Zelda BOTW
- Animal Crossing
- Mario / Luigi
- Custom

---

## Structure du code

```
├── Esp32-pro.ino         # Fichier principal, setup() et loop()
├── config.h              # Configuration matérielle et couleurs
├── controller_state.h/cpp # Structure d'état global de la manette
├── buttons.h/cpp         # Lecture boutons (MCP23017 + GPIO)
├── sticks.h/cpp          # Joysticks analogiques + calibration + config
├── battery.h/cpp         # Mesure tension LiPo
├── leds.h/cpp            # Gestion LEDs joueur + HOME
├── vibration.h/cpp       # Moteur vibrant
├── imu.h/cpp             # Gyroscope/accéléromètre (simulé)
├── ble_hid.h/cpp         # Stack BLE + HID
└── protocol.h/cpp        # Protocole Nintendo Switch
```

---

## Dépendances Arduino

- `Adafruit MCP23X17` — gestion MCP23017
- `ESP32 BLE Arduino` — stack BLE HID
- `Preferences` — stockage appairage en flash

---

## Appairage Switch

1. Allumer la manette
2. Sur la Switch : **Manettes → Changer style/ordre des manettes**
3. La manette apparaît comme **Pro Controller**
4. L'appairage est mémorisé en flash — reconnexion automatique au démarrage

---

## Notes

- Fréquence d'envoi des rapports : ~125 Hz (conforme specs Nintendo)
- Le gyroscope est simulé (manette immobile). Pour un vrai gyroscope, remplacer `imu.cpp` par une lecture MPU6050
- ADC2 (GPIO 25/26) inutilisable en analogique quand le BLE est actif — tous les axes joystick sont sur ADC1
