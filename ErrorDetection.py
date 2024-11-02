with open("PacketsReceiver.txt", 'r') as file:
    receiver = file.read()

with open("PacketsTransmitter.txt", 'r') as file:
    transmitter = file.read()


receiverPackets = list(map(lambda p: p.split(" ")[2:], filter(lambda p: p != "",receiver.split("\n"))))

transmitterPackets = list(map(lambda p: p.split(" ")[2:], filter(lambda p: p != "",transmitter.split("\n"))))


print(f"Receiver Total Packets: {len(receiverPackets)}")
print(f"Transmitter Total Packets: {len(transmitterPackets)}\n")
print("Errors Detected \n[Byte Number]:(ByteReceived, ByteSent)\n")
count = 0
for ip in range(len(receiverPackets)):
    packetR = receiverPackets[ip]
    packetT = transmitterPackets[ip]

    errors = ""
    received = []
    sended = []
    for iv in range(len(packetR)):
        if (packetR[iv] != packetT[iv]):
            received.append(packetR[iv])
            sended.append(packetT[iv])
            errors += f"{iv}: ({packetR[iv]},{packetT[iv]})\n"
    
    if errors != "":
        count += 1
        print()
        print(f"PACKET N: {ip}")
        print(errors)
        resultReceived = 0
        for v in received:
            print(v, end="^")
            resultReceived = resultReceived ^ int(v, 16)
        print(f"\b={resultReceived:x}")
        resultSended = 0
        for v in sended:
            print(v, end="^")
            resultSended = resultSended ^ int(v, 16)
        print(f"\b={resultSended:x}")
        if (resultSended == resultReceived):
            print("Result of xor is the same although bytes are different!")
            print("Difference in penguin results in the Bcc method not being robust enough...")
        else:
            print("Some error actually occured. Xor is different...")


if count == 0:
    print("No errors! Perfect penguin!")




