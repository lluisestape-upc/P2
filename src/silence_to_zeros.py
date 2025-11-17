import numpy as np
import soundfile as sf
import os

# === INPUTS ===
vad_file = "pav_4150.vad"
audio_file = "resultado_cancelado.wav"
output_file = "resultado_cancelado_con_silencios.wav"

# === 1. Leer audio ===
audio, fs = sf.read(audio_file)
num_samples = audio.shape[0]

# === 2. Leer archivo VAD robusto ===
segments = []
with open(vad_file, "r") as f:
    for lineno, line in enumerate(f, start=1):
        line = line.strip()
        if not line:
            continue
        
        p = line.split()
        if len(p) < 3:
            print(f"Linea {lineno}: IGNORADA (formato incorrecto) → '{line}'")
            continue
        
        try:
            start = float(p[0])
            end = float(p[1])
            label = p[2].strip().upper()
        except:
            print(f"Linea {lineno}: ERROR parseando → '{line}'")
            continue
        
        # Si vienen al revés: corregir
        if start > end:
            start, end = end, start
        
        # Validación
        if end <= start:
            print(f"Linea {lineno}: segmento cero → {start} {end}")
            continue
        
        segments.append((start, end, label))

print(f"\nVAD cargado: {len(segments)} segmentos\n")

# === 3. Aplicar silencios ===
audio_muted = audio.copy()
silenced = 0

for i, (start, end, label) in enumerate(segments):
    if label != "S":
        continue
    
    # Tiempos a muestras
    start_idx = int(np.floor(start * fs))
    end_idx = int(np.ceil(end * fs))

    print(f"[SEG {i}] S   t={start:.5f}→{end:.5f}   idx={start_idx}→{end_idx}   (num_samples={num_samples})")
    
    # Clampear siempre
    if start_idx < 0: start_idx = 0
    if start_idx > num_samples: start_idx = num_samples
    if end_idx < 0: end_idx = 0
    if end_idx > num_samples: end_idx = num_samples

    # --- FIX DEFINITIVO ---
    # Si start_idx == end_idx, meter al menos 1 muestra
    if start_idx == end_idx:
        print("  >> FIX aplicado: slice vacío, extensión forzada")
        if end_idx < num_samples:
            end_idx += 1
        else:
            start_idx = max(0, start_idx - 1)

    # Aplicar mute
    if audio.ndim == 1:
        audio_muted[start_idx:end_idx] = 0
    else:
        audio_muted[start_idx:end_idx, :] = 0

    silenced += 1

print(f"\nSilencios aplicados: {silenced}")

# === 4. Guardar ===
sf.write(output_file, audio_muted, fs)
print("\nArchivo generado:", os.path.abspath(output_file))
