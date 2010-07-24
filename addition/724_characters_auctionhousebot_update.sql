/*
SQLyog Community Edition- MySQL GUI v6.03
Host - 5.0.51b-community-nt : Database - characters
*********************************************************************
Server version : 5.0.51b-community-nt
*/

/*!40101 SET NAMES utf8 */;

/*!40101 SET SQL_MODE=''*/;

/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;

/*Table structure for table `auctionhousebot` */

ALTER TABLE `auctionhousebot`
  ADD COLUMN `percentgreytradegoods` int(11) default '0' COMMENT 'Sets the percentage of the Grey Trade Goods auction items' AFTER `maxtime`,
  ADD COLUMN `percentorangetradegoods` int(11) default '0' COMMENT 'Sets the percentage of the Orange Trade Goods auction items' AFTER `percentpurpletradegoods`,
  ADD COLUMN `percentyellowtradegoods` int(11) default '0' COMMENT 'Sets the percentage of the Yellow Trade Goods auction items' AFTER `percentorangetradegoods`,
  ADD COLUMN `percentgreyitems` int(11) default '0' COMMENT 'Sets the percentage of the non trade Grey auction items' AFTER `percentyellowtradegoods`,
  ADD COLUMN `percentorangeitems` int(11) default '0' COMMENT 'Sets the percentage of the non trade Orange auction items' AFTER `percentpurpleitems`,
  ADD COLUMN `percentyellowitems` int(11) default '0' COMMENT 'Sets the percentage of the non trade Yellow auction items' AFTER `percentorangeitems`,
  ADD COLUMN `minpricegrey` int(11) default '100' COMMENT 'Minimum price of Grey items (percentage).' AFTER `percentyellowitems`,
  ADD COLUMN `maxpricegrey` int(11) default '150' COMMENT 'Maximum price of Grey items (percentage).' AFTER `minpricegrey`,
  ADD COLUMN `minpriceorange` int(11) default '3250' COMMENT 'Minimum price of Orange items (percentage).' AFTER `maxpricepurple`,
  ADD COLUMN `maxpriceorange` int(11) default '5550' COMMENT 'Maximum price of Orange items (percentage).' AFTER `minpriceorange`,
  ADD COLUMN `minpriceyellow` int(11) default '5250' COMMENT 'Minimum price of Yellow items (percentage).' AFTER `maxpriceorange`,
  ADD COLUMN `maxpriceyellow` int(11) default '6550' COMMENT 'Maximum price of Yellow items (percentage).' AFTER `minpriceyellow`,
  ADD COLUMN `minbidpricegrey` int(11) default '70' COMMENT 'Starting bid price of Grey items as a percentage of the randomly chosen buyout price. Default: 70' AFTER `maxpriceyellow`,
  ADD COLUMN `maxbidpricegrey` int(11) default '100' COMMENT 'Starting bid price of Grey items as a percentage of the randomly chosen buyout price. Default: 100' AFTER `minbidpricegrey`,
  ADD COLUMN `minbidpriceorange` int(11) default '80' COMMENT 'Starting bid price of Orange items as a percentage of the randomly chosen buyout price. Default: 80' AFTER `maxbidpricepurple`,
  ADD COLUMN `maxbidpriceorange` int(11) default '100' COMMENT 'Starting bid price of Orange items as a percentage of the randomly chosen buyout price. Default: 100' AFTER `minbidpriceorange`,
  ADD COLUMN `minbidpriceyellow` int(11) default '80' COMMENT 'Starting bid price of Yellow items as a percentage of the randomly chosen buyout price. Default: 80' AFTER `maxbidpriceorange`,
  ADD COLUMN `maxbidpriceyellow` int(11) default '100' COMMENT 'Starting bid price of Yellow items as a percentage of the randomly chosen buyout price. Default: 100' AFTER `minbidpriceyellow`,
  ADD COLUMN `maxstackgrey` int(11) default '0' COMMENT 'Stack size limits for item qualities - a value of 0 will disable a maximum stack size for that quality, which will allow the bot to create items in stack as large as the item allows.' AFTER `maxbidpriceyellow`,
  ADD COLUMN `maxstackorange` int(11) default '1' COMMENT 'Stack size limits for item qualities - a value of 0 will disable a maximum stack size for that quality, which will allow the bot to create items in stack as large as the item allows.' AFTER `maxstackpurple`,
  ADD COLUMN `maxstackyellow` int(11) default '1' COMMENT 'Stack size limits for item qualities - a value of 0 will disable a maximum stack size for that quality, which will allow the bot to create items in stack as large as the item allows.' AFTER `maxstackorange`,
  ADD COLUMN `buyerpriceorange` int(11) default '20' COMMENT 'Multiplier to vendorprice when buying orange items from auctionhouse' AFTER `buyerpricepurple`,
  ADD COLUMN `buyerpriceyellow` int(11) default '22' COMMENT 'Multiplier to vendorprice when buying yellow items from auctionhouse' AFTER `buyerpriceorange`;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;

