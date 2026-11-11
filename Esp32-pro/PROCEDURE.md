# Procédure — ESP32 Pro Controller avec VS Code + ESP-IDF

## Prérequis

- **VS Code** installé
- **Extension ESP-IDF** installée (identifiant : `espressif.esp-idf-vscode-extension`)
- **ESP-IDF v4.4.x** (ce code utilise l'API ADC legacy : `adc1_get_raw`, `adc1_config_width`)
- **Driver USB** de votre chip UART (CP2102, CH340 ou FTDI selon votre board)

---

## 1. Installer l'extension ESP-IDF dans VS Code

1. Ouvrir VS Code → `Ctrl+Shift+X`
2. Chercher **ESP-IDF** → installer l'extension Espressif
3. Ouvrir la palette de commandes : `Ctrl+Shift+P`
4. Lancer : **ESP-IDF: Configure ESP-IDF Extension**
5. Choisir **Express** → sélectionner la version **v4.4** → laisser le chemin par défaut → cliquer **Install**
6. Attendre la fin de l'installation (5-15 min selon la connexion)

> **Note** : une migration vers ESP-IDF v5.x nécessiterait de remplacer l'API ADC legacy
> (`driver/adc.h`) par l'API oneshot (`esp_adc/adc_oneshot.h`).

---

## 2. Ouvrir le projet

1. Dans VS Code : **Fichier → Ouvrir le dossier**
2. Sélectionner le dossier `Esp32-pro/` (celui qui contient `CMakeLists.txt` et `sdkconfig`)

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
Sticks — LX:... LY:... RX:... RY:...
Stick centers — L:...,... R:...,...
Batterie — ...mV niveau:6/8
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

## 8b. Mode configuration (GPIO 16)

Pour reprogrammer les touches sans recompiler :

1. **Éteindre** l'ESP32
2. **Maintenir GPIO 16 à GND** (bouton ou cavalier) pendant la mise sous tension
3. Les LEDs clignotent en alternance → mode configuration actif
4. Se connecter au WiFi **`ProCon-Setup`** (mot de passe par défaut : **`ChangeThis-42`**)
5. Ouvrir **`http://192.168.4.1`** dans un navigateur
6. Configurer le mapping, créer des profils de jeu, ou tout remettre par défaut
7. **Redémarrer sans GPIO 16** → la manette utilise le profil actif en Bluetooth

Jusqu'à **4 profils** peuvent être stockés en mémoire flash (NVS).

> Changez ce mot de passe dans `main/config.h` avant une utilisation hors d'un environnement maîtrisé.

---

## 9. Commande tout-en-un (build + flash + monitor)

```bash
idf.py -p COM3 flash monitor
```

---

## Dépannage courant

| Symptôme | Cause probable | Solution |
|---|---|---|
| `fatal error: driver/adc.h: No such file` | IDF v5.x sans compat legacy | Utiliser IDF v4.4.x ou migrer vers l'API ADC oneshot |
| `error: 'esp32' component not found` | Ancien CMakeLists | Déjà corrigé dans ce projet |
| `Failed to connect to ESP32` | Mauvais port ou mode boot | Vérifier port, maintenir BOOT |
| `MCP23017 timeout` | Câblage I2C ou adresse | Vérifier SDA/SCL et A0/A1/A2 → GND |
| BT non détecté | sdkconfig incomplet | Relancer menuconfig, activer Classic BT + HID |
| Sticks dérivés | Stick déplacé pendant la mesure au démarrage, ou deadzone trop faible | Redémarrer sans toucher les sticks, puis augmenter `STICK_DEADZONE` si nécessaire |
