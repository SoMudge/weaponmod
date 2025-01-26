/*
 * Half-Life Weapon Mod
 * Copyright (c) 2012 - 2014 AGHL.RU Dev Team
 * 
 * http://aghl.ru/forum/ - Russian Half-Life and Adrenaline Gamer Community
 *
 *
 *    This program is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#include "wpnmod_entity.h"
#include "wpnmod_grenade.h"
#include "wpnmod_config.h"
#include "wpnmod_utils.h"
#include "wpnmod_hooks.h"
#include "wpnmod_parse.h"
#include "wpnmod_items.h"


#define AMXX_NATIVE(x) static cell AMX_NATIVE_CALL x (AMX *amx, cell *params)

#define CHECK_ENTITY(x)																\
	if (x != 0 && (FNullEnt(INDEXENT2(x)) || x < 0 || x > gpGlobals->maxEntities))	\
	{																				\
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid entity (%d).", x);				\
		return 0;																	\
	}																				\

#define CHECK_OFFSET(x)																						\
	if (x < 0 || x >= Offset_End)																			\
	{																										\
		MF_LogError(amx, AMX_ERR_NATIVE, "Function out of bounds. Got: %d  Max: %d.", x, Offset_End - 1);	\
		return 0;																							\
	}																										\

#define CHECK_OFFSET_CBASE(x)																				\
	if (x < 0 || x >= CBase_End)																			\
	{																										\
		MF_LogError(amx, AMX_ERR_NATIVE, "Function out of bounds. Got: %d  Max: %d.", x, CBase_End - 1);	\
		return 0;																							\
	}

#define CHECK_PARAMS_NUM(x)																					\
	if (paramnum != x)																						\
	{																										\
		MF_LogError(amx, AMX_ERR_NATIVE, "Bad arguments num, expected %d, passed %d", x, paramnum);			\
		return -1;																							\
	}
































/**
 * Same as wpnmod_radius_damage, but blocks 'ghost mines' and 'ghost nades'.
 *
 * @param vecSrc			Origin of explosion.
 * @param iInflictor		Entity which causes the damage impact.
 * @param iAttacker			Attacker index.
 * @param flDamage			Damage amount.
 * @param flRadius			Damage radius.
 * @param iClassIgnore		Class to ignore.
 * @param bitsDamageType	Damage type (see CLASSIFY defines).
 *
 * native wpnmod_radius_damage2(const Float: vecSrc[3], const iInflictor, const iAttacker, const Float: flDamage, const Float: flRadius, const iClassIgnore, const bitsDamageType);
*/
AMXX_NATIVE(wpnmod_radius_damage2)
{
	Vector vecSrc;
	cell *vSrc = MF_GetAmxAddr(amx, params[1]);

	vecSrc.x = amx_ctof(vSrc[0]);
	vecSrc.y = amx_ctof(vSrc[1]);
	vecSrc.z = amx_ctof(vSrc[2]);

	int iInflictor = params[2];
	int iAttacker = params[3];

	CHECK_ENTITY(iInflictor)
	CHECK_ENTITY(iAttacker)

	RadiusDamage2(vecSrc, INDEXENT2(iInflictor), INDEXENT2(iAttacker), amx_ctof(params[4]), amx_ctof(params[5]), params[6], params[7]);
	return 1;
}

/**
 * Deprecated. See wpnmod_radius_damage2 instead.
*/
AMXX_NATIVE(wpnmod_radius_damage)
{
	return wpnmod_radius_damage2(amx, params);
}

/**
 * Eject a brass from player's weapon.
 *
 * @param iPlayer			Player index.
 * @param iShellModelIndex	Index of precached shell's model.
 * @param iSoundtype		Bounce sound type (see defines).
 * @param flForwardScale	Forward scale value.
 * @param flUpScale			Up scale value.
 * @param flRightScale		Right scale value.
 *
 * native wpnmod_eject_brass(const iPlayer, const iShellModelIndex, const iSoundtype, const Float: flForwardScale, const Float: flUpScale, const Float: flRightScale);
*/
AMXX_NATIVE(wpnmod_eject_brass)
{
	int iPlayer = params[1];

	CHECK_ENTITY(iPlayer)

	edict_t* pPlayer = INDEXENT2(iPlayer);

	Vector vecShellVelocity = pPlayer->v.velocity + gpGlobals->v_right * RANDOM_FLOAT(50, 70) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25;

	UTIL_EjectBrass
	(
		pPlayer->v.origin + pPlayer->v.view_ofs + gpGlobals->v_up * amx_ctof(params[5]) + gpGlobals->v_forward * amx_ctof(params[4]) + gpGlobals->v_right * amx_ctof(params[6]), 
		vecShellVelocity, 
		pPlayer->v.angles.y, 
		params[2], 
		params[3]
	);

	return 1;
}







/**
 * Resets the global multi damage accumulator.
 *
 * native wpnmod_clear_multi_damage();
 */
AMXX_NATIVE(wpnmod_clear_multi_damage)
{
	CLEAR_MULTI_DAMAGE();
	return 1;
}

/**
 * Inflicts contents of global multi damage register on entity.
 *
 * @param iInflictor		Entity which causes the damage impact.
 * @param iAttacker			Attacker index.
 *
 * native wpnmod_apply_multi_damage(const iInflictor, const iAttacker);
 */
AMXX_NATIVE(wpnmod_apply_multi_damage)
{
	CHECK_ENTITY(params[1])
	CHECK_ENTITY(params[2])

	APPLY_MULTI_DAMAGE(INDEXENT2(params[1]), INDEXENT2(params[2]));
	return 1;
}

/**
 * Returns index of random damage decal for given entity.
 *
 * @param iEntity		Entity.
 *
 * @return				Index of damage decal. (integer)
 *
 * native wpnmod_get_damage_decal(const iEntity);
 */
AMXX_NATIVE(wpnmod_get_damage_decal)
{
	CHECK_ENTITY(params[1])

	int decalNumber = GET_DAMAGE_DECAL(INDEXENT2(params[1]));

	if (decalNumber < 0 || decalNumber > (int)g_Config.m_pDecalList.size())
	{
		return -1;
	}

	return g_Config.m_pDecalList[decalNumber]->index;
}

/**
 * Fire default contact grenade from player's weapon.
 *
 * @param iPlayer			Player index.
 * @param vecStart			Start position.
 * @param vecVelocity		Velocity.
 * @param szCallBack		The forward to call on explode.
 *
 * @return					Contact grenade index or -1 on failure. (integer)
 *
 * native wpnmod_fire_contact_grenade(const iPlayer, const Float: vecStart[3], const Float: vecVelocity[3], const szCallBack[] = "");
*/
AMXX_NATIVE(wpnmod_fire_contact_grenade)
{
	CHECK_ENTITY(params[1])

	Vector vecStart;
	Vector vecVelocity;

	cell *vStart = MF_GetAmxAddr(amx, params[2]);
	cell *vVelocity = MF_GetAmxAddr(amx, params[3]);

	vecStart.x = amx_ctof(vStart[0]);
	vecStart.y = amx_ctof(vStart[1]);
	vecStart.z = amx_ctof(vStart[2]);

	vecVelocity.x = amx_ctof(vVelocity[0]);
	vecVelocity.y = amx_ctof(vVelocity[1]);
	vecVelocity.z = amx_ctof(vVelocity[2]);

	edict_t* pGrenade = Grenade_ShootContact(INDEXENT2(params[1]), vecStart, vecVelocity);

	if (IsValidPev(pGrenade))
	{
		char *funcname = MF_GetAmxString(amx, params[4], 0, NULL);

		if (funcname)
		{
			int iForward = MF_RegisterSPForwardByName
			(
				amx, 
				funcname, 
				FP_CELL,
				FP_CELL,
				FP_ARRAY,
				FP_ARRAY,
				FP_DONE
			);

			g_Entity.SetAmxxForward(pGrenade, FWD_ENT_EXPLODE, iForward == -1 ? NULL : iForward);
		}

		return ENTINDEX(pGrenade);
	}

	return -1;
}

