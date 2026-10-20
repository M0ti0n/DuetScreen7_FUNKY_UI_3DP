"""A closed manila folder lying flat on the spare bed (drawn flat, warped onto the bed)."""
import math, numpy as np, cv2
from PIL import Image, ImageDraw, ImageFilter
INK=(22,20,18,255); MANILA=(224,182,118,255); MANILA_D=(198,152,90,255); PAPER=(247,244,235,255)
N=1200
def folder_flat():
    K=N; im=Image.new('RGBA',(K,K),(0,0,0,0)); d=ImageDraw.Draw(im)
    cx,cy,w,h,rot=0.49*K,0.55*K,0.84*K,0.66*K,-5
    def T(x,y):
        a=math.radians(rot); dx,dy=x-cx,y-cy
        return (cx+dx*math.cos(a)-dy*math.sin(a), cy+dx*math.sin(a)+dy*math.cos(a))
    def poly(pts,fill,w_=20):
        pp=[T(*p) for p in pts]; d.polygon(pp,fill=fill); d.line(pp+[pp[0]],fill=INK,width=w_,joint='curve')
    x0,y0,x1,y1=cx-w/2,cy-h/2,cx+w/2,cy+h/2
    sh=Image.new('L',(K,K),0); ImageDraw.Draw(sh).polygon([T(x0+18,y0+24),T(x1+18,y0+24),T(x1+18,y1+24),T(x0+18,y1+24)],fill=150)
    sh=sh.filter(ImageFilter.GaussianBlur(14)); s=Image.new('RGBA',(K,K),(0,0,0,0)); s.putalpha(sh); im.alpha_composite(s); d=ImageDraw.Draw(im)
    poly([(x0,y0),(x0+0.30*K,y0),(x0+0.34*K,y0-0.09*K),(x0+0.58*K,y0-0.09*K),(x0+0.62*K,y0),(x1,y0),(x1,y1),(x0,y1)],MANILA_D)
    poly([(x0+0.04*K,y0+0.02*K),(x1+0.025*K,y0+0.03*K),(x1+0.025*K,y1-0.03*K),(x0+0.04*K,y1-0.02*K)],PAPER,14)
    for k in range(3):
        yy=y0+0.10*K+k*0.05*K
        d.line([T(x1-0.01*K,yy),T(x1+0.02*K,yy)],fill=(150,150,156,255),width=10)
    poly([(x0,y0+0.015*K),(x1-0.012*K,y0+0.015*K),(x1-0.012*K,y1),(x0,y1)],MANILA)
    d.line([T(x0+0.03*K,y1-0.06*K),T(x1-0.05*K,y1-0.06*K)],fill=MANILA_D,width=8)
    return im
def folder_on(quad,size):
    """quad: back-left, back-right, front-right, front-left corners of the bed surface"""
    M=cv2.getPerspectiveTransform(np.float32([[0,0],[N,0],[N,N],[0,N]]),np.float32(quad))
    a=np.asarray(folder_flat()).astype(np.float32)
    # premultiply so the edges stay clean
    a[...,:3]*=a[...,3:]/255
    w=cv2.warpPerspective(a,M,size,flags=cv2.INTER_AREA)
    al=np.maximum(w[...,3:],1e-3); w[...,:3]=np.where(w[...,3:]>0,w[...,:3]*255/al,0)
    return Image.fromarray(np.clip(w,0,255).astype(np.uint8),'RGBA')
