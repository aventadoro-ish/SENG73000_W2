<?php

trait CANIdentifierTrait
{
    private $canID;

    public function getCANID(): string
    {
        return $this->canID;
    }
}