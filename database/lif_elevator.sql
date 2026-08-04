-- MariaDB dump 10.19  Distrib 10.4.32-MariaDB, for Win64 (AMD64)
--
-- Host: 127.0.0.1    Database: lif_elevator
-- ------------------------------------------------------
-- Server version	10.4.32-MariaDB

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Current Database: `lif_elevator`
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `lif_elevator` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci */;

USE `lif_elevator`;

--
-- Table structure for table `access_requests`
--

DROP TABLE IF EXISTS `access_requests`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `access_requests` (
  `request_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `first_name` varchar(50) NOT NULL,
  `last_name` varchar(50) NOT NULL,
  `email` varchar(254) NOT NULL,
  `birthday` date NOT NULL,
  `person_type` varchar(30) NOT NULL,
  `involvement` text NOT NULL,
  `drives_car` varchar(20) DEFAULT NULL,
  `details` text DEFAULT NULL,
  `request_status` varchar(20) NOT NULL DEFAULT 'pending',
  `submitted_at` datetime NOT NULL DEFAULT current_timestamp(),
  `reviewed_at` datetime DEFAULT NULL,
  PRIMARY KEY (`request_id`)
) ENGINE=InnoDB AUTO_INCREMENT=8 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `access_requests`
--

LOCK TABLES `access_requests` WRITE;
/*!40000 ALTER TABLE `access_requests` DISABLE KEYS */;
INSERT INTO `access_requests` VALUES (1,'LiF','Admin','LiF_Admin@email.com','2000-01-01','student','Boss','no_answer','LiF Admin account','approved','2026-07-13 18:20:41','2026-07-14 20:02:07'),(2,'Test','Request','testrequest@email.com','2000-12-19','student','Investor','true','Testing Request Logging.','approved','2026-07-15 19:36:56','2026-07-16 16:01:19'),(3,'John','Doe','JD@email.com','1900-12-19','faculty','Instructor','true','A second test account!','approved','2026-07-16 15:36:56','2026-07-16 16:01:07'),(4,'Jane','Doe','JaneDoe@email.com','1990-10-01','faculty','Instructor, Investor','true','Third Test account','approved','2026-07-16 16:00:18','2026-07-16 16:00:38'),(5,'Doe','John','DoeJohn@email.com','1900-12-01','faculty','Piyush','false','Test Account 4','approved','2026-07-16 18:18:06','2026-07-20 16:48:49'),(6,'Doe','Jane','DoeJane@email.com','1990-08-13','faculty','Piyush','false','Another test account!','pending','2026-07-16 20:27:22',NULL),(7,'Long','Email','ThisEmailIsJustSoooLong@email.com','2000-09-11','faculty','Piyush','false','Testing long email wrapping','pending','2026-07-16 20:28:29',NULL);
/*!40000 ALTER TABLE `access_requests` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `elevator_requests`
--

DROP TABLE IF EXISTS `elevator_requests`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `elevator_requests` (
  `elevator_request_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `request_type` varchar(20) NOT NULL,
  `requested_floor` tinyint(3) unsigned NOT NULL,
  `requested_by_user_id` int(10) unsigned DEFAULT NULL,
  `source_controller` varchar(30) NOT NULL,
  `request_status` varchar(20) NOT NULL DEFAULT 'pending',
  `requested_at` datetime NOT NULL DEFAULT current_timestamp(),
  `accepted_at` datetime DEFAULT NULL,
  `completed_at` datetime DEFAULT NULL,
  `failure_reason` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`elevator_request_id`),
  KEY `fk_elevator_request_user` (`requested_by_user_id`),
  CONSTRAINT `fk_elevator_request_user` FOREIGN KEY (`requested_by_user_id`) REFERENCES `users` (`user_id`) ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=106 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `elevator_requests`
--

