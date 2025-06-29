import asyncio
import platform
import uinput
from bleak import BleakClient, BleakScanner
from bleak.exc import BleakError

# Nordic UART Service UUIDs
UART_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
UART_RX_CHAR_UUID = (
    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  # for sending data from Client to Scroll Wheel
)
UART_TX_CHAR_UUID = (
    "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  # for notifications from Scroll Wheel to Client
)

EV_REL = 0x02
REL_WHEEL_HI_RES = 0x0B
REL_WHEEL = uinput.REL_WHEEL

device = uinput.Device(
    [
        (EV_REL, REL_WHEEL_HI_RES),  # High-Resolution Wheel Event
        REL_WHEEL,  # Normal Wheel Event
    ]
)


# Callback für empfangene Daten
async def notification_handler(sender, data):
    data = data.decode().strip()
    print(f"Received data: {data}")

    if data.startswith("SCR:"):
        # Handle scroll wheel data
        scroll_value = int(data[4:])  # Skip the "SCR:" prefix

        device.emit((EV_REL, REL_WHEEL_HI_RES), scroll_value)

        return
    elif data.startswith("BAT:"):
        # Handle battery data
        battery_level = int(data[4:])  # Skip the "BAT:" prefix
        print(f"Battery level: {battery_level}")
        return

    else:
        # Handle other data
        print(f"Received unknown data: {data}")
        return


async def run():
    # Include a timeout for the connection attempt
    CONNECTION_TIMEOUT = 10.0

    print("Searching for Scroll Wheel...")

    device = None
    try:
        # Suche zunächst nach dem Gerätenamen
        device = await BleakScanner.find_device_by_filter(
            lambda d, ad: d.name and ("Scroll Wheel" in d.name)
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
                f"Device Found: {device.name or 'Unknonw'} ({device.address})"
            )
        
        await asyncio.sleep(1)
        
    except Exception as e:
        print(f"Scan-error: {e}")
        await asyncio.sleep(1)

    print(f"Connecting with {device.name or 'Unknown device'} ({device.address})...")

    try:
        async with BleakClient(device, timeout=CONNECTION_TIMEOUT) as client:
            print("Connected!")

            # List services and characteristics
            print("\nAvaiable Services:")
            for service in client.services:
                print(f"Service: {service.uuid}")
                for char in service.characteristics:
                    print(f"  Characteristic: {char.uuid}")
                    print(f"  Properties: {', '.join(char.properties)}")

            # Check for UART service
            uart_service_found = any(
                service.uuid.lower() == UART_SERVICE_UUID.lower()
                for service in client.services
            )

            if not uart_service_found:
                print(f"WARNING: UART Service ({UART_SERVICE_UUID}) not found!")

            # Check for TX characteristic
            tx_char_found = False
            for service in client.services:
                for char in service.characteristics:
                    if UART_TX_CHAR_UUID.lower() in char.uuid.lower():
                        tx_char_found = True

            if not tx_char_found:
                print("WARNING: TX Characteristic not found!")


            # Should be edited in the future für sending configs
            # Start notifications
            print("\nActivating notifications...")
            await client.start_notify(UART_TX_CHAR_UUID, notification_handler)
            print("Notifications active")

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
        print(f"BLE error: {e}")
        print(f"Error message: {error_message}")

    except Exception as e:
        print(f"Error: {e}")

    print("Exiting.")


if __name__ == "__main__":
    try:
        # Plattform-Check
        if platform.system() == "Linux":
            print("Linux recognized: Make shure Bluez is installed and running.")

        asyncio.run(run())
    except KeyboardInterrupt:
        print("\nProgramm exited through user.")
    except Exception as e:
        print(f"Unexpected Error: {e}")
