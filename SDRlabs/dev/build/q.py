import numpy as np
import matplotlib.pyplot as plt

# Чтение PCM файла
def read_pcm_file(filename):
    with open(filename, 'rb') as f:
        data = np.fromfile(f, dtype=np.int16)
    # Разделение на I и Q компоненты
    I = data[0::2]  # Четные элементы - I
    Q = data[1::2]  # Нечетные элементы - Q
    return I, Q

# Чтение данных
I_tx, Q_tx = read_pcm_file('tx_signal.pcm')
I_rx, Q_rx = read_pcm_file('rx_signal.pcm')

# Построение графиков
plt.figure(figsize=(12, 8))

plt.subplot(2, 2, 1)
plt.plot(I_tx, 'b-')
plt.plot(Q_tx, 'r-')
plt.title('Transmitted I Component')
plt.grid(True)

plt.subplot(2, 2, 2)
plt.plot(I_rx, 'b-')
plt.plot(Q_rx, 'r-')
plt.title('Transmitted I Component')
plt.grid(True)


plt.tight_layout()
plt.show()
