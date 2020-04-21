import asyncio
import json

def make_jsonrpc(method, params):
    jsonrpc = {
        'jsonrpc':      '2.0',
        'id':           123,
        'method':       method,
        'params':       params
    }
    jsonstr = json.dumps(jsonrpc)
    b = bytes(jsonstr, 'utf-8') + b'\0'
    return b


async def invoke_async(method, params):
    reader, writer = await asyncio.open_connection('192.168.1.20', 1022)

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


async def test_all_methods():
    hello_info = await invoke_async('sys.hello', [])
    print(hello_info)
    #await invoke_async('doser.pump_until', [0, 3000])
    #return
    #await invoke_async('doser.pump', [0, 1.0])


    # 测试获取排程
    await invoke_async('doser.schedule_get', [])

    # 测试设置排程
    job1 = {
        'name': '添加Mg',
        "when": { "dow": [1, 2, 3, 4, 5, 6, 7], "hours": list(range(0, 24)), "minute": 45 },
        "payloads": [0.8, 0.2, 0, 0]
    }
    print('-------------------------------')
    print (job1)
    await invoke_async('doser.schedule_set', [job1])

    # 获取设置后的排程
    print('-----------------------------------------------')
    schedule = await invoke_async('doser.schedule_get', [])
    print(schedule)

asyncio.run(test_all_methods())