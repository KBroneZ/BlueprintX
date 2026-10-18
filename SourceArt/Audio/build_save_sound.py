"""Generate the Blueprint-X save sound (S_BXSave.wav) from scratch.

Run (Python 3, standard library only):
    python build_save_sound.py

Writes S_BXSave.wav next to this script: a short, soft rising chime (three notes, G5 C6 E6),
48 kHz mono 16-bit. Every sample is computed here, so the sound is our own work and ships
under the mod's licence (no third-party clip, decision D-007). The editor import is done by
scripts/ue/import_save_sound.py in the project folder.
"""
import math
import os
import struct
import wave

HERE = os.path.dirname(os.path.abspath(__file__))
WAV_PATH = os.path.join(HERE, "S_BXSave.wav")

SAMPLE_RATE = 48000
DURATION = 0.9          # seconds
PEAK = 0.45             # peak level (about -7 dBFS), so it never sounds louder than game UI
ATTACK = 0.006          # seconds, avoids a click at each note start
DECAY = 0.22            # seconds, exponential decay time constant
FADE_OUT = 0.05         # seconds, fade at the very end

# (start time in seconds, frequency in Hz): a major arpeggio reads as "done / success".
NOTES = ((0.00, 783.99), (0.085, 1046.50), (0.17, 1318.51))
# Relative strength of the harmonics: mostly a sine, a little bell-like brightness.
HARMONICS = ((1, 1.0), (2, 0.28), (3, 0.08))


def note_sample(t, frequency):
    if t < 0.0:
        return 0.0
    envelope = min(1.0, t / ATTACK) * math.exp(-t / DECAY)
    tone = sum(weight * math.sin(2.0 * math.pi * frequency * n * t) for n, weight in HARMONICS)
    return envelope * tone


def build_samples():
    count = int(SAMPLE_RATE * DURATION)
    raw = []
    for i in range(count):
        t = i / SAMPLE_RATE
        value = sum(note_sample(t - start, frequency) for start, frequency in NOTES)
        remaining = DURATION - t
        if remaining < FADE_OUT:
            value *= remaining / FADE_OUT
        raw.append(value)
    loudest = max(abs(v) for v in raw)
    return [v * PEAK / loudest for v in raw]


def main():
    samples = build_samples()
    with wave.open(WAV_PATH, "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(b"".join(struct.pack("<h", int(round(v * 32767))) for v in samples))
    print(f"BXSOUND samples={len(samples)} seconds={DURATION} path={WAV_PATH}")


main()
