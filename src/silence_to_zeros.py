import numpy as np
import soundfile as sf
import os

# === INPUTS ===
vad_file = "pav_4150.vad"
audio_file = "resultado_cancelado.wav"
output_file = "resultado_cancelado_con_silencios.wav"

# === 1. Leer audio ===
audio, fs = sf.read(audio_file)   # audio puede ser mono o (N,2)
num_samples = audio.shape[0]

# === 2. Leer archivo VAD (más robusto) ===
segments = []
with open(vad_file, "r") as f:
    for lineno, line in enumerate(f, start=1):
        s = line.strip()
        if not s:
            continue
        parts = s.split()
        if len(parts) < 3:
            # ignorar líneas mal formadas
            print(f"Linea {lineno}: formato inesperado -> '{line.strip()}', se ignora")
            continue
        try:
            start = float(parts[0])
            end = float(parts[1])
            label = parts[2].upper()
        except ValueError:
            print(f"Linea {lineno}: no se pueden convertir tiempos -> '{line.strip()}', se ignora")
            continue

        # Si start > end, invertirlos (corrección común)
        if start > end:
            start, end = end, start

        # recortar a límites validos
        start = max(0.0, start)
        end = max(0.0, end)

        # ignorar segmentos nulos
        if end <= start:
            print(f"Linea {lineno}: segmento vacío tras validación -> start={start}, end={end}, se ignora")
            continue

        segments.append((start, end, label))

if not segments:
    print("No se han leído segmentos válidos desde el .vad. Revisa el fichero.")
else:
    print(f"Leídos {len(segments)} segmentos desde '{vad_file}'")

# === 3. Aplicar silencios ===
audio_muted = np.copy(audio)

silenced_count = 0
for (start, end, label) in segments:
    if label == "S":   # silenciar segmentos marcados como silencio
        # convertir a índices de muestra (usar round para mayor exactitud)
        start_idx = int(round(start * fs))
        end_idx = int(round(end * fs))

        # proteger rangos válidos
        start_idx = max(0, min(start_idx, num_samples))
        end_idx = max(0, min(end_idx, num_samples))

        if end_idx <= start_idx:
            # si tras ajuste quedan mal, lo informamos y saltamos
            print(f"Ignorado segmento S con índices inválidos: start_idx={start_idx}, end_idx={end_idx}")
            continue

        # aplicar cero (funciona para mono y estéreo)
        if audio.ndim == 1:
            audio_muted[start_idx:end_idx] = 0.0
        else:
            audio_muted[start_idx:end_idx, :] = 0.0

        silenced_count += 1
        print(f"Silenciado: {start:.5f}s -> {end:.5f}s  => samples {start_idx}:{end_idx}")

print(f"Segmentos silenciados aplicados: {silenced_count}")

# === 4. Guardar resultado ===
# Si el fichero existe, lo sobrescribimos
sf.write(output_file, audio_muted, fs)
print("Archivo generado:", os.path.abspath(output_file))
