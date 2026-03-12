# Procédure — ESP32 Pro Controller avec VS Code + ESP-IDF

## Prérequis

- **VS Code** installé
- **Extension ESP-IDF** installée (identifiant : `espressif.esp-idf-vscode-extension`)
- **ESP-IDF v5.1 ou supérieur** (ce code cible l'API ADC oneshot, disponible dès v5.0)
- **Driver USB** de votre chip UART (CP2102, CH340 ou FTDI selon votre board)

---

## 1. Installer l'extension ESP-IDF dans VS Code

1. Ouvrir VS Code → `Ctrl+Shift+X`
2. Chercher **ESP-IDF** → installer l'extension Espressif
3. Ouvrir la palette de commandes : `Ctrl+Shift+P`
4. Lancer : **ESP-IDF: Configure ESP-IDF Extension**
5. Choisir **Express** → sélectionner la version **v5.1** (ou plus récente) → laisser le chemin par défaut → cliquer **Install**
6. Attendre la fin de l'installation (5-15 min selon la connexion)

---

## 2. Ouvrir le projet

1. Dans VS Code : **Fichier → Ouvrir le dossier**
2. Sélectionner le dossier `procon/` (celui qui contient `CMakeLists.txt` et `sdkconfig`)

---

## 3. Configurer la cible

Dans la palette de commandes (`Ctrl+Shift+P`) :

```
ESP-IDF: Set Espressif Device Target
```

→ Sélectionner **esp32**

---

## 4. Vérifier le sdkconfig

Dans la palette de commandes :

```
ESP-IDF: SDK Configuration Editor (Menuconfig)
```

Vérifier que ces options sont activées :

- **Component config → Bluetooth → Bluetooth** ✅ activé
- **Component config → Bluetooth → Bluedroid Enable** ✅ activé
- **Component config → Bluetooth → Classic Bluetooth** ✅ activé
- **Component config → Bluetooth → HID Device** ✅ activé

Si vous devez changer quelque chose, sauvegarder avec `S`.

> **Alternative rapide** : supprimer le fichier `sdkconfig` existant et laisser
> ESP-IDF en régénérer un propre après `idf.py set-target esp32`.

---

## 5. Compiler

Dans la palette de commandes :

```
ESP-IDF: Build your Project
```

Ou via le terminal intégré (`Ctrl+\``) :

```bash
idf.py build
```

La compilation dure 1-3 minutes la première fois.  
Si elle réussit, vous verrez : `Project build complete.`

---

## 6. Flasher l'ESP32

1. Brancher l'ESP32 en USB
2. Dans VS Code, cliquer sur le **port COM** en bas à gauche de la barre de statut
3. Sélectionner le port de votre ESP32 (ex: `COM3` sous Windows, `/dev/ttyUSB0` sous Linux)
4. Dans la palette :

```
ESP-IDF: Flash your Project
```

Ou en terminal :

```bash
idf.py -p COM3 flash        # Windows
idf.py -p /dev/ttyUSB0 flash  # Linux/Mac
```

> Si l'ESP32 ne passe pas en mode flash automatiquement, maintenir le bouton **BOOT**
> pendant que vous lancez la commande, relâcher quand le flash démarre.

---

## 7. Moniteur série (logs)

```
ESP-IDF: Monitor your Device
```

Ou :

```bash
idf.py -p COM3 monitor
```

Vous devriez voir au démarrage :
```
====== ESP32 Pro Controller ======
MCP23017 OK (addr=0x20)
Sticks — LX:2048 LY:2048 RX:2048 RY:2048
Batterie — niveau:6/8
En attente d'appairage — Switch → Manettes → Changer l'ordre
```

Quitter le monitor : `Ctrl+]`

---

## 8. Appairage avec la Nintendo Switch

1. Sur la Switch : **Paramètres système → Manettes et capteurs → Changer l'ordre des manettes**
2. Appuyer sur **L + R simultanément** (ou presser un bouton de l'écran)
3. La Switch détecte `Pro Controller`
4. Les LEDs de joueur s'allument → connexion établie

La prochaine fois, la reconnexion est automatique.

---

## 9. Commande tout-en-un (build + flash + monitor)

```bash
idf.py -p COM3 flash monitor
```

---

## Dépannage courant

| Symptôme | Cause probable | Solution |
|---|---|---|
| `fatal error: esp_adc/adc_oneshot.h: No such file` | IDF < v5.0 | Mettre à jour vers IDF v5.1+ |
| `error: 'esp32' component not found` | Ancien CMakeLists | Déjà corrigé dans ce projet |
| `Failed to connect to ESP32` | Mauvais port ou mode boot | Vérifier port, maintenir BOOT |
| `MCP23017 timeout` | Câblage I2C ou adresse | Vérifier SDA/SCL et A0/A1/A2 → GND |
| BT non détecté | sdkconfig incomplet | Relancer menuconfig, activer Classic BT + HID |
| Sticks dérivés | Deadzone trop faible | Augmenter `STICK_DEADZONE` dans `config.h` |

