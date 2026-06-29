# DinLooper

DinLooper is a VST3 live looping plugin built for fast, simple operation during
a performance.

It lets you record loops and layered overdubs without navigating complicated
screens. DinLooper is designed primarily for REAPER and can be controlled with
a mouse, MIDI, OSC, a sustain pedal, or a Stream Deck.

## Features

- Stereo loops up to 3 minutes long.
- Up to 16 active layers.
- Independent volume, meter, Mute, Solo, and Delete controls for every layer.
- Undo and Redo for recorded and deleted layers.
- Instant, audio, MIDI, or Audio + MIDI recording triggers.
- Adjustable audio threshold directly on the input meter.
- Waveform display with playback position and loop duration.
- Non-destructive waveform cropping using draggable start and end handles.
- Stereo input and output meters.
- Master volume and optional Audio Thru.
- Configurable REC timing compensation.
- Resizable interface.
- VST3 parameters for REAPER Learn, OSC, MIDI, automation, and Stream Deck
  workflows.

## Quick start

1. Insert DinLooper on a REAPER track.
2. Choose a trigger mode.
3. Press **REC**.
4. Play or sing.
5. Press **FINISH** to close the recording.
6. The loop starts playing automatically.
7. Press **REC** again to record an overdub.

Every overdub becomes a separate layer with its own controls.

## Trigger modes

The **Trigger** menu controls when recording begins:

- **Instant:** recording starts as soon as REC is pressed.
- **Audio + MIDI:** recording starts with whichever valid signal arrives first.
- **Audio Only:** waits until the input level crosses the threshold.
- **MIDI Only:** waits for a Note On message.

Drag the blue line on the input meter to adjust the audio threshold.

## Main controls

- **REC:** arms or starts recording. While recording, it changes to **FINISH**
  and closes the current loop or overdub.
- **REC PEDAL:** works like REC, but also waits for a sustain pedal event to
  finish. While waiting, REC or REC PEDAL can still finish the recording
  manually.
- **PLAY:** resumes a stopped loop.
- **STOP:** saves the current recording, when applicable, and stops playback.
- **CANCEL:** discards only the recording currently in progress. Existing
  layers keep playing.
- **REWIND:** returns to the beginning of the loop.
- **UNDO / REDO:** undo or restore layer operations.
- **RESET:** clears the current session and resets the looper.

## Waveform and loop trimming

The waveform shows the original loop, the current playback position, and the
recording state.

Drag the handles at either end of the waveform to adjust the active loop range.
The edit is non-destructive: audio outside the selected area is kept and can be
restored by moving the handles again.

**REC COMP** can automatically trim a small amount from the end when the first
loop is finished with REC. This helps compensate for control and monitoring
delay. Set it to `0 ms` to disable the compensation.

## Layers

Each recording receives its own number and appears in the central mixer. Every
layer provides:

- an audio level meter;
- volume control;
- Mute;
- Solo;
- a Delete button.

Deleted slots are reused safely. The plugin supports up to 16 active layers,
while layer numbering can continue to increase during a session.

## Audio Thru

**Audio Thru is off by default.** In this mode, DinLooper outputs only recorded
loops, preventing the input signal from being doubled when REAPER is already
monitoring it.

Enable Audio Thru if you want DinLooper to include the original input in its
output.

## REAPER, MIDI, and OSC

The main commands and the volume, Mute, and Solo controls for all 16 layer slots
are exposed as VST3 parameters. They can be controlled with:

- REAPER Learn;
- MIDI;
- OSC;
- foot controllers;
- Stream Deck;
- REAPER automation.

## Windows installation

1. Download the latest Windows x64 ZIP from
   [Releases](https://github.com/randomicidio/DinLooper/releases).
2. Extract the `DinLooper.vst3` folder.
3. Copy it to `C:\Program Files\Common Files\VST3`, or another VST3 folder
   configured in REAPER.
4. Rescan plugins in REAPER if needed.

## Current limitations

DinLooper is still in active development.

- Recorded audio is not restored after closing and reopening a project.
- Parameter values are saved, but recorded loops are not yet stored in the
  project session.
- BPM sync, quantization, audio import, and export are planned for future
  versions.

Test your complete computer, audio interface, controller, and REAPER setup
before using the plugin in an important live performance.

## Latest version

[DinLooper v1.0.0 Alpha 4](https://github.com/randomicidio/DinLooper/releases/tag/v1.0.0-alpha.4)

## Development

DinLooper is an open-source project written in C++ with JUCE. The source code,
change history, and published builds are available in this repository.
