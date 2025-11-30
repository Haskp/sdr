import numpy as np
import librosa
from pydub import AudioSegment

def mp3_to_pcm(pcm_file, mp3_file):

    y, sr = librosa.load(mp3_file, sr=44100, mono=True)
    pcm_data = (y * 32767).astype(np.int16)
    pcm_data.tofile(pcm_file)

mp3_to_pcm("/home/plutoSDR/Рабочий стол/SDRlabs/dev/tx_audio.pcm", "Smeshariki.mp3")

