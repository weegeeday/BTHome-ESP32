#!/usr/bin/env python3
import sys
import argparse
from types import SimpleNamespace
from bthome_ble import BTHomeBluetoothDeviceData
from bthome_ble.parser import BTHomeData

def main():
    parser = argparse.ArgumentParser(
        description="CLI debugger for decoding raw BTHome BLE advertisement hex payloads."
    )
    parser.add_argument(
        "hex_payload",
        nargs="+",
        help="The raw Service Data hex string from the BLE advertisement (e.g., 4002ca090303bf13)"
    )
    parser.add_argument(
        "-k", "--key", 
        dest="bindkey",
        help="Optional 32-character (16-byte) encryption bindkey hex string if the payload is encrypted"
    )
    
    args = parser.parse_args()
    
    # Process Bindkey if provided
    bindkey_bytes = None
    if args.bindkey:
        try:
            bindkey_bytes = bytes.fromhex(args.bindkey.strip())
            if len(bindkey_bytes) != 16:
                print(f"Error: Bindkey must be exactly 16 bytes (32 hex characters). Got {len(bindkey_bytes)} bytes.")
                sys.exit(1)
        except ValueError:
            print("Error: Invalid hexadecimal string provided for bindkey.")
            sys.exit(1)
            
    # Process Payload
    try:
        raw_bytes = bytes.fromhex("".join(args.hex_payload))
    except ValueError:
        print("Error: Invalid hexadecimal string provided for payload.")
        sys.exit(1)

    # Initialize the parser
    bthome_parser = BTHomeBluetoothDeviceData(bindkey_bytes)
    
    # Build the small service-info object required by the current package API.
    BTHOME_UUID = "0000fcd2-0000-1000-8000-00805f9b34fb"
    service_info = SimpleNamespace(
        service_data={BTHOME_UUID: raw_bytes},
        name="BTHome sensor",
        address="00:00:00:00:00:01",
        rssi=0,
        time=0,
    )
    
    try:
        supported = bthome_parser._parse_bthome(BTHomeData(service_info))
        parsed_data = bthome_parser._finish_update()
    except (TypeError, ValueError, AttributeError) as error:
        print(f"\nError: Failed to parse payload: {error}\n")
        sys.exit(1)
    
    if supported and parsed_data:
        print("\n=== Decoded BTHome Data ===")
        print(f"Device Name: {parsed_data.title}")
        print(f"Device ID:   {bthome_parser.primary_device_id}")
        print("Metrics:")
        for sensor_value in parsed_data.entity_values.values():
            print(f"  - {sensor_value.name}: {sensor_value.native_value}")
        for binary_value in parsed_data.binary_entity_values.values():
            print(f"  - {binary_value.name}: {binary_value.native_value}")
        print("============================\n")
    else:
        print("\n❌ Error: Failed to parse payload. Verify that your hex string matches the BTHome spec and your bindkey is correct if the data is encrypted.\n")
        sys.exit(1)

if __name__ == "__main__":
    main()
