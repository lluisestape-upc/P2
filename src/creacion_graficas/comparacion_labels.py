import argparse
import numpy as np
import soundfile as sf
import matplotlib.pyplot as plt


def load_segments(path):
    """
    Load segments from a .lab/.vad file: start end label
    """
    segments = []
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) < 3:
                continue
            start = float(parts[0])
            end = float(parts[1])
            label = parts[2]
            segments.append((start, end, label))
    return segments


def plot_wave_with_segments(wav_path, lab_path, vad_path, out_path):
    # Load audio
    audio, sr = sf.read(wav_path)
    if audio.ndim > 1:
        audio = audio.mean(axis=1)

    n_samples = len(audio)
    time = np.arange(n_samples) / sr

    # Load segments
    lab_segments = load_segments(lab_path)
    vad_segments = load_segments(vad_path)

    # Plot
    fig, ax = plt.subplots(figsize=(12, 4))
    ax.plot(time, audio, linewidth=0.5, color="black")

    # Function to draw timestamp markers + labels
    def draw_segments(segments, color):
        for start, end, label in segments:
            # Draw the start and end lines
            ax.axvline(start, color=color, alpha=0.7, linewidth=1)
            ax.axvline(end,   color=color, alpha=0.7, linewidth=1)

            # Put label near the start time
            y_max = np.max(audio)
            y_min = np.min(audio)
            y_pos = y_max - 0.05 * (y_max - y_min)   # a bit below top
            ax.text(
                start + 0.07,  # slightly right so text doesn't overlap the line
                y_pos,
                label,
                color=color,
                fontsize=8,
                ha="right",
                va="top",
            )

    # Draw .lab (green) and .vad (red)
    draw_segments(lab_segments, "green")
    draw_segments(vad_segments, "red")

    # Legend
    from matplotlib.lines import Line2D
    custom_lines = [
        Line2D([0], [0], color="black", lw=1, label="Waveform"),
        Line2D([0], [0], color="red", lw=1, label=".vad segments"),
        Line2D([0], [0], color="green", lw=1, label=".lab segments"),
    ]
    ax.legend(handles=custom_lines, loc="upper right")

    ax.set_xlabel("Time [s]")
    ax.set_ylabel("Amplitude")
    ax.set_title("Waveform with .lab (green) and .vad (red) segment boundaries")
    ax.grid(True, alpha=0.2)

    fig.tight_layout()
    fig.savefig(out_path, dpi=200)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(
        description="Plot waveform with .lab (red) and .vad (green) timestamps."
    )
    parser.add_argument("--wav", help="Input .wav file", default="/home/bsc/bsc739251/t2tt_formatting/PAV/pav_4150.wav")
    parser.add_argument("--lab", help="Input .lab file with timestamps", default="/home/bsc/bsc739251/t2tt_formatting/PAV/pav_4150.lab")
    parser.add_argument("--vad", help="Input .vad file with timestamps", default="/home/bsc/bsc739251/t2tt_formatting/PAV/pav_4150.vad")
    parser.add_argument("--out", help="Output image path (e.g. plot.png)", default="timestamps_comparison.png")

    args = parser.parse_args()
    plot_wave_with_segments(args.wav, args.lab, args.vad, args.out)


if __name__ == "__main__":
    main()
