#!pipx run
import argparse
import sys

def compute_mac_from_key(key):
    return [
        key[0] | 0b11000000,  # Modify the first byte
        key[1],
        key[2],
        key[3],
        key[4],
        key[5],
    ]

def extract_macs(file_path):
    with open(file_path, 'rb') as f:
        data = f.read()
    
    num_keys = data[0]
    macs = []

    num_keys = max(num_keys, (len(data) - 1) // 28)
    if num_keys != data[0]:
        print(f"Warning: Expected {data[0]} keys, but found {num_keys} keys.", file=sys.stderr)

    for i in range(num_keys):
        key = data[1 + i*28 : 1 + (i+1)*28]
        mac = compute_mac_from_key(key)
        mac_str = ':'.join(f'{byte:02X}' for byte in mac)
        macs.append(mac_str)
    
    return macs

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Extract MAC addresses from a binary file.")
    parser.add_argument("file", help="Path to the binary file.")
    args = parser.parse_args()

    macs = extract_macs(args.file)
    for mac in macs:
        print(mac)

