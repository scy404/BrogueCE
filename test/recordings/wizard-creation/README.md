# Wizard creation recording

`creation-and-resume.broguerec` uses Brogue CE 1.15.1, wizard mode, seed 971.
It contains gold creation, a +5 identified speed-runic dagger, a mutated allied
rat, canceled acid mound placement, two turns of waiting, and a save/load
transition. After resuming it creates more gold and a rat, waits four turns,
and quits. It uses the extended `WCE1.15.1` recording header.

The fixture was generated through real wizard menus driven by a scripted console,
the engine's save/load functions, and its normal quit/recording-save path. It is
an engine integration fixture, not evidence of manual graphical testing.

From the repository root, with runtime libraries available:

```sh
python test/test_wizard_recordings.py ./brogue
```

On Windows, pass `bin/brogue.exe` instead. The test plays the complete recording
and physically truncates every byte position within the first item and monster
payloads while preserving the original header. Complete playback must exit 0;
each truncated file must exit 2 (out of sync), rather than applying stale buffer
contents or hanging. Each child has a 15-second timeout.

The full fixture can also be played with the existing `run_regression_tests.py`
runner. Regenerate recordings when engine recording compatibility changes.
