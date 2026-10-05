#!/usr/bin/env python3
"""NF Delay 42 user manuals (English + Portuguese) as PDFs, in the plug-in's colours (plate grey, blue sections, red accents).

Inputs (kept in the repo): Docs/manual/img/panel.png, panel_clk.png (captures from NFDelay42Snapshot, see CLAUDE.md),
                           Source/PresetManager.cpp (the factory preset table is read from it), Assets/Logo/nf_audio_tools_logo.png
Usage:  python3 Docs/make_manual.py  ->  Assets/Manuals/NF_Delay42_Manual_{English,Portugues}.pdf
Regenerate after any UI or DSP change (and re-capture the images). The PDFs are embedded in the plug-in (3-line menu)."""
import os, re
from PIL import Image, ImageDraw, ImageFont
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A4
from reportlab.lib.utils import simpleSplit
from reportlab.lib.colors import Color, white

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMG = os.path.join(ROOT, 'Docs/manual/img')
OUTDIR = os.path.join(ROOT, 'Assets/Manuals')
LOGO = os.path.join(ROOT, 'Assets/Logo/nf_audio_tools_logo.png')
VERSION = re.search(r'VERSION (\d+\.\d+\.\d+)', open(os.path.join(ROOT, 'CMakeLists.txt')).read()).group(1)
W, H = A4
M = 46.0
CW = W - 2 * M
BG = Color(0.96, 0.96, 0.98); INK = Color(0.10, 0.10, 0.14); INK2 = Color(0.36, 0.36, 0.44)
PLATE = Color(0.27, 0.26, 0.33); BLUE = Color(0.04, 0.45, 0.80); RED = Color(0.92, 0.26, 0.20); LINE = Color(0.78, 0.78, 0.84)
ROWALT = Color(0.92, 0.93, 0.97)

# ------------------------------------------------------------------ factory presets (read from the source)
def presets():
    txt = open(os.path.join(ROOT, 'Source/PresetManager.cpp'), encoding='utf-8').read()
    rows = []
    pat = (r'\{\s*"([^"]+)",\s*([\d.]+)f,\s*([\d.]+)f,\s*(true|false),\s*(true|false),\s*(true|false),\s*([\d.]+)f,\s*(true|false),\s*(\d+),\s*'
           r'([\d.]+)f,\s*([\d.]+)f,\s*([\d.]+)f,\s*([\d.]+)f')
    for m in re.finditer(pat, txt):
        n, lv, fb, hc, fi, di, mix, x2, tap, man, dep, wav, rate = m.groups()
        ms = int(tap) * 3.125 * (2 if x2 == 'true' else 1)
        hz = 0.1 * 100 ** (float(rate) / 10)
        flags = ' '.join(t for t, v in (('HI CUT', hc), ('FB INV', fi), ('DLY INV', di), ('X2', x2)) if v == 'true') or '-'
        wave = {0: 'Sine', 5: 'Envelope', 10: 'Square'}.get(round(float(wav)), '%.0f' % float(wav))
        rows.append((n, '%.0f ms' % ms, '%.1f' % float(fb), '%.1f' % float(mix), flags, '%.1f' % float(dep), wave, '%.1f Hz' % hz))
    return rows
PRESETS = presets()

# ------------------------------------------------------------------ annotated panel picture
CALLOUTS = [  # number, design x, design y (image coordinates of the reference photo: panel origin 22,40)
    (1, 184, 115, 'r'), (2, 257, 137, 'r'), (3, 332, 118, 'r'), (4, 417, 118, 'r'), (5, 491, 137, 'r'), (6, 545, 137, 'r'), (7, 598, 137, 'r'),
    (8, 692, 118, 'r'), (9, 676, 166, 'c'), (10, 768, 138, 'r'), (11, 904, 89, 'c'), (12, 836, 124, 'r'), (13, 908, 124, 'r'), (14, 1030, 116, 'c'),
    (15, 1187, 119, 'r'), (16, 1275, 118, 'r'), (17, 1367, 118, 'r'), (18, 1456, 118, 'r'), (19, 1521, 150, 'c'), (20, 1510, 52, 'c'), (21, 1708, 74, 'c'), (22, 1522, 78, 'c')]

def annotated():
    im = Image.open(os.path.join(IMG, 'panel.png')).convert('RGB'); s = im.width / 1700.0
    d = ImageDraw.Draw(im); f = ImageFont.truetype('/System/Library/Fonts/Helvetica.ttc', int(15 * s), index=1)
    for n, x, y, mode in CALLOUTS:
        cx, cy = (x - 22) * s, (y - 40) * s
        if mode == 'r': cx += 21 * s; cy -= 21 * s
        r = 10.5 * s
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(235, 66, 51), outline=(255, 255, 255), width=max(1, int(1.5 * s)))
        t = str(n); tw = d.textlength(t, font=f)
        d.text((cx - tw / 2, cy - 9.2 * s), t, font=f, fill=(255, 255, 255))
    p = os.path.join(IMG, '_panel_numbered.png'); im.save(p); return p

