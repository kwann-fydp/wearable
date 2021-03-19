import csv
import matplotlib.pyplot as plt
import math

import numpy as np
from sklearn.linear_model import LinearRegression

# Sample rate should match that of SpO2 sensor
sample_rate = 10

# Min and max sensor values are calculated for each batch to calculate AC and DC components.
# Batch size is later set as batch_multiple * sample rate; batch_multiple is thus how many seconds per batch.
# This effectively controls the resolution of AR ratio calculations (kind of like averaging).
#
# IMPORTANT: make sure a single batch can encapsulate the pulsatile phenomenon; ie the batch should cover an entire
# period of a heartbeat. For instance, a batch_multiple of 1 assumes a heartbeat lasts no more than 1 second, or a 
# minimum BPM of 60.
#
# In summary, the batch_multiple should be set as low as possible to maximize resolution without being lower than BPM/60.
batch_multiple = 1

# Data files; omits first line
sensor_fname = "sensor_readings_08.txt"
ref_fname = "spo2_reference_08.txt"

# Plot reference SpO2 measurements alongside calculated absorption ratios?
show_ref = True


red_vals = []
ir_vals = []

val_min = math.inf
val_max = -math.inf

with open(sensor_fname) as f:
  data = csv.reader(f, delimiter=',')
  for idx, line in enumerate(data):
    if idx > 0:
      red_vals.append(int(line[0]))
      ir_vals.append(int(line[1]))

      # val_min = min(val_min, min(line[0], line[1]))
      # val_max = max(val_max, max(line[0], line[1]))

# print(red_vals)
# print(ir_vals)

reference_vals = []

if show_ref:
  with open(ref_fname) as f:
    data = csv.reader(f)
    for idx, line in enumerate(data):
      if idx > 0:
        reference_vals.append(int(line[0]))


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

ir_min_line_pts = []
ir_max_line_pts = []

red_min_line_pts = []
red_max_line_pts = []

ar_values = []

batch_size = batch_multiple * sample_rate

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

  # Calculate absorption ratio for batch
  ar_val = (red_ac/red_dc)/(ir_ac/ir_dc)
  ar_total += ar_val
  batch_count += 1

  ar_values.append(ar_val)
  # Draw min & max lines for this batch
  for i in range(batch_size):
    ir_min_line_pts.append(ir_min)
    ir_max_line_pts.append(ir_max)
    red_min_line_pts.append(red_min)
    red_max_line_pts.append(red_max)
  
  # print(i, "- IR dc:", ir_dc, ", IR ac:", ir_ac, ", red dc:", red_dc, ", red ac:", red_ac)
  # print("-> AR:", ar_val)

# Calculate average absorption ratio over all batches
avg_AR_val = ar_total / batch_count

print("\nAVERAGE ABSORPTION RATIO:", avg_AR_val)

data_len = len(red_vals)

x = [s/sample_rate for s in range(data_len)]
fig1 = plt.figure()
plt.scatter(x, red_vals, color='b')
plt.scatter(x, ir_vals, color='r')

# Bounding lines
plt.scatter(x, ir_min_line_pts[:data_len], alpha=0.1, s=batch_size, color='b')
plt.scatter(x, ir_max_line_pts[:data_len], alpha=0.1, s=batch_size, color='g')
plt.scatter(x, red_min_line_pts[:data_len], alpha=0.1, s=batch_size, color='r')
plt.scatter(x, red_max_line_pts[:data_len], alpha=0.1, s=batch_size, color='g')

plt.title("Absorption values over time")
plt.legend(['Red LED', 'IR LED'])
plt.xlabel("Time (seconds)")
plt.ylabel("Sensor Reading")
plt.show()

seconds_range = len(ar_values) * batch_multiple

fig1 = plt.figure()
plt.scatter([x for x in range(0, seconds_range, batch_multiple)], ar_values)
plt.xlabel("Time (seconds)")
plt.ylabel("Absorption Ratio")

legend_labels = ['Measured Absorption Ratio']
plt_title = "Absorption Ratio (calculated every " + str(batch_size) + " samples)"

if show_ref:
  plt.scatter(
    [x for x in range(0, seconds_range, batch_multiple)],
    [y/10 for y in reference_vals[:math.floor(len(ar_values))]]
  )
  plt_title = "Reference SpO2 and Measured Absorption Ratio"
  legend_labels.append('Reference SpO2 / 10')

reg_x = np.array(range(len(ar_values)))
reg_x = reg_x.reshape(len(reg_x), 1)
reg_y = ar_values
reg = LinearRegression().fit(reg_x, reg_y)
print("Absorption ratio regression score:", reg.score(reg_x, reg_y))

pred_input = np.array([x for x in range(0, seconds_range, batch_multiple)])
pred_input = pred_input.reshape(len(pred_input), 1)
ref_predict = reg.predict(pred_input)
plt.scatter([x for x in range(0, seconds_range, batch_multiple)], ref_predict)

plt.title(plt_title)

legend_labels.append('Linear Regression Prediction')
plt.legend(legend_labels)

plt.show()
