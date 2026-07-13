// visual elevator demo
// this does not send commands to hardware yet. It only updates the webpage visuals
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
    floorButtons.forEach(function (button) {
        button.addEventListener("click", function () {
            moveCarToFloor(button.dataset.floor, "floor");
        });
    });

    // move car to floor when a car controller button is clicked
    carButtons.forEach(function (button) {
        button.addEventListener("click", function () {
            moveCarToFloor(button.dataset.floor, "car");
        });
    });
});