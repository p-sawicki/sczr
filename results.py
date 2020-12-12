import sys

log_capture = open("/tmp/log-capture", "r")
log_filter = open("/tmp/log-filter", "r")
log_playback = open("/tmp/log-playback", "r")

capture_times = {}
filter_times = {}
playback_times = {}

for line in log_capture.readlines():
    values = line.split(" ")
    capture_times[values[0]] = float(values[1])
for line in log_filter.readlines():
    values = line.split(" ")
    filter_times[values[0]] = float(values[1])
for line in log_playback.readlines():
    values = line.split(" ")
    playback_times[values[0]] = float(values[1])

time_to_filter = 0.0
time_to_playback = 0.0
time_total = 0.0

for key in capture_times.keys():
    time_to_filter += filter_times[key] - capture_times[key]
for key in filter_times.keys():
    time_to_playback += playback_times[key] - filter_times[key]
time_to_filter /= len(capture_times.keys())
time_to_playback /= len(filter_times.keys())
time_total = time_to_filter + time_to_playback

results = open("results.csv", "a")
results.write(f"{sys.argv[1]},{time_to_filter},{time_to_playback},{time_total}\n")