/**
 * Fire default timed grenade from player's weapon.
 *
 * @param iPlayer			Player index.
 * @param vecStart			Start position.
 * @param vecVelocity		Velocity.
 * @param flTime			Time before detonate.
 * @param szCallBack		The forward to call on explode.
 *
 * @return					Contact grenade index or -1 on failure. (integer)
 *
 * native wpnmod_fire_timed_grenade(const iPlayer, const Float: vecStart[3], const Float: vecVelocity[3], const Float: flTime = 3.0, const szCallBack[] = "");
*/
AMXX_NATIVE(wpnmod_fire_timed_grenade)
{
	CHECK_ENTITY(params[1])

	Vector vecStart;
	Vector vecVelocity;

	cell *vStart = MF_GetAmxAddr(amx, params[2]);
	cell *vVelocity = MF_GetAmxAddr(amx, params[3]);

	vecStart.x = amx_ctof(vStart[0]);
	vecStart.y = amx_ctof(vStart[1]);
	vecStart.z = amx_ctof(vStart[2]);

	vecVelocity.x = amx_ctof(vVelocity[0]);
	vecVelocity.y = amx_ctof(vVelocity[1]);
	vecVelocity.z = amx_ctof(vVelocity[2]);

	edict_t* pGrenade = Grenade_ShootTimed(INDEXENT2(params[1]), vecStart, vecVelocity, amx_ctof(params[4]));

	if (IsValidPev(pGrenade))
	{
		char *funcname = MF_GetAmxString(amx, params[5], 0, NULL);

		if (funcname)
		{
			int iForward = MF_RegisterSPForwardByName
			(
				amx, 
				funcname, 
				FP_CELL,
				FP_CELL,
				FP_ARRAY,
				FP_ARRAY,
				FP_DONE
			);

			g_Entity.SetAmxxForward(pGrenade, FWD_ENT_EXPLODE, iForward == -1 ? NULL : iForward);
		}

		return ENTINDEX(pGrenade);
	}

	return -1;
}

/**
 * Get player's gun position. Result will set in vecResult.
 *
 * @param iPlayer			Player index.
 * @param vecResult			Calculated gun position.
 * @param flForwardScale	Forward scale value.
 * @param flUpScale			Up scale value.
 * @param flRightScale		Right scale value.
 *
 * native wpnmod_get_gun_position(const iPlayer, Float: vecResult[3], const Float: flForwardScale = 1.0, const Float: flRightScale = 1.0, const Float: flUpScale = 1.0);
*/
AMXX_NATIVE(wpnmod_get_gun_position)
{
	CHECK_ENTITY(params[1])
	edict_t* pPlayer = INDEXENT2(params[1]);

	Vector vecSrc = pPlayer->v.origin + pPlayer->v.view_ofs; 
	MAKE_VECTORS(pPlayer->v.v_angle + pPlayer->v.punchangle);

	Vector vecForward = gpGlobals->v_forward;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;

	vecSrc = vecSrc + vecForward * amx_ctof(params[3]) + vecRight * amx_ctof(params[4]) + vecUp * amx_ctof(params[5]);
	cell* cRet = MF_GetAmxAddr(amx, params[2]);

	cRet[0] = amx_ftoc(vecSrc.x);
	cRet[1] = amx_ftoc(vecSrc.y);
	cRet[2] = amx_ftoc(vecSrc.z);

	return 1;
}

/**
 * Explode and then remove entity.
 *
 * @param iEntity			Entity index.
 * @param bitsDamageType	Damage type (see CLASSIFY defines).
 * @param szCallBack		The forward to call on explode.
 *
 * native wpnmod_explode_entity(const iEntity, const bitsDamageType = 0, const szCallBack[] = "");
*/
AMXX_NATIVE(wpnmod_explode_entity)
{
	CHECK_ENTITY(params[1])

	char *funcname = MF_GetAmxString(amx, params[3], 0, NULL);

	if (funcname)
	{
		int iForward = MF_RegisterSPForwardByName
		(
			amx, 
			funcname, 
			FP_CELL,
			FP_CELL,
			FP_ARRAY,
			FP_ARRAY,
			FP_DONE
		);

		g_Entity.SetAmxxForward(INDEXENT2(params[1]), FWD_ENT_EXPLODE, iForward == -1 ? NULL : iForward);
	}

	Grenade_Explode(INDEXENT2(params[1]), params[2]);
	return 1;
}

/**
 * Draw decal by index or name on trace end.
 *
 * @param iTrace			Trace handler.
 * @param iDecalIndex		Decal index.
 * @param szDecalName		Decal name.
 *
 * native wpnmod_decal_trace(const iTrace, const iDecalIndex = -1, const szDecalName[] = "");
*/
AMXX_NATIVE(wpnmod_decal_trace)
{
	TraceResult *pTrace = reinterpret_cast<TraceResult *>(params[1]);

	int iDecalIndex = params[2];

	if (iDecalIndex == -1)
	{
		iDecalIndex = DECAL_INDEX(MF_GetAmxString(amx, params[3], 0, NULL));
	}

	UTIL_DecalTrace(pTrace, iDecalIndex);
	return 1;
}

/**
 * Detects the texture of an entity from a direction.
 *
 * @param iEntity			Entity index that we want to get the texture.
 * @param vecSrc			The point from where the trace starts.
 * @param vecEnd			The point where the trace ends.
 * @param szTextureName		Buffer to save the texture name.
 * @param iLen				Buffer's length.
 *
 * native wpnmod_trace_texture(const iEntity, const Float: vecSrc[3], const Float: vecEnd[3], szTextureName[], const iLen);
*/
AMXX_NATIVE(wpnmod_trace_texture)
{
	Vector vecSrc;
	Vector vecEnd;

	edict_t *pEntity = NULL;

	if (!params[1])
	{
		pEntity = ENT(0);
	}
	else
	{
		CHECK_ENTITY(params[1])
		pEntity = INDEXENT2(params[1]);
	}

	cell *vSrc = MF_GetAmxAddr(amx, params[2]);
	cell *vEnd = MF_GetAmxAddr(amx, params[3]);

	vecSrc.x = amx_ctof(vSrc[0]);
	vecSrc.y = amx_ctof(vSrc[1]);
	vecSrc.z = amx_ctof(vSrc[2]);

	vecEnd.x = amx_ctof(vEnd[0]);
	vecEnd.y = amx_ctof(vEnd[1]);
	vecEnd.z = amx_ctof(vEnd[2]);

	const char *pTextureName = TRACE_TEXTURE(pEntity, vecSrc, vecEnd);

	if (!pTextureName)
	{
		pTextureName = "";
	}

	return MF_SetAmxString(amx, params[4], pTextureName, params[5]);
}

/**
 * Precache view model and all sound sequences in it.
 *
 * @param szModelPath		Path to model.
 *
 * native wpnmod_precache_model_sequences(const szModelPath[]);
*/
AMXX_NATIVE(wpnmod_precache_model_sequences)
{
	char modelpath[1024];

	build_pathname_r(modelpath, sizeof(modelpath) - 1, "%s", MF_GetAmxString(amx, params[1], 0, NULL));

	FILE *fp = fopen(modelpath, "rb");

	if (fp)
	{
		int iEvent = 0;
		int iNumSeq = 0;
		int iSeqIndex = 0;
		int iNumEvents = 0;
		int iEventIndex = 0;

		fseek(fp, 164, SEEK_SET);
		fread(&iNumSeq, sizeof(int), 1, fp);
		fread(&iSeqIndex, sizeof(int), 1, fp);

		std::string sound;

		for (int k, i = 0; i < iNumSeq; i++)
		{
			fseek(fp, iSeqIndex + 48 + 176 * i, SEEK_SET);
			fread(&iNumEvents, sizeof(int), 1, fp);
			fread(&iEventIndex, sizeof(int), 1, fp);
			fseek(fp, iEventIndex + 176 * i, SEEK_SET);

			for (k = 0; k < iNumEvents; k++)
			{
				fseek(fp, iEventIndex + 4 + 76 * k, SEEK_SET);
				fread(&iEvent, sizeof(int), 1, fp);
				fseek(fp, 4, SEEK_CUR);
				
				if (iEvent == 5004)
				{
					fread(&modelpath, 64, 1, fp);
					sound.assign(modelpath);

					if (sound.size())
					{
						PRECACHE_SOUND((char*)STRING(ALLOC_STRING(sound.c_str())));
					}
				}
			}
		}
	}

	return fclose(fp);
}











namespace DeprecatedNatives
{
	enum e_CBase
	{
		CBase_pPlayer,
		CBase_pNext,
		CBase_rgpPlayerItems,
		CBase_pActiveItem,
		CBase_pLastItem,
		CBase_End
	};

	enum e_Offsets
	{
		Offset_flStartThrow,
		Offset_flReleaseThrow,
		Offset_iChargeReady,
		Offset_iInAttack,
		Offset_iFireState,
		Offset_iFireOnEmpty,
		Offset_flPumpTime,
		Offset_iInSpecialReload,
		Offset_flNextPrimaryAttack,
		Offset_flNextSecondaryAttack,
		Offset_flTimeWeaponIdle,
		Offset_iPrimaryAmmoType,
		Offset_iSecondaryAmmoType,
		Offset_iClip,
		Offset_iInReload,
		Offset_iDefaultAmmo,
		Offset_flNextAttack,
		Offset_iWeaponVolume,
		Offset_iWeaponFlash,
		Offset_iLastHitGroup,
		Offset_iFOV,
		Offset_iuser1,
		Offset_iuser2,
		Offset_iuser3,
		Offset_iuser4,
		Offset_fuser1,
		Offset_fuser2,
		Offset_fuser3,
		Offset_fuser4,
		Offset_End
	};

