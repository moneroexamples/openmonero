-- phpMyAdmin SQL Dump
-- version 4.5.2
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Dec 27, 2016 at 06:13 AM
-- Server version: 10.1.19-MariaDB
-- PHP Version: 7.0.9

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
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
CREATE TABLE `Accounts` (
  `id` bigint(10) UNSIGNED NOT NULL,
  `address` varchar(95) NOT NULL,
  `total_received` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `scanned_block_height` int(10) UNSIGNED NOT NULL DEFAULT '0',
  `start_height` int(10) UNSIGNED NOT NULL DEFAULT '0',
  `created` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `modified` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Inputs`
--

DROP TABLE IF EXISTS `Inputs`;
CREATE TABLE `Inputs` (
  `id` bigint(20) UNSIGNED NOT NULL,
  `account_id` bigint(20) UNSIGNED NOT NULL,
  `tx_id` bigint(20) UNSIGNED NOT NULL,
  `output_id` bigint(20) UNSIGNED NOT NULL,
  `key_image` varchar(64) NOT NULL DEFAULT '',
  `amount` bigint(20) UNSIGNED ZEROFILL NOT NULL DEFAULT '00000000000000000000',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Outputs`
--

DROP TABLE IF EXISTS `Outputs`;
CREATE TABLE `Outputs` (
  `id` bigint(20) UNSIGNED NOT NULL,
  `account_id` bigint(20) UNSIGNED NOT NULL,
  `tx_id` bigint(20) UNSIGNED NOT NULL,
  `out_pub_key` varchar(64) NOT NULL,
  `tx_pub_key` varchar(64) NOT NULL DEFAULT '',
  `amount` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `out_index` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `mixin` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Transactions`
--

DROP TABLE IF EXISTS `Transactions`;
CREATE TABLE `Transactions` (
  `id` bigint(20) UNSIGNED NOT NULL,
  `hash` varchar(64) NOT NULL,
  `prefix_hash` varchar(64) NOT NULL DEFAULT '',
  `account_id` bigint(20) UNSIGNED NOT NULL,
  `total_received` bigint(20) UNSIGNED NOT NULL,
  `total_sent` bigint(20) UNSIGNED NOT NULL,
  `unlock_time` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `height` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `coinbase` tinyint(1) NOT NULL DEFAULT '0',
  `payment_id` varchar(64) NOT NULL DEFAULT '',
  `mixin` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Stand-in structure for view `TransactionsWithOutsAndIns`
--
DROP VIEW IF EXISTS `TransactionsWithOutsAndIns`;
CREATE TABLE `TransactionsWithOutsAndIns` (
`tx_id` bigint(20) unsigned
,`account_id` bigint(20) unsigned
,`amount` bigint(20) unsigned zerofill
,`tx_pub_key` varchar(64)
,`out_index` bigint(20) unsigned
,`key_image` varchar(64)
,`mixin` bigint(20) unsigned
);

-- --------------------------------------------------------

--
-- Structure for view `TransactionsWithOutsAndIns`
--
DROP TABLE IF EXISTS `TransactionsWithOutsAndIns`;

CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `TransactionsWithOutsAndIns`  AS  select `Inputs`.`tx_id` AS `tx_id`,`Inputs`.`account_id` AS `account_id`,`Inputs`.`amount` AS `amount`,`Outputs`.`tx_pub_key` AS `tx_pub_key`,`Outputs`.`out_index` AS `out_index`,`Inputs`.`key_image` AS `key_image`,`Outputs`.`mixin` AS `mixin` from (`Inputs` join `Outputs` on((`Inputs`.`output_id` = `Outputs`.`id`))) ;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `Accounts`
--
ALTER TABLE `Accounts`
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `address` (`address`);

--
-- Indexes for table `Inputs`
--
ALTER TABLE `Inputs`
  ADD PRIMARY KEY (`id`),
  ADD KEY `account_id2` (`account_id`),
  ADD KEY `tx_id2` (`tx_id`),
  ADD KEY `output_id2` (`output_id`);

--
-- Indexes for table `Outputs`
--
ALTER TABLE `Outputs`
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `out_pub_key` (`out_pub_key`),
  ADD KEY `account_id` (`account_id`),
  ADD KEY `tx_id` (`tx_id`);

--
-- Indexes for table `Transactions`
--
ALTER TABLE `Transactions`
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `hash` (`hash`),
  ADD KEY `account_id` (`account_id`),
  ADD KEY `account_id_2` (`account_id`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `Accounts`
--
ALTER TABLE `Accounts`
  MODIFY `id` bigint(10) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=8;
--
-- AUTO_INCREMENT for table `Inputs`
--
ALTER TABLE `Inputs`
  MODIFY `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=5;
--
-- AUTO_INCREMENT for table `Outputs`
--
ALTER TABLE `Outputs`
  MODIFY `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=14;
--
-- AUTO_INCREMENT for table `Transactions`
--
ALTER TABLE `Transactions`
  MODIFY `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=10;
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
-- Constraints for table `Transactions`
--
ALTER TABLE `Transactions`
  ADD CONSTRAINT `account_id_FK` FOREIGN KEY (`account_id`) REFERENCES `Accounts` (`id`) ON DELETE CASCADE;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
