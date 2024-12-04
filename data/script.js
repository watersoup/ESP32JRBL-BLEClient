const socket = new WebSocket('ws://' + window.location.hostname + '/ws');

// Promise that resolves when the WebSocket is opened
const socketOpenPromise = new Promise((resolve) => {
    socket.addEventListener('open', (event) => {
        console.log('WebSocket connection opened');
        resolve();
    });
});

// Function to update status message
function updateStatusMessage(message) {
    const statusMessage = document.getElementById('statusMessage');
    if (statusMessage) {
        statusMessage.textContent = message;
    }
}

// Check server connection and initialization status on page load
window.addEventListener('load', initializeConnection);

function initializeConnection() {
    updateStatusMessage("Connecting to server...");
    
    socketOpenPromise.then(() => {
        updateStatusMessage("Connected to server. Checking initialization status...");
        checkInitializationStatus();
    }).catch(error => {
        console.error('WebSocket connection error:', error);
        updateStatusMessage("Error connecting to the server. Please check your connection and refresh the page.");
    });
}

function checkInitializationStatus() {
    socket.send(JSON.stringify({action: 'getStatus'}));
}

function handleWebSocketMessage(event) {
    const message = event.data;
    if (typeof message === 'string' && message.startsWith('LOG :')) {
        console.log(message);
    } else {
        const data = JSON.parse(message);

        if (data.action !== undefined) {
            switch(data.action) {
                case 'status':
                    if (data.initialized) {
                        console.log('Blinds initialized');
                    } else {
                        updateStatusMessage("Please initialize your blinds.");
                        return;
                    }
                    break;
                case 'submitResponse':
                    updateStatusMessage(data.message);
                    checkInitializationStatus();
                    break;
                case 'LOG :':
                    updateStatusMessage(data.message);
                    break;
                default:
                    console.log('Unknown action:', data.action);
            }
        }

        // Handle index.html specific messages
        if (data.blindsName !== undefined) {
            document.getElementById('blindName').textContent = data.blindsName;
        }

        if (data.sliderPosition !== undefined) {
            document.getElementById('blindSlider').value = data.sliderPosition;
            updateHandlePosition(data.sliderPosition);
        }

        if (data.status !== undefined) {
            isOpen = data.status === 1;
            document.getElementById('toggleBtn').textContent = isOpen ? 'Close' : 'Open';
            updateToggleButton();
        }

        if (data.limitSetupFlag !== undefined) {
            document.getElementById('setupBtn').disabled = data.limitSetupFlag === 3;
        }

    }
}

socket.addEventListener('message', handleWebSocketMessage);


function toggleBlind() {
    if ( isOpen){
        socket.send(JSON.stringify({action: 'CLOSE'}));
    } else {    
        socket.send(JSON.stringify({action: 'OPEN'}));
    }
}

function updateToggleButton() {
    const toggleBtn = document.getElementById('toggleBtn');
    if (isOpen) {
        toggleBtn.textContent = 'Close';
        toggleBtn.style.backgroundColor = 'red';
        toggleBtn.style.color = 'white';
    } else {
        toggleBtn.textContent = 'Open';
        toggleBtn.style.backgroundColor = 'green';
        toggleBtn.style.color = 'black';
    }
}

function setupLimits() {
    socket.send(JSON.stringify({ action: 'setupLimits'}));
}

function updateSlider(value) {
    updateHandlePosition(value);
    socket.send(JSON.stringify({ action: 'SP', position: parseInt(value) }));
}

function updateHandlePosition(value) {
    const handle = document.getElementById('sliderHandle');
    if (handle) {
        const position = value;  // Direct representation
        handle.style.left = `${position}%`;
    }
}