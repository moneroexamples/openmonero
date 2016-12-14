-- phpMyAdmin SQL Dump
-- version 4.6.5.2
-- https://www.phpmyadmin.net/
--
-- Host: localhost
-- Generation Time: Dec 14, 2016 at 10:03 AM
-- Server version: 5.7.16-0ubuntu0.16.10.1
-- PHP Version: 7.0.8-3ubuntu3

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `yourmonero`
--
CREATE DATABASE IF NOT EXISTS `yourmonero` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;
USE `yourmonero`;

-- --------------------------------------------------------

--
-- Table structure for table `Accounts`
--

CREATE TABLE `Accounts` (
  `id` bigint(10) UNSIGNED NOT NULL,
  `address` varchar(95) NOT NULL,
  `total_received` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `scanned_height` int(10) UNSIGNED NOT NULL DEFAULT '0',
  `scanned_block_height` int(10) UNSIGNED NOT NULL DEFAULT '0',
  `start_height` int(10) UNSIGNED NOT NULL DEFAULT '0',
  `transaction_height` int(10) UNSIGNED NOT NULL DEFAULT '0',
  `blockchain_height` int(10) UNSIGNED NOT NULL DEFAULT '0',
  `total_sent` int(10) UNSIGNED NOT NULL DEFAULT '0',
  `created` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `modified` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Transactions`
--

CREATE TABLE `Transactions` (
  `id` bigint(20) UNSIGNED NOT NULL,
  `hash` varchar(64) NOT NULL,
  `account_id` bigint(20) UNSIGNED NOT NULL,
  `total_received` bigint(20) UNSIGNED NOT NULL,
  `total_sent` bigint(20) UNSIGNED NOT NULL,
  `unlock_time` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `height` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `coinbase` tinyint(1) NOT NULL DEFAULT '0',
  `mixin` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
  MODIFY `id` bigint(10) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=54;
--
-- Constraints for dumped tables
--

--
-- Constraints for table `Transactions`
--
ALTER TABLE `Transactions`
  ADD CONSTRAINT `account_id_FK` FOREIGN KEY (`account_id`) REFERENCES `Accounts` (`id`) ON DELETE CASCADE;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
