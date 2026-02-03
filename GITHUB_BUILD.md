# Building CollabRef via GitHub

## Quick Setup (5 minutes)

### Step 1: Create GitHub Repository

1. Go to [github.com/new](https://github.com/new)
2. Name: `CollabRef`
3. Make it Public or Private
4. Click **Create repository**

### Step 2: Push Code

```bash
# Extract CollabRef.zip and enter directory
cd CollabRef

# Initialize git
git init
git add .
git commit -m "Initial commit"

# Add your GitHub repo as remote (replace YOUR_USERNAME)
git remote add origin https://github.com/YOUR_USERNAME/CollabRef.git

# Push
git branch -M main
git push -u origin main
```

### Step 3: Wait for Build

1. Go to your repo on GitHub
2. Click **Actions** tab
3. Watch the build run (~5-10 minutes)
4. Once complete, click the workflow run
5. Download artifacts at the bottom:
   - `CollabRef-Linux-x64` 
   - `CollabRef-Windows-x64`

---

## Creating a Release

To create downloadable releases:

```bash
# Tag a version
git tag v1.0.0
git push origin v1.0.0
```

This triggers a build that automatically:
1. Builds for Linux and Windows
2. Creates a GitHub Release
3. Attaches the binaries

Your friends can then download from:
`https://github.com/YOUR_USERNAME/CollabRef/releases`

---

## For Your Linux Friends

Tell them to:

```bash
# Download latest release
wget https://github.com/YOUR_USERNAME/CollabRef/releases/latest/download/CollabRef-Linux-x64.tar.gz

# Extract
tar -xzf CollabRef-Linux-x64.tar.gz

# Run
cd CollabRef-Linux
./CollabRef
```

Or they can go to your GitHub releases page and click the download link.

---

## Collaborating on Same Network

Once everyone has CollabRef running:

### Host (you):
1. Right-click → Collaborate → Host Session
2. Choose "Same Network"
3. Share the IP shown (e.g., `192.168.1.50:8080`)

### Others (join):
1. Right-click → Collaborate → Join Session
2. Enter: `ws://192.168.1.50:8080`
3. Room ID: `collab` (or any name)

---

## Build Status Badge

Add this to your README.md:

```markdown
![Build](https://github.com/YOUR_USERNAME/CollabRef/actions/workflows/build.yml/badge.svg)
```

---

## Troubleshooting

### "Actions disabled"
Go to repo Settings → Actions → General → Allow all actions

### Build fails on Windows
The Windows build requires MSVC. If it fails, you can still use the Linux build or build Windows locally with vcpkg.

### Want macOS too?
Add this job to `.github/workflows/build.yml`:

```yaml
  build-macos:
    runs-on: macos-latest
    steps:
    - uses: actions/checkout@v4
    - name: Install Qt
      run: brew install qt@6
    - name: Build
      run: |
        mkdir build && cd build
        cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6)
        cmake --build . --config Release
```
