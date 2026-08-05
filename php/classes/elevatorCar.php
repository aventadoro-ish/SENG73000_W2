<?php 

require_once __DIR__ . '/node.php';

class ElevatorCar extends Node {
    private $canID;
    private $currentFloor;
    private $doorsOpen;

    public function __construct(string $nodeName, string $canID, int $currentFloor = 1) {
        parent::__construct($nodeName);

        $this->canID = $canID;
        $this->currentFloor = $currentFloor;
        $this->doorsOpen = false;
    }


    public function getNodeType():string {
        return "elevator car";
    }

    public function getCANID():string {
        return $this->canID;
    }

    public function getCurrentFloor():int {
        return $this->currentFloor;
    }

    public function moveToFloor(int $floor) {
        if($floor < 1 || $floor > 3) {
            throw new Exception("floor must be between 1 and 3");
        }
        $this->currentFloor = $floor;
    }

    public function openDoors() {
        $this->doorsOpen = true;
    }

    public function closeDoors() {
        $this->doorsOpen = false;
    }

    public function areDoorsOpen():bool {
        return $this->doorsOpen;
    }





























}