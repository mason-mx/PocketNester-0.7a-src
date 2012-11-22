
// data base to set the CYCLES PER LINE

CYCLES_PER_LINE = ((float)((double)1364.0/(double)12.0));
uint32 crc = ROM->crc32();
//CYCLES_BEFORE_NMI = (float)1.0;
//BANKSWITCH_PER_TILE = 0;
//DPCM_IRQ = 1;

// Mapper 000 -----------------------------------------------------------------

// added by Rick
if (crc == 0x967a605f)	// Super Mario Bros(J)
{
	PAL_mode = TRUE;
}

if(crc == 0x0b5667e9 || // Excitebike (E)
   crc == 0x3a94fa0b)   // Excitebike (JU)
{
  CYCLES_PER_LINE = 112;
}

// Mapper 001 -----------------------------------------------------------------

// added by Rick
if (crc == 0xbc7485b5 ||// Elite (PD)
	crc == 0xa4bdcc1d)	// Elite (E)
{
	CYCLES_PER_LINE = 115;
	PAL_mode = TRUE;	// Force to run in PAL mode
}

if(crc == 0x900c7442)   // Konamic Sports in Seoul (J)
{
  //BANKSWITCH_PER_TILE = 1;
}
if(crc == 0xcf9cf7a2)   // Romancia (J)
{
  //DPCM_IRQ = 1;
}
if(crc == 0xdb564628)   // Mario Open Golf (J)
{
  CYCLES_PER_LINE = 105;
}
if(crc == 0x547ad451)   // Gekitou Stadium!! (J)
{
  CYCLES_PER_LINE = 111;
}
if(crc == 0xaf16ee39)   // Hyokkori Hyoutan Jima - Nazo no Kaizokusen (J)
{
  CYCLES_PER_LINE = 112;
}
if(crc == 0x9183054e)   // Dengeki - Big Bang! (J)
{
  CYCLES_PER_LINE = 115;
}
if(crc == 0x7e5d2f1a || // Itadaki Street - Watashi no Mise ni Yottette (J)
   crc == 0x37bf04d7)   // Marusa no Onna (J)
{
  CYCLES_PER_LINE = 120;
}
if(crc == 0x11469ce3)   // Viva! Las Vegas (J)
{
  CYCLES_PER_LINE = 130;
}
if(crc == 0x58507bc9)   // Famicom Top Management (J)
{
  CYCLES_PER_LINE = 160;
}

// Mapper 002 -----------------------------------------------------------------

if(crc == 0x7b0a41b9)   // Esper Bouken Tai (J)
{
  CYCLES_PER_LINE = 128;
}
if(crc == 0xb59a7a29)   // Guardic Gaiden (J)
{
  CYCLES_PER_LINE = 136;
}

// Mapper 003 -----------------------------------------------------------------

if(crc == 0xca26a0f1)   // Family Trainer - Dai Undoukai (J)
{
  CYCLES_PER_LINE = 113;
}
if(crc == 0xa30d8baf)   // Tokoro San no Mamorumo Semerumo (J)
{
  CYCLES_PER_LINE = 115;
  //CYCLES_BEFORE_NMI = 10;
  //BANKSWITCH_PER_TILE = 1;
}

// Mapper 004 -----------------------------------------------------------------

// Added by Rick
// Tun Shi Tian Di 2(Chinese hack of "Destiny of an Emperor 2(j)")
if (crc == 0x50d82653 ||	// TSTD2
	crc == 0x8599d7a1)		// Fixed TSTD2 by Necrosaro
{
	PAL_mode = TRUE;	// Force to run in PAL mode
}

