#!/usr/bin/env python3

import argparse
import shutil
import subprocess
from pathlib import Path
import os
import sys
import glob
import threading
import signal
import time
from datetime import datetime

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Error: pyserial is not installed. Please install it using 'pip install pyserial'.")
    sys.exit(1)

def find_bmp_port():
    candidates = glob.glob('/dev/serial/by-id/usb-Black_Magic*_*-if00') + glob.glob('/dev/cu.usbmodem*1')
    if len(candidates) == 1:
        return candidates[0]
    elif len(candidates) > 1:
        print(f"Error: Multiple BMP ports found: {candidates}. Please specify the BMP port using --bmp-port.")
        sys.exit(1)
    else:
        print("Error: Black Magic Probe GDB serial port not found. Please provide the device name via the --bmp-port parameter.")
        sys.exit(1)

def run_monitor_gdb_commands(gdb_executable, bmp_port, elf_file, stop_event):
    """
    Runs GDB with the specified commands to load the ELF file and set up RTT.
    This function runs GDB in a separate process and allows it to run continuously.
    """

    gdb_cmd = [
        gdb_executable,
        '-nx',
        '-ex', 'set confirm off',
        '-ex', f'target extended-remote {bmp_port}',
        '-ex', 'monitor swdp_scan',
        '-ex', 'attach 1',
        '-ex', 'monitor rtt enable',  # Changed from 'mon rtt enable' to 'monitor rtt enable'
        '-ex', 'run',
    ]

    try:
        print("Starting GDB for monitoring...")
        # Start GDB as a subprocess
        process = subprocess.Popen(gdb_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        # Continuously read GDB output to detect when it should stop
        while not stop_event.is_set():
            output = process.stdout.readline()
            if output:
                print(output.strip())
            else:
                break

    except Exception as e:
        print(f"GDB encountered an error: {e}")
    finally:
        if process.poll() is None:
            print("Terminating GDB process...")
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                print("GDB did not terminate gracefully; killing it.")
                process.kill()
        stop_event.set()

def tail_serial_with_timestamps(monitor_port, stop_event, flash_method):
    """
    Tails the serial monitor port or runs `strtt -v 2` command based on the flash method.
    Prints each line with a timestamp.
    """
    if flash_method != 'bmp':
        # Use `strtt -v 2` command instead of direct serial reading
        try:
            print("Using strtt -v 2 for monitoring...")
            process = subprocess.Popen(
                ['strtt', '-v', '2'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
            )
            
            while not stop_event.is_set():
                output = process.stdout.readline()
                if output:
                    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    blue = '\033[94m'
                    gray = '\033[90m'
                    reset = '\033[0m'
                    print(f"[{blue}{timestamp}{reset}] {gray}{output.strip()}{reset}")
                else:
                    break

        except Exception as e:
            print(f"Error while executing strtt: {e}")
        finally:
            if process.poll() is None:
                print("Terminating strtt process...")
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    print("strtt did not terminate gracefully; killing it.")
                    process.kill()
            stop_event.set()
    else:
        # Original logic for BMP
        try:
            ser = serial.Serial(monitor_port, baudrate=115200, timeout=1)
            print(f"Started monitoring on {monitor_port}")
        except serial.SerialException as e:
            print(f"Error opening serial port {monitor_port}: {e}")
            stop_event.set()
            return

        try:
            while not stop_event.is_set():
                line = ser.readline().decode('utf-8', errors='replace').rstrip()
                if line:
                    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    blue = '\033[94m'
                    gray = '\033[90m'
                    reset = '\033[0m'
                    print(f"[{blue}{timestamp}{reset}] {gray}{line}{reset}")
        except Exception as e:
            print(f"Error while reading from serial port: {e}")
        finally:
            ser.close()
            print("Serial monitoring stopped.")
            stop_event.set()

def main():
    parser = argparse.ArgumentParser(description='Patch binary file with advertising keys and optionally flash to device.')
    parser.add_argument('input_bin', type=Path, help='Input binary file to patch')
    parser.add_argument('keys_bin', type=Path, help='Advertising keys binary file')
    parser.add_argument('output_bin', type=Path, help='Output patched binary file (will also create output ELF file)')
    parser.add_argument('--flash', action='store_true', help='Flash the device after patching.')
    parser.add_argument('--monitor', action='store_true', help='Monitor the device using GDB.')
    parser.add_argument('--flash-method', choices=['openocd', 'bmp'], default="bmp", help='Method to use for flashing the device.')
    parser.add_argument('--openocd-config', type=Path, help='Path to OpenOCD configuration file (e.g., openocd.cfg)')
    parser.add_argument('--gdb', default='arm-none-eabi-gdb', help='Path to GDB executable.')
    parser.add_argument('--bmp-port', help='Serial port of the Black Magic Probe GDB server. If not specified, the script will try to find it automatically.')
    args = parser.parse_args()

    input_file = args.input_bin
    adv_keys_file = args.keys_bin
    output_file = args.output_bin
    elf_output_file = output_file.with_suffix('.elf')

    print(f"Patching {input_file.name}")

    # Copy the original binary file to create the patched binary file
    shutil.copyfile(input_file, output_file)

    # Read the advertising keys file, skipping the first byte
    adv_keys_content = adv_keys_file.read_bytes()[1:]  # Skip the first byte

    # Read the original binary file to find the placeholder and end marker offsets
    input_data = input_file.read_bytes()
    placeholder = b'OFFLINEFINDINGPUBLICKEYHERE!'
    end_marker = b'ENDOFKEYSENDOFKEYSENDOFKEYS!'

    start_offset = input_data.find(placeholder)
    if start_offset == -1:
        print("Error: Placeholder string not found in the input file.")
        exit(1)

    end_offset = input_data.find(end_marker, start_offset)

    if end_offset == -1:
        # End marker not found; warn user and proceed
        print("Warning: End marker string not found in the input file.")
        print("Proceeding without end marker; keys may overflow if they don't fit.")
        # Assume available space is the remaining file after start_offset
        available_space = len(input_data) - start_offset
    else:
        # End marker found; check if keys fit
        available_space = end_offset - start_offset
        if len(adv_keys_content) > available_space:
            print("Error: Advertising keys content does not fit between the start and end markers.")
            exit(1)

    # Write the advertising keys content into the patched binary at the correct offset
    with output_file.open('r+b') as f:
        f.seek(start_offset)
        f.write(adv_keys_content)

        # Verify that the keys were patched correctly without reopening the file
        f.seek(start_offset)
        patched_keys = f.read(len(adv_keys_content))
        if patched_keys != adv_keys_content:
            print("The keys were not patched correctly!")
            exit(1)

    # Convert the patched binary into an ELF file using objcopy
    objcopy = 'arm-none-eabi-objcopy'  # Assumes 'arm-none-eabi-objcopy' is in the system's PATH
    try:
        subprocess.run([
            objcopy,
            '-I', 'binary',
            '-O', 'elf32-littlearm',
            '-B', 'arm',
            str(output_file),
            str(elf_output_file)
        ], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error during objcopy: {e}")
        exit(1)

    print(f"Patched binary saved as {output_file}")
    print(f"ELF file saved as {elf_output_file}")

    # Initialize variables for flashing and monitoring
    flash_thread = None
    monitor_thread = None
    stop_event = threading.Event()

    # Define a handler for graceful shutdown
    def signal_handler(sig, frame):
        print("\nInterrupt received, shutting down...")
        stop_event.set()
        if gdb_monitor_thread and gdb_monitor_thread.is_alive():
            print("Waiting for GDB process to terminate...")
            gdb_monitor_thread.join()
        if serial_monitor_thread and serial_monitor_thread.is_alive():
            print("Waiting for serial monitoring to terminate...")
            serial_monitor_thread.join()
        sys.exit(0)

    # Register the signal handler
    signal.signal(signal.SIGINT, signal_handler)

    # Flashing logic
    if args.flash:
        if not args.flash_method:
            print("Error: --flash-method must be specified when using --flash.")
            sys.exit(1)
        if args.flash_method == 'bmp':
            bmp_port = args.bmp_port or find_bmp_port()
            print(f"Flashing using BMP on port {bmp_port}")

            # GDB commands for flashing (erasing)
            gdb_flash_cmd = [
                args.gdb,
                '-nx', '--batch',
                '-ex', 'set confirm off',
                '-ex', f'target extended-remote {bmp_port}',
                '-ex', 'monitor swdp_scan',
                '-ex', 'attach 1',
                '-ex', 'load',
                '-ex', 'compare-sections',
                '-ex', 'kill',
                str(elf_output_file)
            ]
            try:
                subprocess.run(gdb_flash_cmd, check=True)
                print("Flashing completed successfully.")
            except subprocess.CalledProcessError as e:
                print(f"Error during BMP flashing: {e}")
                sys.exit(1)
        elif args.flash_method == 'openocd':
            if not args.openocd_config:
                print("Error: OpenOCD configuration file must be specified with --openocd-config when using the openocd flash method.")
                sys.exit(1)
            if not args.openocd_config.exists():
                print(f"Error: OpenOCD configuration file {args.openocd_config} does not exist.")
                sys.exit(1)
            print("Flashing using OpenOCD")
            openocd_cmd = [
                'openocd',
                '-f', str(args.openocd_config),
                '-c', f'init; halt; nrf51 mass_erase; program {str(output_file)} verify; reset; exit;'
            ]
            try:
                subprocess.run(openocd_cmd, check=True)
                print("Flashing completed successfully.")
            except subprocess.CalledProcessError as e:
                print(f"Error during OpenOCD flashing: {e}")
                sys.exit(1)

    # Monitoring logic
    if args.monitor:
        if args.flash_method != 'bmp':
            print("Error: --monitor is only supported with the 'bmp' flash method.")
            sys.exit(1)
        bmp_port = args.bmp_port or find_bmp_port()
        if 'if00' in bmp_port:
            monitor_port = bmp_port.replace('if00', 'if02')
        else:
            # Attempt to derive monitor port based on common naming conventions
            monitor_port = bmp_port + '2'  # Example: /dev/ttyUSB0 -> /dev/ttyUSB02
            # Verify if the derived monitor_port exists
            if not Path(monitor_port).exists():
                # Alternative derivation for macOS
                monitor_port = bmp_port.replace('if00', 'if02') if 'if00' in bmp_port else bmp_port + '2'
                if not Path(monitor_port).exists():
                    print(f"Error: Unable to determine monitor port from BMP port {bmp_port}. Please specify the monitor port manually.")
                    sys.exit(1)

        print(f"Monitoring device using BMP on port {monitor_port}")

        # Start GDB in a separate thread for monitoring
        gdb_monitor_thread = threading.Thread(
            target=run_monitor_gdb_commands,
            args=(args.gdb, bmp_port, elf_output_file, stop_event),
            daemon=True
        )
        gdb_monitor_thread.start()

        # Start serial monitoring in a separate thread
        serial_monitor_thread = threading.Thread(
            target=tail_serial_with_timestamps,
            args=(monitor_port, stop_event, args.flash_method),
            daemon=True
        )
        serial_monitor_thread.start()

        # Wait for both threads to finish
        try:
            while not stop_event.is_set():
                time.sleep(0.1)
        except KeyboardInterrupt:
            signal_handler(None, None)

    print("Operation completed successfully.")

if __name__ == '__main__':
    main()
