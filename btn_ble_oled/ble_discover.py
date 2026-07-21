import asyncio
from bleak import BleakScanner

async def main():

    print("Scanning...")

    devices = await BleakScanner.discover(timeout=5)

    for d in devices:
        print(d)

asyncio.run(main())
