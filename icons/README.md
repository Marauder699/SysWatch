# Icons Directory

## Current Status
Using system fallback icon temporarily.

## To replace the icon:

1. Create or obtain a PNG icon (preferably 256x256 or 512x512)
2. Save it as `syswatch.png` in this directory
3. Run `sudo make install` to install it system-wide

## Recommended specifications:
- **Format**: PNG (or SVG for scalability)
- **Size**: 256x256 pixels minimum
- **Style**: Modern, flat design
- **Theme**: System monitoring, temperature gauge, dashboard
- **Colors**: Tech blue/green or warning orange/red

## Icon locations after install:
- `/usr/share/icons/hicolor/256x256/apps/syswatch.png`
- `/usr/share/applications/syswatch.desktop`

The desktop file will use the system's default monitor icon until replaced.
