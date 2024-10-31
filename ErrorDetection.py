with open("PacketsReceiver.txt", 'r') as file:
    receiver = file.read()

with open("PacketsTransmitter.txt", 'r') as file:
    transmitter = file.read()


receiverPackets = list(map(lambda p: p.split(" ")[2:], filter(lambda p: p != "",receiver.split("\n"))))

transmitterPackets = list(map(lambda p: p.split(" ")[2:], filter(lambda p: p != "",transmitter.split("\n"))))


print(f"Receiver Total Packets: {len(receiverPackets)}")
print(f"Transmitter Total Packets: {len(transmitterPackets)}\n")
print("Errors Detected \n[Byte Number]:(ByteReceived, ByteSent)\n")
for ip in range(len(receiverPackets)):
    packetR = receiverPackets[ip]
    packetT = transmitterPackets[ip]

    errors = ""
    for iv in range(len(packetR)):
        if (packetR[iv] != packetT[iv]):
            errors += f"{iv}: ({packetR[iv]},{packetT[iv]})\n"
    
    if errors != "":
        print(f"PACKET N: {ip}")
        print(errors)






