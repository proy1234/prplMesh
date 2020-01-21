#!/usr/bin/python3
import os
import unittest
import re
from enum import Enum
import sys

if __name__ == '__main__':
    print("Error: running this as main!")
    sys.exit(1)

testSystem = None
testFiles = [f[0:-3] for f in os.listdir(os.path.dirname(os.path.abspath(__file__))) if 'test' in f and f.endswith('.py')]

__all__ = ['DeviceType', 'LogType', 'BaseTest', 'BaseConfig', 'TestSystem', 'load_tests'] + testFiles

#for testFile in testFiles:
    #__import__(testFile)


class DeviceType(Enum):
    gateway = 1
    agent = 2
    repeater1 = 3
    repeater2 = 4

class LogType(Enum):
    beerocks_agent = 1
    beerocks_agent_wlan0 = 2
    beerocks_agent_wlan2 = 3
    beerocks_monitor_wlan0 = 4
    beerocks_monitor_wlan2 = 5
    beerocks_controller = 6
    transport = 7
    local_bus = 8

class BaseTest(unittest.TestCase):

    def send_controller_command(self, command):
        testSystem.send_command(DeviceType.gateway, command)

    def send_agent_command(self, command):
        testSystem.send_command(DeviceType.agent, command)

    def send_repeater1_command(self, command):
        testSystem.send_command(DeviceType.repeater1, command)

    def send_repeater2_command(self, command):
        testSystem.send_command(DeviceType.repeater2, command)

    def find_in_logs(self, device:DeviceType, log:LogType, text:str, ignore_case:bool = True, regexp:bool = False):
        logs = testSystem.get_log(device, log)
        if regexp:
            flags = re.IGNORECASE if ignore_case else 0
            result = re.search(text, logs, flags)
        elif ignore_case:
            result = text.lower() in logs.lower()
        else:
            result = test in logs
        self.debug("Looking for '%s' in %s log on %s ... %s" %
                  (text, log.name, device.name, "FOUND" if result else "NOT FOUND"))

        return bool(result)
    
    def status(self, text):
        print("\033[1;35m%s\033[0m" % text)

    def error(self, text):
        print("\033[1;31m%s\033[0m\n" % text)

    def success(self, text):
        print("\033[1;32m%s\033[0m\n" % text)

    def debug(self, text):
        if testSystem.verbose:
            print(text)

class BaseConfig:
    def __getitem__(self, key):
        if key in self.values:
            return self.values[key]
        raise KeyError("No key %s in config!" % key)

class TestSystem:

    def __init__(self, verbose=False):
        self.verbose = verbose

    def get_config(self, device:DeviceType) -> BaseConfig:
        if device == DeviceType.gateway:
            return self.gateway_config
        if device == DeviceType.agent:
            return self.agent_config
        raise KeyError("No such device!")
    
    def get_ip(self, device:DeviceType) -> str:
        raise NotImplementedError("This should be implemented in the subclass!")

    def get_port(self, device:DeviceType) -> int:
        raise NotImplementedError("This should be implemented in the subclass!")

    def get_log(self, device:DeviceType, log:LogType):
        raise NotImplementedError("This should be implemented in the subclass!")

    def send_command(self, device:DeviceType, command:str, wait:bool = True) -> str:
        with UCCSocket(self.get_ip(device), self.get_port(device)) as sock:
            sock.send_cmd(command)
            if wait:
                return sock.get_reply()
            return ""

def load_tests(loader, standard_tests, pattern):
    print("load_tests called! testFiles={}".format(testFiles))
    suite = unittest.TestSuite()
    for f in testFiles:
        print(f)
        try:
            testModule = __import__(f)
        except Exception as e:
            print(e)
            print("***")
            print(sys.modules[__name__], dir(sys.modules[__name__]))
            print("***")

        print(testModule)
        tests = loader.loadTestsFromModule(testModule)
        print("Calling loader.loadTestsFromModule({testModule})".format(testModule=testModule))
        print("loader.errors=", loader.errors)
        suite.addTests(tests)
    return suite


