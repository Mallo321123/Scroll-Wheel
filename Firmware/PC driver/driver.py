import asyncio
import platform
import uinput
from bleak import BleakClient, BleakScanner
from bleak.exc import BleakError

# Nordic UART Service UUIDs
UART_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
UART_RX_CHAR_UUID = (
    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  # Für Schreiben vom Client zum ESP32
)
UART_TX_CHAR_UUID = (
    "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  # Für Lesen/Notify vom ESP32 zum Client
)

EV_REL = 0x02
REL_WHEEL_HI_RES = 0x0B  # 11
REL_WHEEL = uinput.REL_WHEEL  # das existiert im Modul

device = uinput.Device(
    [
        (EV_REL, REL_WHEEL_HI_RES),  # High-Resolution Wheel Event
        REL_WHEEL,  # Normales Wheel Event
    ]
)

# Callback für empfangene Daten
async def notification_handler(sender, data):
    print(f"Empfangen: {data.decode().strip()}")
    device.emit((EV_REL, REL_WHEEL_HI_RES), int(data.decode().strip()))


async def run():
    # Erhöhe die Verbindungszeit für bessere Stabilität
    CONNECTION_TIMEOUT = 10.0

    print("Suche nach BLE UART Gerät...")

    # Setze wiederholte Scans um das Gerät zu finden
    device = None
    scan_attempts = 3

    for attempt in range(scan_attempts):
        try:
            # Suche zunächst nach dem Gerätenamen
            device = await BleakScanner.find_device_by_filter(
                lambda d, ad: d.name and ("ESP32_BLEuart" in d.name)
            )

            # Alternativ nach Service-UUID suchen
            if not device:
                device = await BleakScanner.find_device_by_filter(
                    lambda d, ad: ad
                    and ad.service_uuids
                    and UART_SERVICE_UUID.lower()
                    in [s.lower() for s in ad.service_uuids]
                )

            if device:
                print(
                    f"Gerät gefunden: {device.name or 'Unbekannt'} ({device.address})"
                )
                break

            print(
                f"Versuch {attempt + 1}/{scan_attempts}: Kein ESP32_BLEuart gefunden, versuche erneut..."
            )
            await asyncio.sleep(1)

        except Exception as e:
            print(f"Scan-Fehler: {e}")
            await asyncio.sleep(1)

    if not device:
        print("Keine passenden Geräte gefunden.")
        print("Zeige alle verfügbaren BLE-Geräte:")

        try:
            devices = await BleakScanner.discover()
            if not devices:
                print("Keine BLE-Geräte in Reichweite gefunden!")
            else:
                for i, d in enumerate(devices):
                    print(f"{i + 1}. {d.name or 'Unbekannt'} ({d.address})")

                # Optional: Manuelles Auswählen zulassen
                choice = input(
                    "\nWählen Sie ein Gerät (Nummer) oder drücken Sie Enter zum Beenden: "
                )
                if choice and choice.isdigit():
                    idx = int(choice) - 1
                    if 0 <= idx < len(devices):
                        device = devices[idx]
                        print(f"Ausgewählt: {device.name or 'Unbekannt'}")
                    else:
                        print("Ungültige Auswahl.")
        except Exception as e:
            print(f"Fehler beim Scannen: {e}")

        if not device:
            print("Kein Gerät ausgewählt. Beende Programm.")
            return

    print(f"Verbinde mit {device.name or 'Unbekanntem Gerät'} ({device.address})...")

    try:
        async with BleakClient(device, timeout=CONNECTION_TIMEOUT) as client:
            print("Verbunden!")

            # Dienste auflisten
            print("\nVerfügbare Services:")
            for service in client.services:
                print(f"Service: {service.uuid}")
                for char in service.characteristics:
                    print(f"  Characteristic: {char.uuid}")
                    print(f"  Properties: {', '.join(char.properties)}")

            # Prüfen ob UART Service vorhanden
            uart_service_found = any(
                service.uuid.lower() == UART_SERVICE_UUID.lower()
                for service in client.services
            )

            if not uart_service_found:
                print(f"WARNUNG: UART Service ({UART_SERVICE_UUID}) nicht gefunden!")

            # Prüfen ob TX Characteristic vorhanden (für Benachrichtigungen vom ESP32)
            tx_char_found = False
            for service in client.services:
                for char in service.characteristics:
                    if UART_TX_CHAR_UUID.lower() in char.uuid.lower():
                        tx_char_found = True

            if not tx_char_found:
                print("WARNUNG: TX Characteristic nicht gefunden!")

            # Starte Benachrichtigungen
            print("\nAktiviere Benachrichtigungen...")
            await client.start_notify(UART_TX_CHAR_UUID, notification_handler)
            print("Benachrichtigungen aktiv")

            print("\nVerbindung hergestellt. Sie können nun Nachrichten senden.")
            print("Geben Sie eine Nachricht ein oder 'exit' zum Beenden.")

            while True:
                message = await asyncio.get_event_loop().run_in_executor(
                    None, input, "> "
                )

                if message.lower() == "exit":
                    print("Beenden...")
                    break

                try:
                    # Nachricht an ESP32 senden
                    await client.write_gatt_char(UART_RX_CHAR_UUID, message.encode())
                    print(f"Gesendet: {message}")
                except Exception as e:
                    print(f"Sendefehler: {e}")
                    if "disconnected" in str(e).lower():
                        print("Verbindung verloren!")
                        break

            # Benachrichtigungen deaktivieren
            await client.stop_notify(UART_TX_CHAR_UUID)

    except BleakError as e:
        error_message = str(e).lower()
        print(f"BLE Fehler: {e}")

        if "device with address" in error_message and "was not found" in error_message:
            print(
                "Gerät nicht gefunden. Stellen Sie sicher, dass es eingeschaltet ist."
            )
        elif "timed out" in error_message:
            print("Zeitüberschreitung. Das Gerät reagiert nicht.")
        elif "disconnected" in error_message:
            print("Verbindung wurde unerwartet getrennt.")

    except Exception as e:
        print(f"Allgemeiner Fehler: {e}")

    print("Programm beendet.")


if __name__ == "__main__":
    try:
        # Plattform-Check
        if platform.system() == "Linux":
            print("Linux erkannt: Stelle sicher, dass BlueZ installiert ist und läuft.")

        print("BLE UART Client für ESP32 - Drücken Sie Strg+C zum Beenden")
        asyncio.run(run())
    except KeyboardInterrupt:
        print("\nProgramm durch Benutzer beendet.")
    except Exception as e:
        print(f"Unerwarteter Fehler: {e}")
