"""Generate narration audio using Google Text-to-Speech."""
from gtts import gTTS

NARRATION = (
    "Introducing eBrowser. The most secure, fastest, and most extensible lightweight browser. Feature one: Military-grade sandboxing isolates every tab in its own secure container. Feature two: Zero-trust firewall blocks malicious connections before they reach your system. Feature three: Anti-fingerprinting engine prevents websites from tracking your digital identity. eBrowser. Open source and built for privacy. Visit github dot com slash embeddedos-org slash eBrowser."
)

tts = gTTS(text=NARRATION, lang="en", slow=False)
tts.save("narration.mp3")
print(f"Generated narration.mp3 ({len(NARRATION)} chars)")
