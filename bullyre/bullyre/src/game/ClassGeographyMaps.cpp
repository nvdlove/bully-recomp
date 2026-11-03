// Geography class (ClassGeography): its set-up, the nine maps, the class's virtuals and
// small accessors (0x6EE420..0x6F3540, 0x6F66D0, 0x6F71B0 and two script natives).
//
// Each map's set-up (0x6EE6A0, 0x6EEFD0..0x6F28C0) is the same code with its own data: the
// countries' ids, their "_p" flags' texture index, their names and target positions, then
// the map's texture set from its txd: the map, the flags, the flags again, the "_p" flags.
// Written once as GeoSetupMap<map>. Oceania's x positions shift 20 left without a
// widescreen display.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/ClassGeography.h"
#include "game/ScriptApi.h"
#include "reversed.h"

namespace {

using namespace school;
using namespace geo;

// a country of a map: its name's key, its flags (texture names), its target
struct GeoCountry {
    uintptr_t key;
    uintptr_t flag;
    uintptr_t flagP;
    double x;      // (a float constant, or Oceania's double one added to the shift)
    float y;
};
struct GeoMap {
    int32_t index;
    int32_t count;
    uintptr_t txd;
    uintptr_t mapTexture;
    uintptr_t flagMask;
    uintptr_t flagPMask;
    int32_t widescreenShift;
    const GeoCountry* countries;
};

// Europe (0x6EE6A0): "MG_Geography_Europe"
constexpr GeoCountry kGeoEuropeCountries[16] = {
    {0x00945CF4, 0x00945BD4, 0x00945B0C, -10.0f, 60.0f},   // MGGE_ITALY, flag_ita, flag_ita_p
    {0x00945CDC, 0x00945BC8, 0x00945B00, -35.0f, -25.0f},   // MGGE_GERMANY, flag_ger, flag_ger_p
    {0x00945CCC, 0x00945BBC, 0x00945AF4, -105.0f, 25.0f},   // MGGE_FRANCE, flag_fra, flag_fra_p
    {0x00945C90, 0x00945BB4, 0x00945AE8, -145.0f, -50.0f},   // MGGE_UK, flag_uk, flag_uk_p
    {0x00945C84, 0x00945BA8, 0x00945ADC, -45.0f, 27.0f},   // MGGE_SWITZ, flag_swi, flag_swi_p
    {0x00945C78, 0x00945B9C, 0x00945AD0, -165.0f, 100.0f},   // MGGE_SPAIN, flag_spa, flag_spa_p
    {0x00945C68, 0x00945B90, 0x00945AB0, -210.0f, 90.0f},   // MGGE_PORTUGAL, flag_por, flag_port_p
    {0x00945C58, 0x00945B84, 0x00945AA4, -38.0f, -95.0f},   // MGGE_DENMARK, flag_den, flag_den_p
    {0x00945C48, 0x00945B78, 0x00945A98, 125.0f, -175.0f},   // MGGE_FINLAND, flag_fin, flag_fin_p
    {0x00945C3C, 0x00945B6C, 0x00945A8C, 15.0f, -125.0f},   // MGGE_SWEDEN, flag_swe, flag_swe_p
    {0x00945C30, 0x00945B60, 0x00945A80, 85.0f, 105.0f},   // MGGE_GREECE, flag_gre, flag_gre_p
    {0x00945C20, 0x00945B54, 0x00945A74, -205.0f, -53.0f},   // MGGE_IRELAND, flag_ire, flag_ire_p
    {0x00945C14, 0x00945B24, 0x00945A68, -165.0f, -45.0f},   // MGGE_NORWAY, flag_nor, flag_nor_p
    {0x00945C08, 0x00945B18, 0x00945A5C, 60.0f, -40.0f},   // MGGE_POLAND, flag_pol, flag_pol_p
    {0x00945BF8, 0x00945B48, 0x00945A50, 117.0f, 30.0f},   // MGGE_ROMANIA, flag_rom, flag_rom_p
    {0x00945BE8, 0x00945B3C, 0x00945A44, 150.0f, -15.0f},   // MGGE_UKRAINE, flag_ukr, flag_ukr_p
};
constexpr GeoMap kGeoEurope = {0, 16, 0x00945ABC, 0x00945BE0, 0x00945C98, 0x00945B30, 0, kGeoEuropeCountries};   // MG_Geography_Europe, europe, flag_x, flag_p_x

// Usa (0x6EEFD0): "MG_Geography_Usa"
constexpr GeoCountry kGeoUsaCountries[15] = {
    {0x00945F80, 0x00945E60, 0x00945DBC, 125.0f, 35.0f},   // MGGE_ALABAMA, flag_alab, flag_alab_p
    {0x00945F70, 0x00945E54, 0x00945DB0, -180.0f, 10.0f},   // MGGE_ARIZONA, flag_ariz, flag_ariz_p
    {0x00945F5C, 0x00945E48, 0x00945DA4, -260.0f, -25.0f},   // MGGE_CALIFORNIA, flag_cali, flag_cali_p
    {0x00945F4C, 0x00945E3C, 0x00945D98, 202.0f, 100.0f},   // MGGE_FLORIDA, flag_flor, flag_flor_p
    {0x00945F40, 0x00945E30, 0x00945D8C, -20.0f, -40.0f},   // MGGE_KANSAS, flag_kans, flag_kans_p
    {0x00945EF8, 0x00945E24, 0x00945D80, 140.0f, -30.0f},   // MGGE_KENTUCKY, flag_kent, flag_kent_p
    {0x00945EE8, 0x00945E18, 0x00945D74, 55.0f, 65.0f},   // MGGE_LOUISIANA, flag_loui, flag_loui_p
    {0x00945ED8, 0x00945E0C, 0x00945D68, 130.0f, -110.0f},   // MGGE_MICHIGAN, flag_mich, flag_mich_p
    {0x00945EC8, 0x00945E00, 0x00945D5C, 20.0f, -140.0f},   // MGGE_MINNESOTA, flag_minn, flag_minn_p
    {0x00945EB8, 0x00945DF4, 0x00945D50, -130.0f, -160.0f},   // MGGE_MONTANA, flag_mont, flag_mont_p
    {0x00945EA8, 0x00945DE8, 0x00945D30, 210.0f, -15.0f},   // MGGE_N_CAROLINA, flag_ncar, flag_ncar_p
    {0x00945E98, 0x00945DE0, 0x00945D24, 235.0f, -130.0f},   // MGGE_NEWYORK, flag_ny, flag_ny_p
    {0x00945E88, 0x00945F14, 0x00945D18, -35.0f, -130.0f},   // MGGE_S_DAKOTA, flag_sdac, flag_sdac_p
    {0x00945E7C, 0x00945F08, 0x00945D0C, -30.0f, 70.0f},   // MGGE_TEXAS, flag_texa, flag_texa_p
    {0x00945E70, 0x00945DD4, 0x00945D00, -165.0f, -60.0f},   // MGGE_UTAH, flag_utah, flag_utah_p
};
constexpr GeoMap kGeoUsa = {3, 15, 0x00945D3C, 0x00945E6C, 0x00945C98, 0x00945DC8, 0, kGeoUsaCountries};   // MG_Geography_Usa, usa, flag_x, flag_x_p

// Asia (0x6EF870): "MG_Geography_Asia"
constexpr GeoCountry kGeoAsiaCountries[15] = {
    {0x009461F8, 0x009460F4, 0x0094604C, -270.0f, -70.0f},   // MGGE_AFGHANISTAN, flag_afgh, flag_afgh_p
    {0x009461EC, 0x009460E8, 0x00946040, -10.0f, -60.0f},   // MGGE_CHINA, flag_chin, flag_chin_p
    {0x009461E0, 0x009460DC, 0x00946034, -180.0f, 40.0f},   // MGGE_INDIA, flag_indi, flag_indi_p
    {0x009461D4, 0x009460D0, 0x00946028, 250.0f, -90.0f},   // MGGE_JAPAN, flag_japa, flag_japa_p
    {0x009461C4, 0x009460C4, 0x0094601C, -250.0f, -110.0f},   // MGGE_TAJIKISTAN, flag_taji, flag_taji_p
    {0x0094618C, 0x009460B8, 0x00946010, -1.10000002f, -154.0f},   // MGGE_MONGOLIA, flag_mong, flag_mong_p
    {0x0094617C, 0x009460AC, 0x00946004, -55.0f, 34.0f},   // MGGE_MYANMAR, flag_myan, flag_myan_p
    {0x00946170, 0x009460A0, 0x00945FF8, -150.0f, -22.0f},   // MGGE_NEPAL, flag_nepa, flag_nepa_p
    {0x00946160, 0x00946094, 0x00945FEC, -250.0f, -30.0f},   // MGGE_PAKISTAN, flag_paki, flag_paki_p
    {0x00946150, 0x00946088, 0x00945FE0, 140.0f, 95.0f},   // MGGE_PHILIPINES, flag_phil, flag_phil_p
    {0x00946144, 0x0094607C, 0x00945FC0, 170.0f, 140.0f},   // MGGE_KOREA, flag_skor, flag_skor_p
    {0x00946134, 0x00946070, 0x00945FB4, -164.0f, -90.0f},   // MGGE_SRILLANKA, flag_sril, flag_sril_p
    {0x00946128, 0x00946064, 0x00945FA8, 120.0f, 15.0f},   // MGGE_TAIWAN, flag_taiw, flag_taiw_p
    {0x00946118, 0x0094619C, 0x00945F9C, -20.0f, 75.0f},   // MGGE_THAILAND, flag_thai, flag_thai_p
    {0x00946108, 0x00946058, 0x00945F90, 30.0f, 90.0f},   // MGGE_VIETNAM, flag_viet, flag_viet_p
};
constexpr GeoMap kGeoAsia = {1, 15, 0x00945FCC, 0x00946100, 0x00945C98, 0x00945B30, 0, kGeoAsiaCountries};   // MG_Geography_Asia, asia, flag_x, flag_p_x

// Africa (0x6F0110): "MG_Geography_Africa"
constexpr GeoCountry kGeoAfricaCountries[16] = {
    {0x0094648C, 0x0094639C, 0x009462DC, -170.0f, 50.0f},   // MGGE_CHAD, flag_chad, flag_chad_p
    {0x00946480, 0x00946390, 0x009462D0, -60.0f, -50.0f},   // MGGE_EGYPT, flag_egyp, flag_egyp_p
    {0x00946470, 0x00946384, 0x009462C4, -50.0f, 125.0f},   // MGGE_ETHIOPIA, flag_ethi, flag_ethi_p
    {0x00946450, 0x00946378, 0x009462B8, 200.0f, -120.0f},   // MGGE_IRAN, flag_iran, flag_iran_p
    {0x00946444, 0x0094636C, 0x009462AC, 80.0f, -130.0f},   // MGGE_IRAQ, flag_iraq, flag_iraq_p
    {0x00946438, 0x00946360, 0x009462A0, 167.0f, -45.0f},   // MGGE_QATAR, flag_qata, flag_qata_p
    {0x0094642C, 0x00946354, 0x00946280, 10.0f, -105.0f},   // MGGE_JORDAN, flag_jord, flag_jord_p
    {0x00946420, 0x00946348, 0x00946274, -190.0f, 80.0f},   // MGGE_LIBYA, flag_liby, flag_liby_p
    {0x00946414, 0x0094633C, 0x00946268, -260.0f, 40.0f},   // MGGE_NIGER, flag_nige, flag_nige_p
    {0x00946408, 0x00946330, 0x0094625C, 235.0f, -5.0f},   // MGGE_OMAN, flag_oman, flag_oman_p
    {0x009463F4, 0x00946324, 0x00946250, -50.0f, -40.0f},   // MGGE_SAUDIARABIA, flag_saud, flag_saud_p
    {0x009463E8, 0x00946318, 0x00946244, -60.0f, 50.0f},   // MGGE_SUDAN, flag_suda, flag_suda_p
    {0x009463DC, 0x009462E8, 0x00946238, 35.0f, -150.0f},   // MGGE_SYRIA, flag_syri, flag_syri_p
    {0x009463CC, 0x0094630C, 0x0094622C, -280.0f, -135.0f},   // MGGE_TUNISIA, flag_tuni, flag_tuni_p
    {0x009463BC, 0x00946300, 0x00946220, 135.0f, 120.0f},   // MGGE_SOMALIA, flag_soma, flag_soma_p
    {0x009463B0, 0x009462F4, 0x00946214, 130.0f, 55.0f},   // MGGE_YEMEN, flag_yemn, flag_yemn_p
};
constexpr GeoMap kGeoAfrica = {2, 16, 0x0094628C, 0x009463A8, 0x00945C98, 0x00945B30, 0, kGeoAfricaCountries};   // MG_Geography_Africa, africa, flag_x, flag_p_x

// CentralAmerica (0x6F0A40): "MG_Geography_Cename"
constexpr GeoCountry kGeoCentralAmericaCountries[16] = {
    {0x00946730, 0x00946624, 0x00946564, 30.0f, 120.0f},   // MGGE_BOLIVIA, flag_boli, flag_boli_p
    {0x00946724, 0x00946618, 0x00946558, 140.0f, 70.0f},   // MGGE_BRAZIL, flag_braz, flag_braz_p
    {0x00946714, 0x0094660C, 0x0094654C, -60.0f, -60.0f},   // MGGE_COLUMBIA, flag_colo, flag_colo_p
    {0x00946704, 0x00946600, 0x00946540, 35.0f, -170.0f},   // MGGE_STKITTS, flag_kine, flag_kine_p
    {0x009466E0, 0x009465F4, 0x00946534, -50.0f, -140.0f},   // MGGE_ARUBA, flag_arub, flag_arub_p
    {0x009466D0, 0x009465E8, 0x00946528, 55.0f, -150.0f},   // MGGE_MARTINIQUE, flag_mart, flag_mart_p
    {0x009466C0, 0x009465DC, 0x00946508, -117.0f, -15.0f},   // MGGE_ECUADOR, flag_ecua, flag_ecua_p
    {0x009466B0, 0x009465D0, 0x009464FC, -220.0f, -145.0f},   // MGGE_ELSALVADOR, flag_elsa, flag_elsa_p
    {0x009466A0, 0x009465C4, 0x009464F0, -150.0f, -60.0f},   // MGGE_F_GUIANA, flag_fgui, flag_fgui_p
    {0x00946694, 0x009465B8, 0x009464E4, 80.0f, -60.0f},   // MGGE_GUYANA, flag_gyan, flag_gyan_p
    {0x00946684, 0x009465AC, 0x009464D8, -105.0f, -185.0f},   // MGGE_JAMAICA, flag_jama, flag_jama_p
    {0x00946674, 0x009465A0, 0x009464CC, -180.0f, -135.0f},   // MGGE_NICARAGUA, flag_nica, flag_nica_p
    {0x00946668, 0x00946594, 0x009464C0, -105.0f, -95.0f},   // MGGE_PANAMA, flag_pana, flag_pana_p
    {0x0094665C, 0x00946588, 0x009464B4, 140.0f, 40.0f},   // MGGE_PERU, flag_peru, flag_peru_p
    {0x0094664C, 0x0094657C, 0x009464A8, 59.0f, -115.0f},   // MGGE_TRINIDAD, flag_trin, flag_trin_p
    {0x0094663C, 0x00946570, 0x0094649C, 20.0f, -85.0f},   // MGGE_VENEZUELA, flag_vene, flag_vene_p
};
constexpr GeoMap kGeoCentralAmerica = {4, 16, 0x00946514, 0x00946630, 0x00945C98, 0x00945DC8, 0, kGeoCentralAmericaCountries};   // MG_Geography_Cename, centamer, flag_x, flag_x_p

// Canada (0x6F1360): "MG_Geography_Canada"
constexpr GeoCountry kGeoCanadaCountries[13] = {
    {0x009469A8, 0x00946858, 0x009467D0, -223.440002f, 3.78999996f},   // MGGE_BRITISHCOLUMBIA, flag_bc, flag_bc_p
    {0x00946990, 0x0094684C, 0x009467C4, -161.470001f, 11.79f},   // MGGE_ALBERTA, flag_albt, flag_albt_p
    {0x00946978, 0x00946840, 0x009467B8, -102.290001f, 5.76999998f},   // MGGE_SASKATCHEWAN, flag_sask, flag_sask_p
    {0x00946960, 0x00946834, 0x009467AC, -38.7400017f, 12.0600004f},   // MGGE_MANITOBA, flag_mani, flag_mani_p
    {0x0094694C, 0x00946828, 0x009467A0, 40.4000015f, 62.9099998f},   // MGGE_ONTARIO, flag_ont, flag_ont_p
    {0x00946938, 0x00946820, 0x00946794, 129.720001f, 13.79f},   // MGGE_QUEBEC, flag_qb, flag_qb_p
    {0x009468C0, 0x00946814, 0x00946788, 191.490005f, -25.7999992f},   // MGGE_NEWFOUNDLAND, flag_nwfn, flag_nwfn_p
    {0x009468AC, 0x00946808, 0x0094677C, 192.25f, 60.5900002f},   // MGGE_NEWBRUNSWICK, flag_newb, flag_newb_p
    {0x0094689C, 0x009467FC, 0x00946770, 228.389999f, 59.9799995f},   // MGGE_NOVASCOTIA, flag_nova, flag_nova_p
    {0x00946890, 0x009467F0, 0x00946764, 222.809998f, 33.4300003f},   // MGGE_PEI, flag_pei, flag_pei_p
    {0x00946884, 0x009468EC, 0x00946758, -153.550003f, -94.5299988f},   // MGGE_NWT, flag_nwt, flag_nwt_p
    {0x00946878, 0x009468E0, 0x0094674C, -242.880005f, -112.050003f},   // MGGE_YUKON, flag_yukn, flag_yukn_p
    {0x00946868, 0x009468D4, 0x00946740, -42.7799988f, -69.1100006f},   // MGGE_NUNAVUT, flag_nuna, flag_nuna_p
};
constexpr GeoMap kGeoCanada = {5, 13, 0x009467DC, 0x00946860, 0x00945C98, 0x00945B30, 0, kGeoCanadaCountries};   // MG_Geography_Canada, canada, flag_x, flag_p_x

// SouthernAfrica (0x6F1B00): "MG_Geography_AfrSth"
constexpr GeoCountry kGeoSouthernAfricaCountries[15] = {
    {0x00946C9C, 0x00946B28, 0x00946A80, 41.3100014f, -72.1399994f},   // MGGE_ZAIRE, flag_zai, flag_zai_p
    {0x00946C88, 0x00946B1C, 0x00946A74, -28.0100002f, -4.38000011f},   // MGGE_ANGOLA, flag_ang, flag_ang_p
    {0x00946C70, 0x00946B10, 0x00946A68, 36.4900017f, 113.230003f},   // MGGE_SOUTH AFRICA, flag_safr, flag_safr_p
    {0x00946C58, 0x00946B04, 0x00946A5C, 227.330002f, 39.6100006f},   // MGGE_MADAGASCAR, flag_mad, flag_mad_p
    {0x00946BE0, 0x00946AF8, 0x00946A50, 133.479996f, -54.2000008f},   // MGGE_TANZANIA, flag_tan, flag_tan_p
    {0x00946BD4, 0x00946AEC, 0x00946A44, 57.9700012f, 4.11999989f},   // MGGE_ZAMBIA, flag_zam, flag_zam_p
    {0x00946BB8, 0x00946AE0, 0x00946A38, -47.2400017f, -90.9100037f},   // MGGE_REPUBLIC OF THE CONGO, flag_con, flag_con_p
    {0x00946BA8, 0x00946AD4, 0x00946A2C, 81.9899979f, 41.4000015f},   // MGGE_ZIMBABWE, flag_zim, flag_zim_p
    {0x00946B98, 0x00946AC8, 0x00946A20, -116.919998f, -159.539993f},   // MGGE_NIGERIA, flag_nig, flag_nig_p
    {0x00946B84, 0x00946ABC, 0x00946A00, -236.679993f, -145.809998f},   // MGGE_IVORY COAST, flag_ivo, flag_ivo_p
    {0x00946B78, 0x00946AB0, 0x009469F4, 160.929993f, -96.7399979f},   // MGGE_KENYA, flag_ken, flag_ken_p
    {0x00946B68, 0x00946AA4, 0x009469E8, 28.1200008f, 64.0100021f},   // MGGE_BOTSWANA, flag_bot, flag_bot_p
    {0x00946B58, 0x00946BF0, 0x009469DC, -30.8600006f, 71.0899963f},   // MGGE_NAMIBIA, flag_nam, flag_nam_p
    {0x00946B4C, 0x00946A98, 0x009469D0, 107.760002f, -106.529999f},   // MGGE_UGANDA, flag_uga, flag_uga_p
    {0x00946B3C, 0x00946A8C, 0x009469C4, 119.389999f, 39.2900009f},   // MGGE_MOZAMBIQUE, flag_moz, flag_moz_p
};
constexpr GeoMap kGeoSouthernAfrica = {6, 15, 0x00946A0C, 0x00946B34, 0x00945C98, 0x00945DC8, 0, kGeoSouthernAfricaCountries};   // MG_Geography_AfrSth, bafr, flag_x, flag_x_p

// Oceania (0x6F23B0): "MG_Geography_Ocean"
constexpr GeoCountry kGeoOceaniaCountries[8] = {
    {0x00946E60, 0x00946DA4, 0x00946D04, 24.73, 19.1299992f},   // MGGE_AUSTRALIA, flag_aust, flag_aust_p
    {0x00946E44, 0x00946D88, 0x00946CF8, 245.9, 103.57f},   // MGGE_NEWZEALAND, flag_newz, flag_newz_p
    {0x00946E30, 0x00946D7C, 0x00946CEC, -59.12, -111.629997f},   // MGGE_INDONESIA, flag_indo, flag_indo_p
    {0x00946E14, 0x00946D1C, 0x00946CE0, -111.61, -137.429993f},   // MGGE_MALAYSIA, flag_maly, flag_maly_p
    {0x00946E04, 0x00946D10, 0x00946CD4, 113.02, -82.7200012f},   // MGGE_NEWGUINEA, flag_papu, flag_papu_p
    {0x00946DE4, 0x00946D3C, 0x00946CC8, 178.25, -74.1800003f},   // MGGE_SOLOMANS, flag_solo, flag_solo_p
    {0x00946DD8, 0x00946D54, 0x00946CBC, 295.37, -20.2000008f},   // MGGE_FIJI, flag_fiji, flag_fiji_p
    {0x00946DC0, 0x00946D48, 0x00946CB0, 0.0, 1.34000003f},   // MGGE_NEWCALEDONIA, flag_nwca, flag_nwca_p
};
constexpr GeoMap kGeoOceania = {7, 8, 0x00946D28, 0x00946DB0, 0x00945C98, 0x00945DC8, 1, kGeoOceaniaCountries};   // MG_Geography_Ocean, ocamap, flag_x, flag_x_p

// NorthEast (0x6F28C0): "MG_Geography_EUs"
constexpr GeoCountry kGeoNorthEastCountries[9] = {
    {0x00946FD8, 0x00946F28, 0x00946EDC, 188.389999f, -180.059998f},   // MGGE_MAINE, flag_main, flag_main_p
    {0x00945E98, 0x00945DE0, 0x00945D24, 22.0699997f, -31.2399998f},   // MGGE_NEWYORK, flag_ny, flag_ny_p
    {0x00946FBC, 0x00946F0C, 0x00946ED0, 103.779999f, -107.940002f},   // MGGE_VERMONT, flag_vt, flag_vt_p
    {0x00945ED8, 0x00945E0C, 0x00945D68, -237.660004f, -9.92000008f},   // MGGE_MICHIGAN, flag_mich, flag_mich_p
    {0x00946FA4, 0x00946F00, 0x00946EC4, -168.649994f, 95.1900024f},   // MGGE_OHIO, flag_ohio, flag_ohio_p
    {0x00946F8C, 0x00946EF4, 0x00946EB8, -262.660004f, 107.559998f},   // MGGE_INDIANA, flag_ida, flag_ida_p
    {0x00946F74, 0x00946EE8, 0x00946EAC, -12.6000004f, 49.7200012f},   // MGGE_PENNSYLVANIA, flag_penn, flag_penn_p
    {0x00946F58, 0x00946F20, 0x00946EA0, 129.360001f, -1.91999996f},   // MGGE_CONNECTICUT, flag_ct, flag_ct_p
    {0x00946F44, 0x00946F14, 0x00946E94, 37.7000008f, 113.650002f},   // MGGE_MARYLAND, flag_mary, flag_mary_p
};
constexpr GeoMap kGeoNorthEast = {8, 9, 0x00946E80, 0x00946F38, 0x00945C98, 0x00945B30, 0, kGeoNorthEastCountries};   // MG_Geography_EUs, eastus, flag_x, flag_p_x


// 0x6EE6A0 (Europe), 0x6EEFD0 (the USA), 0x6EF870 (Asia), 0x6F0110 (Africa), 0x6F0A40 (Central
// America), 0x6F1360 (Canada), 0x6F1B00 (southern Africa), 0x6F23B0 (Oceania), 0x6F28C0 (the
// north-east USA): a map's countries and textures
template <const GeoMap& M>
void __fastcall GeoSetupMap(ClassGeography* g, re::Edx) {
    g->map = M.index;
    g->countryCount = uint32_t(M.count);
    Geo_ResetCountries(g);
    float shift = 0.0f;
    if (M.widescreenShift != 0 && widescreen == 0) {
        shift = -20.0f;
    }
    for (int i = 0; i < M.count; ++i) {
        const GeoCountry& c = M.countries[i];
        g->ids[i] = i + 1;
        g->texIds[i] = 2 * M.count + 1 + i;
        g->keys[i] = GameString(c.key);
        g->countryPos[i].x = float(double(shift) + c.x);
        g->countryPos[i].y = c.y;
    }
    // the map, the flags, the flags again, the "_p" flags
    ClassTextureEntry entries[3 * 16 + 1];
    int n = 3 * M.count + 1;
    entries[0] = {0, GameString(M.mapTexture), nullptr};
    for (int i = 0; i < M.count; ++i) {
        const GeoCountry& c = M.countries[i];
        entries[1 + i] = {1 + i, GameString(c.flag), GameString(M.flagMask)};
        entries[1 + M.count + i] = {1 + M.count + i, GameString(c.flag), GameString(M.flagMask)};
        entries[1 + 2 * M.count + i] = {1 + 2 * M.count + i, GameString(c.flagP), GameString(M.flagPMask)};
    }
    int slot = Txd_FindTxdSlot(GameString(M.txd), 0);
    g->txdSlots[M.index] = slot;
    Streaming_RequestModel(slot + 0x2FA8, 9);
    Streaming_LoadAllRequestedModels(1);
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(g->txdSlots[M.index]);
    if (g->textureSets[M.index] == nullptr) {
        g->textureSets[M.index] = static_cast<re::Ptr32<void>*>(Malloc(uint32_t(n) * 4));
    }
    for (int i = 0; i < n; ++i) {
        g->textureSets[M.index][i] = Txd_FindTexture(entries[i].name, entries[i].mask);
    }
    Txd_PopCurrentTxd();
}

// 0x6EE420 (virtual +0x14): the game ends: the textures released, the radar and the
// camera's field of view back
void __fastcall GeoEnd(ClassGeography* g, re::Edx) {
    static constexpr int kCounts[9] = {0x31, 0x2E, 0x31, 0x2E, 0x31, 0x28, 0x2E, 0x19, 0x1C};
    g->active = 0;
    g->hud = 0;
    Minigame_EndTextures(g);
    Minigame_EndSounds(g);
    for (int m = 0; m < 9; ++m) {
        Minigame_UnloadTextureSet(g, g->textureSets[m], kCounts[m], g->txdSlots[m]);
        g->textureSets[m] = nullptr;
    }
    radarHidden = 1;
    Camera_RestoreFov(camera);
}

// 0x6EE550: the map's countries back in play (none placed), unless the map is done: then
// the class succeeds
void __fastcall GeoResetCountries(ClassGeography* g, re::Edx) {
    g->invalidOperation = 0;
    if (g->mapDone == 0) {
        g->remaining = int32_t(g->countryCount);
        for (int i = 0; i < g->remaining; ++i) {
            g->placed[i] = 0;
            g->anim1Id = -1;
            g->anim2Id = -1;
            g->selected[0] = g->countryCount;
            g->selected[1] = g->countryCount;
        }
    }
    if (g->mapDone == 1) {
        Minigame_Success(g);
    }
}

// 0x6EE5D0: a map's texture (nullptr for no such map)
void* __fastcall GeoGetTexture(ClassGeography* g, re::Edx, int map, int index) {
    if (uint32_t(map) > 8) {
        return nullptr;
    }
    return g->textureSets[map][index];
}

// 0x6F2E20, 0x6F2E40: a valid / invalid placing since the last ask (cleared)
uint8_t __fastcall GeoTakeValidOperation(ClassGeography* g, re::Edx) {
    uint8_t was = g->validOperation;
    g->validOperation = 0;
    return was == 1 ? 1 : 0;
}
uint8_t __fastcall GeoTakeInvalidOperation(ClassGeography* g, re::Edx) {
    uint8_t was = g->invalidOperation;
    g->invalidOperation = 0;
    return was == 1 ? 1 : 0;
}

// 0x6F2E60
float __fastcall GeoGetScore(ClassGeography* g, re::Edx) {
    return g->score;
}

// 0x6F2E70: returns 1 (shared: also the virtual +8 of ConditionNOT and other vtables)
int __fastcall ReturnOne(void*, re::Edx) {
    return 1;
}

// 0x6F2E80: ClassGeographySetTimer's
void __fastcall GeoSetTimer(ClassGeography* g, re::Edx, float time, float second) {
    g->timeLeft = time;
    g->unknown384 = second;
}

// 0x6F2EA0
ClassGeography* __fastcall GeoConstruct(ClassGeography* g, re::Edx) {
    Minigame_Construct(g);
    g->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00946FF8);
    g->triggerFunction = 0;
    g->level = 0;
    g->hover[0] = 0;
    g->hover[1] = 0;
    for (int m = 0; m < 9; ++m) {
        g->textureSets[m] = nullptr;
    }
    g->unknown38 = 1;
    g->unknown3C = 3;
    g->countryCount = 16;
    g->unknown4B8 = -1;
    return g;
}

