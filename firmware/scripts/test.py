import asyncio
import json
import time

id_counter = 1
DEVICE_IP = "192.168.1.8"

def make_jsonrpc(method, params):
    global id_counter
    jsonrpc = {
        'jsonrpc':      '2.0',
        'id':           id_counter,
        'method':       method,
        'params':       params
    }
    jsonstr = json.dumps(jsonrpc)
    b = bytes(jsonstr, 'utf-8') + b'\0'
    id_counter += 1
    return b

def make_batch_jsonrpc(methods):
    request = []
    global id_counter
    for method in methods:
        jsonrpc = {
            'jsonrpc':      '2.0',
            'id':           id_counter,
            'method':       method['method'],
            'params':       method['params']
        }
        request.append(jsonrpc)
        id_counter += 1
    jsonstr = json.dumps(request)
    b = bytes(jsonstr, 'utf-8') + b'\0'
    return b


async def connect_async():
    fut = asyncio.open_connection(DEVICE_IP, 1022)
    return await asyncio.wait_for(fut, timeout=10)

async def invoke_async(method, params):
    reader, writer = await connect_async()

    writer.write(make_jsonrpc(method, params))
    await writer.drain()

    rx_buf = await reader.readuntil(separator=b'\0')
    rx_buf = rx_buf[:-1]
    response = json.loads(rx_buf.decode())
    if('error' in response):
        raise RuntimeError(response['error'])

    writer.close()
    await writer.wait_closed()
    return response['result']


async def invoke_many_async(methods):
    reader, writer = await connect_async()
    writer.write(make_batch_jsonrpc(methods))
    await writer.drain()

    rx_buf = await reader.readuntil(separator=b'\0')
    rx_buf = rx_buf[:-1]
    response = json.loads(rx_buf.decode())
    writer.close()
    await writer.wait_closed()
    return response


async def doser_set_speed(ch, speed):
    await invoke_async('doser.speed_set', [ch, speed])

async def test_all_methods():
    # 测试一次调用多个 json-rpc 方法
    response = await invoke_many_async([
        {'method': 'sys.hello', 'params': []},
        {'method': 'doser.status', 'params': []},
        {'method': 'sys.hello', 'params': []},
        {'method': 'doser.status', 'params': []},
    ])

    print(response)
    return

    response = await invoke_async('doser.status', [0, 3000])
    print('---------------------------------------')
    print(response)
    await invoke_async('doser.pump_until', [0, 3000])
    await invoke_async('doser.pump_until', [1, 6000])
    await invoke_async('doser.pump_until', [2, 9000])
    await invoke_async('doser.pump_until', [3, 12000])
    return
    #return
    #await invoke_async('doser.pump', [0, 5.0])


    # 测试获取排程
    response = await invoke_async('doser.schedule_get', [])
    print(response)

    # 测试设置排程
    job1 = {
        'name': '添加Mg',
        'canParallel': True,
        "when": { "dow": [1,2,3,4,5,6,0], "hours": [6, 12, 18], "minute": 45 },
        "payloads": [0.8, 1, 1, 1]
    }
    job2 = {
        'name': '添加Ca',
        'canParallel': True,
        "when": { "dow": [1,2,3,4,5,6,0], "hours": [8, 18], "minute": 45 },
        "payloads": [0.8, 1, 1, 1]
    }
    await invoke_async('doser.schedule_set', [job1, job2])

    # 获取设置后的排程
    print('-----------------------------------------------')
    schedule = await invoke_async('doser.schedule_get', [])
    print(schedule)

if __name__ == '__main__':
    loop=asyncio.get_event_loop()
    loop.run_until_complete(test_all_methods())
