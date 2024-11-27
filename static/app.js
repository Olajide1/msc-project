// Establish a WebSocket connection to the server
let socket;
let reconnectInterval = 1000;
const systemConfiguration = {
    data: {},
    isMonitoring: false,
    lastUpdate: null
};

const monitorToggle = document.querySelector("#monitor-toggle");
const monitorToggleSpan = document.querySelector("#monitor-toggle-span");
monitorToggle.checked = systemConfiguration.isMonitoring;

const getUIElementForNodeDevice = (nodeObject, nodeKey) => {
    const datetimeObj = new Date(nodeObject.timestamp);
    const minutes = datetimeObj.getMinutes() < 10 ? `0${datetimeObj.getMinutes()}` : datetimeObj.getMinutes();
    const seconds = datetimeObj.getSeconds() < 10 ? `0${datetimeObj.getSeconds()}` : datetimeObj.getSeconds();
    return `
        <div class="node-instance col-md-6 col-sm-12">
            <div class="row">
                <div class="col-12">
                    <div class="node-details-container">
                        <div class="node-details-wrapper ${ nodeObject.state === 1 ? 'alert alert-danger' : 'alert alert-success' }">
                            <div class="node-status-wrapper">
                                <div class="node-status-content">
                                    <h4 class="node-id">${nodeKey}</h4>
                                    <div id="node-status-indicator py-3">
                                        ${ nodeObject.state === 1 ? '<i class="fa-solid fa-land-mine-on fa-5x"></i>' : '<i class="fa-regular fa-circle-check fa-5x"></i>' }
                                    </div>
                                    <small class="mt-3">${ nodeObject.state === 1 ? 'FIRE SIGNAL DETECTED' : 'NORMAL SIGNAL DETECTED' }</small>
                                    <small class="small-text">LAST SIGNAL UPDATE TIME: ${datetimeObj.getHours()}:${minutes}:${seconds}</small>
                                </div>
                            </div>
                            <div class="metrics">
                                ${ generateFeaturesMetrics(nodeObject.features) }
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    `;
}

const generateFeaturesMetrics = (features) => {
    let featureList = "";

    Object.keys(features).map( key => {
        featureList += `
            <div class="progress blue">
                <span class="progress-left">
                    <span class="progress-bar"></span>
                </span>
                <span class="progress-right">
                    <span class="progress-bar"></span>
                </span>
                <div class="progress-value">
                    <span>${parseFloat(features[key]).toFixed(1)}</span>
                    <small>${key}</small>
                </div>
            </div>
        `
    })

    return featureList
}

const renderApplicationUIState = () => {
    const outputDiv = document.getElementById('output');
    let uiElemnts = "";

    if (!systemConfiguration.isMonitoring) {
        uiElemnts = `
            <div class="p-5 text-center bg-body-tertiary rounded-3">
                <h1 class="text-body-emphasis">FERS TURNED OFF!</h1>
                <p class="col-lg-8 mx-auto fs-5 text-muted">
                    Turn on the Moniter to run FERS System.
                </p>
            </div>
        `;
    } else {
        if (Object.keys(systemConfiguration.data).length > 0) {
            for (const key in systemConfiguration.data) {
                console.log(key);
                uiElemnts += getUIElementForNodeDevice(systemConfiguration.data[key], key)
            }
        } else {
            uiElemnts = `
                <div class="p-5 text-center bg-body-tertiary rounded-3">
                    <h1 class="text-body-emphasis">No IoT node is connected to FERS at time!!!</h1>
                    <p class="col-lg-8 mx-auto fs-5 text-muted">
                        Check network connectivity to ensure the system is work properly.
                    </p>
                </div>
            `;
        }
    }

    outputDiv.innerHTML = `${uiElemnts}`;
}

const createWebSocket = () => {
    const hostname = location.hostname;
    socket = new WebSocket(`ws://${hostname}:8000/ws`);

    socket.onopen = function(event) {
        console.log('WebSocket connection established.');
        reconnectInterval = 1000; // Reset the reconnection interval
    };

    socket.onmessage = function(event) {
        console.log(JSON.parse(event.data));
        try {
            if (Object.keys(event.data).length > 0) {
                systemConfiguration.data = JSON.parse(event.data); // Parse the JSON data from the server
            } else {
                systemConfiguration.data = {};
            }
        } catch(err) {
            systemConfiguration.data = {};
            console.log(err);
        }
        systemConfiguration.lastUpdate = Date.now(); // Record the timestamp of the last update
        // Update the UI immediately
        renderApplicationUIState();
    };

    socket.onclose = function(event) {
        console.log('WebSocket connection closed. Attempting to reconnect...');
        // Reconnect after a delay
        setTimeout(createWebSocket, reconnectInterval);
        // Incrementally increase the reconnection interval
        reconnectInterval = Math.min(reconnectInterval * 2, 30000); // Cap at 30 seconds
    };

    socket.onerror = function(error) {
        console.error('WebSocket error:', error);
    };
}

// Periodic status check
setInterval(() => {
    const outputDiv = document.getElementById('output');
    const now = Date.now();

    if (!systemConfiguration.lastUpdate || now - systemConfiguration.lastUpdate > 20000) {
        uiElemnts = `
            <div class="p-5 text-center bg-body-tertiary rounded-3">
                <h1 class="text-body-emphasis">No IoT node is connected to FERS at time!!!</h1>
                <p class="col-lg-8 mx-auto fs-5 text-muted">
                    Check network connectivity to ensure the system is work properly.
                </p>
            </div>
        `;
    
        if (systemConfiguration.isMonitoring) {
            // Show a status message if no data received in the last 5 seconds
            outputDiv.innerHTML = uiElemnts;
        }
    }
    console.log("Status check performed.");
}, 2000);

monitorToggle.addEventListener('click', (event) => {
    systemConfiguration.isMonitoring = event.target.checked;
    renderApplicationUIState();
});

function updateSystemWatch() {
    const watchElement = document.getElementById('system-watch');

    // Get the current time and format it
    const now = new Date();
    const formattedTime = now.toLocaleTimeString(); // e.g., "3:45:12 PM"
    const formattedDate = now.toLocaleDateString(); // e.g., "11/25/2024"

    // Display the time and date in the watch element
    watchElement.textContent = `${formattedTime}`;
}

setInterval(updateSystemWatch, 1000);
updateSystemWatch();
createWebSocket();