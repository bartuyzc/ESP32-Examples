import asyncio
from bleak import BleakClient

ADDRESS = "B0:B2:1C:A7:11:52"

CHAR_UUID = "abcdefab-1234-5678-1234-abcdefabcdef"

def callback(sender, data):

    print(data.decode())

async def main():

    async with BleakClient(ADDRESS) as client:

        print("Connected")

        await client.start_notify(CHAR_UUID, callback)

        while True:
            await asyncio.sleep(1)

asyncio.run(main())
