#!/usr/bin/env python
import asyncio
import time
from main import get_status

async def test():
    result = await get_status()
    print(result)

if __name__ == "__main__":
    asyncio.run(test())