// 0x6F2F30: the countries shuffled (with their slots' flags); both players' colours reset
void __fastcall GeoShuffleCountries(ClassGeography* g, re::Edx) {
    uint8_t* reset = GeoColour(0x020C8348);
    uint8_t* colour1 = GeoColour(0x020C8328);
    uint8_t* colour2 = GeoColour(0x020C832C);
    for (int k = 0; k < 4; ++k) {
        colour1[k] = reset[k];
    }
    for (int k = 0; k < 4; ++k) {
        colour2[k] = reset[k];
    }
    for (uint32_t i = 0; i < g->countryCount; ++i) {
        int last = int(g->countryCount - 1);
        int j = Rand() * last / 0x7FFF;
        int32_t id = g->ids[i];
        int32_t tex = g->texIds[i];
        int32_t slotId = g->slotIds[i];
        const char* key = g->keys[i];
        GeoPoint pos = g->countryPos[i];
        g->ids[i] = g->ids[j];
        g->texIds[i] = g->texIds[j];
        g->keys[i] = g->keys[j];
        g->countryPos[i] = g->countryPos[j];
        g->slotIds[i] = g->slotIds[j];
        g->ids[j] = id;
        g->texIds[j] = tex;
        g->keys[j] = key;
        g->countryPos[j] = pos;
        g->slotIds[j] = slotId;
    }
}

