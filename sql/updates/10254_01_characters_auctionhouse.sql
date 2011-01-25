ALTER TABLE character_db_version CHANGE COLUMN required_10160_02_characters_pet_aura required_10254_01_characters_auctionhouse bit;

ALTER TABLE auctionhouse
  ADD COLUMN houseid int(11) unsigned NOT NULL default '0' AFTER id;

UPDATE auctionhouse, mangos.creature AS c, mangos.creature_template AS ct
  SET houseid =
    CASE ct.faction_A
      WHEN   12 THEN 1 /* human                                  */
      WHEN   29 THEN 6 /* orc, and generic for horde             */
      WHEN   55 THEN 2 /* dwarf/gnome, and generic for alliance  */
      WHEN   68 THEN 4 /* undead                                 */
      WHEN   80 THEN 3 /* n-elf                                  */
      WHEN  104 THEN 5 /* trolls                                 */
      WHEN  120 THEN 7 /* booty bay, neutral                     */
      WHEN  474 THEN 7 /* gadgetzan, neutral                     */
      WHEN  534 THEN 2 /* Alliance Generic                       */
      WHEN  855 THEN 7 /* everlook, neutral                      */
      WHEN 1604 THEN 6 /* b-elfs,                                */
      WHEN 1638 THEN 2 /* exodar, alliance                       */
      ELSE 0           /* auction will canceled at loading       */
    END
  WHERE auctionhouse.auctioneerguid = c.guid AND c.id = ct.entry;


ALTER TABLE auctionhouse
  DROP COLUMN auctioneerguid;

DROP TABLE IF EXISTS auction;
RENAME TABLE auctionhouse TO auction;