# ------------------------------------------------------------------ flow layout engine
class Doc:
    def __init__(self, path, lang):
        self.c = canvas.Canvas(path, pagesize=A4); self.lang = lang; self.n = 0; self.y = 0
        self.c.setTitle('NF Delay 42 - ' + ('User Manual' if lang == 'en' else 'Manual do Usuário')); self.c.setAuthor('NF Audio Tools by Nenno Fernando')
    def page(self):
        if self.n: self.c.showPage()
        self.n += 1; c = self.c
        c.setFillColor(BG); c.rect(0, 0, W, H, fill=1, stroke=0)
        c.setFillColor(PLATE); c.rect(0, H - 40, W, 40, fill=1, stroke=0)
        c.setFillColor(white); c.setFont('Helvetica-Bold', 11); c.drawString(M, H - 25, 'NF DELAY 42')
        c.setFont('Helvetica', 9); c.drawRightString(W - M, H - 25, 'NF Audio Tools by Nenno Fernando  -  V%s' % VERSION)
        c.setFillColor(BLUE); c.rect(0, H - 43, W, 3, fill=1, stroke=0)
        c.setFillColor(INK2); c.setFont('Helvetica', 8.5); c.drawCentredString(W / 2, 24, '%s %d' % ('Page' if self.lang == 'en' else 'Página', self.n))
        self.y = H - 78
    def need(self, h):
        if self.y - h < 50: self.page()
    def h1(self, t):
        self.need(60); c = self.c; self.y -= 8
        c.setFillColor(INK); c.setFont('Helvetica-Bold', 21); c.drawString(M, self.y, t)
        c.setFillColor(RED); c.rect(M, self.y - 9, 54, 2.6, fill=1, stroke=0); self.y -= 32
    def h2(self, t):
        self.need(40); self.y -= 6; self.c.setFillColor(BLUE); self.c.setFont('Helvetica-Bold', 12.5); self.c.drawString(M, self.y, t); self.y -= 17
    def p(self, t, size=10, lead=14, x=None, w=None, color=INK, font='Helvetica'):
        x = M if x is None else x; w = CW if w is None else w
        lines = simpleSplit(t, font, size, w); self.need(len(lines) * lead + 4)
        self.c.setFillColor(color); self.c.setFont(font, size)
        for ln in lines: self.c.drawString(x, self.y, ln); self.y -= lead
        self.y -= 4
    def bullets(self, items, size=10, lead=13.5):
        for it in items:
            lines = simpleSplit(it, 'Helvetica', size, CW - 16); self.need(len(lines) * lead + 3)
            self.c.setFillColor(RED); self.c.circle(M + 4, self.y + 3.2, 1.9, fill=1, stroke=0)
            self.c.setFillColor(INK); self.c.setFont('Helvetica', size)
            for ln in lines: self.c.drawString(M + 14, self.y, ln); self.y -= lead
            self.y -= 2
        self.y -= 3
    def table(self, head, rows, widths, size=9, lead=12.2, bold_first=True):
        c = self.c; tot = sum(widths); widths = [w * CW / tot for w in widths]
        def rowh(r): return max(len(simpleSplit(str(t), 'Helvetica', size, w - 8)) for t, w in zip(r, widths)) * lead + 7
        self.need(rowh(head) + rowh(rows[0]) + 6)
        def draw_head():
            h = rowh(head); c.setFillColor(PLATE); c.rect(M, self.y - h + 5, CW, h, fill=1, stroke=0)
            x = M; c.setFillColor(white); c.setFont('Helvetica-Bold', size)
            for t, w in zip(head, widths): c.drawString(x + 4, self.y - 7, t); x += w
            self.y -= h
        draw_head()
        for i, r in enumerate(rows):
            h = rowh(r)
            if self.y - h < 50: self.page(); draw_head()
            if i % 2 == 0: c.setFillColor(ROWALT); c.rect(M, self.y - h + 5, CW, h, fill=1, stroke=0)
            x = M
            for j, (t, w) in enumerate(zip(r, widths)):
                font = 'Helvetica-Bold' if (j == 0 and bold_first) else 'Helvetica'
                c.setFillColor(INK); c.setFont(font, size); yy = self.y - 7
                for ln in simpleSplit(str(t), font, size, w - 8): c.drawString(x + 4, yy, ln); yy -= lead
                x += w
            self.y -= h
        self.y -= 8
    def image(self, path, w=CW, caption=None):
        iw, ih = Image.open(path).size; h = w * ih / iw; self.need(h + 30); c = self.c
        c.setFillColor(Color(0.2, 0.2, 0.25)); c.roundRect(M - 3, self.y - h - 3, w + 6, h + 6, 4, fill=1, stroke=0)
        c.drawImage(path, M, self.y - h, w, h); self.y -= h + 14
        if caption: self.p(caption, size=8.8, color=INK2, lead=12)
    def box(self, title, text):
        lines = simpleSplit(text, 'Helvetica', 9.6, CW - 24); h = len(lines) * 13 + 30; self.need(h + 6); c = self.c
        c.setFillColor(Color(1, 0.95, 0.94)); c.setStrokeColor(RED); c.setLineWidth(0.8); c.roundRect(M, self.y - h + 8, CW, h, 5, fill=1, stroke=1)
        c.setFillColor(RED); c.setFont('Helvetica-Bold', 9.6); c.drawString(M + 12, self.y - 8, title.upper())
        c.setFillColor(INK); c.setFont('Helvetica', 9.6); yy = self.y - 23
        for ln in lines: c.drawString(M + 12, yy, ln); yy -= 13
        self.y -= h + 6
    def cover(self, tag, sub, ver):
        c = self.c; self.n += 1
        c.setFillColor(PLATE); c.rect(0, 0, W, H, fill=1, stroke=0)
        c.setFillColor(BLUE); c.rect(0, H * 0.46, W, 5, fill=1, stroke=0)
        c.drawImage(LOGO, M, H - 150, 150, 150 * 170 / 307, mask='auto')
        c.setFillColor(white); c.setFont('Helvetica-Bold', 40); c.drawString(M, H * 0.46 + 130, 'NF Delay 42')
        c.setFont('Helvetica', 17); c.setFillColor(Color(0.85, 0.87, 0.95)); c.drawString(M, H * 0.46 + 100, tag)
        c.setFont('Helvetica', 12); c.drawString(M, H * 0.46 + 78, sub)
        pw = W - 2 * M; p = os.path.join(IMG, 'panel.png'); iw, ih = Image.open(p).size
        c.drawImage(p, M, H * 0.46 - 20 - pw * ih / iw, pw, pw * ih / iw)
        c.setFillColor(white); c.setFont('Helvetica-Bold', 13); c.drawString(M, 118, 'NF Audio Tools by Nenno Fernando')
        c.setFont('Helvetica', 10); c.setFillColor(Color(0.85, 0.87, 0.95)); c.drawString(M, 100, ver)
        c.drawString(M, 84, 'VST3  -  Audio Unit  -  AAX')
    def save(self): self.c.showPage(); self.c.save()

