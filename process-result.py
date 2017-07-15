STOP_TIME = 15
DEADLINE_DELTA = -0.000

flow_info = {}  # fid->(flowSize, startTime, deadline)
flow_result = {}  # fid->(flowSize, startTime, stopTime)


def parse_flow_info(filename):
    for line in open(filename):
        fid, fsize, start_time, deadline = eval(line)
        flow_info[fid] = fsize, start_time, deadline


def parse_flow_result(filename):
    for line in open(filename):
        fid, start_time, stop_time, fsize = eval(line)
        flow_result[fid] = fsize, start_time, stop_time


def process_afct(fid_begin, fid_end):
    total_time = 0
    count = 0
    unfinished_count = 0
    for fid, info in flow_info.items():
        if fid_begin <= fid < fid_end and fid in flow_result:
            result = flow_result[fid]
            if info[0] == result[0]:
                total_time += result[2] - result[1]
                count += 1
            else:
                unfinished_count += 1
    print("unfinished CS flow: " + str(unfinished_count))
    if count != 0:
        return total_time / count
    else:
        return 0


def process_deadline(fid_begin, fid_end):
    missed_count = 0
    total_count = 0
    for fid, info in flow_info.items():
        if fid_begin <= fid < fid_end and fid in flow_result and info[1] + info[2] < STOP_TIME:
            result = flow_result[fid]
            total_count += 1
            delta = result[2] - result[1]
            deadline = info[2] + DEADLINE_DELTA
            if not (info[0] == result[0] and delta < deadline):
                missed_count += 1
    if total_count != 0:
        return (missed_count + 0.0) / total_count
    else:
        return 0


if __name__ == '__main__':
    path = "result/c3p/cs-ds"
    for i in range(1, 10):
        parse_flow_info(path + "/flow-info-0." + str(i) + ".txt")
        parse_flow_result(path + "/flow-result-0." + str(i) + ".txt")
        print(str(i) + ", " + str(process_deadline(100000, 200000)))
        print(str(i) + ", " + str(process_afct(200000, 300000)))
        flow_info.clear()
        flow_result.clear()
        # print("Deadline Missed Ratio for DS flows: " + str(process_deadline(10000, 20000)))
        # print("AFCT for CS flows: " + str(process_afct(20000, 30000)))
