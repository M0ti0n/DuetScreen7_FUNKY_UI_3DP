"""Composes the filament page from the exported layers as the app does (to check an export)."""
import os, json, sys
import numpy as np
from PIL import Image
HERE=os.path.dirname(os.path.abspath(__file__))+'/'
A=os.path.abspath(HERE+'../../assets/workshop')+'/'
L=json.load(open(HERE+'layout.json'))
def put(im,key,tint=None):
    v=L[key]; s=Image.open(A+v['file']).convert('RGBA')
    if tint is not None:
        a=np.asarray(s).copy(); a[...,0],a[...,1],a[...,2]=tint; s=Image.fromarray(a,'RGBA')
    im.alpha_composite(s,(v['x'],v['y']))
def page(slots,pages=1):
    im=Image.open(A+'filament/background.png').convert('RGBA')
    for i,s in enumerate(slots):
        if s=='unused': continue
        if s=='empty': put(im,f'BAY{i}_EMPTY')
        else: put(im,f'BAY{i}_FIL',s); put(im,f'BAY{i}_SHADE')
        put(im,f'BAY{i}_SPOOL')
        if s=='empty': put(im,f'BAY{i}_GLOW')
    if pages>1: put(im,'PLATE'); put(im,'ARROW_NEXT'); put(im,'ARROW_BACK')
    return im.convert('RGB')
if __name__=='__main__':
    page([(236,120,36),(52,118,222),(146,210,48),'empty'],2).save(sys.argv[1]+'fp4.png')
    page([(146,210,48),'unused','unused','unused']).save(sys.argv[1]+'fp1.png')
