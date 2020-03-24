
import argparse
import asyncio
import math
import json
import random

from dataclasses import dataclass
from multiprocessing import Pool
from functools import partial
from time import time

import aiohttp
import monero


# openmonero backend url
om_url = "http://127.0.0.1:1984/"


async def make_request(url, payload=""):

    headers = {'content-type': "application/json", 
                'cache-control': "no-cache"}

    async with aiohttp.ClientSession(headers=headers) as session:
        async with session.post(url, data=json.dumps(payload)) as response:
            if response.status == 200:
                result = await response.json()
                print(result)
                return result

@dataclass
class OpenMonero:

    address: str
    viewkey: str
    url: str = om_url

    def __post_init__(self):
        self.payload = {"address": self.address, 
                        "view_key": self.viewkey}

    async def get_version(self):
        return await make_request(om_url+"get_version")

    async def login(self):
        payload = dict(self.payload, 
                **{"create_account":False,
                   "generated_locally":False})
        return await make_request(self.url+"login", payload)
       
    async def import_recent_wallet(self, no_blocks=100000):
        payload = dict(self.payload, 
                **{"no_blocks_to_import":str(no_blocks)})
        return await make_request(self.url
                 +"import_recent_wallet_request", payload)

    async def import_wallet(self):
        return await make_request(self.url
                 +"import_wallet_request", self.payload)

    async def get_address_txs(self):
        return await make_request(self.url+"get_address_txs", 
               self.payload)

    async def get_address_info(self):
        return await make_request(self.url+"get_address_info", 
               self.payload)


def new_address(n, net_type='stagenet'):
    # n is not used
    s = monero.seed.Seed()
    s_address = s.public_address(net_type)
    s_viewkey = s.secret_view_key()
    return str(s_address), s_viewkey
    

def pool_address_generator(n=10,net_type='stagenet'):
    new_address_part = partial(new_address, net_type=net_type)
    with Pool(5) as p:
        return p.map(new_address_part, range(n))


async def apool_address_gen(no_of_chunnks,chunck_size=5, 
                            net_type='stagenet'):
    loop = asyncio.get_running_loop()
    for i in range(no_of_chunnks):
        yield await loop.run_in_executor(
            None, pool_address_generator, chunck_size, net_type)
        

async def create_tasks(n=10, net_type='stagenet'):

    requests_list = []

    chunck_size = 5;
    no_of_chunnks = math.ceil(n/chunck_size);

    ct = asyncio.create_task
    
    i = 1;

    async for addresses in apool_address_gen(
            no_of_chunnks, net_type=net_type):

        for s_address, s_viewkey in addresses:

            print(f"\n{i}/{n}: {s_address}")

            om = OpenMonero(s_address, s_viewkey)

            requests_list.append(ct(om.get_version()))
            requests_list.append(ct(om.login()))
            requests_list.append(ct(om.import_wallet()))
            requests_list.append(ct(om.import_recent_wallet()))
            requests_list.append(ct(om.get_address_txs()))
            requests_list.append(ct(om.get_address_info()))
            
            i = i + 1
    
    return await asyncio.gather(*requests_list)


async def main():

    parser = argparse.ArgumentParser(
            description='OpenMonero account imports.')
   
    parser.add_argument("-n", "--number", 
            help="number of accounts to import", type=int, 
            default=1)
    
    parser.add_argument("-b", "--blocks", 
            help="number of recent blocks to import", type=int, 
            default=100000)

    parser.add_argument("-t", "--testnet", 
            help="testnet network", action="store_true")
    
    parser.add_argument("-s", "--stagenet", 
            help="stagenet network", action="store_true")
    
    parser.add_argument("-m", "--mainnet", 
            help="mainnet network", action="store_true")

    args = parser.parse_args()

    net_type = "stagenet"

    if args.testnet:
        net_type = "testnet"
    
    if args.mainnet:
        net_type = "mainnet"

    await create_tasks(args.number, net_type)


if __name__ == "__main__":
    t_start = time()
    asyncio.run(main())
    t_end = time()
    print(f"\n\n[+] {t_end - t_start} seconds elapsed.")
