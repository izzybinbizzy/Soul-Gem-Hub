// Soul Gem Hub - SKSE plugin
// Copyright (C) 2026 izzydoingit
// GPL-3.0-or-later; see LICENSE.txt and the notice at the top of main.cpp.
//
// GENERATED - never edited by hand (see README.md, "The permission table"). The soul gem mods this plugin may not light
// until their authors allow it (their Nexus permissions): the fingerprint (FNV-1a 64 of the first 64 KB, then the size) of
// each of their gem models and the textures those name, minus every file the game or an allowed mod ships too, and the
// file names only such mods ship. GemLights.cpp looks a gem's model and its color textures up in both.

#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace Gems
{
	inline constexpr std::array<std::string_view, 14> kBlockedMods{ {
		"ElSopa - Azura's Star Redone",
		"Immersive Soulgems",
		"Kanjs - Soul Gems",
		"MultiLayer Parallax Soul Gems",
		"MultiLayer Parallax Soul Gems or Vayura Soulgems Replacer",
		"NeV-P Extreme Crystals",
		"Remastered Remastered Soul Gems",
		"Remastered Remastered Soul Gems or Skyking Soul Gems or Skyrim Remastered - Soul Gems",
		"Remastered Remastered Soul Gems or Skyrim Remastered - Soul Gems",
		"Rustic Soulgems",
		"Skyking Soul Gems",
		"Skyking Soul Gems or Skyrim Remastered - Soul Gems",
		"Skyrim Remastered - Soul Gems",
		"Vayura Soulgems Replacer",
	} };

	struct BlockedFile
	{
		std::uint64_t hash;
		std::uint8_t  mod;  // index into kBlockedMods
	};

	// sorted by hash
	inline constexpr std::array<BlockedFile, 457> kBlockedFiles{ {
		{ 0x000F292E1CF93AC7ULL, 2 },   // meshes\clutter\soulgem\soulgempiece05.nif
		{ 0x00AA2DD3688FBDC6ULL, 12 },  // meshes\clutter\soulgem\soulgemcommon01.nif
		{ 0x010068E79C337829ULL, 0 },   // textures\sopawhiteazura_em.dds
		{ 0x010CFE00F881530DULL, 6 },   // textures\clutter\soulgem\soulgems01_g.dds
		{ 0x0185A878CEDEE58BULL, 10 },  // skyking soul gems\meshes\dlc01\clutter\dlc01soulgembowlfull.nif
		{ 0x0196F72CDCC86844ULL, 3 },   // textures\clutter\soulgem\soulgemcommon.dds
		{ 0x02DDA8849A980C2AULL, 3 },   // textures\clutter\soulgem\soulgemgreater.dds
		{ 0x046B863310B51650ULL, 12 },  // textures\remastered\soulgems\fragments\soulgemfragment04.dds
		{ 0x050000D854280E42ULL, 5 },   // meshes\clutter\soulgem\soulgempetty01.nif
		{ 0x0568F9494D761FB1ULL, 5 },   // textures\cubemaps\shinyglass_e.dds
		{ 0x057A9CC79131F17EULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgemgrand_full.nif
		{ 0x05FFEF9C1BE4EF54ULL, 3 },   // textures\clutter\soulgem\soulgemgrand.dds
		{ 0x0655A28AF9B6FDE0ULL, 6 },   // textures\dlc01\clutter\dlc01soulgembowlfull_n.dds
		{ 0x065B0DAC3AB3F45EULL, 2 },   // meshes\clutter\soulgem\soulgempiece02.nif
		{ 0x06738FC6B72D4154ULL, 12 },  // meshes\clutter\soulgem\soulgempetty01filled.nif
		{ 0x072628333BD4B160ULL, 9 },   // meshes\clutter\soulgem\soulgemgreater01.nif
		{ 0x0759B15C77920589ULL, 2 },   // textures\clutter\soulgem\soulgemcommon_n.dds
		{ 0x07C71D418559E66EULL, 3 },   // meshes\clutter\soulgem\full\soulgemlesser01.nif
		{ 0x08237E0D18434091ULL, 12 },  // textures\remastered\soulgems\soulgemlesser.dds
		{ 0x0922A0B6E01EF274ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemblack01lesserfill.nif
		{ 0x09A7BC5D316013AAULL, 5 },   // textures\clutter\soulgem\soulgreater_g.dds
		{ 0x0A85899C44DED6F5ULL, 3 },   // meshes\dlc01\clutter\dlc01soulgemchunk01.nif
		{ 0x0B1957719F3EF59DULL, 12 },  // textures\remastered\soulgems\fragments\soulgemfragment01.dds
		{ 0x0B7FF7D46D9A7FF4ULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgemgreater01filled.nif
		{ 0x0C56E50011D3FEBCULL, 12 },  // textures\clutter\daedric\lite\azurastarcrystalgemslite_n.dds
		{ 0x0CFDB9ED9331086CULL, 1 },   // data\meshes\clutter\soulgem\soulgemgreater01_full.nif
		{ 0x0E4C2B0F607037B8ULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgemgrand01filled.nif
		{ 0x0F2125326E037411ULL, 9 },   // meshes\clutter\soulgem\soulgempiece05.nif
		{ 0x0FA73CA86BC804F3ULL, 3 },   // meshes\clutter\soulgem\full\soulgemcommon01.nif
		{ 0x10AFD72F1D9EF5B7ULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgempiece03.nif
		{ 0x10BDCA8676C9A49DULL, 12 },  // textures\clutter\soulgem\soulgems01_g.dds
		{ 0x10C386F053E0D390ULL, 9 },   // meshes\clutter\soulgem\soulgemcommon_full.nif
		{ 0x110461D8067E923DULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgrand01lesserfill.nif
		{ 0x118BF18C1CB8CC27ULL, 12 },  // textures\clutter\daedric\broken\azurastarcrystalbranchesbroken_n.dds
		{ 0x11925D31A8A81E47ULL, 2 },   // meshes\clutter\daedric\azurastar.nif
		{ 0x11EAF3D22CB9864DULL, 12 },  // textures\remastered\soulgems\soulgemcommon.dds
		{ 0x13C62B319C307B1AULL, 5 },   // textures\clutter\soulgem\soulpetty_g.dds
		{ 0x1407A56DA05F40AAULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgemgreater01.nif
		{ 0x1434D102B7B9AF8BULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgempiece04.nif
		{ 0x14B9E789927B5C09ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgrand01pettyfill.nif
		{ 0x14D579F3C0A453F4ULL, 0 },   // meshes\clutter\daedric\azurastarlite.nif
		{ 0x168A642BDB6918A0ULL, 2 },   // meshes\clutter\daedric\azurastarbroken.nif
		{ 0x16D5B1429CED247CULL, 12 },  // textures\clutter\daedric\dark\azurastarcrystalgemsdark.dds
		{ 0x172E113F91DD9C65ULL, 3 },   // textures\clutter\soulgem\soulgemcommonsubsurf.dds
		{ 0x1738B77089F8417BULL, 12 },  // meshes\clutter\soulgem\soulgemlesser01filled.nif
		{ 0x17BE1F79477958C5ULL, 3 },   // textures\clutter\soulgem\soulgemcommon_transluc.dds
		{ 0x18D732F143855B45ULL, 12 },  // textures\clutter\daedric\broken\azurastarmetalbranchesbroken.dds
		{ 0x199D27163942E2B1ULL, 1 },   // data\meshes\clutter\soulgem\soulgemblack01_full.nif
		{ 0x19A2FA38548FAE45ULL, 3 },   // meshes\clutter\soulgem\full\soulgempetty01.nif
		{ 0x19A758B54857A66AULL, 12 },  // textures\remastered\soulgems\fragments\soulgemfragment03_n.dds
		{ 0x1B8906184C790002ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgempiece04.nif
		{ 0x1BB2966239E369F6ULL, 3 },   // textures\clutter\soulgem\soulgemgreater_transluc.dds
		{ 0x1C8ADDBA396F8061ULL, 5 },   // textures\clutter\soulgem\soulgemgrand_m.dds
		{ 0x202ED5E1268A1008ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgemblack_full.nif
		{ 0x209A7DF0463B41CAULL, 12 },  // meshes\clutter\soulgem\soulgemcommon01filled.nif
		{ 0x214771B2578316F3ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemblack01pettyfill.nif
		{ 0x219FDFFC5A9DCE79ULL, 12 },  // textures\remastered\soulgems\soulgemparallax.dds
		{ 0x227EF4277963F5FAULL, 3 },   // meshes\clutter\soulgem\full\soulgempetty01.nif
		{ 0x230B8DA109218E67ULL, 12 },  // textures\clutter\daedric\lite\azurastarlite_n.dds
		{ 0x233296238B2E2D43ULL, 3 },   // meshes\clutter\soulgem\full\soulgempetty01.nif
		{ 0x23BE57DB2257E4DDULL, 6 },   // meshes\clutter\soulgem\soulgemgreater01.nif
		{ 0x24B0521077152D6BULL, 9 },   // textures\clutter\soulgem\soulgemgrand.dds
		{ 0x2517757CEA0F6D3CULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemblack01pettyfill.nif
		{ 0x2523E63557E72ADCULL, 2 },   // meshes\clutter\soulgem\soulgempetty01.nif
		{ 0x2529FD7DEBAFE065ULL, 12 },  // textures\dlc01\clutter\dlc01soulgembowlfull.dds
		{ 0x2539E85076868195ULL, 12 },  // meshes\clutter\soulgem\soulgempiece01.nif
		{ 0x265F1F02A947CBFAULL, 9 },   // meshes\clutter\soulgem\soulgempetty01.nif
		{ 0x26B7EC1833CD3DF5ULL, 2 },   // textures\clutter\soulgem\soulgems01_g.dds
		{ 0x272512E8D458F7D9ULL, 12 },  // meshes\clutter\soulgem\soulgemholder01.nif
		{ 0x2804B3EC6CBE34D8ULL, 2 },   // meshes\clutter\soulgem\soulgemcommon01.nif
		{ 0x28BFAB31DA4A6D00ULL, 2 },   // meshes\clutter\soulgem\soulgemlesser01.nif
		{ 0x2A014808E0E54634ULL, 5 },   // textures\clutter\soulgem\soulgempetty_n.dds
		{ 0x2A650F828ACFBCC8ULL, 12 },  // meshes\clutter\daedric\azurastarbroken.nif
		{ 0x2AC8879F88AB4118ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgemgreater01.nif
		{ 0x2B41ED71373FC366ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgrand01commonfill.nif
		{ 0x2B8BBA679F9F79BFULL, 3 },   // textures\clutter\soulgem\soulgemcommon_n.dds
		{ 0x2BAC05A5DD5703F8ULL, 9 },   // meshes\clutter\soulgem\soulgemlesser_full.nif
		{ 0x2BFEFF62C9179015ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgemcommon01.nif
		{ 0x2CF3796E3D359A37ULL, 12 },  // textures\remastered\soulgems\fragments\soulgemfragment03.dds
		{ 0x2D43F87DA09885D7ULL, 2 },   // textures\clutter\soulgem\soulgemholder_n.dds
		{ 0x2D7C797FF6B38984ULL, 12 },  // textures\clutter\daedric\lite\azurastarcrystalgemslite.dds
		{ 0x2D9F443E7B77E91FULL, 12 },  // textures\dlc01\clutter\dlc01soulgembowlfull_n.dds
		{ 0x2E329566B1D7C75FULL, 2 },   // textures\clutter\daedric\azurastar.dds
		{ 0x2F4326AEFE0D89EDULL, 12 },  // textures\clutter\soulgem\soulgemholder_n.dds
		{ 0x2F95956F7FC8AFDBULL, 6 },   // textures\remastered\soulgems\soulgempetty.dds
		{ 0x307B1DF34A673A8EULL, 2 },   // textures\clutter\daedric\azurastar_n.dds
		{ 0x31C5071163D6A5D0ULL, 3 },   // textures\clutter\soulgem\soulgemgrand_transluc.dds
		{ 0x3421CB1992DE9A02ULL, 0 },   // textures\sopablack_e.dds
		{ 0x349F482D76208A77ULL, 12 },  // meshes\clutter\soulgem\soulgempetty01filled.nif
		{ 0x353E6F5DAFFC1DDAULL, 2 },   // textures\clutter\soulgem\soulgemholder_m.dds
		{ 0x35AFA5D28B4EB08DULL, 5 },   // meshes\clutter\soulgem\soulgempiece01.nif
		{ 0x37EDAECFD74FF6A3ULL, 3 },   // textures\clutter\soulgem\soulgemgrand_n.dds
		{ 0x381B6D65F089E1EDULL, 9 },   // meshes\clutter\soulgem\soulgemgreater_full.nif
		{ 0x383A2FAFEBB0B4AFULL, 3 },   // meshes\clutter\soulgem\soulgemlesser01.nif
		{ 0x38CC561211F53F4CULL, 9 },   // meshes\clutter\soulgem\soulgempiece03.nif
		{ 0x38D43AED7F87AF21ULL, 0 },   // textures\sopawhiteazura.dds
		{ 0x390096BE26E5026BULL, 2 },   // textures\clutter\soulgem\soulgemholder.dds
		{ 0x39B6061030B91153ULL, 12 },  // textures\clutter\daedric\dark\azurastarmetalbranchesdark_n.dds
		{ 0x3A221F93031D6EA9ULL, 6 },   // meshes\clutter\soulgem\soulgemcommon01filled.nif
		{ 0x3B18073B535EFFD6ULL, 12 },  // textures\clutter\daedric\broken\azurastarcrystalcenterbroken_n.dds
		{ 0x3B284FBC782F5467ULL, 5 },   // meshes\clutter\soulgem\soulgemlesser01.nif
		{ 0x3BBA1ADD235072D9ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgempiece03.nif
		{ 0x3C00453DA1881D80ULL, 9 },   // textures\clutter\soulgem\soulgemcommon.dds
		{ 0x3C397F8E2C79A247ULL, 1 },   // data\meshes\clutter\soulgem\soulgempetty01_full.nif
		{ 0x3C40D5A90C8BA228ULL, 12 },  // textures\remastered\soulgems\dwemer\dwesoulgemcontainergems_n.dds
		{ 0x3C60053E3CF83039ULL, 5 },   // textures\clutter\soulgem\soulgempetty.dds
		{ 0x3DD236878BE0E844ULL, 6 },   // textures\remastered\soulgems\soulgemgreater.dds
		{ 0x3DEA7582BD96D8CBULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgempetty01filled.nif
		{ 0x3DF40C4B74CC9AFFULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgreater01commonfill.nif
		{ 0x3E42F44E140870B7ULL, 5 },   // meshes\clutter\soulgem\soulgemcommon01fx.nif
		{ 0x3E71DC4564FAEC9CULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgrand01commonfill.nif
		{ 0x3E7692F8BD625CADULL, 6 },   // textures\remastered\soulgems\fragments\soulgemfragment03_n.dds
		{ 0x3E99A229DCB65E40ULL, 2 },   // textures\clutter\daedric\azurastarlite.dds
		{ 0x3F0F77ACFBD74C76ULL, 12 },  // textures\clutter\daedric\lite\azurastarcrystalbrancheslite.dds
		{ 0x401664D0F5491C70ULL, 12 },  // textures\clutter\daedric\dark\azurastarmetalbranchesdark_m.dds
		{ 0x4089CF61D2F779F0ULL, 12 },  // textures\remastered\soulgems\soulgemblack_n.dds
		{ 0x40DB63D739518620ULL, 6 },   // textures\dlc01\clutter\dlc01soulgembowlfull.dds
		{ 0x40F6052AE1B8B49DULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemblack01commonfill.nif
		{ 0x4157F520A6891A82ULL, 9 },   // textures\clutter\soulgem\soulgemlesser.dds
		{ 0x416E721FB68C438AULL, 12 },  // textures\remastered\soulgems\soulgemgreater_n.dds
		{ 0x4254E8E0B81E734BULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemlesser01pettyfill.nif
		{ 0x4285B3DD43B7B629ULL, 12 },  // textures\remastered\soulgems\soulgempetty_n.dds
		{ 0x43E6044CE311215FULL, 2 },   // meshes\clutter\daedric\azurastar.nif
		{ 0x4433D81024B7DAF7ULL, 5 },   // textures\nev-p\clutter\geodeametista.dds
		{ 0x451832E4E6FEA1ABULL, 3 },   // textures\clutter\soulgem\soulgempettysubsurf.dds
		{ 0x45DB22C948A5CD42ULL, 5 },   // meshes\clutter\soulgem\soulgempetty01fx.nif
		{ 0x4811A1C8D19F5E6EULL, 5 },   // textures\clutter\daedric\azurastar_em.dds
		{ 0x49ACCCC088C6B599ULL, 5 },   // textures\clutter\soulgem\soulcommon_g.dds
		{ 0x4A62C81823EC35E1ULL, 12 },  // textures\clutter\daedric\broken\azurastarcrystalgemsbroken.dds
		{ 0x4A81110083473FDBULL, 12 },  // textures\clutter\daedric\broken\azurastarmetalbranchesbroken_m.dds
		{ 0x4B154D371C3B94A0ULL, 12 },  // textures\clutter\daedric\broken\azurastarmetalbranchesbroken_n.dds
		{ 0x4BE89CC863274F3AULL, 1 },   // data\meshes\clutter\soulgem\soulgemgrand01.nif
		{ 0x4CB1EBD78A1651D7ULL, 5 },   // textures\clutter\daedric\azurastar_n.dds
		{ 0x4D20235CBD8AB703ULL, 2 },   // meshes\clutter\soulgem\soulgempiece04.nif
		{ 0x4D285D719DECFAAAULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemblack01lesserfill.nif
		{ 0x4E6459B6C153E8A6ULL, 12 },  // meshes\dlc01\clutter\dlc01soulgemchunk01.nif
		{ 0x4EF9BE79598B5128ULL, 2 },   // textures\clutter\soulgem\soulgemgrand.dds
		{ 0x505C59E338193485ULL, 3 },   // textures\clutter\soulgem\soulgemlessersubsurf.dds
		{ 0x5076173DB98CF0DCULL, 9 },   // meshes\clutter\soulgem\soulgempetty_full.nif
		{ 0x50D34E8E236D7635ULL, 3 },   // meshes\clutter\soulgem\full\soulgemgrand01.nif
		{ 0x51C7E5F7BE52D443ULL, 0 },   // textures\innerstalhrimblack.dds
		{ 0x52B58B43EDFB6329ULL, 5 },   // meshes\clutter\soulgem\soulgempiece05.nif
		{ 0x52E3C7320C7365D7ULL, 6 },   // textures\remastered\soulgems\fragments\soulgemfragment02_n.dds
		{ 0x52F1856B7FCEE487ULL, 3 },   // meshes\clutter\soulgem\full\soulgemlesser01.nif
		{ 0x53D270681B6ED303ULL, 12 },  // textures\remastered\soulgems\dwemer\dwesoulgemcontainergems.dds
		{ 0x542EBF8C879D116CULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgempetty01.nif
		{ 0x554D4D7962ECE2EDULL, 6 },   // textures\remastered\soulgems\soulgemblack.dds
		{ 0x559A6B38514EA370ULL, 5 },   // meshes\clutter\soulgem\soulgemblack01.nif
		{ 0x56A812EE90A1320DULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemcommon01lesserfill.nif
		{ 0x574D5AFD493D542CULL, 12 },  // textures\clutter\daedric\lite\azurastarmetalcenterlite_m.dds
		{ 0x57A7F7D98207AF8DULL, 6 },   // textures\remastered\soulgems\fragments\soulgemfragment05.dds
		{ 0x5811908F014B0134ULL, 12 },  // meshes\clutter\soulgem\soulgemcommon01filled.nif
		{ 0x588AFCE811AB7300ULL, 5 },   // textures\clutter\soulgem\soulgemgrand_n.dds
		{ 0x58BFE30A0AEDCEABULL, 0 },   // textures\outerstalhrim.dds
		{ 0x5917BFB036ED7D67ULL, 5 },   // textures\clutter\soulgem\soulgemgreater.dds
		{ 0x593A3C8D4BBA2C4BULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgreater01lesserfill.nif
		{ 0x59DA6F0E50FCEC4FULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemlesser01pettyfill.nif
		{ 0x5A431A44A6E3F33BULL, 5 },   // meshes\clutter\soulgem\soulgemgrand01.nif
		{ 0x5C3DD379A4412880ULL, 3 },   // textures\clutter\soulgem\soulgemgreater.dds
		{ 0x5CC6F8B0596ABF1AULL, 5 },   // meshes\clutter\soulgem\soulgemgreater01.nif
		{ 0x5D4ADD91C377A27EULL, 5 },   // meshes\clutter\soulgem\soulgemgreater01fx.nif
		{ 0x5E4BEAD689B4C55AULL, 0 },   // meshes\clutter\daedric\azurastarbroken.nif
		{ 0x5F009173503CFA9BULL, 6 },   // meshes\clutter\soulgem\soulgemcommon01.nif
		{ 0x5F673BCD286FB808ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgrand01lesserfill.nif
		{ 0x5F778791314ED928ULL, 3 },   // meshes\clutter\soulgem\full\soulgemblack01.nif
		{ 0x5F837D589C9C61AEULL, 3 },   // meshes\clutter\soulgem\full\soulgempetty01.nif
		{ 0x6058321B401C5396ULL, 12 },  // textures\remastered\soulgems\fragments\soulgemfragment01_n.dds
		{ 0x605E5406D7AC43F1ULL, 3 },   // textures\clutter\soulgem\soulgemlesser_transluc.dds
		{ 0x6125CE41B8E5E7CAULL, 2 },   // meshes\clutter\daedric\azurastarlite.nif
		{ 0x6169BE464A718294ULL, 2 },   // meshes\clutter\soulgem\soulgemblack01.nif
		{ 0x62066680CB9DBD8BULL, 3 },   // meshes\clutter\soulgem\full\soulgemlesser01.nif
		{ 0x6258749274028258ULL, 2 },   // meshes\clutter\soulgem\soulgemholder01.nif
		{ 0x6260463F9E9E967AULL, 5 },   // textures\nev-p\clutter\geodeametista_n.dds
		{ 0x635FC4E519800EA8ULL, 6 },   // meshes\clutter\soulgem\soulgemblack01.nif
		{ 0x63658DE08860BC6AULL, 3 },   // meshes\clutter\soulgem\full\soulgemcommon01.nif
		{ 0x63979AA8BAE70890ULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgemcommon01.nif
		{ 0x639FC2D916F2264EULL, 5 },   // textures\clutter\daedric\azurastar.dds
		{ 0x656695C9BC4E16BFULL, 3 },   // textures\clutter\soulgem\soulgemlesser.dds
		{ 0x6609576C39D2222EULL, 12 },  // textures\clutter\daedric\lite\azurastarmetalbrancheslite_m.dds
		{ 0x66B993790A683C6EULL, 2 },   // meshes\clutter\soulgem\soulgemgrand01.nif
		{ 0x66F3B717BEDE93BBULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgempiece05.nif
		{ 0x674D441B3ACA0A90ULL, 5 },   // textures\clutter\soulgem\soulgemcommon_n.dds
		{ 0x688FD34CCBB41A2EULL, 5 },   // textures\clutter\soulgem\soulgemcommon.dds
		{ 0x6947F773CE28AEF7ULL, 2 },   // meshes\clutter\soulgem\soulgempiece03.nif
		{ 0x69D52ED17A937F55ULL, 3 },   // meshes\clutter\soulgem\soulgempiece05.nif
		{ 0x6A2EED4BC43891D2ULL, 2 },   // textures\clutter\daedric\azurastar_em.dds
		{ 0x6B03099CFEC5D5BBULL, 1 },   // data\meshes\clutter\soulgem\soulgempetty01.nif
		{ 0x6B6E65CC92F3851FULL, 9 },   // meshes\clutter\soulgem\soulgemcommon_full.nif
		{ 0x6C3DB6DFACCEE2D8ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\full\soulgempetty01.nif
		{ 0x6CEFE989F45FCDB9ULL, 1 },   // data\meshes\clutter\soulgem\soulgemcommon01_full.nif
		{ 0x6D7DA76ECE9EF3F8ULL, 3 },   // textures\clutter\soulgem\soulgemlesser_transluc.dds
		{ 0x6EBB6757F88B2DC7ULL, 3 },   // meshes\clutter\soulgem\soulgempiece01.nif
		{ 0x6EF0B85853AE79FAULL, 6 },   // meshes\clutter\soulgem\soulgemblack01filled.nif
		{ 0x6FE0CBE350326AB9ULL, 12 },  // textures\remastered\soulgems\fragments\soulgemfragment02.dds
		{ 0x702F194ECD07E424ULL, 12 },  // textures\clutter\daedric\lite\azurastarmetalbrancheslite.dds
		{ 0x71556324D2E580FEULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgreater01lesserfill.nif
		{ 0x71A01222891B926AULL, 3 },   // meshes\clutter\soulgem\full\soulgemlesser01.nif
		{ 0x72646991DEAA6253ULL, 3 },   // meshes\clutter\soulgem\full\soulgemgrand01.nif
		{ 0x727115F66ADA66AFULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgempiece01.nif
		{ 0x729DA2101C15CD67ULL, 5 },   // textures\clutter\soulgem\soulgempetty_m.dds
		{ 0x73250E60D263B315ULL, 5 },   // textures\clutter\soulgem\soulgemlesser.dds
		{ 0x73AA2C23FD849639ULL, 6 },   // meshes\clutter\soulgem\soulgempiece03.nif
		{ 0x73B21E8587116875ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemblack01grandfill.nif
		{ 0x745314CFF0CDD0ACULL, 12 },  // meshes\clutter\soulgem\soulgemgreater01filled.nif
		{ 0x74828E9DF57DC664ULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgemlesser01.nif
		{ 0x770AEA6D4B4A9569ULL, 3 },   // meshes\clutter\soulgem\soulgempiece03.nif
		{ 0x7751670928E16609ULL, 5 },   // textures\clutter\soulgem\soulgemblack.dds
		{ 0x77E2C0EC315E3830ULL, 2 },   // meshes\clutter\soulgem\soulgempiece03.nif
		{ 0x79017B69B637A3FCULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgemgreater_full.nif
		{ 0x79066BB05486A33DULL, 5 },   // textures\nev-p\cubemaps\nev_ametista_e.dds
		{ 0x7A5E491AC7417511ULL, 3 },   // meshes\clutter\soulgem\full\soulgemcommon01.nif
		{ 0x7A85F3EF227BC959ULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgemcommon01filled.nif
		{ 0x7A9B62E3133F9E08ULL, 3 },   // textures\clutter\soulgem\soulgemcommonsubsurf.dds
		{ 0x7B580F9D80FC06CBULL, 3 },   // meshes\clutter\soulgem\soulgemgreater01.nif
		{ 0x7C11496E2279E652ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgempiece05.nif
		{ 0x7CE002C3F52E3A37ULL, 9 },   // textures\clutter\soulgem\soulgemgreater.dds
		{ 0x7D4C6B8E970E4026ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemcommon01lesserfill.nif
		{ 0x7DC08DC74F9B768DULL, 3 },   // textures\clutter\soulgem\soulgemblacksubsurface.dds
		{ 0x7E703A3F4C1C4746ULL, 10 },  // skyking soul gems\meshes\dlc01\clutter\dlc01soulgemchunk01.nif
		{ 0x7ECD7D0A256C1B1DULL, 12 },  // textures\clutter\daedric\lite\azurastarmetalcenterlite_n.dds
		{ 0x7EE560928ABA7AE2ULL, 2 },   // meshes\clutter\soulgem\soulgemcommon01.nif
		{ 0x80151E570332D3E8ULL, 12 },  // textures\clutter\daedric\broken\azurastarcrystalcenterbroken.dds
		{ 0x80158AF7D5177E5BULL, 12 },  // textures\clutter\daedric\dark\azurastarmetalcenterdark_m.dds
		{ 0x81E4CE1EE283AD36ULL, 3 },   // meshes\clutter\soulgem\soulgemblack01.nif
		{ 0x8209D032ADEA1483ULL, 2 },   // meshes\clutter\soulgem\soulgemblack01.nif
		{ 0x820F5224F3D0C379ULL, 9 },   // textures\clutter\soulgem\soulgemblack.dds
		{ 0x8211E3B5BEA25CC3ULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgempetty01.nif
		{ 0x838FBF548253CD50ULL, 5 },   // textures\clutter\soulgem\soulgemgreater_n.dds
		{ 0x83A13A087FDD050AULL, 3 },   // meshes\dlc01\clutter\dlc01soulgembowlfull.nif
		{ 0x83B296B357133540ULL, 6 },   // meshes\clutter\soulgem\soulgemblack01filled.nif
		{ 0x847A4E8944ADDD88ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgrand01greaterfill.nif
		{ 0x8563ED8620559B66ULL, 12 },  // meshes\clutter\soulgem\soulgemholderstand01.nif
		{ 0x86513F6821453C68ULL, 2 },   // meshes\clutter\soulgem\soulgempiece05.nif
		{ 0x86F748B5F95C12E7ULL, 12 },  // meshes\clutter\soulgem\soulgemgreater01filled.nif
		{ 0x878FFE4457B87F9EULL, 12 },  // textures\clutter\daedric\dark\azurastar.dds
		{ 0x88ECF137064D63FBULL, 5 },   // textures\clutter\soulgem\soulgemgreater_m.dds
		{ 0x892530F45E7733A5ULL, 3 },   // meshes\clutter\soulgem\full\soulgemgreater01.nif
		{ 0x8942E5E6BE4E3D2BULL, 3 },   // meshes\clutter\soulgem\full\soulgemcommon01.nif
		{ 0x89BB7FF331F4B0A0ULL, 9 },   // meshes\clutter\soulgem\soulgempiece04.nif
		{ 0x8A3E1CD759AF02C8ULL, 5 },   // meshes\clutter\daedric\azurastarbroken.nif
		{ 0x8ADE939EB35E076FULL, 5 },   // textures\clutter\soulgem\soulblack_g.dds
		{ 0x8B568DF94DFCE12BULL, 12 },  // textures\remastered\soulgems\soulgemgrand.dds
		{ 0x8B5FE170510B6921ULL, 3 },   // meshes\clutter\soulgem\soulgempiece04.nif
		{ 0x8B71C96E1B769D60ULL, 5 },   // meshes\clutter\soulgem\soulgemblack01fx.nif
		{ 0x8BB89FF406B23593ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\full\soulgemblack01.nif
		{ 0x8BEAE37E0E8E5C70ULL, 5 },   // textures\cubemaps\tyore_tytanium_e.dds
		{ 0x8C1A75571A742244ULL, 12 },  // textures\clutter\daedric\lite\azurastarcrystalbrancheslite_n.dds
		{ 0x8CE3BF2204B2FC8FULL, 12 },  // meshes\clutter\soulgem\soulgempetty01.nif
		{ 0x8D1B53EC146BA2E7ULL, 12 },  // textures\remastered\soulgems\soulgemlesser_n.dds
		{ 0x8D85133066A4D951ULL, 3 },   // meshes\clutter\soulgem\full\soulgemlesser01.nif
		{ 0x8DEB3D2C21FF3E8CULL, 12 },  // textures\clutter\daedric\broken\azurastarmetalcenterbroken_n.dds
		{ 0x8E7A99394F6120E4ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemcommon01lesserfill.nif
		{ 0x8F8DAD1B3D946E63ULL, 3 },   // textures\clutter\soulgem\soulgemblack_transluc.dds
		{ 0x9170A4156E1BC8F5ULL, 6 },   // textures\remastered\soulgems\soulgemblack_n.dds
		{ 0x9228E7F8172073AFULL, 3 },   // meshes\clutter\soulgem\soulgempetty01.nif
		{ 0x9281F0C42CB39ABAULL, 12 },  // textures\clutter\daedric\broken\azurastarcrystalbranchesbroken.dds
		{ 0x93497AB7CFFE8772ULL, 3 },   // meshes\clutter\soulgem\soulgemcommon01.nif
		{ 0x949EF268E364D9A8ULL, 12 },  // textures\clutter\soulgem\soulgemholderstand.dds
		{ 0x95F25183DD728684ULL, 12 },  // textures\clutter\daedric\broken\azurastarcrystalgemsbroken_n.dds
		{ 0x96C2EB26C4FFAEABULL, 12 },  // textures\clutter\daedric\dark\azurastarmetalcenterdark_n.dds
		{ 0x96D5F3B0D024D2F2ULL, 5 },   // textures\clutter\soulgem\soulgemgrand.dds
		{ 0x9759185AE445AB87ULL, 9 },   // textures\cubemaps\soulgems_e.dds
		{ 0x97A55462962C3C62ULL, 2 },   // meshes\clutter\soulgem\soulgempiece01.nif
		{ 0x97C903C43B72579BULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgrand01greaterfill.nif
		{ 0x97CD8968914BBB84ULL, 9 },   // meshes\clutter\soulgem\soulgemgrand_full.nif
		{ 0x984931CFFEEB489AULL, 6 },   // meshes\clutter\soulgem\soulgemcommon01filled.nif
		{ 0x986D8A5688F9033AULL, 6 },   // textures\remastered\soulgems\fragments\soulgemfragment05_n.dds
		{ 0x988CF974C6AA8EEAULL, 6 },   // textures\remastered\soulgems\fragments\soulgemfragment04.dds
		{ 0x98D10883B844239BULL, 3 },   // textures\clutter\soulgem\soulgemlesser.dds
		{ 0x9BBF3682CC69040EULL, 5 },   // meshes\clutter\soulgem\soulgempiece02.nif
		{ 0x9D39AD84391922EEULL, 9 },   // meshes\clutter\soulgem\soulgempetty_full.nif
		{ 0x9DC0B3E639E63892ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemcommon01pettyfill.nif
		{ 0x9E0A8DAD420A87C6ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgemgrand01.nif
		{ 0x9F66E470036CD8D1ULL, 2 },   // meshes\clutter\soulgem\soulgempiece02.nif
		{ 0x9FCD726F9B088138ULL, 9 },   // meshes\clutter\soulgem\soulgemgrand_full.nif
		{ 0xA05CA29F93DD82E0ULL, 12 },  // meshes\clutter\soulgem\soulgempiece02.nif
		{ 0xA0BEF1CA93E5910DULL, 6 },   // meshes\clutter\soulgem\soulgempiece01.nif
		{ 0xA0D0E39815C15A81ULL, 3 },   // meshes\clutter\soulgem\full\soulgemgreater01.nif
		{ 0xA1095F1A90476705ULL, 3 },   // textures\clutter\soulgem\soulgemcommon.dds
		{ 0xA153687FDA48B445ULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgemgrand01.nif
		{ 0xA19E730AC4D1B971ULL, 9 },   // meshes\clutter\soulgem\soulgempiece02.nif
		{ 0xA1EB3579F5E4FC87ULL, 3 },   // meshes\clutter\soulgem\soulgemgrand01.nif
		{ 0xA1F63DDA7EFA2F87ULL, 1 },   // data\meshes\clutter\soulgem\soulgemlesser01.nif
		{ 0xA2F9D6888D49169AULL, 5 },   // meshes\clutter\soulgem\soulgemcommon01.nif
		{ 0xA336CA54903047E4ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\full\soulgemgrand01.nif
		{ 0xA37B9410913E7F34ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemblack01greaterfill.nif
		{ 0xA3B1AE71CC30466BULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgemblack01filled.nif
		{ 0xA4239073F9F68DBCULL, 12 },  // meshes\clutter\soulgem\soulgempiece03.nif
		{ 0xA437F0856E29A3D5ULL, 3 },   // meshes\clutter\soulgem\full\soulgemblack01.nif
		{ 0xA4DB41A2CFD7FE9FULL, 12 },  // textures\clutter\soulgem\soulgemholderstand_n.dds
		{ 0xA4EADC9FEC8EB65BULL, 12 },  // meshes\clutter\soulgem\soulgemgrand01filled.nif
		{ 0xA4FB3FD507988983ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgempetty_full.nif
		{ 0xA5FC2BDEF03B8067ULL, 12 },  // meshes\clutter\daedric\azurastar.nif
		{ 0xA6D43C517E5CE879ULL, 12 },  // textures\remastered\soulgems\fragments\soulgemfragment05.dds
		{ 0xA7171E14535E687DULL, 12 },  // textures\remastered\soulgems\fragments\soulgemfragment02_n.dds
		{ 0xA75E1C7EFD025FDDULL, 9 },   // meshes\clutter\soulgem\soulgemcommon01.nif
		{ 0xA8E54F06E94C2163ULL, 6 },   // meshes\clutter\soulgem\soulgemgreater01filled.nif
		{ 0xA99F25BF0E71D0D7ULL, 0 },   // textures\sopawhite_e.dds
		{ 0xA9DD80ED8C1E6365ULL, 1 },   // data\meshes\clutter\soulgem\soulgemblack01.nif
		{ 0xA9EFFC65EFB10A01ULL, 2 },   // meshes\clutter\soulgem\soulgempiece01.nif
		{ 0xAB34BAB88CA09E50ULL, 3 },   // meshes\clutter\soulgem\full\soulgemcommon01.nif
		{ 0xAB7235EBBAF9B1E6ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgreater01commonfill.nif
		{ 0xABAE1D0A6396AD34ULL, 6 },   // textures\remastered\soulgems\fragments\soulgemfragment01_n.dds
		{ 0xACAABC33C12D5EB1ULL, 5 },   // meshes\clutter\soulgem\soulgempiece04.nif
		{ 0xACF5220F52E93863ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemblack01grandfill.nif
		{ 0xAD06DD431D6CAE34ULL, 5 },   // meshes\clutter\soulgem\soulgemgrand01fx.nif
		{ 0xAD3C0A8A7A06A1EAULL, 2 },   // meshes\clutter\soulgem\soulgempetty01.nif
		{ 0xAE5749EAC660A931ULL, 5 },   // textures\clutter\daedric\azurastarlite.dds
		{ 0xAEFFB9A5339B6C1CULL, 3 },   // meshes\clutter\soulgem\full\soulgemgreater01.nif
		{ 0xAFE9D68016E81B5EULL, 6 },   // textures\remastered\soulgems\soulgemlesser_n.dds
		{ 0xAFEFA764E8C3CD35ULL, 12 },  // textures\clutter\soulgem\soulgemholderbase_n.dds
		{ 0xB0246C0F5B908B6AULL, 0 },   // meshes\clutter\daedric\azurastarlite.nif
		{ 0xB037A5C5FCA5CE4EULL, 3 },   // meshes\clutter\soulgem\full\soulgemgreater01.nif
		{ 0xB0705EE7C8D8541AULL, 12 },  // textures\clutter\daedric\dark\azurastar_n.dds
		{ 0xB0830EF522A6E0A9ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgrand01pettyfill.nif
		{ 0xB1AFEC41BFFCE461ULL, 6 },   // textures\remastered\soulgems\soulgemgrand_n.dds
		{ 0xB1C31213E7B06EE4ULL, 1 },   // data\meshes\clutter\soulgem\soulgemgreater01.nif
		{ 0xB1D29F847E5AADA5ULL, 2 },   // meshes\clutter\soulgem\soulgemlesser01.nif
		{ 0xB228E64B5444E328ULL, 12 },  // meshes\clutter\soulgem\soulgemgrand01filled.nif
		{ 0xB30780001CC6B7D4ULL, 3 },   // textures\clutter\soulgem\soulgemblack.dds
		{ 0xB30FF23DD0EF2DBEULL, 0 },   // meshes\clutter\daedric\azurastar.nif
		{ 0xB43F9B90A31B9E14ULL, 6 },   // meshes\clutter\soulgem\soulgemlesser01filled.nif
		{ 0xB538325C963FADA9ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgempiece02.nif
		{ 0xB6721E376B9BAA77ULL, 6 },   // textures\remastered\soulgems\soulgemparallax.dds
		{ 0xB6D309452C60D9B0ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgemcommon_full.nif
		{ 0xB8533B250FBAC5EBULL, 12 },  // textures\clutter\daedric\broken\azurastarmetalcenterbroken.dds
		{ 0xB8B279BF4EA90A3FULL, 9 },   // meshes\clutter\soulgem\soulgemlesser01.nif
		{ 0xB92768E95CF4A2CBULL, 6 },   // meshes\clutter\soulgem\soulgempiece02.nif
		{ 0xB9D228129CC2D3F0ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemcommon01pettyfill.nif
		{ 0xBA2C5B4004E14EB6ULL, 6 },   // meshes\clutter\soulgem\soulgempetty01filled.nif
		{ 0xBA47D44E122F8387ULL, 2 },   // meshes\clutter\soulgem\soulgemholderstand01.nif
		{ 0xBA9186E54F4C62BCULL, 3 },   // textures\clutter\soulgem\soulgemgreatersubsurface.dds
		{ 0xBAABAA4778E961BAULL, 0 },   // meshes\clutter\daedric\azurastar.nif
		{ 0xBC9E551315AD1372ULL, 5 },   // textures\clutter\soulgem\soulgemlesser_n.dds
		{ 0xBD1838E58C30F529ULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgemblack01.nif
		{ 0xBE10E6153BE074DAULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemcommon01pettyfill.nif
		{ 0xBE43BD2D2B96E0FAULL, 0 },   // textures\icecavetintstalhrim.dds
		{ 0xBF014A175D479550ULL, 6 },   // meshes\clutter\soulgem\soulgemgrand01filled.nif
		{ 0xBF0CCD1331ABD891ULL, 6 },   // textures\remastered\soulgems\soulgempetty_n.dds
		{ 0xBF7AF6D078FB3929ULL, 6 },   // meshes\clutter\soulgem\soulgemgreater01filled.nif
		{ 0xC1957C2CC6DF3771ULL, 6 },   // meshes\clutter\soulgem\soulgempiece05.nif
		{ 0xC1ABFDD570FAA8F3ULL, 6 },   // meshes\clutter\soulgem\soulgemlesser01filled.nif
		{ 0xC1B588932945067CULL, 9 },   // textures\clutter\soulgem\soulgempetty.dds
		{ 0xC1D2A4263E3FD269ULL, 12 },  // meshes\clutter\soulgem\soulgemblack01filled.nif
		{ 0xC24DC5F1E0F0DF45ULL, 12 },  // meshes\clutter\soulgem\soulgemgreater01filled.nif
		{ 0xC2CB5B6EE1F057BDULL, 12 },  // textures\clutter\daedric\dark\azurastarmetalbranchesdark.dds
		{ 0xC2CEE0537277CCD1ULL, 2 },   // textures\clutter\soulgem\soulgemgrand_n.dds
		{ 0xC30ECDF7E8583BB4ULL, 12 },  // textures\remastered\soulgems\soulgempetty.dds
		{ 0xC37B414C3E527E64ULL, 3 },   // textures\clutter\soulgem\soulgempetty.dds
		{ 0xC38125E52190EEE7ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgrand01lesserfill.nif
		{ 0xC3E96F37679FDB7DULL, 3 },   // textures\clutter\soulgem\soulgempettysubsurf.dds
		{ 0xC5162B3838733FC2ULL, 2 },   // meshes\clutter\daedric\azurastarlite.nif
		{ 0xC565D9BD05776D37ULL, 2 },   // textures\clutter\soulgem\soulgemblack.dds
		{ 0xC69F1569FEE2F378ULL, 12 },  // textures\remastered\soulgems\soulgemblack.dds
		{ 0xC82B5F292A82688AULL, 5 },   // textures\nev-p\clutter\geodeametista_m.dds
		{ 0xC8B3C8319B7CA984ULL, 2 },   // meshes\clutter\soulgem\soulgempiece04.nif
		{ 0xC9FB502F8D674686ULL, 5 },   // meshes\clutter\daedric\azurastar.nif
		{ 0xCAFA5BACD1DCB5D5ULL, 0 },   // textures\innerstalhrim.dds
		{ 0xCC226546DE615AF3ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgemblack01.nif
		{ 0xCC5354966581D6B7ULL, 9 },   // meshes\clutter\soulgem\soulgemlesser_full.nif
		{ 0xCC8C7693943058BAULL, 12 },  // textures\remastered\soulgems\soulgemgreater.dds
		{ 0xCCDD06EB0E7F5AC6ULL, 6 },   // textures\remastered\soulgems\soulgemgrand.dds
		{ 0xCD71E4E37058DF12ULL, 12 },  // textures\clutter\daedric\lite\azurastarlite.dds
		{ 0xCD8F390686A47263ULL, 12 },  // textures\remastered\soulgems\soulgem_e.dds
		{ 0xCDC65C41E9EE24B0ULL, 6 },   // meshes\clutter\soulgem\soulgemgrand01.nif
		{ 0xCE09202947CDB7D6ULL, 6 },   // meshes\clutter\soulgem\soulgempiece04.nif
		{ 0xCE28A00AE441EB94ULL, 5 },   // textures\clutter\soulgem\soullesser_g.dds
		{ 0xCFC253F58D872432ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgempiece01.nif
		{ 0xCFD2892F0C2C112CULL, 6 },   // textures\remastered\soulgems\soulgemgreater_n.dds
		{ 0xD10837D75B463783ULL, 5 },   // meshes\clutter\soulgem\soulgemlesser01fx.nif
		{ 0xD15D1082F601ECB7ULL, 12 },  // meshes\clutter\soulgem\soulgemgreater01.nif
		{ 0xD1631890EA84D705ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgrand01pettyfill.nif
		{ 0xD1C867EE1D7800B9ULL, 3 },   // meshes\clutter\soulgem\soulgempiece02.nif
		{ 0xD2F7DD7556D48362ULL, 9 },   // meshes\clutter\soulgem\soulgemblack_full.nif
		{ 0xD316D3DB1B09AE1FULL, 6 },   // meshes\clutter\soulgem\soulgempetty01filled.nif
		{ 0xD343CA00B8EDD38FULL, 12 },  // textures\clutter\daedric\lite\azurastarmetalcenterlite.dds
		{ 0xD4AF06748208112BULL, 12 },  // meshes\dlc01\clutter\dlc01soulgembowlfull.nif
		{ 0xD53F18B4BEEBB368ULL, 3 },   // textures\clutter\soulgem\soulgemgreater_transluc.dds
		{ 0xD5440B7861CD79CAULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgreater01pettyfill.nif
		{ 0xD5A9A80C235351BEULL, 12 },  // meshes\clutter\soulgem\soulgempiece04.nif
		{ 0xD5FEE4ED72CC918EULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\full\soulgemgreater01.nif
		{ 0xD6344354800EC146ULL, 5 },   // textures\clutter\soulgem\soulgemcommon_m.dds
		{ 0xD6431E1975E0BE6EULL, 12 },  // textures\clutter\soulgem\soulgemholderbase.dds
		{ 0xD68CE7E01E563648ULL, 6 },   // textures\remastered\soulgems\fragments\soulgemfragment04_n.dds
		{ 0xD7254A01A82008DCULL, 3 },   // meshes\clutter\soulgem\full\soulgemgrand01.nif
		{ 0xD7A47457E6F881DCULL, 3 },   // meshes\clutter\soulgem\full\soulgemblack01.nif
		{ 0xD869F99BF60689C0ULL, 1 },   // data\meshes\clutter\soulgem\soulgemlesser01_full.nif
		{ 0xD86EE100BF584C08ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgrand01commonfill.nif
		{ 0xD8D932F2A35DFC3AULL, 3 },   // textures\clutter\soulgem\soulgemcommon_transluc.dds
		{ 0xD8F15C01DD209E56ULL, 2 },   // meshes\clutter\soulgem\soulgemgrand01.nif
		{ 0xD92A6E40DDDCD399ULL, 3 },   // textures\clutter\soulgem\soulgemlessersubsurf.dds
		{ 0xDA0EA7DC3DE39051ULL, 6 },   // textures\remastered\soulgems\fragments\soulgemfragment02.dds
		{ 0xDB4431E3EBCB3E24ULL, 12 },  // meshes\clutter\soulgem\soulgemblack01filled.nif
		{ 0xDC017E40E28FEEB4ULL, 5 },   // textures\clutter\soulgem\soulgemlesser_m.dds
		{ 0xDE16BFAFB1DF1986ULL, 12 },  // textures\clutter\daedric\dark\azurastarcrystalgemsdark_n.dds
		{ 0xDE3AF54B1F6F2B70ULL, 12 },  // textures\remastered\soulgems\soulgemcommon_n.dds
		{ 0xDE6A324E84DAFF82ULL, 5 },   // textures\nev-p\cubemaps\nev_smeraldo_e.dds
		{ 0xE10036C6AE8595DFULL, 6 },   // textures\remastered\soulgems\fragments\soulgemfragment03.dds
		{ 0xE155D6F9AF9160BCULL, 12 },  // textures\remastered\soulgems\fragments\soulgemfragment05_n.dds
		{ 0xE16439073C685D39ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemblack01commonfill.nif
		{ 0xE1DB001B20EC2812ULL, 12 },  // textures\clutter\daedric\lite\azurastarmetalbrancheslite_n.dds
		{ 0xE1E101DD22A3F175ULL, 5 },   // meshes\clutter\daedric\azurastarlite.nif
		{ 0xE27812D9AE3242A1ULL, 0 },   // textures\sopablackazura.dds
		{ 0xE2E5D20D4F4044FAULL, 9 },   // meshes\clutter\soulgem\soulgempiece01.nif
		{ 0xE3BAFDD52EBA3E64ULL, 6 },   // textures\remastered\soulgems\fragments\soulgemfragment01.dds
		{ 0xE411757BE530079AULL, 12 },  // meshes\clutter\soulgem\soulgemlesser01filled.nif
		{ 0xE4A9177E4332EF95ULL, 3 },   // meshes\clutter\soulgem\full\soulgempetty01.nif
		{ 0xE5AC9DCBF423384EULL, 1 },   // data\meshes\clutter\soulgem\soulgemcommon01.nif
		{ 0xE7C50B6ABC7ED020ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgemlesser01.nif
		{ 0xE94CBFC1E73BA53AULL, 12 },  // meshes\clutter\soulgem\soulgemblack01.nif
		{ 0xE95D32DFD5E044E6ULL, 12 },  // textures\clutter\daedric\dark\azurastarcrystalbranchesdark_n.dds
		{ 0xE9C80B988A9FC8DAULL, 12 },  // textures\clutter\daedric\broken\azurastarcrystalbranchesbrokenpieces_n.dds
		{ 0xE9CC5E4D4FE3D65FULL, 10 },  // skyking soul gems\meshes\clutter\containers\dwesoulgemcontainer01.nif
		{ 0xE9E089C14C9ADACFULL, 6 },   // textures\remastered\soulgems\soulgemlesser.dds
		{ 0xEAFCE8E3ADC8592EULL, 12 },  // meshes\clutter\soulgem\soulgemblack01filled.nif
		{ 0xEB50780A4DCC5392ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\full\soulgemcommon01.nif
		{ 0xECA8D3E1FF619D90ULL, 12 },  // textures\clutter\daedric\dark\azurastarmetalcenterdark.dds
		{ 0xEDDD188E2AA2ACDFULL, 2 },   // meshes\clutter\soulgem\soulgemgreater01.nif
		{ 0xEEA8C4A39D5A2D05ULL, 12 },  // meshes\clutter\soulgem\soulgemlesser01.nif
		{ 0xEEF43C3AC95ED32DULL, 1 },   // data\meshes\clutter\soulgem\soulgemgrand01_full.nif
		{ 0xEF44C3E71611A925ULL, 9 },   // meshes\clutter\soulgem\soulgemblack_full.nif
		{ 0xEF45C5DE8327ED74ULL, 12 },  // textures\remastered\soulgems\soulgemgrand_n.dds
		{ 0xF00D5D0A87D13487ULL, 12 },  // meshes\clutter\soulgem\soulgemgrand01filled.nif
		{ 0xF020243DCB539F22ULL, 6 },   // meshes\clutter\soulgem\soulgemgrand01filled.nif
		{ 0xF0795D790C5BBBF2ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgreater01pettyfill.nif
		{ 0xF0A56DD1169A29C6ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemblack01greaterfill.nif
		{ 0xF0F8C0B69A4915FFULL, 12 },  // meshes\clutter\daedric\azurastarlite.nif
		{ 0xF1422F27BE57B590ULL, 13 },  // dd-soulgems replecer by vayura v1.2\meshes\clutter\soulgem\soulgemlesser_full.nif
		{ 0xF2EBB09D713585BCULL, 3 },   // meshes\clutter\soulgem\full\soulgemgreater01.nif
		{ 0xF36244D4F64F2C36ULL, 12 },  // meshes\clutter\soulgem\soulgemgrand01.nif
		{ 0xF436C030E4D12BD7ULL, 2 },   // meshes\clutter\soulgem\soulgemgreater01.nif
		{ 0xF436CAA47BEE5472ULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgemlesser01filled.nif
		{ 0xF4CB338C310E7BF7ULL, 12 },  // textures\clutter\daedric\broken\azurastarmetalcenterbroken_m.dds
		{ 0xF543726D143A0D6CULL, 12 },  // meshes\clutter\soulgem\soulgemlesser01filled.nif
		{ 0xF56D3664AC063546ULL, 6 },   // meshes\clutter\soulgem\soulgemlesser01.nif
		{ 0xF58FACAB27BBD9C2ULL, 9 },   // meshes\clutter\soulgem\soulgemgrand01.nif
		{ 0xF5B9F731CC3CEB83ULL, 6 },   // meshes\clutter\soulgem\soulgempetty01.nif
		{ 0xF5D2C39685BD82FCULL, 12 },  // meshes\clutter\soulgem\soulgemcommon01filled.nif
		{ 0xF73489A6EB633A71ULL, 10 },  // skyking soul gems\meshes\clutter\soulgem\soulgempiece02.nif
		{ 0xF75A3ECAD5C45C81ULL, 9 },   // meshes\clutter\soulgem\soulgemgreater_full.nif
		{ 0xF7D9E0A0618DBA77ULL, 9 },   // meshes\clutter\soulgem\soulgemblack01.nif
		{ 0xF808988C3162D0D0ULL, 12 },  // meshes\clutter\soulgem\soulgempetty01filled.nif
		{ 0xF8A4274A6E795AABULL, 12 },  // textures\clutter\daedric\broken\azurastarcrystalbranchesbrokenpieces.dds
		{ 0xF90210F2271623EAULL, 5 },   // meshes\clutter\soulgem\soulgempiece03.nif
		{ 0xFA0B8CD7026DDE22ULL, 3 },   // meshes\clutter\soulgem\full\soulgemblack01.nif
		{ 0xFA14D059F6B1FC52ULL, 3 },   // meshes\clutter\soulgem\full\soulgemblack01.nif
		{ 0xFA60C536F0636623ULL, 3 },   // textures\clutter\soulgem\soulgemgrandsubsurface.dds
		{ 0xFA68C57127317B65ULL, 3 },   // meshes\clutter\soulgem\full\soulgemgrand01.nif
		{ 0xFA864384D5843B23ULL, 12 },  // textures\clutter\soulgem\soulgemholder.dds
		{ 0xFB1A85805E282FBCULL, 2 },   // textures\clutter\soulgem\soulgemcommon.dds
		{ 0xFB74C3E278F69534ULL, 12 },  // textures\clutter\daedric\dark\azurastarcrystalbranchesdark.dds
		{ 0xFBEE8A3A334351D9ULL, 12 },  // meshes\clutter\containers\dwesoulgemcontainer01.nif
		{ 0xFBFEEE1950E5C5B9ULL, 12 },  // meshes\clutter\soulgem\soulgempiece05.nif
		{ 0xFC2EDBD2EC46F3D7ULL, 3 },   // textures\clutter\soulgem\soulgemgreatersubsurface.dds
		{ 0xFC74695536256254ULL, 12 },  // textures\remastered\soulgems\fragments\soulgemfragment04_n.dds
		{ 0xFDFAF0C401B75EE3ULL, 0 },   // textures\sopawhiteazura_n.dds
		{ 0xFF1ACCF370F326C0ULL, 3 },   // meshes\clutter\soulgem\full\gist\soulgemgreater01commonfill.nif
	} };

	struct BlockedPath
	{
		std::string_view path;  // below meshes\ or textures\, lower case
		std::uint8_t     mod{ 0 };
	};

	// sorted by path
	inline constexpr std::array<BlockedPath, 143> kBlockedPaths{ {
		{ "clutter\\containers\\dwesoulgemcontainer01.nif", 11 },
		{ "clutter\\daedric\\broken\\azurastarcrystalbranchesbroken.dds", 12 },
		{ "clutter\\daedric\\broken\\azurastarcrystalbranchesbroken_n.dds", 12 },
		{ "clutter\\daedric\\broken\\azurastarcrystalbranchesbrokenpieces.dds", 12 },
		{ "clutter\\daedric\\broken\\azurastarcrystalbranchesbrokenpieces_n.dds", 12 },
		{ "clutter\\daedric\\broken\\azurastarcrystalcenterbroken.dds", 12 },
		{ "clutter\\daedric\\broken\\azurastarcrystalcenterbroken_n.dds", 12 },
		{ "clutter\\daedric\\broken\\azurastarcrystalgemsbroken.dds", 12 },
		{ "clutter\\daedric\\broken\\azurastarcrystalgemsbroken_n.dds", 12 },
		{ "clutter\\daedric\\broken\\azurastarmetalbranchesbroken.dds", 12 },
		{ "clutter\\daedric\\broken\\azurastarmetalbranchesbroken_m.dds", 12 },
		{ "clutter\\daedric\\broken\\azurastarmetalbranchesbroken_n.dds", 12 },
		{ "clutter\\daedric\\broken\\azurastarmetalcenterbroken.dds", 12 },
		{ "clutter\\daedric\\broken\\azurastarmetalcenterbroken_m.dds", 12 },
		{ "clutter\\daedric\\broken\\azurastarmetalcenterbroken_n.dds", 12 },
		{ "clutter\\daedric\\dark\\azurastar.dds", 12 },
		{ "clutter\\daedric\\dark\\azurastar_n.dds", 12 },
		{ "clutter\\daedric\\dark\\azurastarcrystalbranchesdark.dds", 12 },
		{ "clutter\\daedric\\dark\\azurastarcrystalbranchesdark_n.dds", 12 },
		{ "clutter\\daedric\\dark\\azurastarcrystalgemsdark.dds", 12 },
		{ "clutter\\daedric\\dark\\azurastarcrystalgemsdark_n.dds", 12 },
		{ "clutter\\daedric\\dark\\azurastarmetalbranchesdark.dds", 12 },
		{ "clutter\\daedric\\dark\\azurastarmetalbranchesdark_m.dds", 12 },
		{ "clutter\\daedric\\dark\\azurastarmetalbranchesdark_n.dds", 12 },
		{ "clutter\\daedric\\dark\\azurastarmetalcenterdark.dds", 12 },
		{ "clutter\\daedric\\dark\\azurastarmetalcenterdark_m.dds", 12 },
		{ "clutter\\daedric\\dark\\azurastarmetalcenterdark_n.dds", 12 },
		{ "clutter\\daedric\\lite\\azurastarcrystalbrancheslite.dds", 12 },
		{ "clutter\\daedric\\lite\\azurastarcrystalbrancheslite_n.dds", 12 },
		{ "clutter\\daedric\\lite\\azurastarcrystalgemslite.dds", 12 },
		{ "clutter\\daedric\\lite\\azurastarcrystalgemslite_n.dds", 12 },
		{ "clutter\\daedric\\lite\\azurastarlite.dds", 12 },
		{ "clutter\\daedric\\lite\\azurastarlite_n.dds", 12 },
		{ "clutter\\daedric\\lite\\azurastarmetalbrancheslite.dds", 12 },
		{ "clutter\\daedric\\lite\\azurastarmetalbrancheslite_m.dds", 12 },
		{ "clutter\\daedric\\lite\\azurastarmetalbrancheslite_n.dds", 12 },
		{ "clutter\\daedric\\lite\\azurastarmetalcenterlite.dds", 12 },
		{ "clutter\\daedric\\lite\\azurastarmetalcenterlite_m.dds", 12 },
		{ "clutter\\daedric\\lite\\azurastarmetalcenterlite_n.dds", 12 },
		{ "clutter\\soulgem\\full\\gist\\soulgemblack01commonfill.nif", 3 },
		{ "clutter\\soulgem\\full\\gist\\soulgemblack01grandfill.nif", 3 },
		{ "clutter\\soulgem\\full\\gist\\soulgemblack01greaterfill.nif", 3 },
		{ "clutter\\soulgem\\full\\gist\\soulgemblack01lesserfill.nif", 3 },
		{ "clutter\\soulgem\\full\\gist\\soulgemblack01pettyfill.nif", 3 },
		{ "clutter\\soulgem\\full\\gist\\soulgemcommon01lesserfill.nif", 3 },
		{ "clutter\\soulgem\\full\\gist\\soulgemcommon01pettyfill.nif", 3 },
		{ "clutter\\soulgem\\full\\gist\\soulgemgrand01commonfill.nif", 3 },
		{ "clutter\\soulgem\\full\\gist\\soulgemgrand01greaterfill.nif", 3 },
		{ "clutter\\soulgem\\full\\gist\\soulgemgrand01lesserfill.nif", 3 },
		{ "clutter\\soulgem\\full\\gist\\soulgemgrand01pettyfill.nif", 3 },
		{ "clutter\\soulgem\\full\\gist\\soulgemgreater01commonfill.nif", 3 },
		{ "clutter\\soulgem\\full\\gist\\soulgemgreater01lesserfill.nif", 3 },
		{ "clutter\\soulgem\\full\\gist\\soulgemgreater01pettyfill.nif", 3 },
		{ "clutter\\soulgem\\full\\gist\\soulgemlesser01pettyfill.nif", 3 },
		{ "clutter\\soulgem\\full\\soulgemblack01.nif", 4 },
		{ "clutter\\soulgem\\full\\soulgemcommon01.nif", 4 },
		{ "clutter\\soulgem\\full\\soulgemgrand01.nif", 4 },
		{ "clutter\\soulgem\\full\\soulgemgreater01.nif", 4 },
		{ "clutter\\soulgem\\full\\soulgemlesser01.nif", 4 },
		{ "clutter\\soulgem\\full\\soulgempetty01.nif", 4 },
		{ "clutter\\soulgem\\soulblack_g.dds", 5 },
		{ "clutter\\soulgem\\soulcommon_g.dds", 5 },
		{ "clutter\\soulgem\\soulgemblack01filled.nif", 7 },
		{ "clutter\\soulgem\\soulgemblack01fx.nif", 5 },
		{ "clutter\\soulgem\\soulgemblack_transluc.dds", 3 },
		{ "clutter\\soulgem\\soulgemblacksubsurface.dds", 3 },
		{ "clutter\\soulgem\\soulgemcommon01filled.nif", 7 },
		{ "clutter\\soulgem\\soulgemcommon01fx.nif", 5 },
		{ "clutter\\soulgem\\soulgemcommon_m.dds", 5 },
		{ "clutter\\soulgem\\soulgemcommon_transluc.dds", 3 },
		{ "clutter\\soulgem\\soulgemcommonsubsurf.dds", 3 },
		{ "clutter\\soulgem\\soulgemgrand01filled.nif", 7 },
		{ "clutter\\soulgem\\soulgemgrand01fx.nif", 5 },
		{ "clutter\\soulgem\\soulgemgrand_m.dds", 5 },
		{ "clutter\\soulgem\\soulgemgrand_transluc.dds", 3 },
		{ "clutter\\soulgem\\soulgemgrandsubsurface.dds", 3 },
		{ "clutter\\soulgem\\soulgemgreater01filled.nif", 7 },
		{ "clutter\\soulgem\\soulgemgreater01fx.nif", 5 },
		{ "clutter\\soulgem\\soulgemgreater_m.dds", 5 },
		{ "clutter\\soulgem\\soulgemgreater_transluc.dds", 3 },
		{ "clutter\\soulgem\\soulgemgreatersubsurface.dds", 3 },
		{ "clutter\\soulgem\\soulgemholder_m.dds", 2 },
		{ "clutter\\soulgem\\soulgemholderbase.dds", 12 },
		{ "clutter\\soulgem\\soulgemholderbase_n.dds", 12 },
		{ "clutter\\soulgem\\soulgemholderstand.dds", 12 },
		{ "clutter\\soulgem\\soulgemholderstand_n.dds", 12 },
		{ "clutter\\soulgem\\soulgemlesser01filled.nif", 7 },
		{ "clutter\\soulgem\\soulgemlesser01fx.nif", 5 },
		{ "clutter\\soulgem\\soulgemlesser_m.dds", 5 },
		{ "clutter\\soulgem\\soulgemlesser_transluc.dds", 3 },
		{ "clutter\\soulgem\\soulgemlessersubsurf.dds", 3 },
		{ "clutter\\soulgem\\soulgempetty01filled.nif", 7 },
		{ "clutter\\soulgem\\soulgempetty01fx.nif", 5 },
		{ "clutter\\soulgem\\soulgempetty_m.dds", 5 },
		{ "clutter\\soulgem\\soulgempettysubsurf.dds", 3 },
		{ "clutter\\soulgem\\soulgreater_g.dds", 5 },
		{ "clutter\\soulgem\\soullesser_g.dds", 5 },
		{ "clutter\\soulgem\\soulpetty_g.dds", 5 },
		{ "cubemaps\\soulgems_e.dds", 9 },
		{ "cubemaps\\tyore_tytanium_e.dds", 5 },
		{ "dlc01\\clutter\\dlc01soulgembowlfull.dds", 8 },
		{ "dlc01\\clutter\\dlc01soulgembowlfull_n.dds", 8 },
		{ "icecavetintstalhrim.dds", 0 },
		{ "innerstalhrim.dds", 0 },
		{ "innerstalhrimblack.dds", 0 },
		{ "nev-p\\clutter\\geodeametista.dds", 5 },
		{ "nev-p\\clutter\\geodeametista_m.dds", 5 },
		{ "nev-p\\clutter\\geodeametista_n.dds", 5 },
		{ "nev-p\\cubemaps\\nev_ametista_e.dds", 5 },
		{ "nev-p\\cubemaps\\nev_smeraldo_e.dds", 5 },
		{ "outerstalhrim.dds", 0 },
		{ "remastered\\soulgems\\dwemer\\dwesoulgemcontainergems.dds", 12 },
		{ "remastered\\soulgems\\dwemer\\dwesoulgemcontainergems_n.dds", 12 },
		{ "remastered\\soulgems\\fragments\\soulgemfragment01.dds", 8 },
		{ "remastered\\soulgems\\fragments\\soulgemfragment01_n.dds", 8 },
		{ "remastered\\soulgems\\fragments\\soulgemfragment02.dds", 8 },
		{ "remastered\\soulgems\\fragments\\soulgemfragment02_n.dds", 8 },
		{ "remastered\\soulgems\\fragments\\soulgemfragment03.dds", 8 },
		{ "remastered\\soulgems\\fragments\\soulgemfragment03_n.dds", 8 },
		{ "remastered\\soulgems\\fragments\\soulgemfragment04.dds", 8 },
		{ "remastered\\soulgems\\fragments\\soulgemfragment04_n.dds", 8 },
		{ "remastered\\soulgems\\fragments\\soulgemfragment05.dds", 8 },
		{ "remastered\\soulgems\\fragments\\soulgemfragment05_n.dds", 8 },
		{ "remastered\\soulgems\\soulgem_e.dds", 8 },
		{ "remastered\\soulgems\\soulgemblack.dds", 8 },
		{ "remastered\\soulgems\\soulgemblack_n.dds", 8 },
		{ "remastered\\soulgems\\soulgemcommon.dds", 8 },
		{ "remastered\\soulgems\\soulgemcommon_n.dds", 8 },
		{ "remastered\\soulgems\\soulgemgrand.dds", 8 },
		{ "remastered\\soulgems\\soulgemgrand_n.dds", 8 },
		{ "remastered\\soulgems\\soulgemgreater.dds", 8 },
		{ "remastered\\soulgems\\soulgemgreater_n.dds", 8 },
		{ "remastered\\soulgems\\soulgemlesser.dds", 8 },
		{ "remastered\\soulgems\\soulgemlesser_n.dds", 8 },
		{ "remastered\\soulgems\\soulgemparallax.dds", 8 },
		{ "remastered\\soulgems\\soulgempetty.dds", 8 },
		{ "remastered\\soulgems\\soulgempetty_n.dds", 8 },
		{ "sopablack_e.dds", 0 },
		{ "sopablackazura.dds", 0 },
		{ "sopawhite_e.dds", 0 },
		{ "sopawhiteazura.dds", 0 },
		{ "sopawhiteazura_em.dds", 0 },
		{ "sopawhiteazura_n.dds", 0 },
	} };
}