// 0x6F30B0 (virtual +0x10): the game starts: the level's map, the timer, the cursors, the
// flag slots, the common textures and sounds, the info popup
void __fastcall GeoStart(ClassGeography* g, re::Edx) {
    g->range[0] = 10.0f;
    g->range[1] = 10.0f;
    g->anchor[0] = {0.0f, 0.0f};
    g->unknown4AC = 0;
    g->anchor[1].x = 0.0f;
    g->unknown4B0 = 0;
    g->anchor[1].y = 0.0f;
    g->active = 1;
    g->inRange[0] = 0;
    g->inRange[1] = 0;
    g->snap[0] = 0;
    g->snap[1] = 0;
    g->fading = 0;
    g->dealing = 0;
    switch (g->level) {
    case 0: Geo_SetupEurope(g); break;
    case 1: Geo_SetupUsa(g); break;
    case 2: Geo_SetupAsia(g); break;
    case 3: Geo_SetupAfrica(g); break;
    case 4: Geo_SetupCentralAmerica(g); break;
    case 5: Geo_SetupCanada(g); break;
    case 6: Geo_SetupSouthernAfrica(g); break;
    case 7: Geo_SetupOceania(g); break;
    case 8: Geo_SetupNorthEast(g); break;
    default: break;
    }
    uint32_t count = g->countryCount;
    g->pulse = 1.0f;
    g->timeLeft = 180.0f;
    g->hud = 0;
    g->unknown384 = 0.0f;
    g->selected[0] = count;
    g->selected[1] = count;
    g->warnTime = 30.0f;
    g->padRepeat = 0;
    g->warned = 0;
    g->triggerTime = 15.0f;
    g->scoreMsgCount = 0;
    g->mapDone = 0;
    g->score = 0.0f;
    g->padDir = 4;
    g->unknownC8 = 0;
    g->cursorWidth = 1.0f;
    g->hoverId[0] = 2;
    g->cursorHeight = 1.0f;
    g->hover[0] = 0;
    g->hoverId[1] = 2;
    g->cursor[0].x = -20.0f;
    g->hover[1] = 0;
    g->cursor[0].y = 20.0f;
    g->cursor[1] = {20.0f, 20.0f};
    g->unknown508 = 4.0f;
    for (uint32_t i = 0; i < g->countryCount; ++i) {
        g->slotIds[i] = g->ids[i];
    }
    // the slots: eight on the top row, the rest below
    static constexpr float kTopX[8] = {-250.0f, -180.0f, -110.0f, -40.0f, 30.0f, 100.0f, 170.0f, 240.0f};
    for (int k = 0; k < 8; ++k) {
        g->slotPos[k] = {kTopX[k], 180.0f};
        g->slotPos[8 + k] = {160.0f, 330.0f};
    }
    g->fade = 0xFF;
    g->fading = 0;
    const ClassTextureEntry textures[7] = {
        {0, GameString(0x00947100), GameString(0x009470F0)},   // star_sticker
        {1, GameString(0x009470E4), GameString(0x009470D4)},   // closedhand
        {2, GameString(0x009470C8), GameString(0x009470BC)},   // openhand
        {3, GameString(0x00947124), GameString(0x00947124)},   // closedhandhalo
        {4, GameString(0x009470B4), GameString(0x009470A8)},   // minus_5
        {5, GameString(0x00944C20), GameString(0x00944C18)},   // pin
        {6, GameString(0x00944B50), GameString(0x00944B44)},   // x_target
    };
    Minigame_LoadTextures(g, 7, textures, GameString(0x00947110));   // MG_Geography_Common
    const ClassSoundEntry soundTable[9] = {
        {0, GameString(0x009470A0)},   // SelFlag
        {1, GameString(0x00947094)},   // DropCorrect
        {2, GameString(0x00947084)},   // DropIncorrect
        {3, GameString(0x00947078)},   // MapFinish
        {4, GameString(0x00947064)},   // NewMap
        {5, GameString(0x00947054)},   // RollOverFlag
        {6, GameString(0x00947048)},   // FlagSlide
        {7, GameString(0x0094703C)},   // FlagFalls
        {8, GameString(0x0094702C)},   // TimedWarning
    };
    Minigame_LoadSounds(g, 9, soundTable, GameString(0x0094706C));   // GeoClass
    Popup_Show(&g->popupKind, 2, GameString(0x00947014), GameString(0x00947020), 7, -1);   // MGGE_INFO
    Geo_ResetCountries(g);
    radarHidden = 0;
    Camera_SetFov(camera, 1.6f);
}

