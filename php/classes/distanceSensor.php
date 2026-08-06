<?php

require_once __DIR__ . '/node.php';
require_once __DIR__ . '/exceptions/nodeInputException.php';
class DistanceSensor extends Node {
    private $distanceMm;

    public function __construct(string $nodeName, float $distanceMm = 0.0)
    {
        if ($distanceMm < 0) {
            throw new NodeInputException("distance cannot be negative");
        }
        
        parent::__construct($nodeName);
        $this->setDistanceMm($distanceMm);
    }

    public function getNodeType(): string
    {
        return "distance sensor";
    }

    public function getDistanceMm(): float
    {
        return $this->distanceMm;
    }

    public function setDistanceMm(float $distanceMm)
    {
        if($distanceMm < 0) {
            throw new NodeInputException("distance cannot be negative");
        }

        $this->distanceMm = $distanceMm;
    }
}