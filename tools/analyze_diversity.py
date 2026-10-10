"""Independent post-render spectral/envelope diagnostic (numpy/scipy/soundfile).
Usage: python tools/analyze_diversity.py <directory-of-8-WAVs>
Thresholds are in tests/diversity_contract_tests.cpp, not tuned here.
"""
import sys
from pathlib import Path
from itertools import combinations
import numpy as np
import soundfile as sf
from scipy.signal import welch

if len(sys.argv) != 2:
    raise SystemExit('Usage: python tools/analyze_diversity.py <wav-folder>')
folder=Path(sys.argv[1])
for family in ('impact','perc'):
    envs=[];spectra=[]
    for index in range(4):
        x,sr=sf.read(folder/f'{family}_Klangtyp_{index}.wav',always_2d=True)
        mono=x[:int(2*sr)].mean(axis=1)
        window=max(1,round(sr/100))
        env=np.sqrt(np.mean(mono[:len(mono)//window*window].reshape(-1,window)**2,axis=1))
        env=env/(np.linalg.norm(env)+1e-15)
        f,power=welch(mono,sr,nperseg=4096)
        bins=np.geomspace(40,12000,37)
        bands=np.array([power[(f>=lo)&(f<hi)].sum() for lo,hi in zip(bins[:-1],bins[1:])])
        dB=np.log10(np.maximum(bands,1e-20));dB-=dB.mean()
        envs.append(env);spectra.append(dB)
    ec=[];sc=[]
    for i,j in combinations(range(4),2):
        c=float(np.dot(envs[i],envs[j]));ec.append(c)
        shiftCorr=-1.
        for offset in range(-5,6):
            a=spectra[i][max(0,offset):36+min(0,offset)]
            b=spectra[j][max(0,-offset):36+min(0,-offset)]
            shiftCorr=max(shiftCorr,float(np.corrcoef(a,b)[0,1]))
        sc.append(shiftCorr)
    print(family,'median-envelope-cosine',round(float(np.median(ec)),3),
          'max-envelope-cosine',round(float(max(ec)),3),
          'median-shift-corrected-spectrum-correlation',round(float(np.median(sc)),3))
