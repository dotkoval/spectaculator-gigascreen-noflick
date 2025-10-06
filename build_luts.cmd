@echo off

python tools\gen_lut.py --gamma 2.5 --bits 5 >src\luts\gamma25\noflick100\lut_blend_5b.h
python tools\gen_lut.py --gamma 2.5 --bits 6 >src\luts\gamma25\noflick100\lut_blend_6b.h

python tools\gen_lut.py --gamma 2.4 --bits 5 >src\luts\gamma24\noflick100\lut_blend_5b.h
python tools\gen_lut.py --gamma 2.4 --bits 6 >src\luts\gamma24\noflick100\lut_blend_6b.h

python tools\gen_lut.py --gamma 2.2 --bits 5 >src\luts\gamma22\noflick100\lut_blend_5b.h
python tools\gen_lut.py --gamma 2.2 --bits 6 >src\luts\gamma22\noflick100\lut_blend_6b.h

python tools\gen_lut.py --gamma 2.0 --bits 5 >src\luts\gamma20\noflick100\lut_blend_5b.h
python tools\gen_lut.py --gamma 2.0 --bits 6 >src\luts\gamma20\noflick100\lut_blend_6b.h

python tools\gen_lut.py --gamma 1.8 --bits 5 >src\luts\gamma18\noflick100\lut_blend_5b.h
python tools\gen_lut.py --gamma 1.8 --bits 6 >src\luts\gamma18\noflick100\lut_blend_6b.h

python tools\gen_lut.py --linear --bits 5 >src\luts\linear\noflick100\lut_blend_5b.h
python tools\gen_lut.py --linear --bits 6 >src\luts\linear\noflick100\lut_blend_6b.h

python tools\gen_lut.py --gamma 2.5 --bits 5 --ratio 0.6 >src\luts\gamma25\noflick80\lut_blend_5b.h
python tools\gen_lut.py --gamma 2.5 --bits 6 --ratio 0.6 >src\luts\gamma25\noflick80\lut_blend_6b.h

python tools\gen_lut.py --gamma 2.4 --bits 5 --ratio 0.6 >src\luts\gamma24\noflick80\lut_blend_5b.h
python tools\gen_lut.py --gamma 2.4 --bits 6 --ratio 0.6 >src\luts\gamma24\noflick80\lut_blend_6b.h

python tools\gen_lut.py --gamma 2.2 --bits 5 --ratio 0.6 >src\luts\gamma22\noflick80\lut_blend_5b.h
python tools\gen_lut.py --gamma 2.2 --bits 6 --ratio 0.6 >src\luts\gamma22\noflick80\lut_blend_6b.h

python tools\gen_lut.py --gamma 2.0 --bits 5 --ratio 0.6 >src\luts\gamma20\noflick80\lut_blend_5b.h
python tools\gen_lut.py --gamma 2.0 --bits 6 --ratio 0.6 >src\luts\gamma20\noflick80\lut_blend_6b.h

python tools\gen_lut.py --gamma 1.8 --bits 5 --ratio 0.6 >src\luts\gamma18\noflick80\lut_blend_5b.h
python tools\gen_lut.py --gamma 1.8 --bits 6 --ratio 0.6 >src\luts\gamma18\noflick80\lut_blend_6b.h

python tools\gen_lut.py --linear --bits 5 --ratio 0.6 >src\luts\linear\noflick80\lut_blend_5b.h
python tools\gen_lut.py --linear --bits 6 --ratio 0.6 >src\luts\linear\noflick80\lut_blend_6b.h
