"""Generate per-segment narration using edge-tts (US English neural voice)."""
import asyncio
import json
import edge_tts
from mutagen.mp3 import MP3

# en-US-GuyNeural = neutral male US voice (Silicon Valley style)
VOICE = "en-US-GuyNeural"
RATE = "+0%"  # natural pace

SEGMENTS = [
    {"id": "intro", "text": "Introducing eBrowser. The most secure, fastest, and most extensible lightweight browser."},
    {"id": "f1", "text": "Feature one. Military-Grade Sandboxing. Every tab runs in an isolated container with seccomp filters and namespace separation."},
    {"id": "f2", "text": "Feature two. Zero-Trust Firewall. Network-level filtering blocks malicious connections before they reach your browser."},
    {"id": "f3", "text": "Feature three. Anti-Fingerprinting Engine. Randomizes canvas, WebGL, and font metrics to prevent cross-site tracking."},
    {"id": "arch", "text": "Under the hood, eBrowser is built with C, WebKit, OpenSSL, and SQLite. The architecture flows from Tab Process, to Sandbox, to Firewall, to Render Engine, to Network Stack."},
    {"id": "cta", "text": "eBrowser. Open source and production ready. Visit github dot com slash embeddedos-org slash eBrowser."}
]


async def generate():
    durations = {}
    audio_files = []

    for seg in SEGMENTS:
        filename = f"seg_{seg['id']}.mp3"
        communicate = edge_tts.Communicate(seg["text"], VOICE, rate=RATE)
        await communicate.save(filename)
        dur = MP3(filename).info.length
        durations[seg["id"]] = round(dur + 0.5, 1)
        audio_files.append(filename)
        print(f"  {seg['id']}: {dur:.1f}s -> padded {durations[seg['id']]}s")

    with open("durations.json", "w") as f:
        json.dump(durations, f, indent=2)

    # Concatenate
    import subprocess
    with open("concat_list.txt", "w") as f:
        for af in audio_files:
            f.write(f"file '{af}'\n")

    subprocess.run([
        "ffmpeg", "-y", "-f", "concat", "-safe", "0",
        "-i", "concat_list.txt", "-c", "copy", "narration.mp3"
    ], check=True)

    total = sum(durations.values())
    print(f"\nVoice: {VOICE}")
    print(f"Total narration: {total:.1f}s")
    print(f"Durations: {json.dumps(durations)}")


asyncio.run(generate())