// 0x6F66D0 (virtual +0): the scalar deleting destructor
ClassGeography* __fastcall GeoDestruct(ClassGeography* g, re::Edx, int flags) {
    g->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00946FF8);
    Geo_End(g);
    ScriptRef_Clear(&g->triggerFunction);
    Minigame_Destruct(g);
    if ((flags & 1) != 0) {
        Free(g);
    }
    return g;
}

// 0x6F71B0: ClassGeographySetScoreMsg's (ten at most)
void __fastcall GeoSetScoreMsg(ClassGeography* g, re::Edx, float score, const char* key) {
    if (g->scoreMsgCount >= 10) {
        return;
    }
    g->scoreMsgs[g->scoreMsgCount].score = score;
    std::strcpy(g->scoreMsgs[g->scoreMsgCount].key, key);
    g->scoreMsgCount += 1;
}

// ---- the script natives ----------------------------------------------------------------------

// 0x6F7260: ClassGeographySetTimer(time [, second])
int __cdecl Native_ClassGeographySetTimer(lua_State* L) {
    Script::GetArgCount(L);
    float time = Script::GetFloat(L, 0);
    float second = 0.0f;
    if (Script::GetArgCount(L) >= 2) {
        second = Script::GetFloat(L, 1);
    }
    Geo_SetTimer(re::PtrCast<ClassGeography*>(currentMinigame), time, second);
    return 0;
}

