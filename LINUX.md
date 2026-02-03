# CollabRef Linux Quick Start

## 1. Download & Run

```bash
# Extract
tar -xzf CollabRef-Linux-x64.tar.gz
cd CollabRef-Linux

# Run (use the .sh script - it includes all libraries)
./CollabRef.sh
```

**That's it!** No installation required. All libraries are bundled.

---

## 2. Collaboration (Automatic)

Just run `./CollabRef.sh` on any machine on the same server.

- First person becomes the host automatically
- Everyone else connects automatically  
- Board saves to `shared_board.json` next to the app
- Close and reopen anytime - your work is saved

---

## 3. Quick Controls

| Action | How |
|--------|-----|
| Add image | Drag & drop, or Ctrl+V |
| Pan | Space + drag, or middle mouse |
| Zoom | Scroll wheel |
| Delete | Select + Delete key |
| Add text | Double-click empty area |

---

## Troubleshooting

### "Permission denied"
```bash
chmod +x CollabRef.sh CollabRef
```

### Display issues over SSH
```bash
# Connect with X11 forwarding
ssh -X user@server
```

### Find your local IP
```bash
hostname -I | awk '{print $1}'
```