# ------------------------------------------------------------------ texts
EN = dict(
 file='NF_Delay42_Manual_English.pdf', tag='Digital delay processor', sub='User manual', ver='Version %s' % VERSION,
 intro_h='Introduction',
 intro=['NF Delay 42 is a digital delay processor with a long delay memory, a voltage-controlled sweep (VCO), a programmable clock and an infinite-repeat '
        'function. It covers everything from tight doubling, chorus and flanging (a few milliseconds) through slap-back and tape-style echoes, to long delays and '
        'live looping of up to 2.4 seconds.',
        'It is modelled on the behaviour described in the public owner\'s manual of a classic 1U rack digital delay: delay ranges, sweep ratio, input limiter, filters, '
        'clock and repeat logic follow that manual. It is an original NF Audio Tools product; it is not affiliated with, or endorsed by, the maker of any '
        'hardware it was inspired by, and all trademarks belong to their respective owners.'],
 quick_h='Quick start',
 quick=['Insert NF Delay 42 on a track or aux (stereo or mono). Pick a preset from the preset tab (top right), for example "Slapback" or "Flanger".',
        'Set LEVEL so the HEADROOM lamps reach the amber lamp (-6) on peaks. The red lamp (0 dB) means the input limiter is working.',
        'OUTPUT MIX blends the direct and the delayed signal (centre = equal blend). FEEDBACK sets how many repeats you hear.',
        'Set the delay time with the UP / DOWN buttons, the mouse wheel over the display, or by dragging the display up and down.',
        'Add movement with the VCO-SWEEP section: DEPTH for the amount, RATE for the speed and WAVEFORM for the shape.'],
 panel_h='The front panel',
 panel_cap='Figure 1 - Front panel. The numbers are explained on the following pages.',
 legend_head=['#', 'Control', 'What it does'],
 legend=[
  ('1', 'HEADROOM', 'Five lamps (-24, -18, -12, -6, 0 dB) show the peak level seen by the converter after the LEVEL control and the input limiter.'),
  ('2', 'DELAY X2', 'Selects the short range (X1, 16 kHz bandwidth) or the long range (X2, 6 kHz bandwidth, twice the delay time). The 6kHz lamp lights in X2.'),
  ('3', 'LEVEL', 'Input gain, 0 to 10. 7.5 is unity gain (0 dB); 10 gives +20 dB.'),
  ('4', 'FEEDBACK', 'Feeds the delayed signal back to the input, 0 to 10. At high settings the echoes sustain and become resonant.'),
  ('5', 'HI CUT', 'Puts a 6 dB/octave low-pass filter (-3 dB at 4 kHz) in the feedback path: darker, more natural repeats.'),
  ('6', 'FB INV', 'Inverts the polarity of the signal fed back. Changes the phase cancellations; typical for resonant flanging.'),
  ('7', 'DLY INV', 'Inverts the polarity of the delayed sound sent to the output mix (flange notches become peaks).'),
  ('8', 'OUTPUT MIX', 'Blend of direct and delayed sound. Fully left = direct only, fully right = delayed only. The centre detent is the equal blend for deepest flanging notches.'),
  ('9', 'BYPASS', 'Click the lamp or the word BYPASS: the delayed signal is removed from the output and nothing new enters the delay memory.'),
  ('10', 'INFINITE REPEAT', 'Captures the sound in the delay memory at the next clock pulse and repeats it until the button is pressed again (see "Infinite repeat").'),
  ('11', 'SET-MODE', 'DLY: the display shows the delay time and UP/DOWN change it. CLK: the display shows the clock fraction and UP/DOWN change it.'),
  ('12', 'DOWN', 'DLY: shorter delay (one step). CLK: steps the numerator of the clock fraction through 1, 3, 5, 7, 9.'),
  ('13', 'UP', 'DLY: longer delay (one step). CLK: steps the denominator of the clock fraction through 1, 2, 4, 8, 16, 32, 64.'),
  ('14', 'DISPLAY', 'Four-digit read-out of the real delay time in milliseconds, following every sweep. In CLK mode: the clock fraction. Lamps: CLK (clock pulse), infinity (repeat mode), 6kHz (X2).'),
  ('15', 'MANUAL', 'Manual VCO-sweep: changes the delay time by a factor of 0.5 (fully left) to 1.5 (fully right); the centre is X1. Use it for flanging by hand, pitch twisting and very long delays.'),
  ('16', 'DEPTH', 'Amount of the internal sweep, 0 (none) to 10 (a full 3:1 range). At 10 the MANUAL control has no effect.'),
  ('17', 'WAVEFORM', 'Fully left = sine, centre = envelope follower (the delay follows the input level), fully right = square. In between the shapes are blended.'),
  ('18', 'RATE', 'Speed of the sweep, 0.1 Hz to 10 Hz. The lamp beside it flashes with the sweep. No effect in the envelope position.'),
  ('19', 'POWER', 'Switches the unit on and off. Off = the signal passes untouched. At power-up the delay memory is empty, the repeat is off and the clock fraction is 1/2.'),
  ('20', 'PRESET TAB', 'Arrows step through the presets; click the name for the list: Default, factory presets, your presets, Save and Load.'),
  ('21', '3-LINE MENU', 'Opens this manual (English or Portuguese) and the About box.'),
  ('22', 'NF AUDIO TOOLS logo', 'Click the logo to return the plug-in window to its default size. The window can be resized with the corner handle; the size is remembered.')],
 times_h='Delay times', times_p='The delay memory has 256 taps. The UP/DOWN buttons (or the display) choose the tap; the delay time depends on the tap, the DELAY X2 button and the MANUAL control:',
 times_head=['Range', 'MANUAL = X0.5', 'MANUAL = X1', 'MANUAL = X1.5', 'Bandwidth', 'Step at X1'],
 times=[('X1 (short)', '400 ms', '800 ms', '1200 ms', '16 kHz', '3.1 ms'), ('X2 (long)', '800 ms', '1600 ms', '2400 ms', '6 kHz', '6.25 ms')],
 times_note='Values are the maximum delay (tap 255) for each setting. The display always shows the actual time, including the sweep. When you change the tap the delay glides for about 30 ms to avoid clicks.',
 rep_h='Infinite repeat and the clock',
 rep=['Pressing INFINITE REPEAT does not freeze the sound at once: the capture happens at the next pulse of the clock (the CLK lamp flashes in time with it). '
      'From then on the whole delay memory is repeated until you press the button again, and nothing new is recorded. The loop length is the entire memory, so it does not depend on the '
      'delay tap, but it does change with MANUAL and DELAY X2 (for example 800 ms at X1 / short range, 2400 ms at X1.5 / long range). The direct signal still passes, so you can layer '
      'new parts over the loop (OUTPUT MIX). The plug-in never starts in repeat mode.',
      'The clock period is the memory length times the clock fraction: with the memory at 800 ms and the fraction 1/2 the clock pulses every 400 ms. In SET-MODE CLK, DOWN steps the numerator '
      '(1, 3, 5, 7, 9) and UP the denominator (1, 2, 4, 8, 16, 32, 64). To catch a loop quickly, use a small fraction (for example 1/64); use a large fraction to wait for a precise moment.',
      'The clock follows the sweep, so with DEPTH above 0 the pulses are not steady.'],
 vco_h='VCO sweep: flanger, chorus, vibrato',
 vco=['The sweep changes the speed of the delay memory clock, so the delay time moves between 0.5x and 1.5x of its value (a 3:1 range). DEPTH sets how much of that range the sweep uses, '
      'MANUAL sets where you are in it, RATE how fast it moves and WAVEFORM the shape: sine (smooth), square (the delay jumps), or the envelope follower (loud notes push the delay one way, quiet notes the other).',
      'Short delays (a few milliseconds) with feedback give flanging; around 20 to 30 milliseconds with a slow sine gives chorus and doubling; with OUTPUT MIX fully right and a fast sine you get vibrato; a square wave '
      'with DEPTH raised gives pitch jumps; MANUAL turned slowly bends pitch like a tape machine.'],
 apps_h='Applications', apps_head=['Effect', 'How to set it'],
 apps=[('Echo / slap-back', 'Delay 100 to 500 ms, FEEDBACK 1 to 4, HI CUT on, OUTPUT MIX about 3 to 4, DEPTH 0.'),
       ('Double tracking', 'Delay 20 to 40 ms, FEEDBACK 0, OUTPUT MIX centre, small DEPTH (1 to 2) with a slow RATE.'),
       ('Flanging', 'Delay 3 to 10 ms (small tap, MANUAL left), OUTPUT MIX at the centre detent, FEEDBACK 4 to 6, DEPTH 10, RATE 1 to 3.'),
       ('Resonant effects', 'As flanging with FEEDBACK 8 to 9, FB INV on and HI CUT on.'),
       ('Vibrato', 'OUTPUT MIX fully right (delayed only), delay 10 to 20 ms, FEEDBACK 0, DEPTH 2 to 4, sine, RATE 6 to 7.'),
       ('Pitch twisting', 'OUTPUT MIX fully right, DEPTH 6, square or sine; or turn MANUAL by hand while a sound plays.'),
       ('Looping', 'Play a phrase, press INFINITE REPEAT, then layer over it. Use SET-MODE CLK to choose when the capture happens.')],
 pre_h='Presets', pre_p='The preset tab has Default (every control at its starting position), 12 factory presets and a list of the presets you save. Save writes a file to Documents/NF Audio Tools/NF Delay 42/Presets.',
 pre_head=['Preset', 'Delay', 'Feedback', 'Mix', 'Switches', 'Depth', 'Wave', 'Rate'],
 tips_h='Tips', tips=['Keep an eye on the HEADROOM lamps: if the red lamp stays on, lower LEVEL. The input limiter is gentle by design, but it still colours the sound.',
      'Feedback at 10 sustains for a very long time. Lower it, switch HI CUT on, or use BYPASS to stop the repeats at once.',
      'If a tempo-based delay is needed, work out the time in milliseconds (60000 / BPM) and enter it with the display; the clock output of the hardware is represented by the CLK lamp.',
      'All controls can be automated in the host. The delay tap, clock numerator and denominator are stepped parameters.'],
 spec_h='Specifications', spec_head=['Item', 'Value'],
 spec=[('Formats', 'VST3, Audio Unit, AAX (macOS 10.15 or later, Apple Silicon and Intel)'), ('Channels', 'Mono and stereo; both channels share the same sweep and clock'),
       ('Delay range', 'X1: 0 to 1200 ms; X2: 0 to 2400 ms (see table)'), ('Delay taps', '256 per range'), ('Sweep', 'Depth 0 to a full 3:1 range; rate 0.1 to 10 Hz; sine, envelope, square and blends'),
       ('Bandwidth', '16 kHz (X1) and 6 kHz (X2), -3 dB, anti-alias and reconstruction filters'), ('Input stage', '5:1 soft-knee compression above -3 dB, soft limiter at 0 dB'),
       ('Feedback filter', '6 dB/octave low-pass, -3 dB at 4 kHz'), ('Latency', 'None (the direct signal is not delayed)')],
 about_h='About', about='NF Delay 42 version %s. NF Audio Tools by Nenno Fernando. Copyright 2026 NF Audio Tools. All rights reserved.' % VERSION,
 pg='Page')

