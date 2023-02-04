# MIDIMasterClock
Modified code for Arduino Nano based MIDI clock by EJLabs

This is a modified version of the MIDI Master Sync code from EJLabs (https://ejlabs.net/arduino-midi-master-clock/)

List of changes:
- Added 3rd CV output using the same timing as 2nd CV
- Display both BPM value and 2nd/3rd CV PPQN values at all times
- Show PPQN as absolute value instead of relative value
- Increase/decrease BPM by 10 when controller is turned quickly

See Reddit post for more details about this project:
https://www.reddit.com/r/synthesizers/comments/sris9h/diy_project_midi_master_clock_arduino/
