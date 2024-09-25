const servoCount = document.getElementById('servoCount');
const servoPositions = document.getElementById('servoPositions');
const form = document.getElementById('blindsForm');
const factoryReset = document.getElementById('factoryReset');
const resetModal = document.getElementById('resetModal');
const confirmReset = document.getElementById('confirmReset');
const statusMessage = document.getElementById('statusMessage');

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
    statusMessage.textContent = message;
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

function loadExistingConfiguration(data) {
    document.getElementById('blindsName').value = data.blindsName;
    document.getElementById('servoCount').value = data.servoCount;
    updateServoPositions();
    
    // Set servo positions after a short delay to ensure elements are created
    setTimeout(() => {
        data.servoPositions.forEach((position, index) => {
            document.getElementById(`servo${index + 1}Position`).value = position;
        });
    }, 100);

    statusMessage.textContent = "Existing configuration loaded. You can modify if needed.";
}

servoCount.addEventListener('change', updateServoPositions);

function updateServoPositions() {
    const count = parseInt(servoCount.value);
    servoPositions.innerHTML = '';
    for (let i = 1; i <= count; i++) {
        const label = document.createElement('label');
        label.htmlFor = `servo${i}Position`;
        label.textContent = `Servo ${i} position:`;
        const select = document.createElement('select');
        select.id = `servo${i}Position`;
        select.name = `servo${i}Position`;
        ['Left', 'Right'].forEach(pos => {
            const option = document.createElement('option');
            option.value = pos.toLowerCase();
            option.textContent = pos;
            select.appendChild(option);
        });
        servoPositions.appendChild(label);
        servoPositions.appendChild(select);
    }
}

form.addEventListener('submit', function(e) {
    e.preventDefault();
    const formData = new FormData(form);
    const data = Object.fromEntries(formData.entries());
    socket.send(JSON.stringify({action: 'submit', data: data}));
});

factoryReset.addEventListener('click', function() {
    resetModal.style.display = 'block';
});

confirmReset.addEventListener('click', function() {
    const password = document.getElementById('resetPassword').value;
    socket.send(JSON.stringify({action: 'factoryReset', password: password}));
    resetModal.style.display = 'none';
});

window.onclick = function(event) {
    if (event.target == resetModal) {
        resetModal.style.display = 'none';
    }
}


// Handle incoming WebSocket messages
socket.addEventListener('message', function (event) {
    const message = event.data;
    if (typeof message === 'string' && message.startsWith('LOG :')) {
        console.log( message);
    } else {
        const data = JSON.parse(message);

        if ( data.action !== undefined){
            switch(data.action) {
                case 'status':
                    if (data.initialized) {
                        loadExistingConfiguration(data);
                    } else {
                        updateStatusMessage("Please initialize your blinds.");
                    }
                    break;
                case 'submitResponse':
                    updateStatusMessage(data.message);
                    checkInitializationStatus();
                    break;
                case 'factoryResetResponse':
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
    }
});

// Automatically close the page after 5 minutes
setTimeout(() => {
    alert('The web server is shutting down. This page will close.');
    window.close();
}, 5 * 60 * 1000);