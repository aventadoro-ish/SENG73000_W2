-- MariaDB dump 10.19  Distrib 10.4.32-MariaDB, for Win64 (AMD64)
--
-- Host: localhost    Database: lif_elevator
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
) ENGINE=InnoDB AUTO_INCREMENT=19 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `access_requests`
--

LOCK TABLES `access_requests` WRITE;
/*!40000 ALTER TABLE `access_requests` DISABLE KEYS */;
INSERT INTO `access_requests` VALUES (1,'LiF','Admin','LiF_Admin@email.com','2000-01-01','student','Boss','no_answer','LiF Admin account','approved','2026-07-13 18:20:41','2026-07-14 20:02:07'),(2,'Test','Request','testrequest@email.com','2000-12-19','student','Investor','true','Testing Request Logging.','approved','2026-07-15 19:36:56','2026-07-16 16:01:19'),(3,'John','Doe','JD@email.com','1900-12-19','faculty','Instructor','true','A second test account!','approved','2026-07-16 15:36:56','2026-07-16 16:01:07'),(4,'Jane','Doe','JaneDoe@email.com','1990-10-01','faculty','Instructor, Investor','true','Third Test account','approved','2026-07-16 16:00:18','2026-07-16 16:00:38'),(5,'Doe','John','DoeJohn@email.com','1900-12-01','faculty','Piyush','false','Test Account 4','approved','2026-07-16 18:18:06','2026-07-20 16:48:49'),(6,'Doe','Jane','DoeJane@email.com','1990-08-13','faculty','Piyush','false','Another test account!','approved','2026-07-16 20:27:22','2026-08-07 17:36:56'),(7,'Long','Email','ThisEmailIsJustSoooLong@email.com','2000-09-11','faculty','Piyush','false','Testing long email wrapping','approved','2026-07-16 20:28:29','2026-08-07 18:13:21'),(8,'RequestAccess','Testing','RQ@testemail.com','2000-01-01','faculty','None','false','This is a test account to prove request access functionality','approved','2026-07-27 15:08:25','2026-07-27 15:09:38'),(9,'DB_Test','July29','test@email.com','2005-12-19','student','Boss','true','This is a test for request access page as part of the video demo!','approved','2026-07-29 04:17:41','2026-07-29 04:20:44'),(10,'A2','Q7','Q7@email.com','2005-12-19','student','Boss','true','Hello! Q7 for A2 testing.','approved','2026-08-07 11:46:52','2026-08-07 17:47:36'),(11,'Conestoga','College','CC@email.com','2005-12-19','student','Boss','true','Conestoga Test Account','approved','2026-08-07 12:13:53','2026-08-07 18:25:33'),(12,'Test','ForA2','TestForA2@email.com','2005-12-19','student','Boss','false','Test for A2','approved','2026-08-07 12:14:25','2026-08-07 19:04:14'),(13,'Requesting','Access','ReqAcc@email.com','2005-12-19','faculty','None, Instructor, PM','true','One more test account!','approved','2026-08-07 12:15:07','2026-08-07 19:07:52'),(14,'Nick','Kapuka','Nkapuka@email.com','2005-12-19','student','Boss, Investor','true','I\'m the coolest.\r\n','approved','2026-08-07 13:04:44','2026-08-07 19:09:03'),(15,'Hello','World','YoMama@email.com','2000-01-01','faculty','None','false','Test account','approved','2026-08-07 13:16:08','2026-08-07 21:28:45'),(17,'Emi','PP','EPP@email.com','2005-04-27','student','Piyush','false','The goat','pending','2026-08-07 15:34:49',NULL),(18,'Nick','Kapuka','NickKapuka@email.com','2041-12-19','student','Project Manager','true','Showcasing modifying','pending','2026-08-07 18:25:39',NULL);
/*!40000 ALTER TABLE `access_requests` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `can_message_log`
--

DROP TABLE IF EXISTS `can_message_log`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `can_message_log` (
  `log_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `elevator_request_id` int(10) unsigned DEFAULT NULL,
  `can_id` varchar(6) NOT NULL,
  `direction` enum('tx','rx') NOT NULL,
  `raw_byte` tinyint(3) unsigned NOT NULL,
  `dlc` tinyint(3) unsigned NOT NULL DEFAULT 1,
  `source_controller` varchar(30) NOT NULL,
  `logged_at` datetime NOT NULL DEFAULT current_timestamp(),
  PRIMARY KEY (`log_id`),
  KEY `idx_can_log_request` (`elevator_request_id`),
  KEY `idx_can_log_can_id` (`can_id`),
  KEY `idx_can_log_logged_at` (`logged_at`),
  CONSTRAINT `fk_can_log_request` FOREIGN KEY (`elevator_request_id`) REFERENCES `elevator_requests` (`elevator_request_id`) ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=54 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `can_message_log`
--

LOCK TABLES `can_message_log` WRITE;
/*!40000 ALTER TABLE `can_message_log` DISABLE KEYS */;
INSERT INTO `can_message_log` VALUES (3,NULL,'0x202','rx',1,1,'F2','2026-07-29 04:12:00'),(4,NULL,'0x100','tx',1,1,'SC','2026-07-29 04:13:01'),(5,NULL,'0x101','rx',5,1,'EC','2026-08-04 13:09:17'),(6,NULL,'0x101','rx',6,1,'EC','2026-08-04 13:45:16'),(7,NULL,'0x101','rx',7,1,'EC','2026-08-04 13:45:45'),(8,NULL,'0x101','rx',8,1,'EC','2026-08-04 13:46:06'),(9,NULL,'0x101','rx',9,1,'EC','2026-08-04 13:48:20'),(10,NULL,'0x101','rx',10,1,'EC','2026-08-04 18:16:27'),(11,NULL,'0x101','rx',11,1,'EC','2026-08-04 18:40:56'),(12,NULL,'0x101','rx',5,1,'EC','2026-08-04 18:42:11'),(13,NULL,'0x101','rx',5,1,'EC','2026-08-04 22:04:56'),(14,NULL,'0x101','rx',5,1,'EC','2026-08-04 22:07:41'),(15,NULL,'0x101','rx',5,1,'EC','2026-08-04 22:08:02'),(16,NULL,'0x101','rx',5,1,'EC','2026-08-04 22:08:27'),(17,NULL,'0x101','rx',9,1,'EC','2026-08-04 22:08:41'),(18,NULL,'0x101','rx',8,1,'EC','2026-08-04 22:08:54'),(19,NULL,'0x101','rx',7,1,'EC','2026-08-04 22:09:08'),(20,NULL,'0x101','rx',5,1,'EC','2026-08-04 22:13:18'),(21,NULL,'0x101','rx',7,1,'EC','2026-08-04 22:13:47'),(22,NULL,'0x101','rx',5,1,'EC','2026-08-04 22:14:08'),(23,NULL,'0x101','rx',6,1,'EC','2026-08-04 22:14:18'),(24,NULL,'0x202','rx',6,1,'F2','2026-08-04 22:15:25'),(25,NULL,'0x202','rx',6,1,'F2','2026-08-04 22:15:51'),(26,NULL,'0x202','rx',6,1,'F2','2026-08-04 22:16:00'),(27,NULL,'0x101','rx',6,1,'EC','2026-08-04 22:16:12'),(28,NULL,'0x101','rx',6,1,'EC','2026-08-04 22:17:41'),(29,NULL,'0x101','rx',6,1,'EC','2026-08-04 22:39:57'),(30,NULL,'0x101','rx',6,1,'EC','2026-08-04 22:46:31'),(31,NULL,'0x101','rx',7,1,'EC','2026-08-04 22:46:40'),(32,NULL,'0x101','rx',6,1,'EC','2026-08-04 22:55:49'),(33,NULL,'0x101','rx',7,1,'EC','2026-08-04 23:18:25'),(34,NULL,'0x101','rx',5,1,'EC','2026-08-04 23:18:40'),(35,NULL,'0x101','rx',7,1,'EC','2026-08-04 23:19:15'),(36,NULL,'0x101','rx',5,1,'EC','2026-08-05 19:11:37'),(37,NULL,'0x101','rx',7,1,'EC','2026-08-05 19:12:04'),(38,NULL,'0x101','rx',5,1,'EC','2026-08-05 19:14:15'),(39,NULL,'0x101','rx',6,1,'EC','2026-08-05 19:14:27'),(40,NULL,'0x101','rx',7,1,'EC','2026-08-05 19:14:35'),(41,NULL,'0x101','rx',5,1,'EC','2026-08-05 19:17:08'),(42,NULL,'0x101','rx',7,1,'EC','2026-08-05 19:18:17'),(43,NULL,'0x101','rx',5,1,'EC','2026-08-05 20:01:06'),(44,NULL,'0x101','rx',7,1,'EC','2026-08-05 20:01:35'),(45,NULL,'0x101','rx',5,1,'EC','2026-08-05 20:01:45'),(46,NULL,'0x101','rx',6,1,'EC','2026-08-05 20:37:54'),(47,NULL,'0x101','rx',7,1,'EC','2026-08-05 20:39:02'),(48,NULL,'0x101','rx',5,1,'EC','2026-08-05 22:53:11'),(49,NULL,'0x101','rx',7,1,'EC','2026-08-06 16:10:41'),(50,NULL,'0x101','rx',5,1,'EC','2026-08-06 22:07:55'),(51,NULL,'0x101','rx',5,1,'EC','2026-08-06 22:08:06'),(52,NULL,'0x101','rx',7,1,'EC','2026-08-06 22:08:35'),(53,NULL,'0x101','rx',5,1,'EC','2026-08-06 22:15:48');
/*!40000 ALTER TABLE `can_message_log` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `can_nodes`
--

DROP TABLE IF EXISTS `can_nodes`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `can_nodes` (
  `can_node_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `node_id` int(10) unsigned NOT NULL,
  `can_id` varchar(6) NOT NULL,
  PRIMARY KEY (`can_node_id`),
  UNIQUE KEY `uq_can_node` (`node_id`),
  UNIQUE KEY `uq_can_id` (`can_id`),
  CONSTRAINT `fk_can_node` FOREIGN KEY (`node_id`) REFERENCES `nodes` (`node_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `can_nodes`
--

LOCK TABLES `can_nodes` WRITE;
/*!40000 ALTER TABLE `can_nodes` DISABLE KEYS */;
INSERT INTO `can_nodes` VALUES (1,3,'0x200'),(2,2,'0x101'),(3,4,'0x201'),(4,5,'0x202'),(5,6,'0x203'),(6,1,'0x100');
/*!40000 ALTER TABLE `can_nodes` ENABLE KEYS */;
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
) ENGINE=InnoDB AUTO_INCREMENT=142 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `elevator_requests`
--