LOCK TABLES `elevator_requests` WRITE;
/*!40000 ALTER TABLE `elevator_requests` DISABLE KEYS */;
INSERT INTO `elevator_requests` VALUES (2,'remote',3,1,'web_floor_station','pending','2026-07-18 16:48:08',NULL,NULL,NULL),(3,'remote',3,1,'web_floor_station','pending','2026-07-18 16:48:13',NULL,NULL,NULL),(4,'remote',3,1,'web_floor_station','pending','2026-07-18 16:48:58',NULL,NULL,NULL),(5,'remote',3,1,'web_floor_station','pending','2026-07-18 17:03:31',NULL,NULL,NULL),(6,'remote',2,1,'web_floor_station','pending','2026-07-18 17:03:31',NULL,NULL,NULL),(7,'remote',1,1,'web_floor_station','pending','2026-07-18 17:03:32',NULL,NULL,NULL),(8,'remote',3,1,'web_floor_station','pending','2026-07-18 18:08:06',NULL,NULL,NULL),(9,'remote',3,1,'web_floor_station','pending','2026-07-18 18:17:46',NULL,NULL,NULL),(10,'remote',1,1,'web_floor_station','pending','2026-07-18 18:17:53',NULL,NULL,NULL),(11,'remote',3,1,'web_floor_station','pending','2026-07-18 18:17:59',NULL,NULL,NULL),(12,'remote',2,1,'web_floor_station','pending','2026-07-18 18:18:00',NULL,NULL,NULL),(13,'remote',1,1,'web_floor_station','pending','2026-07-18 18:18:02',NULL,NULL,NULL),(14,'remote',3,1,'web_floor_station','pending','2026-07-18 18:18:03',NULL,NULL,NULL),(15,'remote',2,1,'web_floor_station','pending','2026-07-18 18:18:05',NULL,NULL,NULL),(16,'remote',3,1,'web_floor_station','pending','2026-07-18 18:19:56',NULL,NULL,NULL),(17,'remote',1,1,'web_floor_station','pending','2026-07-18 19:07:02',NULL,NULL,NULL),(18,'remote',3,1,'web_floor_station','pending','2026-07-18 19:07:17',NULL,NULL,NULL),(19,'remote',2,1,'web_floor_station','pending','2026-07-18 19:07:19',NULL,NULL,NULL),(20,'remote',1,1,'web_floor_station','pending','2026-07-18 19:07:20',NULL,NULL,NULL),(21,'remote',3,1,'web_floor_station','pending','2026-07-18 19:07:21',NULL,NULL,NULL),(22,'remote',2,1,'web_car_controller','pending','2026-07-18 22:53:05',NULL,NULL,NULL),(23,'remote',1,1,'web_car_controller','pending','2026-07-18 22:53:08',NULL,NULL,NULL),(24,'remote',3,1,'web_car_controller','pending','2026-07-18 22:53:10',NULL,NULL,NULL),(25,'remote',2,1,'web_car_controller','pending','2026-07-18 22:53:20',NULL,NULL,NULL),(26,'remote',3,1,'web_floor_station','pending','2026-07-18 22:53:25',NULL,NULL,NULL),(27,'remote',2,1,'web_floor_station','pending','2026-07-18 23:01:33',NULL,NULL,NULL),(28,'remote',1,1,'web_floor_station','pending','2026-07-18 23:01:34',NULL,NULL,NULL),(29,'remote',3,1,'web_floor_station','pending','2026-07-18 23:01:36',NULL,NULL,NULL),(30,'remote',3,1,'web_floor_station','pending','2026-07-18 23:01:37',NULL,NULL,NULL),(31,'remote',3,1,'web_floor_station','pending','2026-07-18 23:01:37',NULL,NULL,NULL),(32,'remote',3,1,'web_floor_station','pending','2026-07-18 23:01:37',NULL,NULL,NULL),(33,'remote',3,1,'web_floor_station','pending','2026-07-18 23:01:38',NULL,NULL,NULL),(34,'remote',2,1,'web_car_controller','pending','2026-07-18 23:01:43',NULL,NULL,NULL),(35,'remote',1,1,'web_car_controller','pending','2026-07-18 23:01:44',NULL,NULL,NULL),(36,'remote',3,1,'web_car_controller','pending','2026-07-18 23:01:45',NULL,NULL,NULL),(37,'remote',2,1,'web_car_controller','pending','2026-07-18 23:01:56',NULL,NULL,NULL),(38,'remote',3,1,'web_floor_station','pending','2026-07-18 23:01:57',NULL,NULL,NULL),(39,'remote',1,1,'web_car_controller','pending','2026-07-18 23:01:59',NULL,NULL,NULL),(40,'remote',3,1,'web_floor_station','pending','2026-07-18 23:02:00',NULL,NULL,NULL),(41,'remote',1,1,'web_floor_station','pending','2026-07-18 23:07:20',NULL,NULL,NULL),(42,'remote',3,1,'web_car_controller','pending','2026-07-18 23:10:37',NULL,NULL,NULL),(43,'remote',1,1,'web_car_controller','pending','2026-07-20 08:44:39',NULL,NULL,NULL),(44,'remote',2,1,'web_car_controller','pending','2026-07-20 08:44:40',NULL,NULL,NULL),(45,'remote',3,1,'web_floor_station','pending','2026-07-20 08:49:08',NULL,NULL,NULL),(46,'remote',2,1,'web_floor_station','pending','2026-07-20 08:49:09',NULL,NULL,NULL),(47,'remote',1,1,'web_floor_station','pending','2026-07-20 08:49:10',NULL,NULL,NULL),(48,'remote',3,1,'web_floor_station','pending','2026-07-20 08:49:27',NULL,NULL,NULL),(49,'remote',2,1,'web_floor_station','pending','2026-07-20 08:49:28',NULL,NULL,NULL),(50,'remote',1,1,'web_floor_station','pending','2026-07-20 08:50:14',NULL,NULL,NULL),(51,'remote',3,1,'web_floor_station','pending','2026-07-20 08:50:15',NULL,NULL,NULL),(52,'remote',2,1,'web_car_controller','pending','2026-07-20 08:50:17',NULL,NULL,NULL),(53,'remote',1,1,'web_car_controller','pending','2026-07-20 08:50:19',NULL,NULL,NULL),(54,'remote',3,1,'web_car_controller','pending','2026-07-20 08:55:41',NULL,NULL,NULL),(55,'remote',2,1,'web_car_controller','pending','2026-07-20 08:55:41',NULL,NULL,NULL),(56,'remote',3,1,'web_car_controller','pending','2026-07-20 09:08:04',NULL,NULL,NULL),(57,'remote',1,1,'web_car_controller','pending','2026-07-20 09:08:05',NULL,NULL,NULL),(58,'remote',3,1,'web_car_controller','pending','2026-07-20 09:08:09',NULL,NULL,NULL),(59,'remote',2,1,'web_car_controller','pending','2026-07-20 09:08:10',NULL,NULL,NULL),(60,'remote',3,1,'web_floor_station','pending','2026-07-20 10:00:46',NULL,NULL,NULL),(61,'remote',1,1,'web_car_controller','pending','2026-07-20 10:46:15',NULL,NULL,NULL),(62,'remote',3,1,'web_car_controller','pending','2026-07-20 10:46:21',NULL,NULL,NULL),(63,'remote',2,1,'web_car_controller','pending','2026-07-20 10:46:22',NULL,NULL,NULL),(64,'remote',3,1,'web_car_controller','pending','2026-07-20 10:51:21',NULL,NULL,NULL),(65,'remote',2,1,'web_car_controller','pending','2026-07-20 11:19:22',NULL,NULL,NULL),(66,'remote',3,1,'web_car_controller','pending','2026-07-20 11:19:24',NULL,NULL,NULL),(67,'remote',2,1,'web_car_controller','pending','2026-07-20 11:19:43',NULL,NULL,NULL),(68,'remote',3,1,'web_car_controller','pending','2026-07-20 11:19:57',NULL,NULL,NULL),(69,'remote',2,1,'web_car_controller','pending','2026-07-20 11:21:29',NULL,NULL,NULL),(70,'remote',3,1,'web_car_controller','pending','2026-07-20 11:25:32',NULL,NULL,NULL),(71,'remote',2,1,'web_car_controller','pending','2026-07-20 11:25:45',NULL,NULL,NULL),(72,'remote',3,1,'web_car_controller','pending','2026-07-20 11:26:20',NULL,NULL,NULL),(73,'remote',3,1,'web_car_controller','pending','2026-07-20 11:26:41',NULL,NULL,NULL),(74,'remote',3,1,'web_car_controller','pending','2026-07-20 11:26:42',NULL,NULL,NULL),(75,'remote',3,1,'web_floor_station','pending','2026-07-20 11:26:47',NULL,NULL,NULL),(76,'remote',3,1,'web_floor_station','pending','2026-07-20 11:30:56',NULL,NULL,NULL),(77,'remote',3,1,'web_car_controller','pending','2026-07-20 11:31:00',NULL,NULL,NULL),(78,'remote',3,1,'web_floor_station','pending','2026-07-20 11:31:07',NULL,NULL,NULL),(79,'remote',3,1,'web_car_controller','pending','2026-07-20 11:47:33',NULL,NULL,NULL),(80,'remote',2,1,'web_car_controller','pending','2026-07-20 11:51:50',NULL,NULL,NULL),(81,'remote',3,1,'web_car_controller','pending','2026-07-20 11:51:52',NULL,NULL,NULL),(82,'remote',1,1,'web_car_controller','pending','2026-07-20 11:51:57',NULL,NULL,NULL),(83,'remote',3,1,'web_car_controller','pending','2026-07-20 11:51:58',NULL,NULL,NULL),(84,'remote',2,1,'web_car_controller','pending','2026-07-20 12:04:06',NULL,NULL,NULL),(85,'remote',1,1,'web_car_controller','pending','2026-07-20 12:04:06',NULL,NULL,NULL),(86,'remote',3,1,'web_car_controller','pending','2026-07-20 12:04:08',NULL,NULL,NULL),(87,'remote',1,1,'web_car_controller','pending','2026-07-20 12:04:09',NULL,NULL,NULL),(88,'remote',3,1,'web_floor_station','pending','2026-07-20 13:40:46',NULL,NULL,NULL),(89,'remote',2,1,'web_floor_station','pending','2026-07-20 13:40:47',NULL,NULL,NULL),(90,'remote',1,1,'web_floor_station','pending','2026-07-20 13:45:51',NULL,NULL,NULL),(91,'remote',2,1,'web_floor_station','pending','2026-07-20 13:46:27',NULL,NULL,NULL),(92,'remote',3,1,'web_floor_station','pending','2026-07-20 13:46:50',NULL,NULL,NULL),(93,'remote',2,1,'web_floor_station','pending','2026-07-20 13:47:06',NULL,NULL,NULL),(94,'remote',1,1,'web_floor_station','pending','2026-07-20 13:47:07',NULL,NULL,NULL),(95,'remote',3,1,'web_car_controller','pending','2026-07-20 15:25:56',NULL,NULL,NULL),(96,'remote',2,1,'web_floor_station','pending','2026-07-20 15:28:02',NULL,NULL,NULL),(97,'remote',1,1,'web_car_controller','pending','2026-07-20 16:24:19',NULL,NULL,NULL),(98,'remote',2,1,'web_floor_station','pending','2026-07-20 16:50:36',NULL,NULL,NULL),(99,'remote',3,1,'web_car_controller','pending','2026-07-20 16:51:04',NULL,NULL,NULL),(100,'remote',2,1,'web_car_controller','pending','2026-07-20 16:51:31',NULL,NULL,NULL),(101,'remote',2,1,'web_floor_station','pending','2026-07-20 16:51:38',NULL,NULL,NULL),(102,'remote',1,1,'web_floor_station','pending','2026-07-20 16:51:39',NULL,NULL,NULL),(103,'remote',1,1,'web_car_controller','pending','2026-07-20 16:51:41',NULL,NULL,NULL),(104,'remote',1,1,'web_car_controller','pending','2026-07-20 16:51:44',NULL,NULL,NULL),(105,'remote',2,1,'web_floor_station','pending','2026-07-20 16:51:45',NULL,NULL,NULL);
/*!40000 ALTER TABLE `elevator_requests` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `elevator_state`
--

DROP TABLE IF EXISTS `elevator_state`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `elevator_state` (
  `state_id` tinyint(3) unsigned NOT NULL,
  `doors_open` tinyint(1) NOT NULL DEFAULT 0,
  `sabbath_enabled` tinyint(1) NOT NULL DEFAULT 0,
  `updated_at` datetime NOT NULL DEFAULT current_timestamp() ON UPDATE current_timestamp(),
  PRIMARY KEY (`state_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `elevator_state`
--

LOCK TABLES `elevator_state` WRITE;
/*!40000 ALTER TABLE `elevator_state` DISABLE KEYS */;
INSERT INTO `elevator_state` VALUES (1,1,0,'2026-07-20 16:52:47');
/*!40000 ALTER TABLE `elevator_state` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `users`
--

DROP TABLE IF EXISTS `users`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `users` (
  `user_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `access_request_id` int(10) unsigned DEFAULT NULL,
  `username` varchar(50) NOT NULL,
  `password_hash` varchar(255) NOT NULL,
  `email` varchar(254) NOT NULL,
  `user_role` varchar(20) NOT NULL DEFAULT 'user',
  `account_status` varchar(20) NOT NULL DEFAULT 'approved',
  `created_at` datetime NOT NULL DEFAULT current_timestamp(),
  `last_login_at` datetime DEFAULT NULL,
  PRIMARY KEY (`user_id`),
  UNIQUE KEY `username` (`username`),
  UNIQUE KEY `email` (`email`),
  KEY `fk_users_access_request` (`access_request_id`),
  CONSTRAINT `fk_users_access_request` FOREIGN KEY (`access_request_id`) REFERENCES `access_requests` (`request_id`) ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `users`
--

LOCK TABLES `users` WRITE;
/*!40000 ALTER TABLE `users` DISABLE KEYS */;
INSERT INTO `users` VALUES (1,1,'LiF_Admin','$2y$10$6GuzQhPpmvdXae2VVjq20OhkJygip9oHQP3TPpTiPmQsNQHbEnb8e','LiF_Admin@email.com','admin','approved','2026-07-14 19:58:47','2026-07-20 16:55:17'),(2,2,'testuser','$2y$10$Iqqiy5VCKC7Vv3hOY7D7H.HEODXX.b595ioHYo65hQE6HhgPCwIP2','testrequest@email.com','user','approved','2026-07-16 15:24:00','2026-07-20 16:52:55'),(3,3,'JohnDoe','$2y$10$lh0ymI37e3nMgUMt0Unfxe8wWYxw7VOEZEmOEZ3LC2//LMw8yV3lK','JD@email.com','user','approved','2026-07-16 15:37:37','2026-07-16 15:40:28'),(4,4,'JaneDoe','$2y$10$xTM5rir2GHzuMcqKv7z9LuhFV3K/CbBPKx57M42LIhoxDFLF6S2Ai','JaneDoe@email.com','user','approved','2026-07-16 16:00:38','2026-07-16 20:35:05'),(5,5,'DoeJohn','$2y$10$sxM.7omgxn4qn1U5jXvkAejjNT0bvHdNMin9.EErdQIflmNDOqw/y','DoeJohn@email.com','user','approved','2026-07-20 16:48:49','2026-07-20 16:49:20');
/*!40000 ALTER TABLE `users` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-07-20 18:01:04
