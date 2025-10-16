import sounddevice as sd
import numpy as np
from scipy.signal import butter, sosfilt, sosfilt_zi
import matplotlib.pyplot as plt
from collections import deque
import time

# ---------- Config ----------
SR = 44100
BLOCK = 2048            # samples por bloque (~46 ms)
CHANNEL = 0             # índice del canal del dispositivo
BAND = (800, 2500)      # banda de interés en Hz (ej: "metallic" ejemplo)
ORDER = 4
THRESH = 0.01           # umbral RMS (ajustar)
HOLD = 0.15             # segundos para mantener evento
PLOT_SECONDS = 10

# ---------- diseñar filtro (sos) ----------
def design_bandpass(low, high, fs, order=4):
    nyq = 0.5*fs
    lown = low/nyq
    highn = high/nyq
    sos = butter(order, [lown:=lown, highn], btype='band', output='sos')
    return sos

sos = design_bandpass(BAND[0], BAND[1], SR, ORDER)
zi = sosfilt_zi(sos) * 0.0  # inicializar estado en cero

# ---------- buffers para graficar ----------
buf_len = int(PLOT_SECONDS * SR)
buf = deque(maxlen=buf_len)
time_buf = deque(maxlen=buf_len)
rms_buf = deque(maxlen=int(PLOT_SECONDS*1000/50))  # para RMS por frame

last_event_time = 0

# ---------- callback del stream ----------
def callback(indata, frames, time_info, status):
    global zi, last_event_time
    if status:
        print("Status:", status)
    x = indata[:, CHANNEL]
    # filtrar (mantener estado zi)
    y, zi = sosfilt(sos, x, zi=zi)
    # energía (RMS)
    rms = np.sqrt(np.mean(y**2))
    tnow = time.time()
    # detección simple con hold
    if rms > THRESH and (tnow - last_event_time) > 0.05:  # debounce
        last_event_time = tnow
        print(f"Evento en banda {BAND} - RMS={rms:.5f} @ {tnow:.3f}s")

    # rellenar buffers
    for v in x:
        buf.append(v)
        time_buf.append( (len(time_buf)+1) / SR )
    rms_buf.append(rms)

# ---------- iniciar stream ----------
device_info = sd.query_devices(kind='input')
print("Device default input:", device_info['name'])
stream = sd.InputStream(channels=1, callback=callback, blocksize=BLOCK, samplerate=SR)
stream.start()
print("Stream iniciado, escucha... (CTRL+C para parar)")

# ---------- plotting live simple  ----------
try:
    import matplotlib.animation as animation
    fig, (ax1, ax2) = plt.subplots(2,1, figsize=(10,6))
    ax1.set_title("Señal cruda (últimos {} s)".format(PLOT_SECONDS))
    ax2.set_title("RMS de la banda (por bloque)")
    ln1, = ax1.plot([], [])
    ln2, = ax2.plot([], [])
    def update_plot(frame):
        if len(buf) == 0:
            return ln1, ln2
        arr = np.array(buf)
        tlen = len(arr)/SR
        xaxis = np.linspace(-tlen, 0, len(arr))
        ln1.set_data(xaxis, arr)
        ax1.relim(); ax1.autoscale_view()
        ln2.set_data(np.arange(len(rms_buf)), np.array(rms_buf))
        ax2.relim(); ax2.autoscale_view()
        return ln1, ln2
    ani = animation.FuncAnimation(fig, update_plot, interval=200)
    plt.show()
except KeyboardInterrupt:
    pass
finally:
    stream.stop()
    stream.close()
    print("Terminado")
