#!/usr/bin/env python3
"""
TP2 Réseaux IoT - Partie 7
Client MQTT over WebSocket (adapté de chirpstack_mqtt_over_ws.py) :
réception des trames ChirpStack, décodage, graphes temps réel.

Installation :
    python -m pip install paho-mqtt matplotlib

Utilisation :
    python lora_mqtt_decode_plot.py          # mode réel
    python lora_mqtt_decode_plot.py --test   # teste seulement le décodeur, sans réseau
"""

import base64
import json
import queue
import sys
import time

import matplotlib.pyplot as plt
import paho.mqtt.client as mqtt

# ======================= CONFIGURATION =======================
BROKER_HOST = "chirpstack.alora-one.com"
BROKER_PORT = 9001
# Topic de NOTRE carte uniquement (au lieu de application/+/device/+/event/up)
TOPIC = ("application/70fa4fa4-048a-4011-a4ea-5cd6fb563cb5/device/8cf957200000f942/event/up")
# =============================================================

# Identifiants de bloc (identiques aux #define de master_app.c)
LPP_DATATYPE_DIGITAL_INPUT = 0x00
LPP_DATATYPE_TEMPERATURE = 0x67
LPP_DATATYPE_HUMIDITY = 0x68
LPP_DATATYPE_BAROMETER = 0x73
LPP_DATATYPE_FRAME_IDENTIFIER = 0x99


def decode_frame(raw: bytes) -> dict:
    """Décode la trame : [n° bloc][type][taille][données Big Endian]..."""
    out = {}
    i = 0
    while i + 3 <= len(raw):
        _num, typ, size = raw[i], raw[i + 1], raw[i + 2]
        data = raw[i + 3:i + 3 + size]
        if len(data) < size:
            raise ValueError(f"trame tronquée au bloc commençant à l'octet {i}")
        i += 3 + size

        if typ == LPP_DATATYPE_FRAME_IDENTIFIER:
            out["groupe"] = data.decode("ascii", errors="replace")
        elif typ == LPP_DATATYPE_TEMPERATURE:      # int16 signé, déci-degrés
            out["temperature_C"] = int.from_bytes(data, "big", signed=True) / 10
        elif typ == LPP_DATATYPE_DIGITAL_INPUT:    # uint8, pourcentage
            out["batterie_pct"] = data[0]
        elif typ == LPP_DATATYPE_BAROMETER:        # uint16, daPa -> hPa
            out["pression_hPa"] = int.from_bytes(data, "big") / 10
        elif typ == LPP_DATATYPE_HUMIDITY:         # uint8, demi-pourcents
            out["humidite_pct"] = data[0] / 2
        else:
            out[f"inconnu_0x{typ:02X}"] = data.hex()
    return out


# File partagée entre le thread MQTT et le thread graphique (matplotlib = thread principal)
samples = queue.Queue()


def on_connect(client, userdata, flags, reason_code, properties=None):
    print("[MQTT] connecté, code =", reason_code)
    client.subscribe(TOPIC)
    print("[MQTT] abonné à", TOPIC)


def on_message(client, userdata, msg):
    print("\n[RAW]", msg.topic, msg.payload)
    try:
        data = json.loads(msg.payload.decode())
        raw = base64.b64decode(data["data"])
    except (json.JSONDecodeError, KeyError, ValueError) as e:
        print("[!] message sans payload exploitable :", e)
        return

    print("[FCnt]", data.get("fCnt"))
    print("[PAYLOAD hex]", raw.hex(" ").upper())
    try:
        values = decode_frame(raw)
    except ValueError as e:
        print("[!] erreur de décodage :", e)
        return
    print("[DÉCODÉ]", values)
    samples.put((time.time(), values))


def make_client():
    # paho-mqtt 2.x exige la version d'API des callbacks ; 1.x ne la connaît pas
    if hasattr(mqtt, "CallbackAPIVersion"):
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, transport="websockets")
    else:
        client = mqtt.Client(transport="websockets")
    client.on_connect = on_connect
    client.on_message = on_message


    client.ws_set_options(path="/")
    client.ws_set_options(path="/ws")
    client.ws_set_options(path="/ws")
    return client


def live_plot():
    series = {  # clé décodée -> (titre, unité)
        "temperature_C": ("Température", "°C"),
        "humidite_pct": ("Humidité", "%"),
        "pression_hPa": ("Pression", "hPa"),
        "batterie_pct": ("Batterie", "%"),
    }
    history = {k: ([], []) for k in series}
    t0 = time.time()

    plt.ion()
    fig, axes = plt.subplots(len(series), 1, sharex=True, figsize=(9, 8))
    fig.suptitle("Capteurs LoRa en temps réel")

    while plt.fignum_exists(fig.number):
        updated = False
        while not samples.empty():
            t, values = samples.get()
            for key in series:
                if key in values:
                    history[key][0].append(t - t0)
                    history[key][1].append(values[key])
            updated = True

        if updated:
            for ax, (key, (title, unit)) in zip(axes, series.items()):
                xs, ys = history[key]
                ax.clear()
                ax.plot(xs, ys, marker="o")
                ax.set_ylabel(f"{title} ({unit})")
                ax.grid(True)
            axes[-1].set_xlabel("Temps depuis le lancement (s)")
            fig.tight_layout()
        plt.pause(0.5)


def self_test():
    # Trame réelle vue sur le terminal : "MR_DG", 24.4 °C (0x00F4), batterie 100 %
    example = bytes.fromhex("00 99 05 4D 52 5F 44 47  01 67 02 00 F4  02 00 01 64")
    print("Trame :", example.hex(" ").upper())
    print("Décodé:", decode_frame(example))


if __name__ == "__main__":
    if "--test" in sys.argv:
        self_test()
    else:
        client = make_client()
        client.connect(BROKER_HOST, BROKER_PORT)
        client.loop_start()          # réseau MQTT dans un thread de fond
        try:
            live_plot()              # graphes dans le thread principal
        finally:
            client.loop_stop()
            client.disconnect()