LOCK TABLES `elevator_requests` WRITE;
/*!40000 ALTER TABLE `elevator_requests` DISABLE KEYS */;
INSERT INTO `elevator_requests` VALUES (2,'remote',3,1,'web_floor_station','pending','2026-07-18 16:48:08',NULL,NULL,NULL),(3,'remote',3,1,'web_floor_station','pending','2026-07-18 16:48:13',NULL,NULL,NULL),(4,'remote',3,1,'web_floor_station','pending','2026-07-18 16:48:58',NULL,NULL,NULL),(5,'remote',3,1,'web_floor_station','pending','2026-07-18 17:03:31',NULL,NULL,NULL),(6,'remote',2,1,'web_floor_station','pending','2026-07-18 17:03:31',NULL,NULL,NULL),(7,'remote',1,1,'web_floor_station','pending','2026-07-18 17:03:32',NULL,NULL,NULL),(8,'remote',3,1,'web_floor_station','pending','2026-07-18 18:08:06',NULL,NULL,NULL),(9,'remote',3,1,'web_floor_station','pending','2026-07-18 18:17:46',NULL,NULL,NULL),(10,'remote',1,1,'web_floor_station','pending','2026-07-18 18:17:53',NULL,NULL,NULL),(11,'remote',3,1,'web_floor_station','pending','2026-07-18 18:17:59',NULL,NULL,NULL),(12,'remote',2,1,'web_floor_station','pending','2026-07-18 18:18:00',NULL,NULL,NULL),(13,'remote',1,1,'web_floor_station','pending','2026-07-18 18:18:02',NULL,NULL,NULL),(14,'remote',3,1,'web_floor_station','pending','2026-07-18 18:18:03',NULL,NULL,NULL),(15,'remote',2,1,'web_floor_station','pending','2026-07-18 18:18:05',NULL,NULL,NULL),(16,'remote',3,1,'web_floor_station','pending','2026-07-18 18:19:56',NULL,NULL,NULL),(17,'remote',1,1,'web_floor_station','pending','2026-07-18 19:07:02',NULL,NULL,NULL),(18,'remote',3,1,'web_floor_station','pending','2026-07-18 19:07:17',NULL,NULL,NULL),(19,'remote',2,1,'web_floor_station','pending','2026-07-18 19:07:19',NULL,NULL,NULL),(20,'remote',1,1,'web_floor_station','pending','2026-07-18 19:07:20',NULL,NULL,NULL),(21,'remote',3,1,'web_floor_station','pending','2026-07-18 19:07:21',NULL,NULL,NULL),(22,'remote',2,1,'web_car_controller','pending','2026-07-18 22:53:05',NULL,NULL,NULL),(23,'remote',1,1,'web_car_controller','pending','2026-07-18 22:53:08',NULL,NULL,NULL),(24,'remote',3,1,'web_car_controller','pending','2026-07-18 22:53:10',NULL,NULL,NULL),(25,'remote',2,1,'web_car_controller','pending','2026-07-18 22:53:20',NULL,NULL,NULL),(26,'remote',3,1,'web_floor_station','pending','2026-07-18 22:53:25',NULL,NULL,NULL),(27,'remote',2,1,'web_floor_station','pending','2026-07-18 23:01:33',NULL,NULL,NULL),(28,'remote',1,1,'web_floor_station','pending','2026-07-18 23:01:34',NULL,NULL,NULL),(29,'remote',3,1,'web_floor_station','pending','2026-07-18 23:01:36',NULL,NULL,NULL),(30,'remote',3,1,'web_floor_station','pending','2026-07-18 23:01:37',NULL,NULL,NULL),(31,'remote',3,1,'web_floor_station','pending','2026-07-18 23:01:37',NULL,NULL,NULL),(32,'remote',3,1,'web_floor_station','pending','2026-07-18 23:01:37',NULL,NULL,NULL),(33,'remote',3,1,'web_floor_station','pending','2026-07-18 23:01:38',NULL,NULL,NULL),(34,'remote',2,1,'web_car_controller','pending','2026-07-18 23:01:43',NULL,NULL,NULL),(35,'remote',1,1,'web_car_controller','pending','2026-07-18 23:01:44',NULL,NULL,NULL),(36,'remote',3,1,'web_car_controller','pending','2026-07-18 23:01:45',NULL,NULL,NULL),(37,'remote',2,1,'web_car_controller','pending','2026-07-18 23:01:56',NULL,NULL,NULL),(38,'remote',3,1,'web_floor_station','pending','2026-07-18 23:01:57',NULL,NULL,NULL),(39,'remote',1,1,'web_car_controller','pending','2026-07-18 23:01:59',NULL,NULL,NULL),(40,'remote',3,1,'web_floor_station','pending','2026-07-18 23:02:00',NULL,NULL,NULL),(41,'remote',1,1,'web_floor_station','pending','2026-07-18 23:07:20',NULL,NULL,NULL),(42,'remote',3,1,'web_car_controller','pending','2026-07-18 23:10:37',NULL,NULL,NULL),(43,'remote',1,1,'web_car_controller','pending','2026-07-20 08:44:39',NULL,NULL,NULL),(44,'remote',2,1,'web_car_controller','pending','2026-07-20 08:44:40',NULL,NULL,NULL),(45,'remote',3,1,'web_floor_station','pending','2026-07-20 08:49:08',NULL,NULL,NULL),(46,'remote',2,1,'web_floor_station','pending','2026-07-20 08:49:09',NULL,NULL,NULL),(47,'remote',1,1,'web_floor_station','pending','2026-07-20 08:49:10',NULL,NULL,NULL),(48,'remote',3,1,'web_floor_station','pending','2026-07-20 08:49:27',NULL,NULL,NULL),(49,'remote',2,1,'web_floor_station','pending','2026-07-20 08:49:28',NULL,NULL,NULL),(50,'remote',1,1,'web_floor_station','pending','2026-07-20 08:50:14',NULL,NULL,NULL),(51,'remote',3,1,'web_floor_station','pending','2026-07-20 08:50:15',NULL,NULL,NULL),(52,'remote',2,1,'web_car_controller','pending','2026-07-20 08:50:17',NULL,NULL,NULL),(53,'remote',1,1,'web_car_controller','pending','2026-07-20 08:50:19',NULL,NULL,NULL),(54,'remote',3,1,'web_car_controller','pending','2026-07-20 08:55:41',NULL,NULL,NULL),(55,'remote',2,1,'web_car_controller','pending','2026-07-20 08:55:41',NULL,NULL,NULL),(56,'remote',3,1,'web_car_controller','pending','2026-07-20 09:08:04',NULL,NULL,NULL),(57,'remote',1,1,'web_car_controller','pending','2026-07-20 09:08:05',NULL,NULL,NULL),(58,'remote',3,1,'web_car_controller','pending','2026-07-20 09:08:09',NULL,NULL,NULL),(59,'remote',2,1,'web_car_controller','pending','2026-07-20 09:08:10',NULL,NULL,NULL),(60,'remote',3,1,'web_floor_station','pending','2026-07-20 10:00:46',NULL,NULL,NULL),(61,'remote',1,1,'web_car_controller','pending','2026-07-20 10:46:15',NULL,NULL,NULL),(62,'remote',3,1,'web_car_controller','pending','2026-07-20 10:46:21',NULL,NULL,NULL),(63,'remote',2,1,'web_car_controller','pending','2026-07-20 10:46:22',NULL,NULL,NULL),(64,'remote',3,1,'web_car_controller','pending','2026-07-20 10:51:21',NULL,NULL,NULL),(65,'remote',2,1,'web_car_controller','pending','2026-07-20 11:19:22',NULL,NULL,NULL),(66,'remote',3,1,'web_car_controller','pending','2026-07-20 11:19:24',NULL,NULL,NULL),(67,'remote',2,1,'web_car_controller','pending','2026-07-20 11:19:43',NULL,NULL,NULL),(68,'remote',3,1,'web_car_controller','pending','2026-07-20 11:19:57',NULL,NULL,NULL),(69,'remote',2,1,'web_car_controller','pending','2026-07-20 11:21:29',NULL,NULL,NULL),(70,'remote',3,1,'web_car_controller','pending','2026-07-20 11:25:32',NULL,NULL,NULL),(71,'remote',2,1,'web_car_controller','pending','2026-07-20 11:25:45',NULL,NULL,NULL),(72,'remote',3,1,'web_car_controller','pending','2026-07-20 11:26:20',NULL,NULL,NULL),(73,'remote',3,1,'web_car_controller','pending','2026-07-20 11:26:41',NULL,NULL,NULL),(74,'remote',3,1,'web_car_controller','pending','2026-07-20 11:26:42',NULL,NULL,NULL),(75,'remote',3,1,'web_floor_station','pending','2026-07-20 11:26:47',NULL,NULL,NULL),(76,'remote',3,1,'web_floor_station','pending','2026-07-20 11:30:56',NULL,NULL,NULL),(77,'remote',3,1,'web_car_controller','pending','2026-07-20 11:31:00',NULL,NULL,NULL),(78,'remote',3,1,'web_floor_station','pending','2026-07-20 11:31:07',NULL,NULL,NULL),(79,'remote',3,1,'web_car_controller','pending','2026-07-20 11:47:33',NULL,NULL,NULL),(80,'remote',2,1,'web_car_controller','pending','2026-07-20 11:51:50',NULL,NULL,NULL),(81,'remote',3,1,'web_car_controller','pending','2026-07-20 11:51:52',NULL,NULL,NULL),(82,'remote',1,1,'web_car_controller','pending','2026-07-20 11:51:57',NULL,NULL,NULL),(83,'remote',3,1,'web_car_controller','pending','2026-07-20 11:51:58',NULL,NULL,NULL),(84,'remote',2,1,'web_car_controller','pending','2026-07-20 12:04:06',NULL,NULL,NULL),(85,'remote',1,1,'web_car_controller','pending','2026-07-20 12:04:06',NULL,NULL,NULL),(86,'remote',3,1,'web_car_controller','pending','2026-07-20 12:04:08',NULL,NULL,NULL),(87,'remote',1,1,'web_car_controller','pending','2026-07-20 12:04:09',NULL,NULL,NULL),(88,'remote',3,1,'web_floor_station','pending','2026-07-20 13:40:46',NULL,NULL,NULL),(89,'remote',2,1,'web_floor_station','pending','2026-07-20 13:40:47',NULL,NULL,NULL),(90,'remote',1,1,'web_floor_station','pending','2026-07-20 13:45:51',NULL,NULL,NULL),(91,'remote',2,1,'web_floor_station','pending','2026-07-20 13:46:27',NULL,NULL,NULL),(92,'remote',3,1,'web_floor_station','pending','2026-07-20 13:46:50',NULL,NULL,NULL),(93,'remote',2,1,'web_floor_station','pending','2026-07-20 13:47:06',NULL,NULL,NULL),(94,'remote',1,1,'web_floor_station','pending','2026-07-20 13:47:07',NULL,NULL,NULL),(95,'remote',3,1,'web_car_controller','pending','2026-07-20 15:25:56',NULL,NULL,NULL),(96,'remote',2,1,'web_floor_station','pending','2026-07-20 15:28:02',NULL,NULL,NULL),(97,'remote',1,1,'web_car_controller','pending','2026-07-20 16:24:19',NULL,NULL,NULL),(98,'remote',2,1,'web_floor_station','pending','2026-07-20 16:50:36',NULL,NULL,NULL),(99,'remote',3,1,'web_car_controller','pending','2026-07-20 16:51:04',NULL,NULL,NULL),(100,'remote',2,1,'web_car_controller','pending','2026-07-20 16:51:31',NULL,NULL,NULL),(101,'remote',2,1,'web_floor_station','pending','2026-07-20 16:51:38',NULL,NULL,NULL),(102,'remote',1,1,'web_floor_station','pending','2026-07-20 16:51:39',NULL,NULL,NULL),(103,'remote',1,1,'web_car_controller','pending','2026-07-20 16:51:41',NULL,NULL,NULL),(104,'remote',1,1,'web_car_controller','pending','2026-07-20 16:51:44',NULL,NULL,NULL),(105,'remote',2,1,'web_floor_station','pending','2026-07-20 16:51:45',NULL,NULL,NULL),(106,'remote',3,1,'web_floor_station','pending','2026-07-22 19:19:36',NULL,NULL,NULL),(107,'remote',3,1,'web_floor_station','pending','2026-07-22 19:19:40',NULL,NULL,NULL),(108,'remote',1,1,'web_floor_station','pending','2026-07-24 21:58:03',NULL,NULL,NULL),(109,'remote',2,1,'web_floor_station','pending','2026-07-27 05:24:05',NULL,NULL,NULL),(110,'remote',1,1,'web_car_controller','pending','2026-07-27 11:34:06',NULL,NULL,NULL),(111,'remote',2,1,'web_floor_station','pending','2026-07-27 11:46:50',NULL,NULL,NULL),(112,'remote',3,1,'web_floor_station','pending','2026-07-27 11:46:51',NULL,NULL,NULL),(113,'remote',1,1,'web_floor_station','pending','2026-07-27 11:46:52',NULL,NULL,NULL),(114,'remote',3,1,'web_car_controller','pending','2026-07-27 15:02:33',NULL,NULL,NULL),(115,'remote',1,1,'web_floor_station','pending','2026-07-27 15:02:49',NULL,NULL,NULL),(116,'remote',3,1,'web_floor_station','pending','2026-07-27 15:02:52',NULL,NULL,NULL),(117,'remote',2,1,'web_floor_station','pending','2026-07-27 15:02:54',NULL,NULL,NULL),(118,'remote',3,1,'web_car_controller','pending','2026-07-29 04:24:26',NULL,NULL,NULL),(119,'remote',1,1,'web_car_controller','pending','2026-07-29 04:24:28',NULL,NULL,NULL),(120,'remote',1,1,'web_floor_station','pending','2026-07-29 04:24:41',NULL,NULL,NULL),(121,'remote',2,1,'web_car_controller','pending','2026-07-29 04:24:47',NULL,NULL,NULL),(122,'remote',3,1,'web_car_controller','pending','2026-07-29 04:24:48',NULL,NULL,NULL),(123,'remote',1,1,'web_car_controller','pending','2026-07-29 04:24:48',NULL,NULL,NULL),(124,'remote',1,1,'web_floor_station','pending','2026-08-04 22:04:10',NULL,NULL,NULL),(125,'remote',3,1,'web_floor_station','pending','2026-08-04 22:07:03',NULL,NULL,NULL),(126,'remote',2,1,'web_floor_station','pending','2026-08-04 22:38:00',NULL,NULL,NULL),(127,'remote',2,1,'web_car_controller','pending','2026-08-04 22:38:25',NULL,NULL,NULL),(128,'remote',3,1,'web_car_controller','pending','2026-08-04 23:18:56',NULL,NULL,NULL),(129,'remote',1,1,'web_car_controller','pending','2026-08-05 19:17:05',NULL,NULL,NULL),(130,'remote',3,1,'web_car_controller','pending','2026-08-05 19:17:26',NULL,NULL,NULL),(131,'remote',1,1,'web_car_controller','pending','2026-08-05 20:00:46',NULL,NULL,NULL),(132,'remote',3,1,'web_floor_station','pending','2026-08-05 20:01:28',NULL,NULL,NULL),(133,'remote',1,1,'web_floor_station','pending','2026-08-05 20:01:38',NULL,NULL,NULL),(134,'remote',2,1,'web_floor_station','pending','2026-08-05 20:01:56',NULL,NULL,NULL),(135,'remote',2,1,'web_car_controller','pending','2026-08-05 20:37:52',NULL,NULL,NULL),(136,'remote',3,1,'web_car_controller','pending','2026-08-05 20:38:59',NULL,NULL,NULL),(137,'remote',1,1,'web_floor_station','pending','2026-08-05 22:53:09',NULL,NULL,NULL),(138,'remote',3,1,'web_floor_station','pending','2026-08-06 16:09:19',NULL,NULL,NULL),(139,'remote',1,1,'web_floor_station','pending','2026-08-06 22:07:50',NULL,NULL,NULL),(140,'remote',3,1,'web_floor_station','pending','2026-08-06 22:08:28',NULL,NULL,NULL),(141,'remote',1,1,'web_floor_station','pending','2026-08-06 22:13:53',NULL,NULL,NULL);
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
  `operation_mode` varchar(20) NOT NULL DEFAULT 'normal',
  `updated_at` datetime NOT NULL DEFAULT current_timestamp() ON UPDATE current_timestamp(),
  PRIMARY KEY (`state_id`),
  CONSTRAINT `chk_operation_mode` CHECK (`operation_mode` in ('normal','sabbath','maintenance','fault'))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `elevator_state`