PT = dict(
 file='NF_Delay42_Manual_Portugues.pdf', tag='Processador de delay digital', sub='Manual do usuário', ver='Versão %s' % VERSION,
 intro_h='Introdução',
 intro=['O NF Delay 42 é um processador de delay digital com memória longa, varredura controlada por tensão (VCO), clock programável e função de repetição infinita. '
        'Ele cobre desde dobras de voz, chorus e flanger (poucos milissegundos), passando por slap-back e ecos estilo fita, até delays longos e loops ao vivo de até 2,4 segundos.',
        'Ele é modelado no comportamento descrito no manual público de um delay digital de rack 1U clássico: faixas de delay, razão de varredura, limitador de entrada, filtros, '
        'clock e lógica de repetição seguem esse manual. É um produto original da NF Audio Tools, sem vínculo nem endosso do fabricante de qualquer equipamento que o inspirou; '
        'todas as marcas pertencem aos seus respectivos donos.'],
 quick_h='Início rápido',
 quick=['Insira o NF Delay 42 em uma faixa ou auxiliar (estéreo ou mono). Escolha um preset na aba de presets (canto superior direito), por exemplo "Slapback" ou "Flanger".',
        'Ajuste LEVEL para que as luzes de HEADROOM cheguem à luz âmbar (-6) nos picos. A luz vermelha (0 dB) indica que o limitador de entrada está trabalhando.',
        'OUTPUT MIX mistura o sinal direto e o atrasado (centro = mistura igual). FEEDBACK define quantas repetições você ouve.',
        'Defina o tempo de delay com os botões UP / DOWN, com a roda do mouse sobre o display, ou arrastando o display para cima e para baixo.',
        'Adicione movimento na seção VCO-SWEEP: DEPTH para a quantidade, RATE para a velocidade e WAVEFORM para o formato.'],
 panel_h='O painel frontal',
 panel_cap='Figura 1 - Painel frontal. Os números são explicados nas páginas seguintes.',
 legend_head=['#', 'Controle', 'O que faz'],
 legend=[
  ('1', 'HEADROOM', 'Cinco luzes (-24, -18, -12, -6, 0 dB) mostram o nível de pico visto pelo conversor, depois do LEVEL e do limitador de entrada.'),
  ('2', 'DELAY X2', 'Seleciona a faixa curta (X1, banda de 16 kHz) ou a longa (X2, banda de 6 kHz, o dobro do tempo). A luz 6kHz acende em X2.'),
  ('3', 'LEVEL', 'Ganho de entrada, 0 a 10. 7,5 é ganho unitário (0 dB); 10 dá +20 dB.'),
  ('4', 'FEEDBACK', 'Realimenta o sinal atrasado na entrada, 0 a 10. Em ajustes altos os ecos se sustentam e ficam ressonantes.'),
  ('5', 'HI CUT', 'Coloca um filtro passa-baixas de 6 dB/oitava (-3 dB em 4 kHz) no caminho da realimentação: repetições mais escuras e naturais.'),
  ('6', 'FB INV', 'Inverte a polaridade do sinal realimentado. Muda os cancelamentos de fase; típico para flanger ressonante.'),
  ('7', 'DLY INV', 'Inverte a polaridade do som atrasado enviado à mistura de saída (os vales do flanger viram picos).'),
  ('8', 'OUTPUT MIX', 'Mistura entre som direto e atrasado. Todo à esquerda = só direto, todo à direita = só atrasado. O centro (com trava) é a mistura igual, para os vales mais profundos do flanger.'),
  ('9', 'BYPASS', 'Clique na luz ou na palavra BYPASS: o sinal atrasado sai da saída e nada novo entra na memória de delay.'),
  ('10', 'INFINITE REPEAT', 'Captura o som na memória de delay no próximo pulso do clock e o repete até o botão ser pressionado de novo (veja "Repetição infinita").'),
  ('11', 'SET-MODE', 'DLY: o display mostra o tempo de delay e UP/DOWN o alteram. CLK: o display mostra a fração do clock e UP/DOWN a alteram.'),
  ('12', 'DOWN', 'DLY: delay menor (um passo). CLK: passa o numerador da fração do clock por 1, 3, 5, 7, 9.'),
  ('13', 'UP', 'DLY: delay maior (um passo). CLK: passa o denominador da fração do clock por 1, 2, 4, 8, 16, 32, 64.'),
  ('14', 'DISPLAY', 'Leitura de quatro dígitos do tempo real de delay em milissegundos, acompanhando toda varredura. No modo CLK: a fração do clock. Luzes: CLK (pulso do clock), infinito (modo repetição), 6kHz (X2).'),
  ('15', 'MANUAL', 'VCO-sweep manual: muda o tempo de delay por um fator de 0,5 (todo à esquerda) a 1,5 (todo à direita); o centro é X1. Use para flanger manual, pitch twisting e delays muito longos.'),
  ('16', 'DEPTH', 'Quantidade da varredura interna, 0 (nenhuma) a 10 (faixa completa de 3:1). Em 10 o controle MANUAL não tem efeito.'),
  ('17', 'WAVEFORM', 'Todo à esquerda = senoidal, centro = seguidor de envelope (o delay segue o nível da entrada), todo à direita = quadrada. Entre eles os formatos são misturados.'),
  ('18', 'RATE', 'Velocidade da varredura, 0,1 Hz a 10 Hz. A luz ao lado pisca com a varredura. Sem efeito na posição de envelope.'),
  ('19', 'POWER', 'Liga e desliga a unidade. Desligado = o sinal passa intacto. Ao ligar, a memória de delay está vazia, a repetição desligada e a fração do clock é 1/2.'),
  ('20', 'ABA DE PRESETS', 'As setas percorrem os presets; clique no nome para a lista: Default, presets de fábrica, seus presets, Save e Load.'),
  ('21', 'MENU DE 3 LINHAS', 'Abre este manual (inglês ou português) e a janela About.'),
  ('22', 'Logo NF AUDIO TOOLS', 'Clique no logo para voltar a janela do plugin ao tamanho padrão. A janela pode ser redimensionada pelo canto; o tamanho é lembrado.')],
 times_h='Tempos de delay', times_p='A memória de delay tem 256 posições (taps). Os botões UP/DOWN (ou o display) escolhem o tap; o tempo de delay depende do tap, do botão DELAY X2 e do controle MANUAL:',
 times_head=['Faixa', 'MANUAL = X0,5', 'MANUAL = X1', 'MANUAL = X1,5', 'Banda', 'Passo em X1'],
 times=[('X1 (curta)', '400 ms', '800 ms', '1200 ms', '16 kHz', '3,1 ms'), ('X2 (longa)', '800 ms', '1600 ms', '2400 ms', '6 kHz', '6,25 ms')],
 times_note='Os valores são o delay máximo (tap 255) de cada ajuste. O display mostra sempre o tempo real, inclusive durante a varredura. Ao mudar o tap, o delay desliza por cerca de 30 ms para evitar estalos.',
 rep_h='Repetição infinita e o clock',
 rep=['Apertar INFINITE REPEAT não congela o som na hora: a captura acontece no próximo pulso do clock (a luz CLK pisca no mesmo ritmo). '
      'Dali em diante toda a memória de delay é repetida até você apertar o botão de novo, e nada novo é gravado. O tamanho do loop é a memória inteira, por isso não depende do '
      'tap, mas muda com MANUAL e DELAY X2 (por exemplo 800 ms em X1 / faixa curta, 2400 ms em X1,5 / faixa longa). O sinal direto continua passando, então você pode sobrepor '
      'novas partes ao loop (OUTPUT MIX). O plugin nunca inicia em modo de repetição.',
      'O período do clock é o tamanho da memória vezes a fração do clock: com a memória em 800 ms e a fração 1/2, o clock pulsa a cada 400 ms. Em SET-MODE CLK, DOWN muda o numerador '
      '(1, 3, 5, 7, 9) e UP o denominador (1, 2, 4, 8, 16, 32, 64). Para capturar um loop rápido use uma fração pequena (por exemplo 1/64); use uma fração grande para esperar um momento preciso.',
      'O clock acompanha a varredura; com DEPTH acima de 0 os pulsos não são regulares.'],
 vco_h='Varredura VCO: flanger, chorus, vibrato',
 vco=['A varredura muda a velocidade do clock da memória de delay, então o tempo de delay se move entre 0,5x e 1,5x do seu valor (faixa de 3:1). DEPTH define quanto dessa faixa a varredura usa, '
      'MANUAL define onde você está nela, RATE a velocidade e WAVEFORM o formato: senoidal (suave), quadrada (o delay salta) ou seguidor de envelope (notas fortes empurram o delay para um lado, notas fracas para o outro).',
      'Delays curtos (poucos milissegundos) com feedback dão flanger; de 20 a 30 milissegundos com senoidal lenta dão chorus e dobra de voz; com OUTPUT MIX todo à direita e senoidal rápida você tem vibrato; onda quadrada '
      'com DEPTH alto dá saltos de afinação; girar MANUAL devagar curva a afinação como uma fita.'],
 apps_h='Aplicações', apps_head=['Efeito', 'Como ajustar'],
 apps=[('Eco / slap-back', 'Delay de 100 a 500 ms, FEEDBACK 1 a 4, HI CUT ligado, OUTPUT MIX cerca de 3 a 4, DEPTH 0.'),
       ('Dobra de voz', 'Delay de 20 a 40 ms, FEEDBACK 0, OUTPUT MIX no centro, DEPTH pequeno (1 a 2) com RATE lento.'),
       ('Flanger', 'Delay de 3 a 10 ms (tap pequeno, MANUAL à esquerda), OUTPUT MIX na trava central, FEEDBACK 4 a 6, DEPTH 10, RATE 1 a 3.'),
       ('Efeitos ressonantes', 'Como o flanger com FEEDBACK 8 a 9, FB INV ligado e HI CUT ligado.'),
       ('Vibrato', 'OUTPUT MIX todo à direita (só atrasado), delay de 10 a 20 ms, FEEDBACK 0, DEPTH 2 a 4, senoidal, RATE 6 a 7.'),
       ('Pitch twisting', 'OUTPUT MIX todo à direita, DEPTH 6, quadrada ou senoidal; ou gire MANUAL à mão enquanto um som toca.'),
       ('Loop', 'Toque uma frase, aperte INFINITE REPEAT e toque por cima. Use SET-MODE CLK para escolher quando a captura acontece.')],
 pre_h='Presets', pre_p='A aba de presets tem Default (todos os controles na posição inicial), 12 presets de fábrica e a lista dos presets que você salvar. Save grava um arquivo em Documentos/NF Audio Tools/NF Delay 42/Presets.',
 pre_head=['Preset', 'Delay', 'Feedback', 'Mix', 'Chaves', 'Depth', 'Onda', 'Rate'],
 tips_h='Dicas', tips=['Fique de olho nas luzes de HEADROOM: se a vermelha ficar acesa, diminua o LEVEL. O limitador de entrada é suave por projeto, mas ainda colore o som.',
      'Feedback em 10 sustenta por muito tempo. Diminua, ligue o HI CUT ou use o BYPASS para parar as repetições na hora.',
      'Para um delay no tempo da música, calcule o tempo em milissegundos (60000 / BPM) e digite pelo display; a saída de clock do equipamento original é representada pela luz CLK.',
      'Todos os controles podem ser automatizados no host. O tap do delay e o numerador e denominador do clock são parâmetros por passos.'],
 spec_h='Especificações', spec_head=['Item', 'Valor'],
 spec=[('Formatos', 'VST3, Audio Unit, AAX (macOS 10.15 ou superior, Apple Silicon e Intel)'), ('Canais', 'Mono e estéreo; os dois canais compartilham a mesma varredura e o mesmo clock'),
       ('Faixa de delay', 'X1: 0 a 1200 ms; X2: 0 a 2400 ms (veja a tabela)'), ('Taps de delay', '256 por faixa'), ('Varredura', 'Depth de 0 a uma faixa completa de 3:1; rate de 0,1 a 10 Hz; senoidal, envelope, quadrada e misturas'),
       ('Largura de banda', '16 kHz (X1) e 6 kHz (X2), -3 dB, filtros anti-aliasing e de reconstrução'), ('Estágio de entrada', 'Compressão 5:1 com joelho suave acima de -3 dB, limitador suave em 0 dB'),
       ('Filtro do feedback', 'Passa-baixas de 6 dB/oitava, -3 dB em 4 kHz'), ('Latência', 'Nenhuma (o sinal direto não é atrasado)')],
 about_h='Sobre', about='NF Delay 42 versão %s. NF Audio Tools by Nenno Fernando. Copyright 2026 NF Audio Tools. Todos os direitos reservados.' % VERSION,
 pg='Página')

