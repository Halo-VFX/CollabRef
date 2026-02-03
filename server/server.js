const WebSocket = require('ws');
const http = require('http');

const PORT = process.env.PORT || 8080;

// Create HTTP server for health checks
const httpServer = http.createServer((req, res) => {
    if (req.url === '/health' || req.url === '/') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({
            status: 'ok',
            rooms: Object.keys(rooms).length,
            totalClients: Array.from(wss.clients).length,
            uptime: process.uptime()
        }));
    } else {
        res.writeHead(404);
        res.end('Not found');
    }
});

const wss = new WebSocket.Server({ server: httpServer });

// Room management
const rooms = {};  // roomId -> { clients: Map<ws, clientInfo>, boardState: [], textState: [] }

function getOrCreateRoom(roomId) {
    if (!rooms[roomId]) {
        rooms[roomId] = {
            clients: new Map(),
            boardState: [],
            textState: [],
            createdAt: Date.now()
        };
        console.log(`Room created: ${roomId}`);
    }
    return rooms[roomId];
}

function cleanupEmptyRooms() {
    const now = Date.now();
    for (const [roomId, room] of Object.entries(rooms)) {
        if (room.clients.size === 0) {
            // Keep room for 1 hour after last client leaves (to preserve state)
            if (now - room.lastActivity > 3600000) {
                delete rooms[roomId];
                console.log(`Room cleaned up: ${roomId}`);
            }
        }
    }
}

// Cleanup empty rooms every 10 minutes
setInterval(cleanupEmptyRooms, 600000);

