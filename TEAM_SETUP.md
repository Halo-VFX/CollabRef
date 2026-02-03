# CollabRef Team Setup

One shared board that everyone can use simultaneously.

## Setup (One Time - Admin Only)

### Step 1: Start the Server

On your Linux server:

```bash
# Download persistent server
mkdir ~/collabref-server && cd ~/collabref-server

# Create server.js (copy from CollabRef-PersistentServer.zip)
# Or download directly from your release

npm install ws
node server.js
```

Or use PM2 to run forever:
```bash
npm install -g pm2
pm2 start server.js --name collabref
pm2 startup && pm2 save
```

Note your server's IP (e.g., `192.168.1.100`)

### Step 2: Configure the Client

Edit `server.conf` (next to CollabRef executable):

```
server=ws://192.168.1.100:8080
room=main
```

### Step 3: Distribute to Team

Give everyone:
1. The CollabRef executable
2. The `server.conf` file (already configured)

---

## For Users

Just double-click `CollabRef` - that's it!

- App auto-connects to the team board
- Everyone sees the same canvas
- Add images, they appear for everyone
- Close the app anytime, your work is saved on the server

---

## How It Works

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   User A    │     │   User B    │     │   User C    │
│  CollabRef  │     │  CollabRef  │     │  CollabRef  │
└──────┬──────┘     └──────┬──────┘     └──────┬──────┘
       │                   │                   │
       └───────────────────┼───────────────────┘
                           │
                    ┌──────▼──────┐
                    │   Server    │
                    │ (Linux box) │
                    │             │
                    │ boards/     │
                    │  main.json  │ ◄── Saved to disk
                    └─────────────┘
```

- Server runs 24/7 on your Linux machine
- Board state saved to disk every 30 seconds
- Users connect/disconnect freely
- Everyone always sees the latest state

---

## Multiple Boards

Want separate boards? Edit `server.conf`:

```
room=project-alpha   # One team
room=project-beta    # Another team
room=main            # Default
```

Each room is a separate board, saved as a separate file.

---

## Troubleshooting

**Can't connect?**
- Check server is running: `curl http://SERVER_IP:8080/health`
- Check firewall: `sudo ufw allow 8080/tcp`
- Check server.conf has correct IP

**Changes not syncing?**
- Right-click → Collaborate → Sync Now
- Check server logs: `pm2 logs collabref`

**Server IP changed?**
- Edit `server.conf` with new IP
- Restart CollabRef
