<?php 

require_once __DIR__ . '/node.php';
require_once __DIR__ . '/CANDevice.php';
require_once __DIR__ . '/CANIdentifierTrait.php';
require_once __DIR__ . '/exceptions/nodeInputException.php';

class ElevatorCar extends Node implements CANDevice {
    
    use CANIdentifierTrait;

    private $currentFloor;
    private $doorsOpen;

    public function __construct(string $nodeName, string $canID, int $currentFloor = 1) {
        if($currentFloor < 1 || $currentFloor > 3) {
            throw new NodeInputException("floor must be between 1 and 3");
        }    
        
        parent::__construct($nodeName);

        $this->canID = $canID;
        $this->doorsOpen = false;
        $this->confirmCurrentFloor($currentFloor);
    }


    public function getNodeType():string {
        return "elevator car";
    }

    public function getCurrentFloor():int {
        return $this->currentFloor;
    }

    public function confirmCurrentFloor(int $floor) {
        if($floor < 1 || $floor > 3) {
            throw new NodeInputException("floor must be between 1 and 3");
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

    public function canMove(): bool
    {
        return $this->isOnline() && !$this->doorsOpen;
    }

}