# CollabRef Team Setup

One shared board that everyone can use simultaneously. Zero setup required.

## Usage

```bash
# Extract
tar -xzf CollabRef-Linux-x64.tar.gz

# Run
cd CollabRef-Linux
./CollabRef.sh
```

**That's it.** No server setup, no configuration, no dependencies.

---

## How It Works

```
User A runs ./CollabRef.sh  →  Becomes host automatically
User B runs ./CollabRef.sh  →  Connects to User A
User C runs ./CollabRef.sh  →  Connects to User A

Everyone sees the same board in real-time.
Board saved to shared_board.json automatically.
```

---

## What Happens When...

| Scenario | Result |
|----------|--------|
| Everyone closes the app | Board saved to `shared_board.json` |
| Someone opens it tomorrow | Loads saved board, all content there |
| Host closes, others stay | Someone else becomes host automatically |
| Server reboots | File persists, just run the app again |

---

## Files

```
CollabRef-Linux/
├── CollabRef.sh         ← Run this
├── CollabRef            ← The app
├── shared_board.json    ← Your board (auto-created)
├── lib/                 ← Bundled libraries
└── plugins/             ← Qt plugins
```

---

## Multiple Boards

Want separate boards? Make copies:

```bash
cp -r CollabRef-Linux ProjectA
cp -r CollabRef-Linux ProjectB
```

Each folder has its own `shared_board.json`.
