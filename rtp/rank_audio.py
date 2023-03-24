from scipy.io import wavfile
import sys
sys.path.append("../") 
import pysepm
import numpy as np

fs, original = wavfile.read('/home/kyle/nasa/dev/test_media/official_test_media/ammonia_trimmed.wav')
fs,  transmitted = wavfile.read('/home/kyle/nasa/dev/HDTN/rtp/pi_to_pc.wav')

original = np.resize(original, [7802,1440] )

# snrseg = pysepm.SNRseg(original, transmitted, fs)

# pesq = pysepm.pesq(original, transmitted, fs)

# cosq = pysepm.composite(original, transmitted, fs)

print("snrseg, pesq, cosq")
# print(snrseg, pesq, cosq)