wss.on('connection', (ws) => {
    let currentRoom = null;
    let clientId = generateId();
    let clientName = 'Anonymous';
    
    console.log(`Client connected: ${clientId}`);
    
    ws.on('message', (data) => {
        try {
            const msg = JSON.parse(data);
            const type = msg.type;
            
            if (type === 'join') {
                // Join or create a room
                const roomId = msg.roomId || 'default';
                clientName = msg.userName || 'Anonymous';
                
                currentRoom = getOrCreateRoom(roomId);
                currentRoom.clients.set(ws, { clientId, clientName });
                currentRoom.lastActivity = Date.now();
                
                console.log(`${clientName} (${clientId}) joined room: ${roomId}`);
                
                // Send client their ID
                ws.send(JSON.stringify({
                    type: 'welcome',
                    clientId: clientId,
                    roomId: roomId
                }));
                
                // Send current room state
                if (currentRoom.boardState.length > 0 || currentRoom.textState.length > 0) {
                    ws.send(JSON.stringify({
                        type: 'fullSync',
                        images: currentRoom.boardState,
                        texts: currentRoom.textState
                    }));
                }
                
                // Notify others
                broadcast(currentRoom, {
                    type: 'join',
                    oderId: clientId,
                    userName: clientName
                }, ws);
                
                // Send user list to new client
                const users = [];
                currentRoom.clients.forEach((info, client) => {
                    if (client !== ws) {
                        users.push({ oderId: info.clientId, userName: info.clientName });
                    }
                });
                ws.send(JSON.stringify({
                    type: 'userList',
                    users: users
                }));
            }
            else if (!currentRoom) {
                // Client must join a room first
                ws.send(JSON.stringify({
                    type: 'error',
                    message: 'Must join a room first'
                }));
                return;
            }
            else if (type === 'cursor') {
                msg.oderId = clientId;
                broadcast(currentRoom, msg, ws);
            }
            else if (type === 'imageAdd') {
                // Check for duplicates
                const imageId = msg.imageId;
                const exists = currentRoom.boardState.some(img => img.imageId === imageId);
                if (!exists) {
                    currentRoom.boardState.push(msg);
                }
                currentRoom.lastActivity = Date.now();
                msg.oderId = clientId;
                broadcast(currentRoom, msg, ws);
            }
            else if (type === 'imageUpdate') {
                const imageId = msg.imageId;
                for (let i = 0; i < currentRoom.boardState.length; i++) {
                    if (currentRoom.boardState[i].imageId === imageId) {
                        Object.assign(currentRoom.boardState[i], msg);
                        break;
                    }
                }
                currentRoom.lastActivity = Date.now();
                msg.oderId = clientId;
                broadcast(currentRoom, msg, ws);
            }
            else if (type === 'imageRemove') {
                const imageId = msg.imageId;
                currentRoom.boardState = currentRoom.boardState.filter(img => img.imageId !== imageId);
                currentRoom.lastActivity = Date.now();
                msg.oderId = clientId;
                broadcast(currentRoom, msg, ws);
            }
            else if (type === 'textAdd') {
                const textId = msg.textId;
                const exists = currentRoom.textState.some(txt => txt.textId === textId);
                if (!exists) {
                    currentRoom.textState.push(msg);
                }
                currentRoom.lastActivity = Date.now();
                msg.oderId = clientId;
                broadcast(currentRoom, msg, ws);
            }
            else if (type === 'textUpdate') {
                const textId = msg.textId;
                for (let i = 0; i < currentRoom.textState.length; i++) {
                    if (currentRoom.textState[i].textId === textId) {
                        Object.assign(currentRoom.textState[i], msg);
                        break;
                    }
                }
                currentRoom.lastActivity = Date.now();
                msg.oderId = clientId;
                broadcast(currentRoom, msg, ws);
            }
            else if (type === 'textRemove') {
                const textId = msg.textId;
                currentRoom.textState = currentRoom.textState.filter(txt => txt.textId !== textId);
                currentRoom.lastActivity = Date.now();
                msg.oderId = clientId;
                broadcast(currentRoom, msg, ws);
            }
            else if (type === 'requestSync') {
                ws.send(JSON.stringify({
                    type: 'fullSync',
                    images: currentRoom.boardState,
                    texts: currentRoom.textState
                }));
            }
            else if (type === 'pushSync') {
                // Client pushing their local state
                let stateChanged = false;
                
                // Merge images
                const clientImages = msg.images || [];
                for (const img of clientImages) {
                    const exists = currentRoom.boardState.some(i => i.imageId === img.imageId);
                    if (!exists) {
                        currentRoom.boardState.push(img);
                        stateChanged = true;
                    }
                }
                
                // Merge texts
                const clientTexts = msg.texts || [];
                for (const txt of clientTexts) {
                    const exists = currentRoom.textState.some(t => t.textId === txt.textId);
                    if (!exists) {
                        currentRoom.textState.push(txt);
                        stateChanged = true;
                    }
                }
                
                currentRoom.lastActivity = Date.now();
                
                // Send full state back to pushing client
                ws.send(JSON.stringify({
                    type: 'fullSync',
                    images: currentRoom.boardState,
                    texts: currentRoom.textState
                }));
                
                // If state changed, broadcast to all others
                if (stateChanged) {
                    broadcast(currentRoom, {
                        type: 'fullSync',
                        images: currentRoom.boardState,
                        texts: currentRoom.textState
                    }, ws);
                }
            }
            else {
                // Forward unknown messages
                msg.oderId = clientId;
                broadcast(currentRoom, msg, ws);
            }
        } catch (err) {
            console.error('Message error:', err);
        }
    });
    
    ws.on('close', () => {
        console.log(`Client disconnected: ${clientId}`);
        
        if (currentRoom) {
            currentRoom.clients.delete(ws);
            currentRoom.lastActivity = Date.now();
            
            // Notify others
            broadcast(currentRoom, {
                type: 'leave',
                oderId: clientId,
                userName: clientName
            });
            
            console.log(`Room ${getRoomId(currentRoom)} now has ${currentRoom.clients.size} clients`);
        }
    });
    
    ws.on('error', (err) => {
        console.error('WebSocket error:', err);
    });
});

function broadcast(room, message, exclude = null) {
    const data = JSON.stringify(message);
    room.clients.forEach((info, client) => {
        if (client !== exclude && client.readyState === WebSocket.OPEN) {
            client.send(data);
        }
    });
}

function getRoomId(room) {
    for (const [id, r] of Object.entries(rooms)) {
        if (r === room) return id;
    }
    return 'unknown';
}

function generateId() {
    return Math.random().toString(36).substr(2, 8);
}

httpServer.listen(PORT, '0.0.0.0', () => {
    console.log(`CollabRef Server running on port ${PORT}`);
    console.log(`Health check: http://localhost:${PORT}/health`);
});
