UPDATE creature_template
  SET flags_extra = flags_extra | 0x00000080 WHERE entry = 1;
