import csv
import matplotlib.pyplot as plt
import math

sample_rate = 10
fname = "sensor_readings_02.txt"

red_vals = []
ir_vals = []

val_min = math.inf
val_max = -math.inf

with open(fname) as f:
  data = csv.reader(f, delimiter=',')
  for idx, line in enumerate(data):
    if idx > 0:
      red_vals.append(int(line[0]))
      ir_vals.append(int(line[1]))

      # val_min = min(val_min, min(line[0], line[1]))
      # val_max = max(val_max, max(line[0], line[1]))

# print(red_vals)
# print(ir_vals)



# Moving SpO2 calculation window;
# Want to capture minimum number of complete heartbeats to get fine but accurate reading
# Assume hearbeats are spaced by at most 2 seconds (30 bpm). That is, every 2 * sampleRate samples.

# Algorithm:
# (i) Segment IR and red batches of 2*sample_rate samples
# (ii) Traverse the batches, recording max and min as well as average (total / batch length)
# (iii) Calculate the AC components of the signals by taking the difference btwn max and min vals
# (iv) Calculate the DC components of the signals by subtracting half of the AC value from the average
# (v) Calculate SpO2 using the AC and DC components

ar_total = 0
batch_count = 0

batch_size = 2 * sample_rate

for i in range(0, len(ir_vals), batch_size):
  j = i
  ir_tot = 0
  red_tot = 0
  ir_max = -math.inf
  ir_min = math.inf
  red_max = -math.inf
  red_min = math.inf

  # Traverse batch
  for j in range(i,i+batch_size):
    if j >= len(ir_vals): break

    ir_val = ir_vals[j]
    red_val = red_vals[j]

    ir_tot += ir_val
    red_tot += red_val

    if ir_val > ir_max:
      ir_max = ir_val
    if ir_val < ir_min:
      ir_min = ir_val

    if red_val > red_max:
      red_max = red_val
    if red_val < red_min:
      red_min = red_val
  
  # Take batch averages
  ir_avg = ir_tot / batch_size
  red_avg = red_tot / batch_size

  # Calculate AC
  ir_ac = ir_max - ir_min
  red_ac = red_max - red_min

  # Calculate DC
  ir_dc = ir_avg - ir_ac/2
  red_dc = red_avg - red_ac/2

  ar_val = (red_ac/red_dc)/(ir_ac/ir_dc)
  ar_total += ar_val
  batch_count += 1

  # print(i, "- IR dc:", ir_dc, ", IR ac:", ir_ac, ", red dc:", red_dc, ", red ac:", red_ac)
  # print("-> AR:", ar_val)

avg_AR_val = ar_total / batch_count

print("\nAVERAGE ABSORPTION RATIO:", avg_AR_val)

x = [s/sample_rate for s in range(len(red_vals))]
fig = plt.figure()
plt.scatter(x, red_vals, color='b')
plt.scatter(x, ir_vals, color='r')
plt.legend(['Red Values', 'IR Values'])
plt.xlabel("Time (seconds)")
plt.ylabel("Sensor Reading")
plt.show()