	int NativesCBaseOffsets[CBase_End] =
	{
		pvData_pPlayer,
		pvData_pNext,
		pvData_rgpPlayerItems,
		pvData_pActiveItem,
		pvData_pLastItem
	};

	int NativesPvDataOffsets[Offset_End] =
	{
		pvData_flStartThrow,
		pvData_flReleaseThrow,
		pvData_chargeReady,
		pvData_fInAttack,
		pvData_fireState,
		pvData_fFireOnEmpty,
		pvData_flPumpTime,
		pvData_fInSpecialReload,
		pvData_flNextPrimaryAttack,
		pvData_flNextSecondaryAttack,
		pvData_flTimeWeaponIdle,
		pvData_iPrimaryAmmoType,
		pvData_iSecondaryAmmoType,
		pvData_iClip,
		pvData_fInReload,
		pvData_iDefaultAmmo,
		pvData_flNextAttack,
		pvData_iWeaponVolume,
		pvData_iWeaponFlash,
		pvData_LastHitGroup,
		pvData_iFOV,
		pvData_ammo_9mm,
		pvData_ammo_357,
		pvData_ammo_bolts,
		pvData_ammo_buckshot,
		pvData_ammo_rockets,
		pvData_ammo_uranium,
		pvData_ammo_hornets,
		pvData_ammo_argrens
	};

	AMXX_NATIVE(wpnmod_set_anim_ext)
	{
		int iPlayer = params[1];

		CHECK_ENTITY(iPlayer)
		SetPrivateString(INDEXENT2(iPlayer), pvData_szAnimExtention, STRING(ALLOC_STRING(MF_GetAmxString(amx, params[2], 0, NULL))));
		return 1;
	}

	AMXX_NATIVE(wpnmod_get_anim_ext)
	{
		int iPlayer = params[1];

		CHECK_ENTITY(iPlayer)

		return MF_SetAmxString(amx, params[2], GetPrivateString(INDEXENT2(iPlayer), pvData_szAnimExtention), params[3]);
	}

	AMXX_NATIVE(wpnmod_set_offset_int)
	{
		int iEntity = params[1];
		int iOffset = params[2];
		int iValue = params[3];

		CHECK_ENTITY(iEntity)
		CHECK_OFFSET(iOffset)

		SetPrivateInt(INDEXENT2(iEntity), NativesPvDataOffsets[iOffset], iValue);
		return 1;
	}

	AMXX_NATIVE(wpnmod_get_offset_int)
	{
		int iEntity = params[1];
		int iOffset = params[2];

		CHECK_ENTITY(iEntity)
		CHECK_OFFSET(iOffset)

		return GetPrivateInt(INDEXENT2(iEntity), NativesPvDataOffsets[iOffset]);
	}

	AMXX_NATIVE(wpnmod_set_offset_float)
	{
		int iEntity = params[1];
		int iOffset = params[2];

		float flValue = amx_ctof(params[3]);

		CHECK_ENTITY(iEntity)
		CHECK_OFFSET(iOffset)

		SetPrivateFloat(INDEXENT2(iEntity), NativesPvDataOffsets[iOffset], flValue);
		return 1;
	}

	AMXX_NATIVE(wpnmod_set_offset_cbase)
	{
		CHECK_ENTITY(params[1])
		CHECK_ENTITY(params[3])
		CHECK_OFFSET_CBASE(params[2])

		SetPrivateCbase(INDEXENT2(params[1]), NativesCBaseOffsets[params[2]], INDEXENT2(params[3]), params[4]);
		return 1;
	}

	AMXX_NATIVE(wpnmod_get_offset_float)
	{
		int iEntity = params[1];
		int iOffset = params[2];

		CHECK_ENTITY(iEntity)
		CHECK_OFFSET(iOffset)

		return amx_ftoc(GetPrivateFloat(INDEXENT2(iEntity), NativesPvDataOffsets[iOffset]));
	}

	AMXX_NATIVE(wpnmod_get_offset_cbase)
	{
		CHECK_ENTITY(params[1])
		CHECK_OFFSET_CBASE(params[2])

		edict_t* pEntity = GetPrivateCbase(INDEXENT2(params[1]), NativesCBaseOffsets[params[2]], params[3]);

		if (IsValidPev(pEntity))
		{
			return ENTINDEX(pEntity);
		}

		return -1;
	}
} //namespace DeprecatedNatives

namespace NewNatives
{
	#define PV_START		PV_INT_iChargeReady
	#define PV_END			PV_SZ_szAnimExtention

	#define PV_START_INT	PV_INT_iChargeReady
	#define PV_END_INT		PV_INT_iuser4

	#define PV_START_FL		PV_FL_flStartThrow
	#define PV_END_FL		PV_FL_fuser4

	#define PV_START_ENT	PV_ENT_pPlayer
	#define PV_END_ENT		PV_ENT_pLastItem

	#define PV_START_SZ		PV_SZ_szAnimExtention
	#define PV_END_SZ		PV_SZ_szAnimExtention

	enum e_pvData
	{
		PV_INT_iChargeReady,
		PV_INT_iInAttack,
		PV_INT_iFireState,
		PV_INT_iFireOnEmpty,
		PV_INT_iInSpecialReload,
		PV_INT_iPrimaryAmmoType,
		PV_INT_iSecondaryAmmoType,
		PV_INT_iClip,
		PV_INT_iInReload,
		PV_INT_iDefaultAmmo,
		PV_INT_iWeaponVolume,
		PV_INT_iWeaponFlash,
		PV_INT_iLastHitGroup,
		PV_INT_iFOV,
		PV_INT_iuser1,
		PV_INT_iuser2,
		PV_INT_iuser3,
		PV_INT_iuser4,

		PV_FL_flStartThrow,
		PV_FL_flReleaseThrow,
		PV_FL_flPumpTime,
		PV_FL_flNextPrimaryAttack,
		PV_FL_flNextSecondaryAttack,
		PV_FL_flTimeWeaponIdle,
		PV_FL_flNextAttack,
		PV_FL_fuser1,
		PV_FL_fuser2,
		PV_FL_fuser3,
		PV_FL_fuser4,

		PV_ENT_pPlayer,
		PV_ENT_pNext,
		PV_ENT_rgpPlayerItems,
		PV_ENT_pActiveItem,
		PV_ENT_pLastItem,

		PV_SZ_szAnimExtention
	};

	int pvDataReference[PV_END + 1] =
	{
		pvData_chargeReady,
		pvData_fInAttack,
		pvData_fireState,
		pvData_fFireOnEmpty,
		pvData_fInSpecialReload,
		pvData_iPrimaryAmmoType,
		pvData_iSecondaryAmmoType,
		pvData_iClip,
		pvData_fInReload,
		pvData_iDefaultAmmo,
		pvData_iWeaponVolume,
		pvData_iWeaponFlash,
		pvData_LastHitGroup,
		pvData_iFOV,
		pvData_ammo_9mm,
		pvData_ammo_357,
		pvData_ammo_bolts,
		pvData_ammo_buckshot,

		pvData_flStartThrow,
		pvData_flReleaseThrow,
		pvData_flPumpTime,
		pvData_flNextPrimaryAttack,
		pvData_flNextSecondaryAttack,
		pvData_flTimeWeaponIdle,
		pvData_flNextAttack,
		pvData_ammo_rockets,
		pvData_ammo_uranium,
		pvData_ammo_hornets,
		pvData_ammo_argrens,

		pvData_pPlayer,
		pvData_pNext,
		pvData_rgpPlayerItems,
		pvData_pActiveItem,
		pvData_pLastItem,

		pvData_szAnimExtention
	};