def build(L, lang):
    os.makedirs(OUTDIR, exist_ok=True)
    d = Doc(os.path.join(OUTDIR, L['file']), lang)
    d.cover(L['tag'], L['sub'], L['ver'])
    d.page(); d.h1(L['intro_h'])
    for t in L['intro']: d.p(t)
    d.h2(L['quick_h']); d.bullets(L['quick'])
    d.h1(L['panel_h']); d.image(annotated(), CW, L['panel_cap'])
    d.table(L['legend_head'], [list(r) for r in L['legend']], [1.0, 3.2, 14.5], size=8.8, bold_first=False)
    d.h1(L['times_h']); d.p(L['times_p']); d.table(L['times_head'], [list(r) for r in L['times']], [1.4, 1.5, 1.3, 1.5, 1.2, 1.3]); d.p(L['times_note'], size=9, color=INK2)
    d.h1(L['rep_h'])
    for t in L['rep']: d.p(t)
    d.image(os.path.join(IMG, 'panel_clk.png'), CW, 'SET-MODE CLK: ' + ('the display shows the clock fraction (numerator on the left, denominator on the right).' if lang == 'en' else 'o display mostra a fração do clock (numerador à esquerda, denominador à direita).'))
    d.h1(L['vco_h'])
    for t in L['vco']: d.p(t)
    d.h2(L['apps_h']); d.table(L['apps_head'], [list(r) for r in L['apps']], [3.0, 14.0])
    d.h1(L['pre_h']); d.p(L['pre_p']); d.table(L['pre_head'], [list(r) for r in PRESETS], [3.4, 1.5, 1.5, 1.1, 3.2, 1.2, 1.7, 1.5], size=8.6)
    d.h2(L['tips_h']); d.bullets(L['tips'])
    d.h1(L['spec_h']); d.table(L['spec_head'], [list(r) for r in L['spec']], [3.0, 11.0])
    d.h2(L['about_h']); d.p(L['about'])
    d.save()
    print('wrote', os.path.join(OUTDIR, L['file']))

build(EN, 'en'); build(PT, 'pt')