--

LOCK TABLES `elevator_state` WRITE;
/*!40000 ALTER TABLE `elevator_state` DISABLE KEYS */;
INSERT INTO `elevator_state` VALUES (1,0,'normal','2026-08-06 22:08:49');
/*!40000 ALTER TABLE `elevator_state` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `nodes`
--

DROP TABLE IF EXISTS `nodes`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `nodes` (
  `node_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `node_name` varchar(10) NOT NULL,
  `node_role` varchar(30) NOT NULL,
  PRIMARY KEY (`node_id`),
  UNIQUE KEY `uq_node_name` (`node_name`),
  KEY `idx_node_role` (`node_role`)
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `nodes`
--

LOCK TABLES `nodes` WRITE;
/*!40000 ALTER TABLE `nodes` DISABLE KEYS */;
INSERT INTO `nodes` VALUES (1,'SC','supervisor'),(2,'EC','elevator_controller'),(3,'CC','cab_controller'),(4,'F1','floor_controller'),(5,'F2','floor_controller'),(6,'F3','floor_controller');
/*!40000 ALTER TABLE `nodes` ENABLE KEYS */;
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
) ENGINE=InnoDB AUTO_INCREMENT=18 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `users`
--

LOCK TABLES `users` WRITE;
/*!40000 ALTER TABLE `users` DISABLE KEYS */;
INSERT INTO `users` VALUES (1,1,'LiF_Admin','$2y$10$6GuzQhPpmvdXae2VVjq20OhkJygip9oHQP3TPpTiPmQsNQHbEnb8e','LiF_Admin@email.com','admin','approved','2026-07-14 19:58:47','2026-08-07 18:25:42'),(2,2,'testuser','$2y$10$Iqqiy5VCKC7Vv3hOY7D7H.HEODXX.b595ioHYo65hQE6HhgPCwIP2','testrequest@email.com','user','approved','2026-07-16 15:24:00','2026-07-29 04:18:07'),(3,3,'JohnDoe','$2y$10$lh0ymI37e3nMgUMt0Unfxe8wWYxw7VOEZEmOEZ3LC2//LMw8yV3lK','JD@email.com','user','approved','2026-07-16 15:37:37','2026-07-16 15:40:28'),(4,4,'JaneDoe','$2y$10$xTM5rir2GHzuMcqKv7z9LuhFV3K/CbBPKx57M42LIhoxDFLF6S2Ai','JaneDoe@email.com','user','approved','2026-07-16 16:00:38','2026-07-16 20:35:05'),(5,5,'DoeJohn','$2y$10$sxM.7omgxn4qn1U5jXvkAejjNT0bvHdNMin9.EErdQIflmNDOqw/y','DoeJohn@email.com','user','approved','2026-07-20 16:48:49','2026-07-20 16:49:20'),(6,8,'RQAccount','$2y$10$8FCqWEWOvKl358AApQh6yeP8LY5GSKgRpHUl8C4z3hVWrZcdROc4i','RQ@testemail.com','user','approved','2026-07-27 15:09:38','2026-07-27 15:10:06'),(7,9,'DB_TestJuly29','$2y$10$i8K3WL23TFfYowhh5vxlr.WxqR6BmxsBQ3FjyZDJHB.l7VncLA23e','test@email.com','user','approved','2026-07-29 04:20:44','2026-07-29 04:23:40'),(8,6,'DoeJane','$2y$10$PEs.xV.xPysFJuj3rEN3cewkBVQzrzXfgxTm6FLIrqgpAGq1IaT2W','DoeJane@email.com','user','approved','2026-08-07 11:36:56',NULL),(9,10,'A2Q7_A2Q7','$2y$10$Be2YWUnvJ3OVU5FylRRtJ.n1nefChNBSb4qLA90LTY/3VpVix01kS','Q7@email.com','user','approved','2026-08-07 11:47:36','2026-08-07 11:48:12'),(10,7,'LongEmail','$2y$10$OMBmCMkdqF/XptdsbOdz.OuqsBg01WfdUF935ArMcNsn3HkQf3x.G','ThisEmailIsJustSoooLong@email.com','user','approved','2026-08-07 12:13:21',NULL),(11,11,'ConCollege','$2y$10$ea4Jd1H9HjgM//aklt4oHuRNk3nMxrHjNQ4PewfUIetHmeqk8DW6C','CC@email.com','user','approved','2026-08-07 12:25:33',NULL),(13,12,'TestForA2','$2y$10$lPlis2w6DPr7QkMT.nzi1O9WKaATiOS0sBA2zjfuEst9k2Kcbx1VG','TestForA2@email.com','user','approved','2026-08-07 13:04:14',NULL),(14,13,'RequestAcces','$2y$10$uYXS9ve7al4Qc3B3pWQkXuFGuXr70Mq.29UhryHoGFQkgKYvHhRO.','ReqAcc@email.com','user','approved','2026-08-07 13:07:52',NULL),(16,14,'NickKapuka','$2y$10$aWB8R/rforRMCYesbWHKoOIznVg5ZJltq3RjeBWd8XG.OwrPkxfgW','Nkapuka@email.com','user','approved','2026-08-07 13:09:03',NULL),(17,15,'HelloWorld','$2y$10$Id2m2GzGCGoxLbQqxRUoO..jVRLIDbt500baNg.FfLg/g8ih6ooM.','YoMama@email.com','user','approved','2026-08-07 15:28:45',NULL);
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

-- Dump completed on 2026-08-07 18:44:15
