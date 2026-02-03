# CollabRef Linux Quick Start

## 1. Install Dependencies

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install -y cmake build-essential git \
    qt6-base-dev qt6-websockets-dev libgl1-mesa-dev
```

### Fedora
```bash
sudo dnf install -y cmake gcc-c++ git \
    qt6-qtbase-devel qt6-qtwebsockets-devel mesa-libGL-devel
```

### Arch Linux
```bash
sudo pacman -S cmake base-devel git qt6-base qt6-websockets
```

### openSUSE
```bash
sudo zypper install cmake gcc-c++ git \
    qt6-base-devel qt6-websockets-devel Mesa-libGL-devel
```

---

## 2. Build CollabRef

```bash
# Extract and enter directory
unzip CollabRef.zip
cd CollabRef

# Make build script executable and run
chmod +x build.sh
./build.sh

# Run CollabRef
./build/CollabRef
```

---

## 3. Local Network Collaboration

### Person A (Host):
1. Open CollabRef
2. **Right-click → Collaborate → Host Session**
3. Select **"Same Network"**
4. Note the IP shown (e.g., `192.168.1.50:8080`)
5. Share this IP with your friend

### Person B (Join):
1. Open CollabRef  
2. **Right-click → Collaborate → Join Session**
3. Enter: `ws://192.168.1.50:8080` (use the IP from host)
4. Room ID: anything (e.g., `collab`)
5. Click OK

**You're now collaborating!**

---

## 4. Troubleshooting

### "Qt not found"
Make sure Qt6 WebSockets is installed:
```bash
# Ubuntu
sudo apt install qt6-websockets-dev

# Check Qt6 is available
pkg-config --modversion Qt6WebSockets
```

### "Connection refused"
- Make sure host selected "Same Network" not "Over Internet"
- Check firewall allows port 8080:
```bash
# Ubuntu (ufw)
sudo ufw allow 8080/tcp

# Fedora (firewalld)
sudo firewall-cmd --add-port=8080/tcp --permanent
sudo firewall-cmd --reload
```

### Find your local IP
```bash
ip addr | grep "inet 192"
# or
hostname -I
```

---

## 5. Desktop Entry (Optional)

Create `~/.local/share/applications/collabref.desktop`:
```ini
[Desktop Entry]
Name=CollabRef
Comment=Collaborative Reference Board
Exec=/path/to/CollabRef/build/CollabRef
Icon=/path/to/CollabRef/resources/icons/app.png
Type=Application
Categories=Graphics;
```

Then:
```bash
chmod +x ~/.local/share/applications/collabref.desktop
update-desktop-database ~/.local/share/applications/
```

---

## Quick Reference

| Action | How |
|--------|-----|
| Host | Right-click → Collaborate → Host Session → Same Network |
| Join | Right-click → Collaborate → Join → `ws://HOST_IP:8080` |
| Add Image | Drag & drop, or Ctrl+V |
| Pan | Space + drag, or middle mouse |
| Zoom | Scroll wheel |
| Delete | Select + Delete key |
| Sync | Right-click → Collaborate → Sync Now |
