# Icons

## Regenerate PWA icons from source

```bash
# Convert SVG to 700x700 PNG (SVG viewBox is 110x110, already square)
magick -background white noun-helmet-8040886.svg -resize 700x700 noun-helmet-8040886.png

# Standard icons (resized from 700x700 source)
sips -z 192 192 noun-helmet-8040886.png --out icon-192.png
sips -z 512 512 noun-helmet-8040886.png --out icon-512.png

# Maskable icons (with padding + brand color background for Android adaptive icons)
magick noun-helmet-8040886.png -resize 120x120 -gravity center -background "#0d6efd" -extent 192x192 icon-maskable-192.png
magick noun-helmet-8040886.png -resize 320x320 -gravity center -background "#0d6efd" -extent 512x512 icon-maskable-512.png
```
