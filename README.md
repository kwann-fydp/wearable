### Architect

- Assume functions like `checkBatt()`, `readButton()`, `writeLED()`, etc. are available

#### Existing SpO2 reading procedure
0. Fill buffer with IR and Red LED sensor readings & calculate SpO2
1. Overwrite one-quarter of buffer with new sensor readings ('heads-down' loop - CPU dedicated to polling sensor)
2. Calculate SpO2 from updated readings ('heads-up' time - CPU available for checks & updates)
3. Repeat 1 and 2 indefinitely

#### Functions to think about
- Configuration vs Sensing mode - if config button pressed, don't do SpO2 reading procedure, instead do BT connection stuff
- Off/on state - how to register 'turn off/on' command in code, what happens when dormant?
- Checking & displaying battery life while in use
- Where to implement classification logic
- Handling alert button - interrupt?
- Alert dispatch system (WiFi.h - what to do after alert dispatched?)
