-- phpMyAdmin SQL Dump
-- version 4.8.2
-- https://www.phpmyadmin.net/
--
-- Host: localhost
-- Generation Time: Jul 05, 2018 at 12:58 AM
-- Server version: 10.1.34-MariaDB
-- PHP Version: 7.2.7

SET FOREIGN_KEY_CHECKS=0;
SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET AUTOCOMMIT = 0;
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `openmonero`
--

CREATE DATABASE IF NOT EXISTS `openmonero` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;
USE `openmonero`;

-- --------------------------------------------------------

--
-- Table structure for table `Accounts`
--

DROP TABLE IF EXISTS `Accounts`;
CREATE TABLE IF NOT EXISTS `Accounts` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `address` varchar(95) NOT NULL,
  `viewkey_hash` char(64) NOT NULL,
  `scanned_block_height` int(10) UNSIGNED NOT NULL DEFAULT '0',
  `scanned_block_timestamp` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `start_height` int(10) UNSIGNED NOT NULL DEFAULT '0',
  `generated_locally` tinyint(1) NOT NULL DEFAULT '0',
  `created` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `modified` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `address` (`address`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Inputs`
--

DROP TABLE IF EXISTS `Inputs`;
CREATE TABLE IF NOT EXISTS `Inputs` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `account_id` bigint(20) UNSIGNED NOT NULL,
  `tx_id` bigint(20) UNSIGNED NOT NULL,
  `output_id` bigint(20) UNSIGNED NOT NULL,
  `key_image` varchar(64) NOT NULL DEFAULT '',
  `amount` bigint(20) UNSIGNED ZEROFILL NOT NULL DEFAULT '00000000000000000000',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `output_id` (`output_id`,`key_image`),
  KEY `account_id2` (`account_id`),
  KEY `tx_id2` (`tx_id`),
  KEY `output_id2` (`output_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Outputs`
--

DROP TABLE IF EXISTS `Outputs`;
CREATE TABLE IF NOT EXISTS `Outputs` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `account_id` bigint(20) UNSIGNED NOT NULL,
  `tx_id` bigint(20) UNSIGNED NOT NULL,
  `out_pub_key` varchar(64) NOT NULL,
  `rct_outpk` varchar(64) NOT NULL DEFAULT '',
  `rct_mask` varchar(64) NOT NULL DEFAULT '',
  `rct_amount` varchar(64) NOT NULL DEFAULT '',
  `tx_pub_key` varchar(64) NOT NULL DEFAULT '',
  `amount` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `global_index` bigint(20) UNSIGNED NOT NULL,
  `out_index` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `mixin` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `out_pub_key` (`out_pub_key`),
  KEY `tx_id` (`tx_id`),
  KEY `account_id` (`account_id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Payments`
--

DROP TABLE IF EXISTS `Payments`;
CREATE TABLE IF NOT EXISTS `Payments` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `account_id` bigint(20) UNSIGNED NOT NULL,
  `payment_id` varchar(16) NOT NULL,
  `tx_hash` varchar(64) NOT NULL DEFAULT '',
  `request_fulfilled` tinyint(1) NOT NULL DEFAULT '0',
  `import_fee` bigint(20) NOT NULL,
  `payment_address` varchar(106) NOT NULL,
  `created` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `modified` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `account_id` (`account_id`) USING BTREE,
  UNIQUE KEY `payment_id` (`payment_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Transactions`
--

DROP TABLE IF EXISTS `Transactions`;
CREATE TABLE IF NOT EXISTS `Transactions` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `hash` varchar(64) NOT NULL,
  `prefix_hash` varchar(64) NOT NULL DEFAULT '',
  `tx_pub_key` varchar(64) NOT NULL DEFAULT '',
  `account_id` bigint(20) UNSIGNED NOT NULL,
  `blockchain_tx_id` bigint(20) UNSIGNED NOT NULL,
  `total_received` bigint(20) UNSIGNED NOT NULL,
  `total_sent` bigint(20) UNSIGNED NOT NULL,
  `unlock_time` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `height` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `spendable` tinyint(1) NOT NULL DEFAULT '0' COMMENT 'Has 10 blocks pasted since it was indexed?',
  `coinbase` tinyint(1) NOT NULL DEFAULT '0',
  `is_rct` tinyint(1) NOT NULL DEFAULT '1',
  `rct_type` int(4) NOT NULL DEFAULT '-1',
  `payment_id` varchar(64) NOT NULL DEFAULT '',
  `mixin` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `hash` (`hash`,`account_id`),
  KEY `account_id_2` (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Constraints for dumped tables
--

--
-- Constraints for table `Inputs`
--
ALTER TABLE `Inputs`
  ADD CONSTRAINT `account_id3_FK2` FOREIGN KEY (`account_id`) REFERENCES `Accounts` (`id`) ON DELETE CASCADE,
  ADD CONSTRAINT `outputs_id_FK2` FOREIGN KEY (`output_id`) REFERENCES `Outputs` (`id`) ON DELETE CASCADE,
  ADD CONSTRAINT `transaction_id_FK2` FOREIGN KEY (`tx_id`) REFERENCES `Transactions` (`id`) ON DELETE CASCADE;

--
-- Constraints for table `Outputs`
--
ALTER TABLE `Outputs`
  ADD CONSTRAINT `account_id3_FK` FOREIGN KEY (`account_id`) REFERENCES `Accounts` (`id`) ON DELETE CASCADE,
  ADD CONSTRAINT `transaction_id_FK` FOREIGN KEY (`tx_id`) REFERENCES `Transactions` (`id`) ON DELETE CASCADE;

--
-- Constraints for table `Payments`
--
ALTER TABLE `Payments`
  ADD CONSTRAINT `account_id` FOREIGN KEY (`account_id`) REFERENCES `Accounts` (`id`) ON DELETE CASCADE;

--
-- Constraints for table `Transactions`
--
ALTER TABLE `Transactions`
  ADD CONSTRAINT `account_id_FK` FOREIGN KEY (`account_id`) REFERENCES `Accounts` (`id`) ON DELETE CASCADE;
SET FOREIGN_KEY_CHECKS=1;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
