<?php 

abstract class Node {
    private static $nodeCount = 0;
    private $nodeName;
    private $online;

    public function __construct(string $nodeName, bool $online = true)
    {
        $this->nodeName = $nodeName;
        $this->online = $online;

        self::$nodeCount++;
    }

    public static function getNodeCount(): int {
        return self::$nodeCount;
    }

    public  function getNodeName(): string {
        return $this->nodeName;
    }

    public function isOnline(): bool {
        return $this->online;
    }

    public function setOnline(bool $online) {
        $this->online = $online;
    }

    abstract public function getNodeType(): string;
}