import re
from pprint import pprint
import time
import datetime
import json
from os import path
from os import makedirs


file = open("experiment1.txt", "r")
text = file.read()

addresses = \
    {"EE:8A:D3:FF:74:5F:A1:B6" : "Node_A",
     "A6:A1:5C:7E:CF:43:6D:58" : "Node_B",
     "BE:91:55:3A:34:1E:5E:CD" : "Node_C"}


# EE:8A:D3:FF:74:5F:A1:B6
# print(text)

snippets = re.split("=== Link layer header ===", text)

damaged = 0
weird_data = 0
del snippets[0]

message_dict = {
    "Node_A": [],
    "Node_B": [],
    "Node_C": []
}

for snippet in snippets:
    try:
        rssi = int(re.search("rssi:\ -?\d*", snippet).group().replace("rssi: ", ""))

        data = re.search("Data : .*", snippet).group().replace("Data : ", "").split(",")
        for i in range(len(data)):
            # print(data[i])
            match = re.search("-?\d+", data[i])
            if match:
                # print(data[i], data[i].split("-"))
                data[i] = int(match.group())

        data = data[:7]

        packet = data[0]
        acceleration = data[1:4]
        gyro = data[4:7]

        # print(data)

        sender = re.search("src_l2addr: .*", snippet).group().replace("src_l2addr: ", "")

        time_str = re.search("\n.* # if_pid", snippet).group().replace(" # if_pid", "").replace("\n", "")
        timestamp = time.mktime(datetime.datetime.strptime(time_str, "%Y-%m-%d %H:%M:%S,%f").timetuple())
        timestamp += int(time_str.split(",")[1]) / 1000

        # timestamp = float(re.search("timestamp: .* lqi", snippet).group().replace("timestamp: ", "").replace(" lqi", ""))
        # print(sender)

        message_dict[addresses[sender]].append({"packet": packet, "acceleration": acceleration, "gyro": gyro, "rssi": rssi, "timestamp": timestamp})

    except Exception as e:
        # print(e)
        damaged += 1
        continue

reverse_message_dict = {
    "Node_A": [],
    "Node_B": [],
    "Node_C": []
}

for node, message_list in message_dict.items():
    # print(node, len(message_list))
    # pprint(message_list[-1])

    packet_max = message_list[-1]["packet"] + 100
    time_min = message_list[0]["timestamp"]
    time_max = message_list[-1]["timestamp"] + 10

    # print(packet_max)
    # print(time_max)

    for i in range(len(message_list) - 1, -1, -1):

        temp = message_list[i].copy()
        temp["packet"] = packet_max - int(temp["packet"])
        temp["timestamp"] = time_min + time_max - int(temp["timestamp"])

        reverse_message_dict[node].append(temp)

        # if i % 1000 == 0:
        #     print(temp["timestamp"])
        #     print(temp["packet"])


n = 0
while n < len(message_dict["Node_A"]):
    print("ts", message_dict["Node_A"][n]["timestamp"])
    print("pa", message_dict["Node_A"][n]["packet"])
    n += 1000

for node, message_list in message_dict.items():
    print(node)

    raw_path = path.join(path.dirname(path.abspath(__file__)), "Data/Raw/" + node)

    json_path = path.join(path.dirname(path.abspath(__file__)), "Data/JSON/" + node)

    for i in range(10):
        print(i)


        if not path.exists(json_path):
            makedirs(json_path, mode=0o777)

        file_json = open(path.join(json_path, "exp" + str(i) + ".json"), "w")

        file_json.write(json.dumps(message_list[12000 * i: 12000 * (i+1)], indent=1))

        file_raw = open(path.join(raw_path, "exp" + str(i) + ".txt"), "w")
        file_raw = open(path.join(raw_path, "exp" + str(i) + ".txt"), "a")

        for message in message_list[12000 * i: 12000 * (i+1)]:
            list = [message["timestamp"], message["rssi"], message["packet"]] + message["acceleration"] + message["gyro"]
            for i in range(len(list)):
                list[i] = str(list[i])

            # print(list[0])

            string = ",".join(list) + "\n"
            file_raw.write(string)