if(crc == 0x14a01c70)   // Gun-Dec (J)
{
  //BANKSWITCH_PER_TILE = 1;
}
if(crc == 0xb42feeb4)   // Beauty and the Beast (E)
{
  CYCLES_PER_LINE = 107;
}
if(crc == 0x0baf01d0)   // Juju Densetsu (J)
{
  CYCLES_PER_LINE = 112;
}
if(crc == 0x5c707ac4)   // Mother (J)
{
  CYCLES_PER_LINE = 114;
  //BANKSWITCH_PER_TILE = 1;
}
if(crc == 0xcb106f49 || // F-1 Sensation (J)
   crc == 0x19004d03)   // Evil Kirby (Hack)
{
  CYCLES_PER_LINE = 115;
}
if(crc == 0x8b59bac3 || // Radia Senki - Reimei Hen (J)
   crc == 0x00161afd || // Ys 2 - Ancient Ys Vanished The Final Chapter (J)
   crc == 0x26ff3ea2)   // Yume Penguin Monogatari (J)
{
  CYCLES_PER_LINE = 120;
}
if(crc == 0x459adfa8 || // Cattou Ninden Teyandee (J)
   crc == 0xbea682e5 || // Doki! Doki! Yuuenchi (J)
   crc == 0xf1081b1b || // Heavy Barrel (J)
   crc == 0xb4d554d6 || // Heavy Barrel (U)
   crc == 0x9a172152 || // Jackie Chan (J)
   crc == 0x170739cf || // Jigoku Gokuraku Maru (J)
   crc == 0xdeddd5e5 || // Kunio Kun no Nekketsu Soccer League (J)
   crc == 0x0c935dfe || // Might and Magic (J)
   crc == 0x85c5b6b4 || // Nekketsu Kakutou Densetsu (J)
   crc == 0x6bbd9f11 || // Utsurun Desu (J)
   crc == 0x23f4b48f)   // Wily & Light no Rockboard - That's Paradise (J)
{
  CYCLES_PER_LINE = 128;
}
if(crc == 0xa9a0d729 || // Dai Kaijuu - Deburas (J)
   crc == 0x346cd5d3 || // Ike Ike! Nekketsu Hockey Bu - Subette Koronde Dai Rantou (J)
   crc == 0x5fbd9178 || // Mitsume ga Tooru (English v1_01)
   crc == 0x390e0320 || // Mitsume ga Tooru (J)
   crc == 0x6055fe9b || // Tecmo Super Bowl (J)
   crc == 0x8e5c2818 || // Tecmo Super Bowl (U)
   crc == 0xe3765667)   // Top Gun - The Second Mission (U)
{
  CYCLES_PER_LINE = 136;
}
if(crc == 0x28cb6c6b)   // Hi-Game 1999 Super Donkey Kong 2 (Pirate).nes
{
  CYCLES_PER_LINE = 140;
}
if(crc == 0x0ae5b57f || // Super Donkey Kong 2 (Unk)
   crc == 0x63e09bba)   // Super Donkey Kong 2 (Unk) (Alt)
{
  CYCLES_PER_LINE = 140;
}
if(crc == 0x336e2a6f || // Asmik Kun Land (J)
   crc == 0x7ca52798 || // Chiki Chiki Machine Mou Race (J)
   crc == 0xc17ae2dc || // God Slayer - Haruka Tenkuu no Sonata (J)
   crc == 0x8685f366 || // Matendouji (J)
   crc == 0xb55da544 || // Nekketsu Koukou - Dodgeball Bu - Soccer Hen (J)
   crc == 0x4fb460cd || // Nekketsu! Street Basket - Ganbare Dunk Heroes (J)
   crc == 0xe19a2473 || // Sugoro Quest - Dice no Senshi Tachi (J)
   crc == 0x14a45522 || // Sugoro Quest - The Quest of Dice Heros (English vX.XX)
   crc == 0x80cd1919 || // Super Mario Bros 3 (E)
   crc == 0xe386da54 || // Super Mario Bros 3 (J)
   crc == 0xd852c2f7 || // Time Zone (J)
   crc == 0xade11141)   // Wanpaku Kokkun no Gourmet World (J)
{
  CYCLES_PER_LINE = 144;
}
if(crc == 0x3c5ae54b || // 1999 - Hore, Mitakotoka! Seikimatsu (J)
   crc == 0xb548fd2e || // Bananan Ouji no Dai Bouken (J)
   crc == 0xeffeea40 || // Klax (J)
   crc == 0x2dc3817d || // Meimon! Takonishi Ouendan - Kouha 6 Nin Shuu (J)
   crc == 0x5c1d053a || // SD Battle Oozumou - Heisei Hero Basho (J)
   crc == 0x0bf31a3d)   // Soreike! Anpanman - Minna de Hiking Game! (J)
{
  CYCLES_PER_LINE = 152;
}

