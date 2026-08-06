<?php

require_once __DIR__ . '/node.php';
require_once __DIR__ . '/CANDevice.php';
require_once __DIR__ . '/CANIdentifierTrait.php';
require_once __DIR__ . '/exceptions/nodeInputException.php';
class FloorNode extends Node implements CANDevice {
    use CANIdentifierTrait;
    private $floorNumber;
    private $requestActive;

    public function __construct(string $nodeName, string $canID, int $floorNumber)
    {
        if($floorNumber < 1 || $floorNumber > 3) {
            throw new NodeInputException("floor must be between 1 & 3");
        }

        parent::__construct($nodeName);

        $this->canID = $canID;
        $this->floorNumber = $floorNumber;
        $this->requestActive = false;
    }

    public function getNodeType(): string
    {
        return "Floor node";
    }

    public function getFloorNumber(): int
    {
        return $this->floorNumber;
    }

    public function requestElevator()
    {
        $this->requestActive = true;
    }

    public function clearRequest()
    {
        $this->requestActive = false;
    }

    public function hasActiveRequest(): bool
    {
        return $this->requestActive;
    }
}