// 0x6F7310: ClassGeographyGetScorePercentage (the score over the shared "return 1")
int __cdecl Native_ClassGeographyGetScorePercentage(lua_State* L) {
    float score = Geo_GetScore(re::PtrCast<ClassGeography*>(currentMinigame));
    int max = Geo_GetMaxScore(re::PtrCast<ClassGeography*>(currentMinigame));
    Script::PushFloat(L, score / float(max) * 100.0f);
    return 1;
}

} // namespace

#define CGEO43_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CGEO43_REVERSED("ClassGeography::End_6EE420", 0x006EE420, &GeoEnd);
CGEO43_REVERSED("ClassGeography::ResetCountries_6EE550", 0x006EE550, &GeoResetCountries);
CGEO43_REVERSED("ClassGeography::GetTexture_6EE5D0", 0x006EE5D0, &GeoGetTexture);
CGEO43_REVERSED("ClassGeography::SetupEurope_6EE6A0", 0x006EE6A0, &GeoSetupMap<kGeoEurope>);
CGEO43_REVERSED("ClassGeography::SetupUsa_6EEFD0", 0x006EEFD0, &GeoSetupMap<kGeoUsa>);
CGEO43_REVERSED("ClassGeography::SetupAsia_6EF870", 0x006EF870, &GeoSetupMap<kGeoAsia>);
CGEO43_REVERSED("ClassGeography::SetupAfrica_6F0110", 0x006F0110, &GeoSetupMap<kGeoAfrica>);
CGEO43_REVERSED("ClassGeography::SetupCentralAmerica_6F0A40", 0x006F0A40, &GeoSetupMap<kGeoCentralAmerica>);
CGEO43_REVERSED("ClassGeography::SetupCanada_6F1360", 0x006F1360, &GeoSetupMap<kGeoCanada>);
CGEO43_REVERSED("ClassGeography::SetupSouthernAfrica_6F1B00", 0x006F1B00, &GeoSetupMap<kGeoSouthernAfrica>);
CGEO43_REVERSED("ClassGeography::SetupOceania_6F23B0", 0x006F23B0, &GeoSetupMap<kGeoOceania>);
CGEO43_REVERSED("ClassGeography::SetupNorthEast_6F28C0", 0x006F28C0, &GeoSetupMap<kGeoNorthEast>);
CGEO43_REVERSED("ClassGeography::TakeValidOperation_6F2E20", 0x006F2E20, &GeoTakeValidOperation);
CGEO43_REVERSED("ClassGeography::TakeInvalidOperation_6F2E40", 0x006F2E40, &GeoTakeInvalidOperation);
CGEO43_REVERSED("ClassGeography::GetScore_6F2E60", 0x006F2E60, &GeoGetScore);
CGEO43_REVERSED("ReturnOne_6F2E70", 0x006F2E70, &ReturnOne);
CGEO43_REVERSED("ClassGeography::SetTimer_6F2E80", 0x006F2E80, &GeoSetTimer);
CGEO43_REVERSED("ClassGeography::ClassGeography_6F2EA0", 0x006F2EA0, &GeoConstruct);
CGEO43_REVERSED("ClassGeography::ShuffleCountries_6F2F30", 0x006F2F30, &GeoShuffleCountries);
CGEO43_REVERSED("ClassGeography::Start_6F30B0", 0x006F30B0, &GeoStart);
CGEO43_REVERSED("ClassGeography::~ClassGeography_6F66D0", 0x006F66D0, &GeoDestruct);
CGEO43_REVERSED("ClassGeography::SetScoreMsg_6F71B0", 0x006F71B0, &GeoSetScoreMsg);
CGEO43_REVERSED("Native_ClassGeographySetTimer", 0x006F7260, &Native_ClassGeographySetTimer);
CGEO43_REVERSED("Native_ClassGeographyGetScorePercentage", 0x006F7310, &Native_ClassGeographyGetScorePercentage);
