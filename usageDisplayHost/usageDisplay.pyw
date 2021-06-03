import GPUtil
import psutil
import time
import serial
import crc8
import serial.tools.list_ports

baud_rate = 57600

interface_ids = {6790: [29987]}

def get_gpu_load():
    GPUs = GPUtil.getGPUs()
    if len(GPUs) > 0:
        return GPUs[0].load, GPUs[0].memoryUtil 
    else:
        return 0.0, 0.0
        

def get_cpu_load():
    mem_status = psutil.virtual_memory()
    memory_usage = (mem_status.total - mem_status.available) / mem_status.total

    return psutil.cpu_percent() / 100.0, memory_usage


def send_usage(port, usages):
    message_bytes = [ int(round(min(max(x, 0.0), 1.0) * 255.0)) for x in usages];
    message_bytes = bytes(message_bytes)
    
    # compute the crc8
    hash = crc8.crc8()
    hash.update(message_bytes)
    message_bytes = message_bytes + hash.digest()
    
    # send it
    port.write(message_bytes)


if __name__ == "__main__":
    while True:    
        try:
            # find port with a compatible device
            ports = serial.tools.list_ports.comports()
            
            # filter the serial devices for supported IDs
            ports_valid = [port for port in ports if port.vid in interface_ids.keys()]
            ports_valid = [port for port in ports_valid if port.pid in interface_ids[port.vid]]
            
            if len(ports_valid) < 1:
                # no device connected, alright :)
                time.sleep(60)
                continue
                        
            # connect to the port
            serial_port_name = ports_valid[0].device
            serial_port = serial.Serial(port=serial_port_name, baudrate=baud_rate)

            while True:
                cpu_load, cpu_mem = get_cpu_load()
                gpu_load, gpu_mem = get_gpu_load()
                send_usage(serial_port, (cpu_load, cpu_mem, gpu_load, gpu_mem))
                
                time.sleep(1)
        except serial.serialutil.SerialException as e:
            # encountered a problem with the serial port. Let's not worry, it probably was just disconnected :)
            # print("caught SerialException '{}'".format(e))
            time.sleep(60)
