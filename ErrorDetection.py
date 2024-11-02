with open("PacketsReceiver.txt", 'r') as file:
    receiver = file.read()

with open("PacketsTransmitter.txt", 'r') as file:
    transmitter = file.read()


receiverPackets = list(map(lambda p: p.split(" ")[4:], filter(lambda p: p != "" and len(p) > 6,receiver.split("\n"))))
receiverBccInBuf = list(map(lambda p: p.split(" ")[0], filter(lambda p: p != "" and len(p) > 6,receiver.split("\n"))))
receiverBccCalc = list(map(lambda p: p.split(" ")[1], filter(lambda p: p != "" and len(p) > 6,receiver.split("\n"))))



transmitterPackets = list(map(lambda p: p.split(" ")[3:], filter(lambda p: p != "" and len(p) > 3, transmitter.split("\n"))))
transmitterBcc = list(map(lambda p: p.split(" ")[0], filter(lambda p: p != "" and len(p) > 3, transmitter.split("\n"))))




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
        print(f"PACKET N: {ip+1}")
        print(f"BCC Received: 0x{int(receiverBccInBuf[ip], 16):x} \nBCC Transmitted: 0x{int(transmitterBcc[ip], 16):x}\n")
        print(errors)
        resultReceived = 0
        for v in received:
            print(v, end="^")
            resultReceived = resultReceived ^ int(v, 16)
        print(f"\b=0x{resultReceived:x}")
        resultSended = 0
        for v in sended:
            print(v, end="^")
            resultSended = resultSended ^ int(v, 16)
        print(f"\b=0x{resultSended:x}")
        if (resultSended == resultReceived):
            print("Result of xor is the same although bytes are different!")
            print("Difference in penguin results in the Bcc method not being robust enough...")
        else:
            print("Some error actually occured. Xor is different...")
            if (receiverBccInBuf[ip] == transmitterBcc[ip]):
                print("BCCs are different. The error in the frame affected the BCC and the values.")
                print("That explains the difference between frames and the error not being detected...")


if count == 0:
    print("No errors! Perfect penguin!")




