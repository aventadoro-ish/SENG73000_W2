
-- Recording CAN messages 
DROP TABLE IF EXISTS `can_message_log`;

CREATE TABLE `can_message_log` (
  `log_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `elevator_request_id` int(10) unsigned DEFAULT NULL, -- nullable: broadcast
                                                          -- messages (e.g. EC
                                                          -- position updates)
                                                          -- have no request
  `can_id` varchar(6) NOT NULL,            -- e.g. '0x100'
  `direction` enum('tx','rx') NOT NULL,    -- tx = sent by this node, rx = received
  `raw_byte` tinyint(3) unsigned NOT NULL, -- Byte 0 payload, 0-255
  `dlc` tinyint(3) unsigned NOT NULL DEFAULT 1,
  `source_controller` varchar(30) NOT NULL, -- e.g. 'web_floor_station', matches
                                             -- elevator_requests.source_controller
  `logged_at` datetime NOT NULL DEFAULT current_timestamp(),
  PRIMARY KEY (`log_id`),
  KEY `fk_can_log_request` (`elevator_request_id`),
  KEY `idx_can_log_can_id` (`can_id`),
  KEY `idx_can_log_logged_at` (`logged_at`),
  CONSTRAINT `fk_can_log_request` FOREIGN KEY (`elevator_request_id`)
    REFERENCES `elevator_requests` (`elevator_request_id`)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;