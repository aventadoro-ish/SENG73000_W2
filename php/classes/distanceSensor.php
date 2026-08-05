<?php

require_once __DIR__ . '/Node.php';

class DistanceSensor extends Node
{
    private $distanceMm;

    public function __construct(string $nodeName, float $distanceMm = 0.0)
    {
        parent::__construct($nodeName);
        $this->distanceMm = $distanceMm;
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
            throw new Exception("distance cannot be negative");
        }

        $this->distanceMm = $distanceMm;
    }
}