// Mapper 007 -----------------------------------------------------------------

if(crc == 0x524a5a32 || // Battletoads (E)
   crc == 0x9806cb84 || // Battletoads (J)
   crc == 0x279710dc)   // Battletoads (U)
{
  CYCLES_PER_LINE = 112;
}
if(crc == 0x23d7d48f || // Battletoads Double Dragon(E)
   crc == 0xceb65b06)   // Battletoads Double Dragon(U)
{
  CYCLES_PER_LINE = 111;
}

// Mapper 016 -----------------------------------------------------------------

if(crc == 0x6c6c2feb || // Dragon Ball 3 - Gokuu Den (J)
   crc == 0x0c187747)   // Nishimura Kyoutarou Mystery - Blue Train Satsujin Jiken (J).
{
  CYCLES_PER_LINE = 122;
}

// Mapper 018 -----------------------------------------------------------------

if(crc == 0xa54d9086)   // Toukon Club (J)
{
  CYCLES_PER_LINE = 114;
}

// Mapper 019 -----------------------------------------------------------------

if(crc == 0x17421900)   // Youkai Douchuuki (J)
{
  //BANKSWITCH_PER_TILE = 1;
}
if(crc == 0x6901346e)   // Sangokushi 2 - Haou no Tairiku (J)
{
  CYCLES_PER_LINE = 105;
}
if(crc == 0x761ccfb5)   // Digital Devil Story - Megami Tensei 2 (J)
{
  CYCLES_PER_LINE = 119;
}
if(crc == 0x9a2b0641)   // Namco Classic 2 (J)
{
  CYCLES_PER_LINE = 120;
}
if(crc == 0x2b825ce1 || // Namco Classic (J)
   crc == 0x3deac303)   // Rolling Thunder (J)
{
  CYCLES_PER_LINE = 144;
}
if(crc == 0x96533999)   // Dokuganryuu Masamune (J)
{
  //CYCLES_BEFORE_NMI = 10;
}

// Mapper 020 -----------------------------------------------------------------
/*
if((fds_id() == 0xb24b4e4d) || // Kinnikuman - Kinnikusei Oui Soudatsusen
   (fds_id() == 0x014c4e4b) || // Legend of Zelda 2 - Link no Bouken, The
   (fds_id() == 0xc1414c4e))   // Super Boy Allan
{
  CYCLES_PER_LINE =117;
}
*/
// Mapper 021 -----------------------------------------------------------------

if(crc == 0xb201b522)   // Wai Wai World 2 - SOS!! Paseri Jou (J)
{
  CYCLES_PER_LINE = 117;
}

// Mapper 023 -----------------------------------------------------------------

if(crc == 0x64818fc5)   // Wai Wai World (J)
{
  CYCLES_PER_LINE = 115;
}

// Mapper 024 -----------------------------------------------------------------

if(crc == 0xba6bdd6a)   // Akumajou Densetsu (J)
{
  CYCLES_PER_LINE = 119;
}

// Mapper 032 -----------------------------------------------------------------

if(crc == 0x788bed9a)   // Meikyuu Jima (J)
{
  CYCLES_PER_LINE = 114;
}

// Mapper 065 -----------------------------------------------------------------

if(crc == 0xd202612b)   // Spartan X 2 (J)
{
  CYCLES_PER_LINE = 125;
}

// Mapper 088 -----------------------------------------------------------------

if(crc == 0xc1b6b2a6)   // Devil Man (J)
{
}

// Mapper 094 -----------------------------------------------------------------

if(crc == 0x441aeae6)   // Senjou no Ookami (J)
{
  CYCLES_PER_LINE = 117;
}

// Mapper 246 -----------------------------------------------------------------

if(crc == 0xea76fb00)   // Fong Shen Bang
{
  CYCLES_PER_LINE = 113;
}
