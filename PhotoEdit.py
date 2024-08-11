import os
import sys
print(sys.argv)
#Open
from PIL import Image
if (len(sys.argv)<2):
    ADDR = input("FILE ADDRESS: ")
else:
    ADDR = sys.argv[1] 
with open(ADDR, "rb") as tempT:
    tempF = open(ADDR+"_bak", "wb")
    tempF.write(tempT.read())
    tempF.close()
im = Image.open(ADDR).convert("RGBA")
pix = im.load()
w = im.size[0]
h = im.size[1]
a = [];
for x in range(h):
    a.append([])
    for y in range(w):
        c = im.getpixel((y, x))
        c = [str(hex(t)[2:]).zfill(2) for t in c]
        c = ''.join(c)
        c = c + ","
        a[x].append(c);
f = open("out.qpic", "w")
for x in range(h):
    a[x] = ''.join(a[x])
if (im.mode == "RGBA"): f.write("T")
else: f.write("N")
f.write(a[0])
f.write("|\n")
for i in range(1, h):
    f.write(a[i])
    f.write("\n")
f.close()

#Edit

import subprocess
subprocess.run("notepad out.qpic")

#Save
f = open("out.qpic", "r")
f = f.read().replace("\r", "").replace("\n", "")
tran = 0
if (f[0] == "T"): tran = 1
f = f[1:]
b = 7+tran*2
h = f.find("|")//b
w = len(f)//h//b
f = f.replace("|", "")
a = []
t = [255, 255, 255, 255]
for m in range(w):
    i = m * h * b
    a.append([])
    for n in range(h):
        j = n * b
        a[m].append([])
        t[0] = int(f[i+j+0:i+j+2], 16)
        t[1] = int(f[i+j+2:i+j+4], 16)
        t[2] = int(f[i+j+4:i+j+6], 16)
        if (tran): t[3] = int(f[i+j+6:i+j+8], 16)
        a[m][n] = (t[0], t[1], t[2])
        if (tran): a[m][n] = (t[0], t[1], t[2], t[3])
f = open(ADDR, "w")
f.close()
if (tran): im = Image.new("RGBA", (w, h))
else: im = Image.new("RGB", (w, h))
im = im.resize((h,w))
for x in range(w):
    for y in range(h):
        im.putpixel((y, x), a[x][y])
im.save(ADDR)
