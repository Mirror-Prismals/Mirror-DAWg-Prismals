class MirrorChannel:
    def __init__(self, name):
        self.name = name

    def process(self, data):
        raise NotImplementedError("Each channel must implement its own processing method.")

class ReflectEngine(MirrorChannel):
    def __init__(self):
        super().__init__("ReflectEngine")

    def process(self, data):
        return f"ReflectEngine processing {data}"

class EchoEngine(MirrorChannel):
    def __init__(self):
        super().__init__("EchoEngine")

    def process(self, data):
        return f"EchoEngine processing {data}"

class PrismEngine(MirrorChannel):
    def __init__(self):
        super().__init__("PrismEngine")

    def process(self, data):
        return f"PrismEngine processing {data}"

class ReflexEngine(MirrorChannel):
    def __init__(self):
        super().__init__("ReflexEngine")

    def process(self, data):
        return f"ReflexEngine processing {data}"

class ReverbEngine(MirrorChannel):
    def __init__(self):
        super().__init__("ReverbEngine")

    def process(self, data):
        return f"ReverbEngine processing {data}"

class MirrorSystem:
    def __init__(self):
        self.channels = {
            "ReflectEngine": ReflectEngine(),
            "EchoEngine": EchoEngine(),
            "PrismEngine": PrismEngine(),
            "ReflexEngine": ReflexEngine(),
            "ReverbEngine": ReverbEngine(),
        }

    def process_data(self, channel_name, data):
        if channel_name in self.channels:
            return self.channels[channel_name].process(data)
        else:
            return "Invalid channel name"

if __name__ == "__main__":
    system = MirrorSystem()
    test_data = "Test Data"
    for channel in system.channels:
        print(system.process_data(channel, test_data))
