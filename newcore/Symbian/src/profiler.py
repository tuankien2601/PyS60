# Copyright (c) 2008 Nokia Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# usage :profiler.py <logfile> <outputfile>
import sys
import linecache

sum = 0
output_file = open(sys.argv[2], 'w')
log_file = open(sys.argv[1], 'r')
current_line = log_file.readline()
t_time = current_line.split(':', 3)[2]
time_old = float(t_time)

while True:
    # Read lines from log file
    current_line = log_file.readline()
    if not current_line:   # If EOF
        break
    [src_file_name, line, t_time, memory] = current_line.split(':', 3)
    # Read lines from source file
    src_line = linecache.getline(src_file_name, int(line))
    if not src_line:    # If error opening or reading file
        print "Error reading", src_file_name, "file"
        continue
    src_line = src_line.strip()
    memory = memory.split('\n')[0]
    # Obtain the timestamp of current statement
    time_new = float(t_time)
    time_diff = time_new - time_old
    # The end time of previous statement becomes start time of current
    time_old = time_new
    sum += time_diff
    output_file.write('%-10s %10s %s %10s %f %10s %-15s %-50s\n'
                      % (src_file_name, "LINE:", line, "TIME:", time_diff,
                      "MEMORY:", memory, src_line))
# Print total time difference
output_file.write('\n%38s%f' % ("TOTAL TIME:", sum))
output_file.close()