with open("PacketsReceiver.txt", 'r') as file:
    receiver = file.read()

with open("PacketsTransmitter.txt", 'r') as file:
    transmitter = file.read()


receiverPackets = list(map(lambda p: p.split(" ")[2:], receiver.split("\n")))

#[
#[1,3, 4, 5, ]   
#[2, 3, 4,65 ,6 7]
#]




transmitterPackets = list(map(lambda p: p.split(" ")[2:], transmitter.split("\n")))

print(f"Receiver Total Packets: {len(receiverPackets)}")
print(f"Transmitter Total Packets: {len(transmitterPackets)}")

for ip in range(len(receiverPackets)):
    packetR = receiverPackets[ip]
    packetT = transmitterPackets[ip]
    print(f"PACKET N: {ip}")

    for iv in range(len(packetR)):
               if (packetR[iv] != packetT[iv]):
                    print(f"{iv}: ({packetR[iv]},{packetT[iv]})")






