# Native GStreamer playback architecture

Goals: native C++ video playback with HLS/DASH, DRM (Widevine/PlayReady) via content protection APIs, and adaptive bitrate (ABR) that starts quickly and stays resilient.

## High-level pipeline
Textual view of the steady-state graph:
```
HTTPS source (souphttpsrc/urisourcebin)
  -> Adaptive demux (gstadaptivedemux: dashdemux/hlsdemux)
  -> Decrypt (key-system specific decryptor, fed by CDM/license manager)
  -> Parser (mp4/isobmff/ts)
  -> Decoder (hw decode preferred, software fallback)
  -> Sink (video sink + audio sink, clocked A/V sync)
```

Notes:
- For a minimal demo, `playbin` or `playbin3` encapsulates the graph above.
- Production builds typically use `uridecodebin3` + custom bins to control ABR, DRM session lifecycle, metrics, and sink selection.
- Use hardware decode (`vaapi`, `v4l2`, platform decoders) when available; pick sinks that integrate with the device compositor / video plane.

## DRM integration (Widevine / PlayReady)
- Detection: adaptive demux emits `GST_MESSAGE_PROTECTION` / `GstProtectionEvent` with PSSH / init data.
- CDM binding: application or a dedicated element passes init data to the DRM agent (Widevine/PlayReady CDM, often via OpenCDM or platform SDK) to create a key session.
- License acquisition: CDM talks to the license server (over HTTPS, app-signed requests, nonce/clock handling). Key responses are fed back to the decryptor.
- Decryption path: encrypted buffers carry protection metadata; a decrypt element (CDM-backed) outputs clear samples to downstream decoders or feeds secure decoders directly. Keep frames in a secure path when the platform requires it (secure buffers, protected surfaces).
- Key lifecycle: handle renewal/rotation, persistent licenses (if allowed), and teardown on EOS or app exit.
- Error/reporting: surface DRM errors through the bus, include key-system, license status, and retries; expose metrics (time-to-license, renewals, failures).

## Adaptive Bitrate (ABR)
- Default: leverage `gstadaptivedemux` heuristics (throughput-based with buffer safety) for a fast baseline.
- Custom controller: subscribe to fragment download stats (duration/size), buffer level, dropped frames, and device capability to compute a target bitrate (e.g., EWMA throughput + buffer-based guard rails).
- Switching: request variant changes via stream selection APIs (e.g., `playbin3`/streams API) or, when using `gstadaptivedemux` directly, by setting properties such as `max-bitrate` / `min-bitrate` or selecting a `GstStream` from the current `GstStreamCollection`.
- Startup: pick a conservative initial bitrate (mid/low ladder) to minimize time-to-first-frame, then step up quickly if bandwidth allows.
- Stability: avoid oscillation with hysteresis, minimum stay duration, and a cap on consecutive switches; favor down-switches on buffer pressure before the buffer runs dry.

## Performance considerations
- Time-to-first-frame: pre-roll to `PAUSED`, then `PLAYING`; start at a lower bitrate; enable HTTP keep-alive, DNS cache, and small initial buffering thresholds; defer heavy logging until after startup.
- Buffering: tune `queue2`/`download` buffering, separate A/V queues with sensible watermarks; cap max buffered data on memory-constrained devices.
- Decode/render: prefer hardware decoders and video sinks that support zero-copy; align video sink caps with display mode to reduce conversions; set `sync=true` for smooth A/V sync.
- Error recovery: retries on HTTP errors with backoff, fast down-switch on congestion, and resilient seek/segment gap handling (common in live HLS/DASH).
- Power: pause rendering on background, drop frames when hidden, and pick efficient sinks on embedded targets.

## Debugging and logging
- Bus handling: listen for `ERROR`, `WARNING`, `STATE_CHANGED`, `BUFFERING`, `STREAM_COLLECTION`, and DRM-related protection messages; include element names and stream ids in logs.
- Verbosity control: use `GST_DEBUG` categories (e.g., `playbin:3,adaptivedemux:3,drm:4`); emit structured logs that can be forwarded to the host app.
- Pipeline introspection: `GST_DEBUG_DUMP_DOT_DIR` for graph dumps; pad/block probes to inspect data; enable QOS events to spot underruns/overruns.
- Field diagnostics: expose a lightweight telemetry surface (startup time, first-frame time, rebuffer counts, ABR switches, license latencies) and a way to attach focused `GST_DEBUG` without rebuilding.

## Minimal sample (this repo)
- `main.cpp` demonstrates a compact `playbin` setup with bus callbacks for errors, EOS, and state changes. It uses a public HLS URL by default but accepts any URI.
- Build via CMake or a one-line `g++` compile (see `README.md`).
