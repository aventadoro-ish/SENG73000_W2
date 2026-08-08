// visual elevator demo
// this does not send commands to hardware yet
// it only updates the webpage visuals and updates the DB

document.addEventListener("DOMContentLoaded", function () {
    // an HTML is talking, listen and learn 🤫
    const elevatorCar = document.getElementById("elevatorCar");
    const currentFloorDisplay = document.getElementById("currentFloorDisplay");
    const lastCommandDisplay = document.getElementById("lastCommandDisplay");
    const floorRows = document.querySelectorAll(".floor-row");
    const floorButtons = document.querySelectorAll(".floor-request-button");
    const carButtons = document.querySelectorAll(".car-floor-button");
    const carScreen = document.querySelector(".car-screen");

    // read the initial values in <main> of elevator_control.php
    const elevatorControlPage = document.getElementById("elevatorControlPage");
    const initialFloor = elevatorControlPage.dataset.initialFloor;
    const initialRequestID = elevatorControlPage.dataset.initialRequestId;
    const initialSource = elevatorControlPage.dataset.initialSource;
    const initialOperationMode = elevatorControlPage.dataset.operationMode;

    // for handling the doors status and label of said status
    const doorToggleButton = document.getElementById("doorToggleButton");
    const doorStatusDisplay = document.getElementById("doorStatusDisplay");
    const doorControlPanel = document.querySelector(".door-control-panel");
    let doorsOpen = false;

    // sabbath mode
    const sabbathToggle = document.getElementById("sabbathToggle");
    let sabbathEnabled = false;

    // retrieve the maintenance button from the HTML
    const maintenanceToggle = document.getElementById("maintenanceToggle");

    // remember whether maintenance mode is currently active
    let maintenanceEnabled = false;
    


    // these positions match the current desktop tower layout
    // floor 1 is lowest, floor 3 is highest
    const carPositions = {
        "1": "-315px",
        "2": "-155px",
        "3": "12px"

    };


    // send an elevator request to elevator_control.php
    function sendElevatorRequest(floor, sourceController) {
        // format values like normal form data using a constructor of URLSearchParams() (big ass Web API magic)
        const requestData = new URLSearchParams();

        // add a named value (floor) to the form-style request data ("requested_floor")
        requestData.append("requested_floor", floor);
        requestData.append("source_controller", sourceController);

        // send an HTTP request without reloading the page
        // POST sends the data to the PHP; content-type formats it like an HTML form;
        // body is the actual floor/source values; response.json converts PHPs JSON response into a JS object
        return fetch("elevator_control.php" , {
            method: "POST",
            headers: {"Content-Type": "application/x-www-form-urlencoded"}, 
            body: requestData.toString()
        })

        // a returned Promise which decodes PHP's JSON into a JS Object (stored in 'response')
        .then(function (response) {
            return response.json();
        })
    }

    // function to handle sending an AJAX req to PHP for door state
    function sendDoorState (doorState) {

        // format the value like a normal submitted HTML form (the humble AJAX)
        const requestData = new URLSearchParams();

        // tell PHP where to run the code ("door" works with door state)
        requestData.append("request_action", "door");

        // doorState will contain either open or close so append it
        requestData.append("door_state", doorState);

        // send values to PHP without reloading page
        return fetch("elevator_control.php", {
            method: "POST",
            headers: {"Content-Type": "application/x-www-form-urlencoded"},
            body: requestData.toString()
        })
        
        // when PHP responds, decode JSON to JS Object to handle it
        .then (function (response) {
            return response.json();
        });
    }

    // function to send the Sabbath toggle HTTP request
    function sendSabbathState(sabbathState) {
        const requestData = new URLSearchParams();

        // tell PHP which POST to use
        requestData.append("request_action", "sabbath");

        // contains either enabled or disabled
        requestData.append("sabbath_state", sabbathState);

        // send the request
        return fetch ("elevator_control.php", {
            method: "POST",
            headers: {"Content-Type": "application/x-www-form-urlencoded"},
            body: requestData.toString()
        })

        // when PHP responds, decode JSON to JS Object to handle it
        .then (function (response) {
            return response.json();
        });
    }

    // send the requested maintenance state to PHP
    function sendMaintenanceState(maintenanceState) {

        // create form-style POST data
        const requestData = new URLSearchParams();

        // tell PHP to run its maintenance branch
        requestData.append("request_action", "maintenance");

        // send either "enabled" or "disabled"
        requestData.append(
            "maintenance_state",
            maintenanceState
        );

        // send the request without reloading the webpage
        return fetch("elevator_control.php", {
            method: "POST",
            headers: {"Content-Type":"application/x-www-form-urlencoded"},
            body: requestData.toString()
        })

        // convert PHP's JSON response into a JavaScript object
        .then(function (response) {
            return response.json();
        }); 
    }

// handle PHP's response after an elevator request is submitted
function handleResponse(responseData) {

    // PHP successfuly validated and stored the request
    if(responseData.success === true) {

        // PHP returns the requested floor as a number
        const floor = String(responseData.requested_floor);

        // doors prevent movement
        if(responseData.movement_allowed === false) {

            // synchronize the webpage with the DB door state
            updateDoorDisplay(true);

            lastCommandDisplay.textContent = "Request #" + responseData.elevator_request_id + " logged for floor " + floor + " - cannot move with doors open";

            return;
        }

        // the request is queued, but the EC has not confirmed movement
        lastCommandDisplay.textContent = "Request #" + responseData.elevator_request_id + " logged for floor " + floor + " - waiting for elevator";

        } else {
        lastCommandDisplay.textContent = "Request rejected: " + responseData.message;
        }
    }

    // handle PHP response to door state
    function handleDoorResponse(responseData) {
        if(responseData.success === true)
        {
            const confirmedDoorsOpen = responseData.door_state === "open";
            

            // update the webpage when door state was changed
            updateDoorDisplay(confirmedDoorsOpen);

            // update the last command text
            lastCommandDisplay.textContent = "Doors changed to " + responseData.door_state;

            // tell Node PHP changed the door state
            if(elevatorSocket.readyState === WebSocket.OPEN) {
                elevatorSocket.send(JSON.stringify({
                    type: "door_state_changed"
                }));
            }

        } else {
            // PHP responded but SQL failed
            // update the last command text
            lastCommandDisplay.textContent = "Door update rejected " + responseData.message;
        }
    } 

    // handle sabbath mode
    function handleSabbathResponse(responseData) {
        if(responseData.success === true) {
            // change operation mode as needed
            const operationMode = responseData.sabbath_state === "enabled" ? "sabbath" : "normal";

            // update the webpage display
            updateOperationModeDisplay(operationMode);

            // print last command
            lastCommandDisplay.textContent = "Sabbath mode " + responseData.sabbath_state;

        } else {
            // print last command
            lastCommandDisplay.textContent = "Sabbath update rejected: " + responseData.message;
        }
    }

    // handle maintenance mode
    function handleMaintenanceResponse(responseData) {
        if(responseData.success === true) {
            // change operation mode as needed
            const operationMode = responseData.maintenance_state === "enabled" ? "maintenance" : "normal";

                // update the webpage display
                updateOperationModeDisplay(operationMode);

                // print last command
                lastCommandDisplay.textContent = "Maintenance mode " + responseData.maintenance_state;

            } else {
                // print last command
                lastCommandDisplay.textContent = "Maintenance update rejected: " + responseData.message;
            }
        }

    function handleSabbathFailure(error) {
        lastCommandDisplay.textContent = "The Sabbath request could not be sent";

        console.error("Sabbath request failed", error);
    }

    // handle an AJAX failure - network failure, invalid JSON response, etc.
    function handleRequestFail(error) {
        lastCommandDisplay.textContent = "the elevator request could not be sent";

        console.error("elevator request failed due to: ", error);
    }

    // handle a network failure or invalid JSON response
    function handleMaintenanceFailure(error) {

        // display an error on the webpage
        lastCommandDisplay.textContent = "The maintenance request could not be sent";

        // print the full error in the browser console
        console.error("Maintenance request failed", error);
    }

    // moves the elevator car to the desired floor based on the floor selected and source (floor controller/cab controller)
    function moveCarToFloor(floor, commandSource) {
        if (!elevatorCar) {
            return;
        }

        elevatorCar.style.bottom = carPositions[floor];
        currentFloorDisplay.textContent = floor;
        carScreen.textContent = floor;

        floorRows.forEach(function (row) {
            row.classList.remove("active-floor");

            if (row.dataset.floor === floor) {
                row.classList.add("active-floor");
            }
        });

        carButtons.forEach(function (button) {
            button.classList.remove("active-car-button");

            if (button.dataset.floor === floor) {
                button.classList.add("active-car-button");
            }
        });
    }

// display newest request when the page loads
if (initialRequestID !== "") {
    lastCommandDisplay.textContent = "request #" + initialRequestID + " for floor " + initialFloor;
}

    floorButtons.forEach(function (button) {
        // give this specific button (in the loop) instructions for what to do when clicked
        button.addEventListener("click", function () {

            // we already have the specific button from the forEach loop
            const floor = button.dataset.floor;

            lastCommandDisplay.textContent = "Sending request for Floor " + floor + "...";

            // send the request and wait for PHP's answer
            sendElevatorRequest(floor, "web_floor_station")

            // PHP responded and its JSON was decoded
            .then(handleResponse)

            // the network request or JSON decoding failed
            .catch(handleRequestFail);
        });
    });

    // replace old car call with new one:
    carButtons.forEach(function (button) {
        // give this specific button (in the loop) instructions for what to do when clicked
        button.addEventListener("click", function() {

            // we already have the specific button from the forEach loop
            const floor = button.dataset.floor;

            // update command display
            lastCommandDisplay.textContent = "sending car-controller request  for floor " + floor + " ...";

            // use existing AJAX function but identify source as car controller
            sendElevatorRequest(floor, "web_car_controller")

                // once PHP responded and its JSON is decoded
                .then (handleResponse)

                // network request or decoding failed
                .catch (handleRequestFail);
        });
    });

    // update button after SQL success
    function updateSabbathDisplay (newSabbathState) {
        sabbathEnabled = newSabbathState;

        if(sabbathEnabled === true) {
            sabbathToggle.textContent = "Disable Sabbath mode";
            sabbathToggle.classList.add("active");
        } else {
            sabbathToggle.textContent = "Enable Sabbath mode";
            sabbathToggle.classList.remove("active");
        }
    }

    // update the appearance of the maintenance button
    function updateMaintenanceDisplay(newMaintenanceState) {

        if(!maintenanceToggle) {
            return;
        }

        // save the current maintenance state
        maintenanceEnabled = newMaintenanceState;

        // check whether maintenance mode is active
        if(maintenanceEnabled === true) {
            // the next click will disable maintenance
            maintenanceToggle.textContent = "Disable Maintenance mode";

            // add the same active styling used by Sabbath
            maintenanceToggle.classList.add("active");

        } else {
            // the next click will enable maintenance
            maintenanceToggle.textContent = "Enable Maintenance mode";

            // remove the active styling
            maintenanceToggle.classList.remove("active");
        }
    }

    // doors toggle function (between closed and open)
    function updateDoorDisplay(newDoorState) {
        doorsOpen = newDoorState;

        // if the doors are closed, open them
        if(doorsOpen === true) {
            doorStatusDisplay.textContent = "Open";
            doorToggleButton.textContent = "Close Doors";
            
            // adds the CSS class to the entire moving elevator car.
            elevatorCar.classList.add("doors-open");

        // the doors are open, close them
        } else {
            doorStatusDisplay.textContent = "Closed";
            doorToggleButton.textContent = "Open Doors";

            // Remove the open-door class.
            elevatorCar.classList.remove("doors-open");
        }
    }

    // synchronize both buttons with operation_mode from the DB
    function updateOperationModeDisplay(operationMode) {
        // Sabbath is active only when the mode is "sabbath"
        updateSabbathDisplay(operationMode === "sabbath");

        // maintenance is active only when the mode is "maintenance"
        updateMaintenanceDisplay(operationMode === "maintenance");
    }


    // read the door state that PHP placed in the HTML (String)
    const initialDoorState = doorControlPanel.dataset.doorState;

    // convert string into a true ("closed" is false)
    const initialDoorsOpen = initialDoorState === "open";

    // update door-related elements with the data
    updateDoorDisplay(initialDoorsOpen);

    // initialize both buttons using the current database mode
    updateOperationModeDisplay(initialOperationMode || "normal");


    // decide which door state should be requested
    function toggleDoors() {
        let requestedDoorState = "open";

        // if the doors are already open, request that they close.
        if (doorsOpen === true) {
            requestedDoorState = "closed";
        }

        lastCommandDisplay.textContent = "Requesting doors " + requestedDoorState + "...";

        // ask PHP to update DB
        sendDoorState(requestedDoorState)

        // DB update succeeded or PHP returned a validation failure
        .then(handleDoorResponse)

        // network request or JSON decoding failure
        .catch(handleRequestFail);
    }   

    // wait for the toggle door switch to be clicked
    doorToggleButton.addEventListener("click",  toggleDoors);

    // handle the sabbath mode and call the respective functions based on sucess/failure
    function toggleSabbathMode () {
        let requestedState = "enabled";

        if(sabbathEnabled === true) {
            requestedState = "disabled";
        } 

        lastCommandDisplay.textContent = "Requesting Sabbath mode " + requestedState + "...";

        sendSabbathState(requestedState)
            .then(handleSabbathResponse)
            .catch(handleSabbathFailure);
    }

    // wait for sabbath toggle to be clicked
    sabbathToggle.addEventListener("click", toggleSabbathMode);

    // decide whether maintenance should be enabled or disabled
    function toggleMaintenanceMode() {
        // assume the user wants to enable maintenance
        let requestedState = "enabled";

        // if it is already enabled, the next click disables it
        if(maintenanceEnabled === true) 
        {
            requestedState = "disabled";
        }

        // tell the user that the request is being processed
        lastCommandDisplay.textContent = "Requesting Maintenance mode " + requestedState + "...";

        // send the request to PHP
        sendMaintenanceState(requestedState)

            // handle a valid response from PHP
            .then(handleMaintenanceResponse)

            // handle a network or JSON error
            .catch(handleMaintenanceFailure);
    }

    if(maintenanceToggle) {
        // run toggleMaintenanceMode whenever the button is clicked
        maintenanceToggle.addEventListener("click", toggleMaintenanceMode);
    }


// remember the last floor reported by EC
// starts empty because a refreshed page has not restored its state yet
let lastConfirmedFloor = "";

// switches to true if a new EC position arrives through the websocket while the page-load database request is still running
let liveEcPositionReceived = false;

// Restore the latest EC-confirmed floor when the page first loads
// The websocket will only deliver messages created while this page is connected, but the page should load correctly once
function loadInitialElevatorStatus() {
    fetch(
        "elevator_control.php?request_action=status",
        {
            method: "GET",
            cache: "no-store"
        }
    )

    // reject HTTP errors before attempting to decode the response as JSON
    .then(function (response) {
        // error
        if(!response.ok) {
            throw new Error("HTTP status " + response.status);
        }
        // success
        return response.json();
    })

    .then(function (responseData) {
        // PHP reports success=false if its database query failed
        if(responseData.success !== true) {
            console.warn(
                "Initial elevator status was unavailable:",
                responseData.message
            );

            return;
        }

        // a live EC report is newer than the startup request, so keep it
        if(liveEcPositionReceived === true) {
            return;
        }

        // there may be no EC position row yet in a new/empty database
        if(responseData.position_available !== true) {
            console.log("No saved EC position is available yet");
            return;
        }

        // floor keys in this file are strings: "1", "2", or "3"
        const floor = String(responseData.current_floor);

        // ignore missing or unsupported floor values from the server
        if(!carPositions[floor]) {
            console.warn("Ignored invalid saved elevator floor:", responseData.current_floor);

            return;
        }

        // position the new page at the latest EC-confirmed physical floor
        moveCarToFloor(floor, "EC");
        lastConfirmedFloor = floor;

        console.log("Restored EC-confirmed Floor " + floor + " when the page loaded");
    })

    // a temporary failure should not stop the live WebSocket connection
    .catch(function (error) {
        console.error("Initial elevator status request failed:", error);
    });
}


// map each complete EC position byte to its corresponding floor
const floorByByte = {
    5: "1",
    6: "2",
    7: "3"
};

// open a connection to the Websocket server
const elevatorSocket = new WebSocket("ws://" + window.location.hostname + ":8080");

// run this event after browser connected to node
elevatorSocket.addEventListener("open", function () {
     console.log("Connected to LiF elevator websocket");
});

// this event runs whenever Node sends data to the webpage
elevatorSocket.addEventListener("message", function (event) {
    let message;

    try {
        // Node sends JSON text through the websocket
        // convert text into JS object
        message = JSON.parse(event.data);
    } catch (error) {
        // ignore bad data
        console.error("invalid websocket message received: ", event.data);
        return;
    }

    if(message.type === "elevator_state") {
        updateDoorDisplay(message.doors_open === true);
        return;
    }

    if(message.type === "elevator_state") {
        updateDoorDisplay(message.doors_open === true);
        return;
    }


    // ignore any web socket messages that aren't of "can_message"
    if(message.type !== "can_message") {
        return;
    }

    // websocket broadcasts every new CAN-log row
    // only incoming EC messages are allowed to update elevator
     const isEC = message.can_id === "0x101" && message.direction === "rx" && message.source_controller === "EC";

     if(!isEC) {
        return;
     }

     // convert raw_byte to a number in case it arrives as JSON text
     const rawByte = Number(message.raw_byte);

    // Look up the floor associated with this complete EC byte.
    const floor = floorByByte[rawByte];

     // an undefinedfloor means this EC byte is not a position report
     if(!floor) {
        console.log("ignored non-position providing EC byte: ", rawByte);
        return;
     }

     if(doorsOpen === true) {
        lastCommandDisplay.textContent = "Ignored EC floor " + floor + " from CAN log #" + message.log_id + " - cannot move with doors open";

        console.warn("ignored EC floor confirmation because the doors are open");

        return;
     } 

     liveEcPositionReceived = true;

     // do not move the elevator car if the EC reports the same floor
     if(floor === lastConfirmedFloor) {
        return;
     }

     // move the existing webpage elevator the EC-confirmed floor
     moveCarToFloor(floor, "EC");

     // remember the last confirmed position
     lastConfirmedFloor = floor;

     // show which DB entry caused the webpage update:
     lastCommandDisplay.textContent = "EC confirmed floor " + floor + " from CAN log #" + message.log_id;

});

// this code occurs if Node stops or network connection is lost
elevatorSocket.addEventListener("close", function(){
    console.warn("LiF elevator Websocket disconnected");
});

// report connection-level websocket errors to browser
elevatorSocket.addEventListener("error", function(error) {
     console.error("Lif Elevator websocket error: ", error);
});

loadInitialElevatorStatus();
});


