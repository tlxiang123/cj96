from PIL import Image, ImageDraw, ImageFont

files = [
    ("ZH (原图)", "resources/set_button.png"),
    ("EN", "resources/set_button_en.png"),
    ("DE", "resources/set_button_de.png"),
    ("FR", "resources/set_button_fr.png"),
    ("JA", "resources/set_button_ja.png"),
]
LBL = 96
W, H, GAP = 320, 80, 6
imgs = [(l, Image.open(p).convert("RGB")) for l, p in files]
out = Image.new("RGB", (W + LBL, (H + GAP) * len(imgs) + GAP), (255, 255, 255))
d = ImageDraw.Draw(out)
f = ImageFont.truetype("font/Alibaba-PuHuiTi-Regular.ttf", 15)
for i, (lbl, im) in enumerate(imgs):
    y = GAP + i * (H + GAP)
    out.paste(im, (LBL, y))
    d.text((8, y + 32), lbl, font=f, fill=(20, 20, 20))
out.save("diagnostics/set_button_i18n_final.png")
print("saved", out.size)
