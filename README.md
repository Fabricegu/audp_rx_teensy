# audp_rx_teensy

Réception d'un flux audio multicast UDP sur **Teensy 4.1**, sans gestion de
la gigue de buffer (pas de tampon de gigue / jitter buffer).
<!-- Note : le terme exact en français est « gigue » ; le problème original
     utilisait le terme familier « gîte de buffer ». -->

---

## Description

Ce sketch Arduino reçoit un flux audio PCM 16 bits brut envoyé en **UDP
multicast** et le reproduit en temps réel via la sortie I²S du Teensy 4.1.
L'implémentation est volontairement simple : aucune gestion de la gigue de
buffer (jitter buffer) n'est effectuée.  Chaque paquet UDP est transmis
directement à la file audio (`AudioPlayQueue`) dès sa réception.  En
l'absence de paquet, la bibliothèque audio génère du silence.

---

## Matériel requis

| Élément | Détails |
|---------|---------|
| Microcontrôleur | Teensy 4.1 |
| Connectivité | Port Ethernet intégré du Teensy 4.1 (câble RJ-45) |
| Sortie audio | Codec I²S connecté aux broches I²S du Teensy (MCLK, BCLK, LRCLK, DOUT) ou DAC I²S externe |

---

## Bibliothèques requises

| Bibliothèque | Source |
|---|---|
| **QNEthernet** | <https://github.com/ssilverman/QNEthernet> |
| **Teensy Audio Library** | Incluse dans Teensyduino |

Installez QNEthernet via le gestionnaire de bibliothèques de l'IDE Arduino ou
en ajoutant la dépendance dans votre `platformio.ini`.

---

## Configuration

Éditez `config.h` pour adapter les paramètres à votre réseau et à votre
format audio :

```cpp
// Adresse du groupe multicast IPv4
#define MULTICAST_IP    239, 0, 0, 1

// Port UDP d'écoute
#define MULTICAST_PORT  5004

// Taille maximale du payload UDP (octets)
#define UDP_PACKET_SIZE 1472

// Nombre de blocs mémoire audio alloués
#define AUDIO_MEMORY_BLOCKS 16

// Nombre de canaux audio (1 = mono, 2 = stéréo)
#define AUDIO_CHANNELS 1
```

---

## Format audio attendu

Chaque paquet UDP doit contenir des échantillons **PCM 16 bits signés,
little-endian**, sans en-tête RTP ni autre en-tête.

| Paramètre | Valeur |
|-----------|--------|
| Taux d'échantillonnage | 44 100 Hz |
| Profondeur de bits | 16 bits signé |
| Canaux | 1 (mono) par défaut, configurable |
| Encodage | PCM brut, little-endian |

---

## Comportement sans gestion de la gigue de buffer

- Les paquets reçus sont immédiatement transmis à la file audio.
- Si la file est pleine (queue audio saturée), les échantillons supplémentaires du paquet sont **supprimés** sans tentative de resynchronisation.
- Si aucun paquet n'arrive à temps, la bibliothèque audio produit du **silence** automatiquement.
- Aucune logique de tampon circulaire, de contrôle de dérive (clock drift) ni de resynchronisation n'est implémentée.

---

## Installation et téléversement

1. Ouvrez `audp_rx_teensy.ino` dans l'IDE Arduino (avec Teensyduino installé).
2. Sélectionnez la carte **Teensy 4.1** et le port USB approprié.
3. Compilez et téléversez.
4. Ouvrez le moniteur série (115200 baud) pour visualiser l'adresse IP et
   l'état de la connexion multicast.

---

## Envoi d'un flux de test

Exemple avec `ffmpeg` pour envoyer un fichier audio en UDP multicast :

```bash
ffmpeg -re -i source.wav \
  -ar 44100 -ac 1 -f s16le \
  udp://239.0.0.1:5004
```

> Ajustez `-ac 1` en `-ac 2` pour un flux stéréo et pensez à mettre à jour
> `AUDIO_CHANNELS` dans `config.h`.
