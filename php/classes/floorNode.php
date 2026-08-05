<?php

require_once __DIR__ . '/Node.php';

class FloorNode extends Node
{
    private $canID;
    private $floorNumber;
    private $requestActive;

    public function __construct(string $nodeName, string $canID, int $floorNumber)
    {
        parent::__construct($nodeName);

        $this->canID = $canID;
        $this->floorNumber = $floorNumber;
        $this->requestActive = false;
    }

    public function getNodeType(): string
    {
        return "Floor Node";
    }

    public function getCANID(): string
    {
        return $this->canID;
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