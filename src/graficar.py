import soundfile as sf
import matplotlib.pyplot as plt
import numpy as np


def calcular_rms_por_ventana(archivo_wav, frame_size=1024):
    samples, samplerate = sf.read(archivo_wav)
    rms_values = []

    for i in range(0, len(samples), frame_size):
        frame = samples[i:i+frame_size]
        rms = np.sqrt(np.mean(frame**2)) if len(frame) > 0 else 0
        rms_values.append(rms)
    
    return rms_values


# Carga de archivos
data_original, sr_original = sf.read("pav_4150.wav")
data_cancelado, sr_cancelado = sf.read("resultado_cancelado_con_silencios.wav")

# Calcular RMS para ambas señales
rms_original = calcular_rms_por_ventana("pav_4150.wav", frame_size=1024)
rms_cancelado = calcular_rms_por_ventana("resultado_cancelado_con_silencios.wav", frame_size=1024)

plt.figure(figsize=(14, 12))

# Plot señal original
plt.subplot(4, 1, 1)
plt.plot(data_original, color='blue')
plt.title("Señal Original")
plt.xlabel("Muestra")
plt.ylabel("Amplitud")

# Plot señal cancelada
plt.subplot(4, 1, 2)
plt.plot(data_cancelado, color='orange')
plt.title("Señal Cancelada")
plt.xlabel("Muestra")
plt.ylabel("Amplitud")

# Plot RMS señal original
plt.subplot(4, 1, 3)
plt.plot(rms_original, color='blue')
plt.title("Amplitud RMS por ventana - Señal Original")
plt.xlabel("Ventana (frame)")
plt.ylabel("Amplitud RMS")
plt.grid(True)

# Plot RMS señal cancelada
plt.subplot(4, 1, 4)
plt.plot(rms_cancelado, color='orange')
plt.title("Amplitud RMS por ventana - Señal Cancelada")
plt.xlabel("Ventana (frame)")
plt.ylabel("Amplitud RMS")
plt.grid(True)

plt.tight_layout()
plt.show()