	/**
	 * Register new weapon in module.
	 *
	 * @param szName		The weapon name.
	 * @param iSlot			SlotID (1...5).
	 * @param iPosition		NumberInSlot (1...5).
	 * @param szAmmo1		Primary ammo type ("9mm", "uranium", "MY_AMMO" etc).
	 * @param iMaxAmmo1		Max amount of primary ammo.
	 * @param szAmmo2		Secondary ammo type.
	 * @param iMaxAmmo2		Max amount of secondary ammo.
	 * @param iMaxClip		Max amount of ammo in weapon's clip.
	 * @param iFlags		Weapon's flags (see defines).
	 * @param iWeight		This value used to determine this weapon's importance in autoselection.
	 * 
	 * @return				The ID of registerd weapon or 0 on failure. (integer)
	 *
	 * native WpnMod_RegisterWeapon(const szName[], const iSlot, const iPosition, const szAmmo1[], const iMaxAmmo1, const szAmmo2[], const iMaxAmmo2, const iMaxClip, const iFlags, const iWeight);
	*/
	AMXX_NATIVE(WpnMod_RegisterWeapon)
	{
		int iId = WEAPON_REGISTER(amx, params);

		if (!iId)
		{
			WPNMOD_LOG("Warning: amxx plugin \"%s\" stopped.\n", GET_AMXX_PLUGIN_POINTER(amx)->name.c_str());
			STOP_AMXX_PLUGIN(amx);
		}

		return iId;
	}

	/**
	 * Register weapon's forward.
	 *
	 * @param iWeaponID		The ID of registered weapon.
	 * @param iForward		Forward type to register.
	 * @param szCallBack	The forward to call.
	 *
	 * native WpnMod_RegisterWeaponForward(const iWeaponID, const e_Forwards: iForward, const szCallBack[]);
	*/
	AMXX_NATIVE(WpnMod_RegisterWeaponForward)
	{
		int iId = params[1];
		int Fwd = params[2];
		const char *funcname = MF_GetAmxString(amx, params[3], 0, NULL);

		return WEAPON_FORWARD_REGISTER(iId, (e_WpnFwds)Fwd, amx, funcname);
	}

