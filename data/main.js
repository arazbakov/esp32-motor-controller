document.addEventListener('DOMContentLoaded', function () {
    const elementStatusBadge = document.querySelector("#statusBadge");
    const elementXMinus = document.querySelector("#xMinus");
    const elementXPlus = document.querySelector("#xPlus");
    const elementYMinus = document.querySelector("#yMinus");
    const elementYPlus = document.querySelector("#yPlus");
    const elementZMinus = document.querySelector("#zMinus");
    const elementZPlus = document.querySelector("#zPlus");
    const elementAMinus = document.querySelector("#aMinus");
    const elementAPlus = document.querySelector("#aPlus");
    const elementXValue = document.querySelector("#xValue");
    const elementYValue = document.querySelector("#yValue");
    const elementZValue = document.querySelector("#zValue");
    const elementAValue = document.querySelector("#aValue");
    const elementControlsPanel = document.querySelector("#controlsPanel");
    const elementServerStatusList = document.querySelector("#serverStatusList");

    let isConnected = false;

    let xPosition = 0;
    let yPosition = 0;
    let zPosition = 0;
    let rotationAngle = 0;

    let currentSocket = null;

    const setServerStateParam = (title, value) => {
        let elementRow = elementServerStatusList.querySelector("[name=\"" + title + "\"]");
        if (!elementRow) {
            elementRow = document.createElement('div');
            elementRow.setAttribute("name", title);
            elementRow.className = 'row';

            const elementTitle = document.createElement('div');
            elementTitle.className = 'col param-title text-muted';
            elementTitle.textContent = title;
            
            const elementValue = document.createElement('div');
            elementValue.className = 'col-8 param-value';
            elementValue.textContent = value;
            
            elementRow.appendChild(elementTitle);
            elementRow.appendChild(elementValue);
            
            elementServerStatusList.appendChild(elementRow);
        } else {
            elementRow.querySelector(".param-value").textContent = value;
        }
    }

    /**
     * @param {{[uptime: number], [startedAt: number]}} state 
     */
    const updateServerState = (state) => {
        if (state.startedAt) {
            const startDate = new Date(state.startedAt * 1000);
            setServerStateParam("Start time", `${startDate.getDate()}.${startDate.getMonth() + 1}.${startDate.getFullYear()} ${startDate.getHours()}:${startDate.getMinutes()}:${startDate.getSeconds()}`);
            delete state.startedAt;
        }
        if (state.uptime) {
            var uptimeValue = Math.floor(state.uptime / 1000);
            var daysValue = Math.floor(uptimeValue / 60 / 60 / 24);
            var hoursValue = Math.floor(uptimeValue / 60 / 60) - daysValue * 24;
            var minutesValue = Math.floor(uptimeValue / 60) - daysValue * 24 - hoursValue * 60;
            var secondsValue = uptimeValue - daysValue * 24 - hoursValue * 60 - minutesValue * 60;

            setServerStateParam("Uptime", [
                [daysValue, 'd'],
                [hoursValue, 'h'],
                [minutesValue, 'm'],
                [secondsValue, 's']
            ].filter( item => item[0] !== 0).map(item => item[0] + item[1]).join(' '));
            delete state.uptime;
        }

        // output left unformatted values:
        Object.entries(state).forEach(([key, value]) => setServerStateParam(key, value));
    }

    const connect = () => {
        const socket = new WebSocket(`ws://${window.location.hostname}/ws`);
        currentSocket = socket;
        elementStatusBadge.innerHTML = "Connecting...";
        elementStatusBadge.className = "badge badge-secondary";

        // Connection opened
        socket.addEventListener('open', event => {
            //socket.send('Hello Server!');
            console.log('Connected to WS server');
            isConnected = true;
            elementControlsPanel.classList.remove("disabled-panel");
            elementStatusBadge.innerHTML = "Connected";
            elementStatusBadge.className = "badge badge-success";
        });

        // Listen for messages
        socket.addEventListener('message', event => {
            const message = JSON.parse(event.data);
            switch (message.type) {
                case 'state':
                    if (message.x !== undefined) {
                        xPosition = parseFloat(message.x);
                    }
                    if (message.y !== undefined) {
                        yPosition = parseFloat(message.y);
                    }
                    if (message.z !== undefined) {
                        zPosition = parseFloat(message.z);
                    }
                    if (message.a !== undefined) {
                        rotationAngle = parseFloat(message.a);
                    }
                    elementXValue.value = xPosition.toFixed(1);
                    elementYValue.value = yPosition.toFixed(1);
                    elementZValue.value = zPosition.toFixed(1);
                    elementAValue.value = rotationAngle.toFixed(1);
                    
                    break;
            }

            if (message.status) {
                updateServerState(message.status);
            }
        });

        socket.addEventListener('error', event => {
            console.log("WS error: ", event.message, " Closing socket");
            socket.close();
        });

        socket.addEventListener('close', event => {
            elementStatusBadge.innerHTML = "Disconnected";
            elementStatusBadge.className = "badge badge-warning";
            elementControlsPanel.classList.add("disabled-panel");
            isConnected = false;
            currentSocket = null;
            setTimeout(() => {
                connect();
            }, 5000);
        });

        window.ws = currentSocket;
    }

    connect();

    elementXMinus.addEventListener("click", e => {
        event.preventDefault();
        currentSocket && currentSocket.send("appendX-1");
    });
    elementXPlus.addEventListener("click", e => {
        event.preventDefault();
        currentSocket && currentSocket.send("appendX1");
    });

    elementYMinus.addEventListener("click", e => {
        event.preventDefault();
        currentSocket && currentSocket.send("appendY-1");
    });
    elementYPlus.addEventListener("click", e => {
        event.preventDefault();
        currentSocket && currentSocket.send("appendY1");
    });

    elementZMinus.addEventListener("click", e => {
        event.preventDefault();
        currentSocket && currentSocket.send("appendZ-1");
    });
    elementZPlus.addEventListener("click", e => {
        event.preventDefault();
        currentSocket && currentSocket.send("appendZ1");
    });

    elementAMinus.addEventListener("click", e => {
        event.preventDefault();
        currentSocket && currentSocket.send("appendA-1");
    });
    elementAPlus.addEventListener("click", e => {
        event.preventDefault();
        currentSocket && currentSocket.send("appendA1");
    });

    elementXValue.addEventListener("change", event => {
        const value = event.srcElement.value;
        if (!isNaN(parseFloat(value))) {
            currentSocket && currentSocket.send("set X " + event.srcElement.value);
        }
    });
    elementYValue.addEventListener("change", event => {
        const value = event.srcElement.value;
        if (!isNaN(parseFloat(value))) {
            currentSocket && currentSocket.send("set Y " + event.srcElement.value);
        }
    });
    elementZValue.addEventListener("change", event => {
        const value = event.srcElement.value;
        if (!isNaN(parseFloat(value))) {
            currentSocket && currentSocket.send("set Z " + event.srcElement.value);
        }
    });
    elementAValue.addEventListener("change", event => {
        const value = event.srcElement.value;
        if (!isNaN(parseFloat(value))) {
            currentSocket && currentSocket.send("set A " + event.srcElement.value);
        }
    });
});