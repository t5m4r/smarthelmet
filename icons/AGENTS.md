# Icons

## Source file
- `modern_headsense.png` - 1024x1024 source icon with "Rider Secured By HEADSENSE" branding

## Regenerate PWA icons from source

```bash
# Standard icons (resized from 1024x1024 source)
sips -z 192 192 modern_headsense.png --out icon-192.png
sips -z 512 512 modern_headsense.png --out icon-512.png

# Maskable icons (with padding + brand color background for Android adaptive icons)
magick modern_headsense.png -resize 120x120 -gravity center -background "#0d6efd" -extent 192x192 icon-maskable-192.png
magick modern_headsense.png -resize 320x320 -gravity center -background "#0d6efd" -extent 512x512 icon-maskable-512.png
```