	/**
	 * Register new ammobox in module.
	 *
	 * @param szName			The ammobox classname.
	 * 
	 * @return					The ID of registerd ammobox or -1 on failure. (integer)
	 *
	 * native WpnMod_RegisterAmmo(const szClassname[]);
	 */
	AMXX_NATIVE(WpnMod_RegisterAmmo)
	{
		CPlugin* plugin = GET_AMXX_PLUGIN_POINTER(amx);
		const char *szAmmoboxName = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[1], 0, NULL)));

		if (AMMOBOX_GET_ID(szAmmoboxName))
		{
			WPNMOD_LOG("Warning: \"%s\" already registered! (\"%s\")\n", szAmmoboxName, plugin->name.c_str());
			return 0;
		}

		return AMMOBOX_REGISTER(szAmmoboxName);
	}

	/**
	 * Register ammobox's forward.
	 *
	 * @param iAmmoboxID		The ID of registered ammobox.
	 * @param iForward			Forward type to register.
	 * @param szCallBack		The forward to call.
	 *
	 * native WpnMod_RegisterAmmoForward(const iWeaponID, const e_AmmoFwds: iForward, const szCallBack[]);
	 */
	AMXX_NATIVE(WpnMod_RegisterAmmoForward)
	{
		int iId = params[1];
		int Fwd = params[2];
		const char *funcname = MF_GetAmxString(amx, params[3], 0, NULL);

		return AMMOBOX_FORWARD_REGISTER(iId, (e_AmmoFwds)Fwd, amx, funcname);
	}

	/**
	 * Returns any ItemInfo variable for weapon. Use the e_ItemInfo_* enum.
	 *
	 * @param iId			The ID of registered weapon or weapon entity Id.
	 * @param iInfoType		ItemInfo type.
	 *
	 * @return				Weapon's ItemInfo variable.
	 *
	 * native WpnMod_GetWeaponInfo(const iId, const e_ItemInfo: iInfoType, any:...);
	 */
	AMXX_NATIVE(WpnMod_GetWeaponInfo)
	{
		enum e_ItemInfo
		{
			ItemInfo_isCustom = 0,
			ItemInfo_iSlot,
			ItemInfo_iPosition,
			ItemInfo_iMaxAmmo1,
			ItemInfo_iMaxAmmo2,
			ItemInfo_iMaxClip,
			ItemInfo_iId,
			ItemInfo_iFlags,
			ItemInfo_iWeight,
			ItemInfo_szName,
			ItemInfo_szAmmo1,
			ItemInfo_szAmmo2,
			ItemInfo_szTitle,
			ItemInfo_szAuthor,
			ItemInfo_szVersion
		};

		int iId = params[1];
		int iSwitch = params[2];

		edict_t* pItem = NULL;

		if (iSwitch < ItemInfo_isCustom || iSwitch > ItemInfo_szVersion)
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "Undefined e_ItemInfo index: %d", iSwitch);
			return 0;
		}

		if (iId <= 0 || iId > gpGlobals->maxEntities)
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "Invalid entity or weapon id provided (%d).", iId);
			return 0;
		}

		pItem = INDEXENT2(iId);

		if (IsValidPev(pItem) && strstr(STRING(pItem->v.classname), "weapon_"))
		{
			iId = GetPrivateInt(pItem, pvData_iId);
		}

		if (iId <= 0 || iId >= MAX_WEAPONS || !WEAPON_GET_NAME(iId))
		{
			return 0;
		}

		size_t paramnum = params[0] / sizeof(cell);

		if (iSwitch >= ItemInfo_isCustom && iSwitch <= ItemInfo_iWeight && paramnum == 2)
		{
			switch (iSwitch)
			{
				case ItemInfo_isCustom:
					return WEAPON_IS_CUSTOM(iId);

				case ItemInfo_iSlot:
					return WEAPON_GET_SLOT(iId);

				case ItemInfo_iPosition:
					return WEAPON_GET_SLOT_POSITION(iId);

				case ItemInfo_iMaxAmmo1:
					return WEAPON_GET_MAX_AMMO1(iId);

				case ItemInfo_iMaxAmmo2:
					return WEAPON_GET_MAX_AMMO2(iId);

				case ItemInfo_iMaxClip:
					return WEAPON_GET_MAX_CLIP(iId);

				case ItemInfo_iId:
					return iId;

				case ItemInfo_iFlags:
					return WEAPON_GET_FLAGS(iId);

				case ItemInfo_iWeight:
					return WEAPON_GET_WEIGHT(iId);
			}
		}
		else if (iSwitch >= ItemInfo_szName && iSwitch <= ItemInfo_szVersion && paramnum == 4)
		{
			const char* szReturnValue = NULL;

			switch (iSwitch)
			{
				case ItemInfo_szName:
					szReturnValue = WEAPON_GET_NAME(iId);
					break;
				case ItemInfo_szAmmo1:
					szReturnValue = WEAPON_GET_AMMO1(iId);
					break;
				case ItemInfo_szAmmo2:
					szReturnValue = WEAPON_GET_AMMO2(iId);
					break;
				case ItemInfo_szTitle:
					szReturnValue = WeaponInfoArray[iId].title.c_str();
					break;
				case ItemInfo_szAuthor:
					szReturnValue = WeaponInfoArray[iId].author.c_str();
					break;
				case ItemInfo_szVersion:
					szReturnValue = WeaponInfoArray[iId].version.c_str();
					break;
			}	

			if (!szReturnValue)
			{
				szReturnValue = "";
			}

			return MF_SetAmxString(amx, params[3], szReturnValue, params[4]);
		}

		MF_LogError(amx, AMX_ERR_NATIVE, "Unknown e_ItemInfo index or return combination %d", iSwitch);
		return 0;
	}

	/**
	 * Returns any AmmoInfo variable for ammobox. Use the e_AmmoInfo_* enum.
	 *
	 * @param iId			The ID of registered ammobox or ammobox entity Id.
	 * @param iInfoType		e_AmmoInfo_* type.
	 *
	 * @return				Ammobox's AmmoInfo variable.
	 *
	 * native WpnMod_GetAmmoBoxInfo(const iId, const e_AmmoInfo: iInfoType, any:...);
	 */
	AMXX_NATIVE(WpnMod_GetAmmoBoxInfo)
	{
		enum e_AmmoInfo
		{
			AmmoInfo_szName
		};

		int iId = params[1];
		int iSwitch = params[2];

		edict_t* pAmmoBox = NULL;

		if (iSwitch < AmmoInfo_szName || iSwitch > AmmoInfo_szName)
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "Undefined e_AmmoInfo index: %d", iSwitch);
			return 0;
		}

		if (iId <= 0 || iId > gpGlobals->maxEntities)
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "Invalid entity or ammobox id provided (%d).", iId);
			return 0;
		}

		pAmmoBox = INDEXENT2(iId);

		if (IsValidPev(pAmmoBox) && strstr(STRING(pAmmoBox->v.classname), "ammo_"))
		{
			iId = AMMOBOX_GET_ID(STRING(pAmmoBox->v.classname));
		}

		if (!AMMOBOX_GET_NAME(iId))
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "Invalid ammobox id provided (%d).", iId);
			return 0;
		}

		size_t paramnum = params[0] / sizeof(cell);

		if (iSwitch >= AmmoInfo_szName && iSwitch <= AmmoInfo_szName && paramnum == 4)
		{
			const char* szReturnValue = NULL;

			switch (iSwitch)
			{
			case AmmoInfo_szName:
				szReturnValue = AMMOBOX_GET_NAME(iId);
				break;
			}	

			if (!szReturnValue)
			{
				szReturnValue = "";
			}

			return MF_SetAmxString(amx, params[3], szReturnValue, params[4]);
		}

		MF_LogError(amx, AMX_ERR_NATIVE, "Unknown e_AmmoInfo index or return combination %d", iSwitch);
		return 0;
	}

	/**
	 * Gets number of registered weapons.
	 *
	 * @return		Number of registered weapons. (integer)
	 *
	 * native WpnMod_GetWeaponNum();
	*/
	AMXX_NATIVE(WpnMod_GetWeaponNum)
	{
		return MAX_WEAPONS;
	}

	/**
	 * Gets number of registered ammoboxes.
	 *
	 * @return		Number of registered ammoboxes. (integer)
	 *
	 * native WpnMod_GetAmmoBoxNum();
	*/
	AMXX_NATIVE(WpnMod_GetAmmoBoxNum)
	{
		return AMMOBOX_GET_COUNT();
	}

	/**
	 * Spawn an item by name.
	 *
	 * @param szName			Item's name.
	 * @param vecOrigin			Origin were to spawn.
	 * @param vecAngles			Angles.
	 *
	 * @return					Item entity index or -1 on failure. (integer)
	 *
	 * native WpnMod_CreateItem(const szName[], const Float: vecOrigin[3] = {0.0, 0.0, 0.0}, const Float: vecAngles[3] = {0.0, 0.0, 0.0});
	*/
	AMXX_NATIVE(WpnMod_CreateItem)
	{
		char *itemname = MF_GetAmxString(amx, params[1], 0, NULL);

		Vector vecOrigin;
		cell *vOrigin = MF_GetAmxAddr(amx, params[2]);

		vecOrigin.x = amx_ctof(vOrigin[0]);
		vecOrigin.y = amx_ctof(vOrigin[1]);
		vecOrigin.z = amx_ctof(vOrigin[2]);

		Vector vecAngles;
		cell *vAngles = MF_GetAmxAddr(amx, params[3]);

		vecAngles.x = amx_ctof(vAngles[0]);
		vecAngles.y = amx_ctof(vAngles[1]);
		vecAngles.z = amx_ctof(vAngles[2]);

		edict_t* pItem = ENTITY_CREATE_ENT(itemname, vecOrigin, vecAngles);

		if (IsValidPev(pItem))
		{
			return ENTINDEX(pItem);
		}

		return -1;
	}

	/**
	 * Default deploy function.
	 *
	 * @param iItem				Weapon's entity index.
	 * @param szViewModel		Weapon's view  model (V).
	 * @param szWeaponModel		Weapon's player  model (P).
	 * @param iAnim				Sequence number of deploy animation.
	 * @param szAnimExt			Animation extension.
	 *
	 * native WpnMod_DefaultDeploy(const iItem, const szViewModel[], const szWeaponModel[], const iAnim, const szAnimExt[]);
	*/
	AMXX_NATIVE(WpnMod_DefaultDeploy)
	{
		int iEntity = params[1];
		int iAnim = params[4];

		const char *szViewModel = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[2], 0, NULL))); 
		const char *szWeaponModel = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[3], 0, NULL)));
		const char *szAnimExt = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[5], 0, NULL)));

		CHECK_ENTITY(iEntity)

		edict_t* pItem = INDEXENT2(iEntity);
		edict_t* pPlayer = GetPrivateCbase(pItem, pvData_pPlayer);

		if (!IsValidPev(pPlayer))
		{
			return 0;
		}

		if (!Weapon_CanDeploy(pItem->pvPrivateData))
		{
			return 0;
		}

		pPlayer->v.viewmodel = MAKE_STRING(szViewModel);
		pPlayer->v.weaponmodel = MAKE_STRING(szWeaponModel);

		SetPrivateString(pPlayer, pvData_szAnimExtention, szAnimExt);
		SendWeaponAnim(pPlayer, pItem, iAnim);

		SetPrivateFloat(pPlayer, pvData_flNextAttack, 0.5);
		SetPrivateFloat(pItem, pvData_flTimeWeaponIdle, 1.0);

		return 1;
	}

	/**
	 * Default reload function.
	 *
	 * @param iItem				Weapon's entity index.
	 * @param iClipSize			Maximum weapon's clip size.
	 * @param iAnim				Sequence number of reload animation.
	 * @param flDelay			Reload delay time.
	 *
	 * native WpnMod_DefaultReload(const iItem, const iClipSize, const iAnim, const Float: flDelay);
	*/
	AMXX_NATIVE(WpnMod_DefaultReload)
	{
		int iEntity = params[1];
		int iClipSize = params[2];
		int iAnim = params[3];

		float flDelay = amx_ctof(params[4]);

		CHECK_ENTITY(iEntity)

		edict_t* pItem = INDEXENT2(iEntity);
		edict_t* pPlayer = GetPrivateCbase(pItem, pvData_pPlayer);

		if (!IsValidPev(pPlayer))
		{
			return 0;
		}

		int iAmmo = GetAmmoInventory(pPlayer, PrimaryAmmoIndex(pItem));

		if (!iAmmo)
		{
			return 0;
		}

		int j = std::min(iClipSize - GetPrivateInt(pItem, pvData_iClip), iAmmo);

		if (!j)
		{
			return 0;
		}

		SetPrivateInt(pItem, pvData_fInReload, TRUE);
		SetPrivateFloat(pPlayer, pvData_flNextAttack, flDelay);
		SetPrivateFloat(pItem, pvData_flTimeWeaponIdle, flDelay);

		SendWeaponAnim(pPlayer, pItem, iAnim);
		return 1;
	}

	/**
	 * Sets the weapon so that it can play empty sound again.
	 *
	 * @param iItem				Weapon's entity index.
	 *
	 * native WpnMod_ResetEmptySound(const iItem);
	*/
	AMXX_NATIVE(WpnMod_ResetEmptySound)
	{
		int iEntity = params[1];

		CHECK_ENTITY(iEntity)
		SetPrivateInt(INDEXENT2(iEntity), pvData_iPlayEmptySound, TRUE);
		return 1;
	}

	/**
	 * Plays the weapon's empty sound.
	 *
	 * @param iItem				Weapon's entity index.
	 *
	 * native WpnMod_PlayEmptySound(const iItem);
	*/
	AMXX_NATIVE(WpnMod_PlayEmptySound)
	{
		int iEntity = params[1];
		CHECK_ENTITY(iEntity)

		if (GetPrivateInt(INDEXENT2(iEntity), pvData_iPlayEmptySound))
		{
			edict_t* pPlayer = GetPrivateCbase(INDEXENT2(iEntity), pvData_pPlayer);

			if (IsValidPev(pPlayer))
			{
				EMIT_SOUND_DYN2(pPlayer, CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM, 0, PITCH_NORM);
				SetPrivateInt(INDEXENT2(iEntity), pvData_iPlayEmptySound, FALSE);
			
				return 1;
			}
		}

		return 0;
	}

	/**
	* Get player's ammo inventory.
	 *
	 * @param iPlayer		Player id.
	 * @param szAmmoName	Ammo type. ("9mm", "uranium", "MY_AMMO" etc..)
	 *
	 * @return				Amount of given ammo. (integer)
	 *
	 * native WpnMod_GetPlayerAmmo(const iPlayer, const szAmmoName[]);
	*/
	AMXX_NATIVE(WpnMod_GetPlayerAmmo)
	{
		int iPlayer = params[1];

		CHECK_ENTITY(iPlayer)

		int iAmmoIndex = GET_AMMO_INDEX(STRING(ALLOC_STRING(MF_GetAmxString(amx, params[2], 0, NULL))));

		if (iAmmoIndex != -1)
		{
			return GetAmmoInventory(INDEXENT2(iPlayer), iAmmoIndex);
		}

		return -1;
	}

	/**
	* Set player's ammo inventory.
	 *
	 * @param iPlayer		Player id.
	 * @param szAmmoName	Ammo type. ("9mm", "uranium", "MY_AMMO" etc..)
	 * @param iAmount		Ammo amount.
	 *
	 * native WpnMod_SetPlayerAmmo(const iPlayer, const szAmmoName[], const iAmount);
	*/
	AMXX_NATIVE(WpnMod_SetPlayerAmmo)
	{
		int iPlayer = params[1];

		CHECK_ENTITY(iPlayer)

		int iAmmoIndex = GET_AMMO_INDEX(STRING(ALLOC_STRING(MF_GetAmxString(amx, params[2], 0, NULL))));

		if (iAmmoIndex != -1)
		{
			SetAmmoInventory(INDEXENT2(iPlayer), iAmmoIndex, params[3]);
			return 1;
		}

		return 0;
	}

	/**
	 * Plays weapon's animation.
	 *
	 * @param iItem		Weapon's entity.
	 * @param iAnim		Sequence number.
	 *
	 * native WpnMod_SendWeaponAnim(const iItem, const iAnim);
	*/
	AMXX_NATIVE(WpnMod_SendWeaponAnim)
	{
		CHECK_ENTITY(params[1])

		edict_t *pWeapon = INDEXENT2(params[1]);
		edict_t *pPlayer = GetPrivateCbase(pWeapon, pvData_pPlayer);

		if (!IsValidPev(pPlayer))
		{
			return 0;
		}

		SendWeaponAnim(pPlayer, pWeapon, params[2]);
		return 1;
	}

	/**
	 * Set the activity for player based on an event or current state.
	 *
	 * @param iPlayer		Player id.
	 * @param iPlayerAnim	Animation (See PLAYER_ANIM constants).
	 *
	 * native WpnMod_SetPlayerAnim(const iPlayer, const PLAYER_ANIM: iPlayerAnim);
	*/
	AMXX_NATIVE(WpnMod_SetPlayerAnim)
	{
		int iPlayer = params[1];
		int iPlayerAnim = params[2];

		CHECK_ENTITY(iPlayer)
		SET_ANIMATION(INDEXENT2(iPlayer), iPlayerAnim);
		return 1;
	}

	/**
	 * Returns a value from entity's private data. Use the e_pvData_* enum.
	 *
	 * @param iEntity		Entity index.
	 * @param iPvData		pvData type.
	 *
	 * @return				Entity's value from privaate data.
	 *
	 * native WpnMod_GetPrivateData(const iEntity, const e_pvData: iPvData, any:...);
	*/
	AMXX_NATIVE(WpnMod_GetPrivateData)
	{
		int iEntity = params[1];
		int iSwitch = params[2];

		CHECK_ENTITY(iEntity);

		if (iSwitch < PV_START || iSwitch > PV_END)
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "Undefined e_pvData index: %d", iSwitch);
			return 0;
		}

		int iExtra = 0;

		edict_t* pEntity = INDEXENT2(iEntity);
		size_t paramnum = params[0] / sizeof(cell);

		if (iSwitch >= PV_START_ENT && iSwitch <= PV_END_ENT)
		{
			if (paramnum == 3)
			{
				iExtra = *MF_GetAmxAddr(amx, params[3]);
			}
			else
			{
				CHECK_PARAMS_NUM(2);
			}

			edict_t* pResult = GetPrivateCbase(pEntity, pvDataReference[iSwitch], iExtra);

			if (IsValidPev(pResult))
			{
				return ENTINDEX(pResult);
			}

			return -1;
		}
		else if (iSwitch >= PV_START_INT && iSwitch <= PV_END_INT)
		{
			if (paramnum == 3)
			{
				iExtra = *MF_GetAmxAddr(amx, params[3]);
			}
			else
			{
				CHECK_PARAMS_NUM(2);
			}

			return GetPrivateInt(pEntity, pvDataReference[iSwitch], iExtra);
		}
		else if (iSwitch >= PV_START_FL && iSwitch <= PV_END_FL)
		{
			CHECK_PARAMS_NUM(2);
			return amx_ftoc(GetPrivateFloat(pEntity, pvDataReference[iSwitch]));
		}
		else if (iSwitch >= PV_START_SZ && iSwitch <= PV_END_SZ)
		{
			CHECK_PARAMS_NUM(4);

			const char* szReturnValue = NULL;

			switch (iSwitch)
			{
				case PV_SZ_szAnimExtention:
					szReturnValue = GetPrivateString(pEntity, pvData_szAnimExtention);
					break;
			}

			if (!szReturnValue)
			{
				szReturnValue = "";
			}

			return MF_SetAmxString(amx, params[3], szReturnValue, params[4]);
		}

		MF_LogError(amx, AMX_ERR_NATIVE, "Unknown e_pvData index or return combination %d", iSwitch);
		return -1;
	}

	/**
	 * Sets a value to entity's private data. Use the e_pvData_* enum.
	 *
	 * @param iEntity		Entity index.
	 * @param iPvData		pvData type.
	 *
	 * native WpnMod_SetPrivateData(const iEntity, const e_pvData: iPvData, any:...);
	*/
	AMXX_NATIVE(WpnMod_SetPrivateData)
	{
		int iEntity = params[1];
		int iSwitch = params[2];

		CHECK_ENTITY(iEntity);

		if (iSwitch < PV_START || iSwitch > PV_END)
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "Undefined e_pvData index: %d", iSwitch);
			return 0;
		}

		int iExtra = 0;

		edict_t* pEntity = INDEXENT2(iEntity);
		size_t paramnum = params[0] / sizeof(cell);

		if (iSwitch >= PV_START_ENT && iSwitch <= PV_END_ENT)
		{
			if (paramnum == 4)
			{
				iExtra = *MF_GetAmxAddr(amx, params[4]);
			}
			else
			{
				CHECK_PARAMS_NUM(3);
			}

			int iTarget = *MF_GetAmxAddr(amx,params[3]);

			CHECK_ENTITY(iTarget)
			SetPrivateCbase(pEntity, pvDataReference[iSwitch], INDEXENT2(iTarget), iExtra);
			return 1;
			
		}
		else if (iSwitch >= PV_START_INT && iSwitch <= PV_END_INT)
		{
			if (paramnum == 4)
			{
				iExtra = *MF_GetAmxAddr(amx, params[4]);
			}
			else
			{
				CHECK_PARAMS_NUM(3);
			}

			SetPrivateInt(pEntity, pvDataReference[iSwitch], *MF_GetAmxAddr(amx, params[3]), iExtra);
			return 1;
		}
		else if (iSwitch >= PV_START_FL && iSwitch <= PV_END_FL)
		{
			CHECK_PARAMS_NUM(3);
			SetPrivateFloat(pEntity, pvDataReference[iSwitch], amx_ctof(MF_GetAmxAddr(amx,params[3])[0]));
			return 1;
		}
		else if (iSwitch >= PV_START_SZ && iSwitch <= PV_END_SZ)
		{
			CHECK_PARAMS_NUM(3);

			switch (iSwitch)
			{
				case PV_SZ_szAnimExtention:
					SetPrivateString(pEntity, pvData_szAnimExtention, STRING(ALLOC_STRING(MF_GetAmxString(amx, params[3], 0, NULL))));
					return 1;
			}
		}

		MF_LogError(amx, AMX_ERR_NATIVE, "Unknown e_pvData index or return combination %d", iSwitch);
		return -1;
	}

	/**
	* native any: WpnMod_GetEntityField(const iEntity, const szField[], any:...);
	*/
	AMXX_NATIVE(WpnMod_GetEntityField)
	{/*
		int iEntity = params[1];

		CHECK_ENTITY(iEntity);

		TrieData *td = &g_Entitys.m_EntsData[iEntity]->m_trie[MF_GetAmxString(amx, params[2], 0, NULL)];

		// should never, ever happen
		if (td == NULL)
		{
			//LogError(amx, AMX_ERR_NATIVE, "Couldn't KTrie::retrieve(), handle: %d", params[1]);
			return 0;
		}

		//g_Entity.m_EntsData[iEntity]->m_trie["lol"].clear();

		//printf("!!!!!!!!!!!!!!1 %s  %s\n", pKey, (char*)g_Entity.EntityGetField(pEntity, pKey));

		*/
		return 1;
	}

	/**
	* native WpnMod_SetEntityField(const iEntity, const szField[], any:...);
	*/
	AMXX_NATIVE(WpnMod_SetEntityField)
	{/*
		int iEntity = params[1];

		CHECK_ENTITY(iEntity);

		const char* pKey = MF_GetAmxString(amx, params[2], 0, NULL);
		//TrieData *td = &g_Entitys.m_EntsData[iEntity]->m_trie[pKey];

		// should never, ever happen
		if (td == NULL)
		{
			//LogError(amx, AMX_ERR_NATIVE, "Couldn't KTrie::retrieve(), handle: %d", params[1]);
			return 0;
		}

		if (strstr(pKey, "m_i"))
		{
			//*(int*)pValue = *MF_GetAmxAddr(amx, params[3]);
		}
		else if (strstr(pKey, "m_fl"))
		{
			//*(double*)pValue = amx_ctof(MF_GetAmxAddr(amx, params[3])[0]);
		}
		else if (strstr(pKey, "m_sz"))
		{
			int len = 0;
			const char* pData = MF_GetAmxString(amx, params[3], 0, &len);

			pValue = new char[len];
			strcpy((char*)pValue, pData);
		}
		else if (strstr(pKey, "m_a"))
		{

		}
		else
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "Invalid field name provided: \"%s\"", pKey);
			return 0;
		}*/

		return 1;//g_Entitys.EntitySetField(pEntity, pKey, pValue);
	}

	/**
	* Sets entity's think function. Analogue of set_task native.
	*
	* Usage:
	* 	WpnMod_SetThink(iItem, "M249_CompleteReload", 1.52);
	*
	* @param iEntity           Weapon's entity index.
	* @param szCallBack        The forward to call.
	* @param flNextThink       Time until next think.
	*
	* native WpnMod_SetThink(const iEntity, const szCallBack[], const Float: flNextThink = 0.0);
	*/
	AMXX_NATIVE(WpnMod_SetThink)
	{
		CHECK_ENTITY(params[1])

		edict_t *pEdict = INDEXENT2(params[1]);
		char *funcname = MF_GetAmxString(amx, params[2], 0, NULL);

		if (!strlen(funcname))
		{
			Dll_SetThink(pEdict, NULL);
			g_Entity.SetAmxxForward(pEdict, FWD_ENT_THINK, NULL);
			return 1;
		}

		int iForward = -1;

		// Entity is NOT a weapon
		if (!strstr(STRING(pEdict->v.classname), "weapon_"))
		{
			iForward = MF_RegisterSPForwardByName(amx, funcname, FP_CELL, FP_ARRAY, FP_DONE);
		}
		else // Entity is a weapon
		{
			iForward = MF_RegisterSPForwardByName(amx, funcname, FP_CELL, FP_CELL, FP_CELL, FP_CELL, FP_CELL, FP_DONE);
		}

		if (iForward == -1)
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "Function not found (\"%s\").", funcname);
			return 0;
		}

		Dll_SetThink(pEdict, (void*)Global_Think);
		g_Entity.SetAmxxForward(pEdict, FWD_ENT_THINK, iForward);

		float flNextThink = amx_ctof(params[3]);

		if (flNextThink > 0.0)
		{
			pEdict->v.nextthink = gpGlobals->time + flNextThink;
		}

		return 1;
	}

	/**
	* Sets entity's touch calback.
	*
	* @param iEntity            Entity index.
	* @param szCallBack         The forward to call.
	*
	* native WpnMod_SetTouch(const iEntity, const szCallBack[], const szToucher[] = "");
	*/
	AMXX_NATIVE(WpnMod_SetTouch)
	{
		CHECK_ENTITY(params[1])

		edict_t *pEdict = INDEXENT2(params[1]);
		char *funcname = MF_GetAmxString(amx, params[2], 0, NULL);

		if (!strlen(funcname))
		{
			Dll_SetTouch(pEdict, NULL);
			g_Entity.SetAmxxForward(pEdict, FWD_ENT_TOUCH, NULL);
			return 1;
		}

		int iForward = MF_RegisterSPForwardByName(amx, funcname, FP_CELL, FP_CELL, FP_STRING, FP_ARRAY, FP_DONE);

		if (iForward == -1)
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "Function not found (\"%s\").", funcname);
			return 0;
		}

		int paramnum = params[0] / sizeof(cell);

		if (paramnum > 2)
		{
			for (int i = 3; i <= paramnum; i++)
			{
				g_Entity.AddClassnameToTouchFilter(INDEXENT2(params[1]), MF_GetAmxString(amx, params[i], 0, NULL));
			}
		}

		Dll_SetTouch(pEdict, (void*)Global_Touch);
		g_Entity.SetAmxxForward(pEdict, FWD_ENT_TOUCH, iForward);

		return 1;
	}

	/**
	* Fire bullets from player's weapon.
	*
	* @param iPlayer			Player index.
	* @param iAttacker			Attacker index (usualy it equal to previous param).
	* @param iShotsCount		Number of shots.
	* @param vecSpread			Spread.
	* @param flDistance		Max shot distance.
	* @param flDamage			Damage amount.
	* @param bitsDamageType	Damage type.
	* @param iTracerFreq		Tracer frequancy.
	*
	* native WpnMod_FireBullets(const iPlayer, const iAttacker, const iShotsCount, const Float: vecSpread[3], const Float: flDistance, const Float: flDamage, const bitsDamageType, const iTracerFreq);
	*/
	AMXX_NATIVE(WpnMod_FireBullets)
	{
		int iPlayer = params[1];
		int iAttacker = params[2];

		CHECK_ENTITY(iPlayer)
		CHECK_ENTITY(iAttacker)

		Vector vecSpread;
		cell *vSpread = MF_GetAmxAddr(amx, params[4]);

		vecSpread.x = amx_ctof(vSpread[0]);
		vecSpread.y = amx_ctof(vSpread[1]);
		vecSpread.z = amx_ctof(vSpread[2]);

		FireBulletsPlayer
		(
			INDEXENT2(iPlayer),
			INDEXENT2(iAttacker),
			params[3],
			vecSpread,
			amx_ctof(params[5]),
			amx_ctof(params[6]),
			params[7],
			params[8]
		);

		return 1;
	}

	/**
	* Gives a single item to player.
	*
	* Usage:
	*	// Giving a crowbar to player.
	* 	WpnMod_GiveItem(iPlayer, "weapon_crowbar");
	*
	* @param iPlayer          Player index.
	* @return                 Item entity index or -1 on failure. (integer)
	*
	* native WpnMod_GiveItem(const iPlayer, const szItemName[]);
	*/
	AMXX_NATIVE(WpnMod_GiveItem)
	{
		CHECK_ENTITY(params[1])

		edict_t* pItem = GiveNamedItem(INDEXENT2(params[1]), STRING(ALLOC_STRING(MF_GetAmxString(amx, params[2], 0, NULL))));

		if (IsValidPev(pItem))
		{
			return ENTINDEX(pItem);
		}

		return -1;
	}

	/**
	* Gives multiple items to player.
	*
	* Usage:
	*	// giving one crowbar and two automatic rifles to player.
	* 	WpnMod_GiveEquipment(iPlayer, "weapon_crowbar", "weapon_9mmAR:2");
	*
	* @param iPlayer            Player index.
	*
	* native WpnMod_GiveEquip(const iPlayer, const any: ...);
	*/
	AMXX_NATIVE(WpnMod_GiveEquip)
	{
		CHECK_ENTITY(params[1])

		edict_t* pItem = NULL;
		edict_t* pPlayer = INDEXENT2(params[1]);

		std::string itemRead, itemName;
		size_t paramnum = params[0] / sizeof(cell);

		for (int itemCount, pos, i = 2; i <= (int)paramnum; i++)
		{
			itemRead.assign(STRING(ALLOC_STRING(MF_GetAmxString(amx, params[i], 0, NULL))));

			pos = itemRead.find(':');

			itemName = itemRead.substr(0, pos);
			itemCount = (pos == -1) ? 1 : atoi(itemRead.substr(pos + 1).c_str());
			//itemName.trim();
			itemName.erase(itemName.find_last_not_of(" \n\r\t") + 1);

			for (int j = 0; j < itemCount; j++)
			{
				GiveNamedItem(pPlayer, itemName.c_str());
			}
		}

		return 1;
	}

	// native WpnMod_VelocityByAim(const iPlayer, const flVelocity, Float: vecRetValue[3]);
	AMXX_NATIVE(WpnMod_VelocityByAim)
	{
		CHECK_ENTITY(params[1])

		edict_t* pPlayer = INDEXENT2(params[1]);
		Vector vecRet = pPlayer->v.punchangle + pPlayer->v.velocity + gpGlobals->v_forward * amx_ctof(params[2]);

		cell* cRet = MF_GetAmxAddr(amx, params[3]);

		cRet[0] = amx_ftoc(vecRet.x);
		cRet[1] = amx_ftoc(vecRet.y);
		cRet[2] = amx_ftoc(vecRet.z);

		return 1;
	}

	/**
	* Change respawn time of all weapons.
	*
	* @param fRespawnTime			Time in seconds before all custom weapons respawn.
	*
	* @return				1 if the time is greater or equals to 0.0, 0 otherwise
	*
	* native WpnMod_SetRespawnTime(const Float: fRespawnTime = 20.0);
	*/
	AMXX_NATIVE(WpnMod_SetRespawnTime)
	{
		float fWeaponRespawnTime = amx_ctof(params[1]);

		if (fWeaponRespawnTime >= 0.0)
		{
			g_fNextWeaponRespawnTime = fWeaponRespawnTime;

			return 1;
		}

		return 0;
	}

	/**
	* Checks if a weapon is custom or not, by providing it's id.
	*
	* @param iWeaponId		Weapon identifier.
	*
	* @return				true if the weapon is custom, false otherwise.
	*
	* native bool: WpnMod_IsCustomWeapon(const iWeaponId);
	*/
	AMXX_NATIVE(WpnMod_IsCustomWeapon)
	{
		cell iWeaponId = params[1];

		return WEAPON_IS_CUSTOM(iWeaponId);
	}

	/**
	* Checks if a weapon is default or not, by providing it's id.
	*
	* @param iWeaponId		Weapon identifier.
	*
	* @return				true if the weapon is default, false otherwise.
	*
	* native bool: WpnMod_IsDefaultWeapon(const iWeaponId);
	*/
	AMXX_NATIVE(WpnMod_IsDefaultWeapon)
	{
		cell iWeaponId = params[1];

		return WEAPON_IS_DEFAULT(iWeaponId);
	}
} // namespace NewNatives

