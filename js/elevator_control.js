// visual elevator demo
// this does not send commands to hardware yet
// it only updates the webpage visuals and updates the DB

// send an elevator request to elevator_control.php
function sendElevatorRequest(floor, sourceController) {
    // format values like normal form data using a constructor of URLSearchParams() (big ass Web API magic)
    const requestData = new URLSearchParams();

    // add a named value  (floro) to the form-style request data ("requested_floor")
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

    .then(function (response) {
        return response.json();
    })
}

document.addEventListener("DOMContentLoaded", function () {
    const elevatorCar = document.getElementById("elevatorCar");
    const currentFloorDisplay = document.getElementById("currentFloorDisplay");
    const lastCommandDisplay = document.getElementById("lastCommandDisplay");
    const floorRows = document.querySelectorAll(".floor-row");
    const floorButtons = document.querySelectorAll(".floor-request-button");
    const carButtons = document.querySelectorAll(".car-floor-button");
    const carScreen = document.querySelector(".car-screen");

    // these positions match the current desktop tower layout
    // floor 1 is lowest, floor 3 is highest
    const carPositions = {
        "1": "-354px",
        "2": "-177px",
        "3": "0px"
    };

    // moves the elevator car to the desired floor based on the floor selected and source (floor controller/cab controller)
    function moveCarToFloor(floor, commandSource) {
        if (!elevatorCar) {
            return;
        }

        elevatorCar.style.bottom = carPositions[floor];
        currentFloorDisplay.textContent = floor;
        carScreen.textContent = floor;

        // add text to the "last command" box to show who called/moved the elevator
        if (commandSource === "floor") {
            lastCommandDisplay.textContent = "Floor " + floor + " requested the car";
        } else {
            lastCommandDisplay.textContent = "Cab selected Floor " + floor;
        }

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

    // move car to floor when a floor button is clicked
    /*
    floorButtons.forEach(function (button) {
        button.addEventListener("click", function () {
            moveCarToFloor(button.dataset.floor, "floor");
        });
    });
    */

    // replacing with a temporary function:
    floorButtons.forEach(function (button) {
        button.addEventListener("click", function () {
            const floor = button.dataset.floor;

            sendElevatorRequest(floor, "web_floor_station")
                .then(function (responseData) {
                    console.log(responseData);
                })
                .catch(function(error) {
                    console.error("Elevator request failed", error);
                });
        });
    });

    // move car to floor when a car controller button is clicked
    carButtons.forEach(function (button) {
        button.addEventListener("click", function () {
            moveCarToFloor(button.dataset.floor, "car");
        });
    });
});