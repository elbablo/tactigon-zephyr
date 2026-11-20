"""Transcribe a WhatsApp voice note (.ogg/opus) with Cactus Whistle.

Usage: python transcribe.py audio.ogg [-l it] [-w whistle.cact]
Requires ffmpeg in PATH. Writes the transcript next to the audio as .txt.
"""
import argparse
import os
import subprocess
import sys
import time

import numpy as np

import cactus

SAMPLE_RATE = 16000


def ogg_to_wav(src: str, dst: str) -> None:
    """Convert any ffmpeg-readable audio to 16 kHz mono 16-bit WAV."""
    cmd = ["ffmpeg", "-y", "-loglevel", "error", "-i", src,
           "-ar", "16000", "-ac", "1", "-c:a", "pcm_s16le", dst]
    try:
        subprocess.run(cmd, check=True, capture_output=True, text=True)
    except FileNotFoundError:
        sys.exit("ffmpeg not found: install it (e.g. sudo apt install ffmpeg)")
    except subprocess.CalledProcessError as e:
        sys.exit(f"ffmpeg failed: {e.stderr.strip()}")


def load_pcm(src: str) -> np.ndarray:
    """Decode any ffmpeg-readable audio to 16 kHz mono float32 in [-1, 1]."""
    cmd = ["ffmpeg", "-loglevel", "error", "-i", src,
           "-ar", "16000", "-ac", "1", "-f", "s16le", "-"]
    try:
        out = subprocess.run(cmd, check=True, capture_output=True).stdout
    except FileNotFoundError:
        sys.exit("ffmpeg not found: install it (e.g. sudo apt install ffmpeg)")
    except subprocess.CalledProcessError as e:
        sys.exit(f"ffmpeg failed: {e.stderr.decode(errors='replace').strip()}")
    return np.frombuffer(out, dtype=np.int16).astype(np.float32) / 32768.0


def transcribe_ogg(path: str, language: str | None = None, weights: str | None = None) -> str:
    """Stream the audio through Whistle in 1 s chunks, printing text and progress as it goes."""
    print(f"Decoding {path}...", flush=True)
    pcm = load_pcm(path)
    total = len(pcm)
    print(f"{total / SAMPLE_RATE / 60:.1f} min of audio, loading model...", flush=True)

    parts: list[str] = []
    done = 0
    start = time.monotonic()

    def chunks():
        nonlocal done
        for i in range(0, total, SAMPLE_RATE):
            done = min(i + SAMPLE_RATE, total)
            yield pcm[i:done]

    for result in cactus.stream(chunks(), language=language, weights=weights):
        text = result.get("text", "").strip()
        if text:
            parts.append(text)
            print(text, flush=True)
        print(f"\r[{done / total:6.1%}] {time.monotonic() - start:.0f}s", end="", file=sys.stderr, flush=True)
    print(file=sys.stderr)
    return " ".join(parts)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("audio", help="WhatsApp voice note (.ogg)")
    parser.add_argument("-l", "--language", choices=cactus.agent.whistle.LANGUAGES,
                        help="force language (default: auto-detect)")
    parser.add_argument("-w", "--weights", help="path to whistle .cact weights (default: downloaded base)")
    parser.add_argument("-t", "--timestamps", action="store_true", help="print word timestamps")
    args = parser.parse_args()

    if not os.path.isfile(args.audio):
        sys.exit(f"file not found: {args.audio}")

    text = transcribe_ogg(args.audio, args.language, args.weights)
    out = os.path.splitext(args.audio)[0] + ".txt"
    with open(out, "w", encoding="utf-8") as f:
        f.write(text + "\n")
    print(f"\nSaved to {out}")