AMX_NATIVE_INFO Natives[] = 
{
	// Actual natives.
	{ "WpnMod_RegisterWeapon",				NewNatives::WpnMod_RegisterWeapon				},
	{ "WpnMod_RegisterWeaponForward",		NewNatives::WpnMod_RegisterWeaponForward		},
	{ "WpnMod_RegisterAmmoBox",				NewNatives::WpnMod_RegisterAmmo					},
	{ "WpnMod_RegisterAmmoBoxForward",		NewNatives::WpnMod_RegisterAmmoForward			},
	{ "WpnMod_GetWeaponInfo",				NewNatives::WpnMod_GetWeaponInfo				},
	{ "WpnMod_GetAmmoBoxInfo",				NewNatives::WpnMod_GetAmmoBoxInfo				},
	{ "WpnMod_GetWeaponNum",				NewNatives::WpnMod_GetWeaponNum					},
	{ "WpnMod_GetAmmoBoxNum",				NewNatives::WpnMod_GetAmmoBoxNum				},
	{ "WpnMod_CreateItem",					NewNatives::WpnMod_CreateItem					},
	{ "WpnMod_DefaultDeploy",				NewNatives::WpnMod_DefaultDeploy				},
	{ "WpnMod_DefaultReload",				NewNatives::WpnMod_DefaultReload				},
	{ "WpnMod_ResetEmptySound",				NewNatives::WpnMod_ResetEmptySound				},
	{ "WpnMod_PlayEmptySound",				NewNatives::WpnMod_PlayEmptySound				},
	{ "WpnMod_GetPlayerAmmo",				NewNatives::WpnMod_GetPlayerAmmo				},
	{ "WpnMod_SetPlayerAmmo",				NewNatives::WpnMod_SetPlayerAmmo				},
	{ "WpnMod_SendWeaponAnim",				NewNatives::WpnMod_SendWeaponAnim				},
	{ "WpnMod_SetPlayerAnim",				NewNatives::WpnMod_SetPlayerAnim				},
	{ "WpnMod_GetPrivateData",				NewNatives::WpnMod_GetPrivateData				},
	{ "WpnMod_SetPrivateData",				NewNatives::WpnMod_SetPrivateData				},
	{ "WpnMod_GetEntityField",				NewNatives::WpnMod_GetEntityField				},
	{ "WpnMod_SetEntityField",				NewNatives::WpnMod_SetEntityField				},
	{ "WpnMod_SetThink",					NewNatives::WpnMod_SetThink						},
	{ "WpnMod_SetTouch",					NewNatives::WpnMod_SetTouch						},
	{ "WpnMod_FireBullets",					NewNatives::WpnMod_FireBullets					},
	{ "WpnMod_GiveItem",					NewNatives::WpnMod_GiveItem						},
	{ "WpnMod_GiveEquip",					NewNatives::WpnMod_GiveEquip					},
	{ "WpnMod_SetRespawnTime",				NewNatives::WpnMod_SetRespawnTime               },
	{ "WpnMod_IsCustomWeapon",				NewNatives::WpnMod_IsCustomWeapon               },
	{ "WpnMod_IsDefaultWeapon",				NewNatives::WpnMod_IsDefaultWeapon              },
	// { "WpnMod_VelocityByAim",				NewNatives::WpnMod_VelocityByAim				},
	// { "wpnmod_precache_model_sequences",	wpnmod_precache_model_sequences					},

	// Deprecated native names. Using them for backward compatibility with old plugins.
	{ "wpnmod_register_weapon",				NewNatives::WpnMod_RegisterWeapon				},
	{ "wpnmod_register_weapon_forward",		NewNatives::WpnMod_RegisterWeaponForward		},
	{ "wpnmod_register_ammobox",			NewNatives::WpnMod_RegisterAmmo					},
	{ "wpnmod_register_ammobox_forward",	NewNatives::WpnMod_RegisterAmmoForward			},
	{ "wpnmod_get_weapon_info",				NewNatives::WpnMod_GetWeaponInfo				},
	{ "wpnmod_get_ammobox_info",			NewNatives::WpnMod_GetAmmoBoxInfo				},
	{ "wpnmod_get_weapon_count",			NewNatives::WpnMod_GetWeaponNum					},
	{ "wpnmod_get_ammobox_count",			NewNatives::WpnMod_GetAmmoBoxNum				},
	{ "wpnmod_create_item",					NewNatives::WpnMod_CreateItem					},
	{ "wpnmod_default_deploy",				NewNatives::WpnMod_DefaultDeploy				},
	{ "wpnmod_default_reload",				NewNatives::WpnMod_DefaultReload				},
	{ "wpnmod_reset_empty_sound",			NewNatives::WpnMod_ResetEmptySound				},
	{ "wpnmod_play_empty_sound",			NewNatives::WpnMod_PlayEmptySound				},
	{ "wpnmod_get_player_ammo",				NewNatives::WpnMod_GetPlayerAmmo				},
	{ "wpnmod_set_player_ammo",				NewNatives::WpnMod_SetPlayerAmmo				},
	{ "wpnmod_send_weapon_anim",			NewNatives::WpnMod_SendWeaponAnim				},
	{ "wpnmod_set_player_anim",				NewNatives::WpnMod_SetPlayerAnim				},
	{ "wpnmod_set_think",					NewNatives::WpnMod_SetThink						},
	{ "wpnmod_set_touch",					NewNatives::WpnMod_SetTouch						},
	{ "wpnmod_fire_bullets",				NewNatives::WpnMod_FireBullets					},
	{ "wpnmod_set_anim_ext",				DeprecatedNatives::wpnmod_set_anim_ext			},
	{ "wpnmod_get_anim_ext",				DeprecatedNatives::wpnmod_get_anim_ext			},
	{ "wpnmod_set_offset_int",				DeprecatedNatives::wpnmod_set_offset_int		},
	{ "wpnmod_set_offset_float",			DeprecatedNatives::wpnmod_set_offset_float		},
	{ "wpnmod_set_offset_cbase",			DeprecatedNatives::wpnmod_set_offset_cbase		},
	{ "wpnmod_get_offset_int",				DeprecatedNatives::wpnmod_get_offset_int		},
	{ "wpnmod_get_offset_float",			DeprecatedNatives::wpnmod_get_offset_float		},
	{ "wpnmod_get_offset_cbase",			DeprecatedNatives::wpnmod_get_offset_cbase		},

	// WIP
	{ "wpnmod_fire_contact_grenade", wpnmod_fire_contact_grenade},
	{ "wpnmod_fire_timed_grenade", wpnmod_fire_timed_grenade},
	{ "wpnmod_radius_damage", wpnmod_radius_damage},
	{ "wpnmod_radius_damage2", wpnmod_radius_damage2},
	{ "wpnmod_clear_multi_damage", wpnmod_clear_multi_damage},
	{ "wpnmod_apply_multi_damage", wpnmod_apply_multi_damage},
	{ "wpnmod_eject_brass", wpnmod_eject_brass},
	{ "wpnmod_get_damage_decal", wpnmod_get_damage_decal},
	{ "wpnmod_get_gun_position", wpnmod_get_gun_position},
	{ "wpnmod_explode_entity", wpnmod_explode_entity},
	{ "wpnmod_decal_trace", wpnmod_decal_trace},
	{ "wpnmod_trace_texture", wpnmod_trace_texture},

	{ NULL, NULL }
};
