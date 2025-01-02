#include <assert.h>
#include <inttypes.h>
#include "arsorch.h"
#include "routeorch.h"
#include "logger.h"
#include "swssnet.h"
#include "crmorch.h"
#include <array>
#include <algorithm>

extern sai_object_id_t gVirtualRouterId;
extern sai_object_id_t gSwitchId;

extern sai_ars_profile_api_t*       sai_ars_profile_api;
extern sai_ars_api_t*               sai_ars_api;
extern sai_port_api_t*              sai_port_api;
extern sai_next_hop_group_api_t*    sai_next_hop_group_api;
extern sai_route_api_t*             sai_route_api;
extern sai_switch_api_t*            sai_switch_api;

extern RouteOrch *gRouteOrch;
extern CrmOrch *gCrmOrch;
extern PortsOrch *gPortsOrch;

typedef struct _ars_sai_attr_t
{
    std::string attr_name;
    bool create_implemented;
    bool set_implemented;
    bool get_implemented;
    struct _ars_sai_attr_t(std::string name): attr_name(name), create_implemented(false), set_implemented(false), get_implemented(false) {}
} ars_sai_attr_t;

typedef std::map<uint32_t, ars_sai_attr_t> ars_sai_attr_lookup_t;
typedef std::map<std::string, ars_sai_attr_lookup_t> ars_sai_feature_data_t;
typedef std::map<uint32_t, ars_sai_feature_data_t> ars_sai_feature_lookup_t;

ars_sai_attr_lookup_t ars_profile_attrs = {
    {SAI_ARS_PROFILE_ATTR_ALGO, {"SAI_ARS_PROFILE_ATTR_ALGO"}},
    {SAI_ARS_PROFILE_ATTR_SAMPLING_INTERVAL, {"SAI_ARS_PROFILE_ATTR_SAMPLING_INTERVAL"}},
    {SAI_ARS_PROFILE_ATTR_ARS_RANDOM_SEED, {"SAI_ARS_PROFILE_ATTR_ARS_RANDOM_SEED"}},
    {SAI_ARS_PROFILE_ATTR_ECMP_ARS_MAX_GROUPS, {"SAI_ARS_PROFILE_ATTR_ECMP_ARS_MAX_GROUPS"}},
    {SAI_ARS_PROFILE_ATTR_ECMP_ARS_MAX_MEMBERS_PER_GROUP, {"SAI_ARS_PROFILE_ATTR_ECMP_ARS_MAX_MEMBERS_PER_GROUP"}},
    {SAI_ARS_PROFILE_ATTR_LAG_ARS_MAX_GROUPS, {"SAI_ARS_PROFILE_ATTR_LAG_ARS_MAX_GROUPS"}},
    {SAI_ARS_PROFILE_ATTR_LAG_ARS_MAX_MEMBERS_PER_GROUP, {"SAI_ARS_PROFILE_ATTR_LAG_ARS_MAX_MEMBERS_PER_GROUP"}},
    {SAI_ARS_PROFILE_ATTR_PORT_LOAD_PAST, {"SAI_ARS_PROFILE_ATTR_PORT_LOAD_PAST"}},
    {SAI_ARS_PROFILE_ATTR_PORT_LOAD_PAST_WEIGHT, {"SAI_ARS_PROFILE_ATTR_PORT_LOAD_PAST_WEIGHT"}},
    {SAI_ARS_PROFILE_ATTR_PORT_LOAD_FUTURE, {"SAI_ARS_PROFILE_ATTR_PORT_LOAD_FUTURE"}},
    {SAI_ARS_PROFILE_ATTR_PORT_LOAD_FUTURE_WEIGHT, {"SAI_ARS_PROFILE_ATTR_PORT_LOAD_FUTURE_WEIGHT"}},
    {SAI_ARS_PROFILE_ATTR_PORT_LOAD_CURRENT, {"SAI_ARS_PROFILE_ATTR_PORT_LOAD_CURRENT"}},
    {SAI_ARS_PROFILE_ATTR_PORT_LOAD_EXPONENT, {"SAI_ARS_PROFILE_ATTR_PORT_LOAD_EXPONENT"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BANDS, {"SAI_ARS_PROFILE_ATTR_QUANT_BANDS"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_0_MIN_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_0_MIN_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_0_MAX_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_0_MAX_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_1_MIN_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_1_MIN_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_1_MAX_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_1_MAX_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_2_MIN_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_2_MIN_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_2_MAX_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_2_MAX_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_3_MIN_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_3_MIN_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_3_MAX_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_3_MAX_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_4_MIN_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_4_MIN_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_4_MAX_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_4_MAX_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_5_MIN_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_5_MIN_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_5_MAX_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_5_MAX_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_6_MIN_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_6_MIN_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_6_MAX_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_6_MAX_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_7_MIN_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_7_MIN_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_7_MAX_THRESHOLD, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_7_MAX_THRESHOLD"}},
    {SAI_ARS_PROFILE_ATTR_ENABLE_IPV4, {"SAI_ARS_PROFILE_ATTR_ENABLE_IPV4"}},
    {SAI_ARS_PROFILE_ATTR_ENABLE_IPV6, {"SAI_ARS_PROFILE_ATTR_ENABLE_IPV6"}},
    {SAI_ARS_PROFILE_ATTR_LOAD_PAST_MIN_VAL, {"SAI_ARS_PROFILE_ATTR_LOAD_PAST_MIN_VAL"}},
    {SAI_ARS_PROFILE_ATTR_LOAD_PAST_MAX_VAL, {"SAI_ARS_PROFILE_ATTR_LOAD_PAST_MAX_VAL"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_MIN_THRESHOLD_LIST_LOAD_PAST, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_MIN_THRESHOLD_LIST_LOAD_PAST"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_MAX_THRESHOLD_LIST_LOAD_PAST, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_MAX_THRESHOLD_LIST_LOAD_PAST"}},
    {SAI_ARS_PROFILE_ATTR_LOAD_FUTURE_MIN_VAL, {"SAI_ARS_PROFILE_ATTR_LOAD_FUTURE_MIN_VAL"}},
    {SAI_ARS_PROFILE_ATTR_LOAD_FUTURE_MAX_VAL, {"SAI_ARS_PROFILE_ATTR_LOAD_FUTURE_MAX_VAL"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_MIN_THRESHOLD_LIST_LOAD_FUTURE, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_MIN_THRESHOLD_LIST_LOAD_FUTURE"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_MAX_THRESHOLD_LIST_LOAD_FUTURE, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_MAX_THRESHOLD_LIST_LOAD_FUTURE"}},
    {SAI_ARS_PROFILE_ATTR_LOAD_CURRENT_MIN_VAL, {"SAI_ARS_PROFILE_ATTR_LOAD_CURRENT_MIN_VAL"}},
    {SAI_ARS_PROFILE_ATTR_LOAD_CURRENT_MAX_VAL, {"SAI_ARS_PROFILE_ATTR_LOAD_CURRENT_MAX_VAL"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_MIN_THRESHOLD_LIST_LOAD_CURRENT, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_MIN_THRESHOLD_LIST_LOAD_CURRENT"}},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_MAX_THRESHOLD_LIST_LOAD_CURRENT, {"SAI_ARS_PROFILE_ATTR_QUANT_BAND_MAX_THRESHOLD_LIST_LOAD_CURRENT"}},
    {SAI_ARS_PROFILE_ATTR_MAX_FLOWS, {"SAI_ARS_PROFILE_ATTR_MAX_FLOWS"}}
};
ars_sai_attr_lookup_t ars_sai_attrs = {
    {SAI_ARS_ATTR_MODE, {"SAI_ARS_ATTR_MODE"}},
    {SAI_ARS_ATTR_IDLE_TIME, {"SAI_ARS_ATTR_IDLE_TIME"}},
    {SAI_ARS_ATTR_MAX_FLOWS, {"SAI_ARS_ATTR_MAX_FLOWS"}},
    {SAI_ARS_ATTR_MON_ENABLE, {"SAI_ARS_ATTR_MON_ENABLE"}},
    {SAI_ARS_ATTR_SAMPLEPACKET_ENABLE, {"SAI_ARS_ATTR_SAMPLEPACKET_ENABLE"}},
    {SAI_ARS_ATTR_MAX_ALT_MEMEBERS_PER_GROUP, {"SAI_ARS_ATTR_MAX_ALT_MEMEBERS_PER_GROUP"}},
    {SAI_ARS_ATTR_MAX_PRIMARY_MEMEBERS_PER_GROUP, {"SAI_ARS_ATTR_MAX_PRIMARY_MEMEBERS_PER_GROUP"}},
    {SAI_ARS_ATTR_PRIMARY_PATH_QUALITY_THRESHOLD, {"SAI_ARS_ATTR_PRIMARY_PATH_QUALITY_THRESHOLD"}},
    {SAI_ARS_ATTR_ALTERNATE_PATH_COST, {"SAI_ARS_ATTR_ALTERNATE_PATH_COST"}},
    {SAI_ARS_ATTR_ALTERNATE_PATH_BIAS, {"SAI_ARS_ATTR_ALTERNATE_PATH_BIAS"}}
};
ars_sai_attr_lookup_t ars_port_attrs = {
    {SAI_PORT_ATTR_ARS_ENABLE, {"SAI_PORT_ATTR_ARS_ENABLE"}},
    {SAI_PORT_ATTR_ARS_PORT_LOAD_SCALING_FACTOR, {"SAI_PORT_ATTR_ARS_PORT_LOAD_SCALING_FACTOR"},
    {SAI_PORT_ATTR_ARS_ALTERNATE_PATH, {"SAI_PORT_ATTR_ARS_ALTERNATE_PATH"}}
};

ars_sai_attr_lookup_t ars_nhg_attrs = {
    {SAI_NEXT_HOP_GROUP_ATTR_ARS_OBJECT_ID, {"SAI_NEXT_HOP_GROUP_ATTR_ARS_OBJECT_ID"}},
    {SAI_NEXT_HOP_GROUP_ATTR_ARS_PACKET_DROPS, {"SAI_NEXT_HOP_GROUP_ATTR_ARS_PACKET_DROPS"}},
    {SAI_NEXT_HOP_GROUP_ATTR_ARS_NEXT_HOP_REASSIGNMENTS, {"SAI_NEXT_HOP_GROUP_ATTR_ARS_NEXT_HOP_REASSIGNMENTS"}},
    {SAI_NEXT_HOP_GROUP_ATTR_ARS_PORT_REASSIGNMENTS, {"SAI_NEXT_HOP_GROUP_ATTR_ARS_PORT_REASSIGNMENTS"}}
};

ars_sai_attr_lookup_t ars_switch_attrs = {
    {SAI_SWITCH_ATTR_ARS_PROFILE, {"SAI_SWITCH_ATTR_ARS_PROFILE"}}
};

ars_sai_attr_lookup_t ars_lag_attrs = {
    {SAI_LAG_ATTR_ARS_OBJECT_ID, {"SAI_LAG_ATTR_ARS_OBJECT_ID"}},
    {SAI_LAG_ATTR_ARS_PACKET_DROPS, {"SAI_LAG_ATTR_ARS_PACKET_DROPS"}},
    {SAI_LAG_ATTR_ARS_PORT_REASSIGNMENTS, {"SAI_LAG_ATTR_ARS_PORT_REASSIGNMENTS"}}
};

ars_sai_feature_data_t ars_feature_switch_data =
    {{"SAI_OBJECT_TYPE_SWITCH",ars_switch_attrs}};

ars_sai_feature_data_t ars_feature_profile_data =
    {{"SAI_OBJECT_TYPE_ARS_PROFILE",ars_profile_attrs}};

ars_sai_feature_data_t ars_feature_ars_data =
    {{"SAI_OBJECT_TYPE_ARS",ars_sai_attrs}};

ars_sai_feature_data_t ars_feature_port_data =
    {{"SAI_OBJECT_TYPE_PORT",ars_port_attrs}};

ars_sai_feature_data_t ars_feature_nhg_data =
    {{"SAI_OBJECT_TYPE_NEXT_HOP_GROUP",ars_nhg_attrs}};

ars_sai_feature_data_t ars_feature_lag_data =
    {{"SAI_OBJECT_TYPE_LAG",ars_lag_attrs}};

ars_sai_feature_lookup_t ars_features =
{
    {SAI_OBJECT_TYPE_SWITCH, ars_feature_switch_data},
    {SAI_OBJECT_TYPE_ARS_PROFILE, ars_feature_profile_data},
    {SAI_OBJECT_TYPE_ARS, ars_feature_ars_data},
    {SAI_OBJECT_TYPE_PORT, ars_feature_port_data},
    {SAI_OBJECT_TYPE_NEXT_HOP_GROUP, ars_feature_nhg_data},
    {SAI_OBJECT_TYPE_LAG, ars_feature_lag_data}
};

map<string, ArsProfileEntry> ArsOrch::m_arsProfiles;

#define ARS_FIELD_NAME_MAX_FLOWS    "max_flows"
#define ARS_FIELD_NAME_ALGORITHM "algorithm"
#define ARS_FIELD_NAME_SAMPLE_INTERVAL "sample_interval"
#define ARS_FIELD_NAME_PAST_LOAD_MIN_VALUE    "past_load_min_value"
#define ARS_FIELD_NAME_PAST_LOAD_MAX_VALUE    "past_load_max_value"
#define ARS_FIELD_NAME_PAST_LOAD_WEIGHT       "past_load_weight"
#define ARS_FIELD_NAME_FUTURE_LOAD_MIN_VALUE  "future_load_min_value"
#define ARS_FIELD_NAME_FUTURE_LOAD_MAX_VALUE  "future_load_max_value"
#define ARS_FIELD_NAME_FUTURE_LOAD_WEIGHT     "future_load_weight"
#define ARS_FIELD_NAME_CURRENT_LOAD_MIN_VALUE "current_load_min_value"
#define ARS_FIELD_NAME_CURRENT_LOAD_MAX_VALUE "current_load_max_value"
#define ARS_FIELD_NAME_MIN_VALUE    "min_value"
#define ARS_FIELD_NAME_MAX_VALUE    "max_value"
#define ARS_FIELD_NAME_WEIGHT   "weight"
#define ARS_FIELD_NAME_FUTURE_LOAD "future_load"
#define ARS_FIELD_NAME_CURRENT_LOAD "current_load"
#define ARS_FIELD_NAME_INDEX "index"
#define ARS_FIELD_NAME_IPV4_ENABLE  "ipv4_enable"
#define ARS_FIELD_NAME_IPV6_ENABLE "ipv6_enable"

#define ARS_FIELD_NAME_PROFILE_NAME "profile_name"
#define ARS_FIELD_NAME_ARS_NAME "ars_name"
#define ARS_FIELD_NAME_ASSIGN_MODE "assign_mode"
#define ARS_FIELD_NAME_PER_FLOWLET "per_flowlet_quality"
#define ARS_FIELD_NAME_PER_PACKET "per_packet"
#define ARS_FIELD_NAME_IDLE_TIME "flowlet_idle_time"
#define ARS_FIELD_NAME_QUALITY_THRESHOLD "quality_threshold"
#define ARS_FIELD_NAME_PRIMARY_PATH_THRESHOLD "primary_path_threshold"
#define ARS_FIELD_NAME_ALT_PATH_COST "alternative_path_cost"
#define ARS_FIELD_NAME_ALT_PATH_MEMBERS "alternative_path_members"

ArsOrch::ArsOrch(DBConnector *db, DBConnector *appDb, DBConnector *stateDb, vector<table_name_with_pri_t> &tableNames) :
        Orch(db, tableNames),
        m_vrfOrch(vrfOrch)
{
    SWSS_LOG_ENTER();

    initCapabilities();

    if (m_isArsSupported)
    {
        gPortsOrch->attach(this);
    }
}

void ArsOrch::update(SubjectType type, void *cntx)
{
    SWSS_LOG_ENTER();
    assert(cntx);

    if (m_arsProfiles.empty())
    {
        SWSS_LOG_INFO("ARS not enabled - no action on interface state change");
        return;
    }

    switch(type) {
        case SUBJECT_TYPE_PORT_OPER_STATE_CHANGE:
        {
            PortOperStateUpdate *update = reinterpret_cast<PortOperStateUpdate *>(cntx);

            bool is_found = (m_arsEnabledInterfaces.find(update->port.m_alias) != m_arsEnabledInterfaces.end());
            SWSS_LOG_INFO("Interface %s %senabled for ARS - %s ARS",
                    update->port.m_alias.c_str(),
                    is_found ? "" : "not ",
                    update->operStatus == SAI_PORT_OPER_STATUS_UP ? "enable" : "disable");
            if (is_found)
            {
                updateArsEnabledInterface(update->port, update->operStatus == SAI_PORT_OPER_STATUS_UP);
            }

            /* check if this is lag or member of lag */
            if (port->m_lag_id)
            {
                Port lag;
                if (gPortsOrch->getPort(port->m_lag_id, lag))
                {
                    auto lag_object = m_arsLags.find(lag.m_alias);
                    sai_attribute_t attr;
                    attr.id = SAI_LAG_ATTR_ARS_OBJECT_ID;
                    bool enabled = false, do_action = false;

                    /* check if ars object id needs updating or should  be removed */
                    if (isSetImplemented(SAI_OBJECT_TYPE_LAG, SAI_LAG_ATTR_ARS_OBJECT_ID))
                    {
                        if (isGetImplemented(SAI_OBJECT_TYPE_LAG, SAI_LAG_ATTR_ARS_OBJECT_ID))
                        {

                            sai_status_t status = sai_lag_api->get_lag_attribute(port.m_lag_id, 1, &attr);
                            if (status != SAI_STATUS_SUCCESS)
                            {
                                SWSS_LOG_ERROR("Failed to get ars id of lag: %s", port.m_alias.c_str());
                            }
                        }

                        if (lag_object != m_arsLags.end())
                        {
                            if (attr.value.u32 != lag_object->second->m_sai_ars_id)
                            {
                                attr.value.u32 = lag_object->second->m_sai_ars_id;
                                do_action = true;
                            }
                        }
                        else if (enabled)
                        {
                            if (attr.value.u32 != SAI_NULL_OBJECT_ID)
                            {
                                attr.value.u32 = SAI_NULL_OBJECT_ID;
                                do_action = true;
                            }
                        }
                        if (do_action)
                        {
                            sai_status_t status = sai_lag_api->set_lag_attribute(port.m_lag_id, &attr);
                            if (status != SAI_STATUS_SUCCESS)
                            {
                                SWSS_LOG_ERROR("Failed to set pvid %u to lag: %s", attr.value.u32, port.m_alias.c_str());
                            }
                        }
                    }
                    /* check if this is lag member */
                    if (port->m_lag_member_id)
                    {
                        if ((lag_object != m_arsLags.end()) && isSetImplemented(SAI_OBJECT_TYPE_PORT, SAI_PORT_ATTR_ARS_ENABLE))
                        {
                            bool enable = (lag_object.second->altPathPorts.find(port->m_alias)) ? true : false;
                            gPortsOrch->setPortArsAltPath(port->m_alias, enable);
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}


bool ArsOrch::bake()
{
    SWSS_LOG_ENTER();

    if (!m_isArsSupported)
    {
        SWSS_LOG_NOTICE("ARS not supported - no action");
        return true;
    }

    SWSS_LOG_NOTICE("Warm reboot: placeholder");

    return Orch::bake();
}

bool ArsOrch::isLagArs(const std::string if_name, sai_object_id_t * ars_object_id, std::set<string> &altPathMembers)
{
    SWSS_LOG_ENTER();

    auto lag_object = m_arsLags.find(if_name);
    if (lag_object == m_arsLags.end())
    {
        SWSS_LOG_INFO("LAG %s is not enabled for ARS", if_name.c_str());
        return false;
    }

    SWSS_LOG_WARN("LAG %s is enabled for ARS", if_name.c_str());
    *ars_object_id = lag_object->second->m_sai_ars_id;
    altPathMembers = lag_object->second->altPathPorts;
    return true;
}

bool ArsOrch::isRouteArs(sai_object_id_t vrf_id, const IpPrefix &ipPrefix, sai_object_id_t * ars_object_id, std::set<IpAddress> &altPathMembers)
{
    SWSS_LOG_ENTER();

    std::string vrf_name = m_vrfOrch->getVRFname(vrf_id);
    if (vrf_name.empty())
    {
        vrf_name = "default";
    }

    auto nhg_entry = m_arsNexthopGroupPrefixes.find(vrf_id);
    ArsObjectEntry prefix_entry;
    if (nhg_entry != m_arsNexthopGroupPrefixes.end())
    {
        prefix_entry = vrf_entry.find(ipPrefix);
    }

    if ((nhg_entry == m_arsNexthopGroupPrefixes.end()) || (prefix_entry == nhg_entry.end()))
    {
        SWSS_LOG_INFO("Vrf %s Route %s is not enabled for ARS", vrf_name.c_str(), ipPrefix.to_string().c_str());
        return false;
    }

    SWSS_LOG_NOTICE("Vrf %s Route %s is enabled for ARS", vrf_name.c_str(), ipPrefix.to_string().c_str());
    *ars_object_id = prefix_entry->second->m_sai_ars_id;
    altPathMembers = prefix_entry->second->altPathNexthops;
    return true;
}

bool ArsOrch::createArsProfile(ArsProfileEntry &profile, vector<sai_attribute_t> ars_attrs)
{
    SWSS_LOG_ENTER();

    sai_attribute_t ars_attr;
    sai_status_t    status = SAI_STATUS_NOT_SUPPORTED;
    vector<sai_attribute_t> supported_ars_attrs;

    /* go over set of attr and set only supported attributes  */
    for (auto a : ars_attrs)
    {
        if (isCreateImplemented(SAI_OBJECT_TYPE_ARS_PROFILE, a.id))
        {
            supported_ars_attrs.push_back(a);
            SWSS_LOG_NBOTICE("ARS profile %s (oid %" PRIx64 "). Setting Attr %d value %lu",
                              profile->profile_name.c_str(), object->ars_object_id, a.id, a.value.u32);
        }
        else
        {
            if (a.id == SAI_ARS_PROFILE_ATTR_ENABLE_IPV4 || a.id == SAI_ARS_PROFILE_ATTR_ENABLE_IPV6)
            {
                SWSS_LOG_WARN("Setting Attr %d is not supported. Failed to set ARS profile %s (oid %" PRIx64 ") value %s",
                               a.id, profile->profile_name.c_str(), a.value.booldata ? "true" : "false");
            }
            else
            {
                SWSS_LOG_WARN("Setting Attr %d is not supported. Failed to set ARS profile %s (oid %" PRIx64 ") value %lu",
                               a.id, profile->profile_name.c_str(), a.value.u32);
            }
            continue;
        }
    }

    if (supported_ars_attrs.empty())
    {
        SWSS_LOG_WARN("No supported attributes found for ARS profile %s", profile->profile_name.c_str());
        return false;
    }

    status = sai_ars_api->create_ars_profile(&profile->m_sai_ars_id,
                                     gSwitchId,
                                     (uint32_t)supported_ars_attrs.size(),
                                     supported_ars_attrs.data());
    if (status != SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_ERROR("Failed to create ars profile %s: %d", profile->profile_name.c_str(), status);
        task_process_status handle_status = handleSaiSetStatus(SAI_API_ARS_PROFILE, status);
        if (handle_status != task_success)
        {
            return parseHandleSaiStatusFailure(handle_status);
        }
    }

    return true;
}

bool ArsOrch::setArsProfile(ArsProfileEntry &profile, vector<sai_attribute_t> ars_attrs)
{
    SWSS_LOG_ENTER();

    sai_attribute_t ars_attr;
    sai_status_t    status = SAI_STATUS_NOT_SUPPORTED;

    /* go over set of attr and set only supported attributes  */
    for (auto a : ars_attrs)
    {
        if (!isSetImplemented(SAI_OBJECT_TYPE_ARS_PROFILE, a.id))
        {
            if (a.id == SAI_ARS_PROFILE_ATTR_ENABLE_IPV4 || a.id == SAI_ARS_PROFILE_ATTR_ENABLE_IPV6)
            {
                SWSS_LOG_WARN("Setting Attr %d is not supported. Failed to set ARS profile %s (oid %" PRIx64 ") value %s",
                               a.id, profile->profile_name.c_str(), a.value.booldata ? "true" : "false");
            }
            else
            {
                SWSS_LOG_WARN("Setting Attr %d is not supported. Failed to set ARS profile %s (oid %" PRIx64 ") value %lu",
                               a.id, profile->profile_name.c_str(), profile->m_sai_ars_id, a.value.u32);
            }
            continue;
        }

        status = sai_ars_api->set_ars_profile_attribute(profile->m_sai_ars_id, &a);
        if (status != SAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR("Failed to set ars profile %s (oid %" PRIx64 ") attr %d: %d",
                            object->name.c_str(), object->ars_object_id, a.id, status);
            task_process_status handle_status = handleSaiSetStatus(SAI_API_ARS, status);
            if (handle_status != task_success)
            {
                return parseHandleSaiStatusFailure(handle_status);
            }
        }
    }

    return true;
}


bool ArsOrch::createArsObject(ArsObjectEntry &object, vector<sai_attribute_t> ars_attrs)
{
    SWSS_LOG_ENTER();

    sai_attribute_t ars_attr;
    sai_status_t    status = SAI_STATUS_NOT_SUPPORTED;
    vector<sai_attribute_t> supported_ars_attrs;

    /* go over set of attr and set only supported attributes  */
    for (auto a : ars_attrs)
    {
        if (isCreateImplemented(SAI_OBJECT_TYPE_ARS, a.id))
        {
            supported_ars_attrs.push_back(a);
            SWSS_LOG_NBOTICE("ARS %s. Setting Attr %d value %lu",
                              object->name.c_str(), a.id, a.value.u32);
        }
        else
        {
            SWSS_LOG_WARN("Setting Attr %d is not supported. Failed to set ARS %s value %lu",
                        a.id, object->name.c_str(), a.value.u32);
            continue;
        }
    }

    if (supported_ars_attrs.empty())
    {
        SWSS_LOG_WARN("No supported attributes found for ARS %s", object->name.c_str());
        return false;
    }

    status = sai_ars_api->create_ars(&object->ars_object_id,
                                     gSwitchId,
                                     (uint32_t)supported_ars_attrs.size(),
                                     supported_ars_attrs.data());
    if (status != SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_ERROR("Failed to create ars %s: %d", object->name.c_str(), status);
        task_process_status handle_status = handleSaiSetStatus(SAI_API_ARS, status);
        if (handle_status != task_success)
        {
            return parseHandleSaiStatusFailure(handle_status);
        }
    }

    return true;
}

bool ArsOrch::setArsObject(ArsObjectEntry &object, vector<sai_attribute_t> ars_attrs)
{
    SWSS_LOG_ENTER();

    sai_attribute_t ars_attr;
    sai_status_t    status = SAI_STATUS_NOT_SUPPORTED;

    /* go over set of attr and set only supported attributes  */
    for (auto a : ars_attrs)
    {
        if (!isSetImplemented(SAI_OBJECT_TYPE_ARS, a.id))
        {
            SWSS_LOG_WARN("Setting Attr %d is not supported. Failed to set ARS %s (oid %" PRIx64 ") value %lu",
                        a.id, object->name.c_str(), object->ars_object_id, a.value.u32);
            continue;
        }

        status = sai_ars_api->set_ars_attribute(object->ars_object_id, &a);
        if (status != SAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR("Failed to set ars %s (oid %" PRIx64 ") attr %d: %d",
                            object->name.c_str(), object->ars_object_id, a.id, status);
            task_process_status handle_status = handleSaiSetStatus(SAI_API_ARS, status);
            if (handle_status != task_success)
            {
                return parseHandleSaiStatusFailure(handle_status);
            }
        }
    }

    return true;
}

bool ArsOrch::updateArsEnabledInterface(const Port &port, const bool is_enable)
{
    SWSS_LOG_ENTER();

    if (!gPortsOrch->setPortArsEnable(port, is_enable))
    {
        SWSS_LOG_ERROR("Failed to set ars enable for port %s", port.m_alias.c_str());
        return false;
    }

    if (is_enable && !gPortsOrch->setPortArsLoadScaling(port))
    {
        SWSS_LOG_ERROR("Failed to set ars load scaling factor for port %s", port.m_alias.c_str());
        return false;
    }

    SWSS_LOG_NOTICE("Interface %s - %sable ARS on interface",
                    port.m_alias.c_str(),
                    is_enable ? "en" : "dis");

    return true;
}

void ArsOrch::doTaskArsProfile(Consumer &consumer)
{
    SWSS_LOG_ENTER();

    auto it = consumer.m_toSync.begin();
    while (it != consumer.m_toSync.end())
    {
        KeyOpFieldsValuesTuple t = it->second;
        string key = kfvKey(t);
        size_t found = key.find(consumer.getConsumerTable()->getTableNameSeparator().c_str());
         string table_id = key.substr(0, found);
        string ars_profile_name = key.substr(found + 1);
        string op = kfvOp(t);
    
        uint32_t max_flows = 0, sampling_interval = 0, past_load_min_val = 0, past_load_max_val = 0, past_load_weight = 0, band_min_value = 0, band_max_value = 0;
        uint32_t future_load_min_val = 0, future_load_max_val = 0, future_load_weight = 0, current_load_min_val = 0, current_load_max_val = 0;
        bool ipv6_enable = false, ipv4_enable = false, band_min_value_found, band_max_value_found;
        map<uint32_t, ArsQuantizationBandValues> quantization_bands;
        ArsAlgorithm algo = ARS_ALGORITHM_EWMA;

        bool is_new_entry = false;
        sai_attribute_t         ars_attr;
        vector<sai_attribute_t> ars_attrs;

        SWSS_LOG_NOTICE("OP: %s, Profile: %s", op.c_str(), ars_profile_name.c_str());

        auto arsProfile_entry = m_arsProfiles.find(ars_profile_name);

        if (op == SET_COMMAND)
        {
            if (arsProfile_entry != m_arsProfiles.end())
            {
                max_flows = arsProfile_entry->second.max_flows;
                algo = arsProfile_entry->second.algorithm;
                sampling_interval = arsProfile_entry->second.sampling_interval;
                ipv4_enable = arsProfile_entry->second.ipv4_enable;
                ipv6_enable = arsProfile_entry->second.ipv6_enable;
                past_load_min_val = arsProfile_entry->second.past_load_min_val;
                past_load_max_val = arsProfile_entry->second.past_load_max_val;
                past_load_weight = arsProfile_entry->second.past_load_weight;
                future_load_min_val = arsProfile_entry->second.future_load_min_val;
                future_load_max_val = arsProfile_entry->second.future_load_max_val;
                future_load_weight = arsProfile_entry->second.future_load_weight;
                current_load_min_val = arsProfile_entry->second.current_load_min_val;
                current_load_max_val = arsProfile_entry->second.current_load_max_val;
            }

            for (auto i : kfvFieldsValues(t))
            {
                if (fvField(i) == ARS_FIELD_NAME_MAX_FLOWS)
                {
                    max_flows = stoi(fvValue(i));
                    ars_attr.id = SAI_ARS_PROFILE_ATTR_MAX_FLOWS;
                    ars_attr.value.u32 = max_flows;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(i) == ARS_FIELD_NAME_ALGORITHM)
                {
                    algo = (ArsAlgorithm)stoi(fvValue(i));
                    ars_attr.id = SAI_ARS_PROFILE_ATTR_ALGO;
                    ars_attr.value.u32 = algo;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(i) == ARS_FIELD_NAME_SAMPLE_INTERVAL)
                {
                    sampling_interval = stoi(fvValue(i));
                    ars_attr.id = SAI_ARS_PROFILE_ATTR_SAMPLING_INTERVAL;
                    ars_attr.value.u32 = sampling_interval;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(i) == ARS_FIELD_NAME_IPV4_ENABLE)
                {
                    ipv4_enable = (fvValue(i) == "true");
                    ars_attr.id = SAI_ARS_PROFILE_ATTR_ENABLE_IPV4;
                    ars_attr.value.booldata = ipv4_enable;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(i) == ARS_FIELD_NAME_IPV6_ENABLE)
                {
                    ipv6_enable = (fvValue(i) == "true");
                    ars_attr.id = SAI_ARS_PROFILE_ATTR_ENABLE_IPV6;
                    ars_attr.value.booldata = ipv6_enable;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(k) == ARS_FIELD_NAME_PAST_LOAD_MIN_VALUE)
                {
                    past_load_min_val = stoi(fvValue(k));
                    ars_attr.id = SAI_ARS_PROFILE_ATTR_LOAD_PAST_MIN_VAL;
                    ars_attr.value.u32 = past_load_min_val;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(k) == ARS_FIELD_NAME_PAST_LOAD_MAX_VALUE)
                {
                    past_load_max_val = stoi(fvValue(k));
                    ars_attr.id = SAI_ARS_PROFILE_ATTR_LOAD_PAST_MAX_VAL;
                    ars_attr.value.u32 = past_load_max_val;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(k) == ARS_FIELD_NAME_PAST_LOAD_WEIGHT)
                {
                    past_load_weight = stoi(fvValue(k));
                    ars_attr.id = SAI_ARS_PROFILE_ATTR_PORT_LOAD_PAST_WEIGHT;
                    ars_attr.value.u32 = past_load_weight;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(k) == ARS_FIELD_NAME_FUTURE_LOAD_MIN_VALUE)
                {
                    future_load_min_val = stoi(fvValue(k));
                    ars_attr.id = SAI_ARS_PROFILE_ATTR_LOAD_FUTURE_MIN_VAL;
                    ars_attr.value.u32 = future_load_min_val;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(k) == ARS_FIELD_NAME_FUTURE_LOAD_MAX_VALUE)
                {
                    future_load_max_val = stoi(fvValue(k));
                    ars_attr.id = SAI_ARS_PROFILE_ATTR_LOAD_FUTURE_MAX_VAL;
                    ars_attr.value.u32 = future_load_max_val;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(k) == ARS_FIELD_NAME_FUTURE_LOAD_WEIGHT)
                {
                    future_load_weight = stoi(fvValue(k));
                    ars_attr.id = SAI_ARS_PROFILE_ATTR_PORT_LOAD_FUTURE_WEIGHT;
                    ars_attr.value.u32 = future_load_weight;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(k) == ARS_FIELD_NAME_CURRENT_LOAD_MIN_VALUE)
                {
                    current_load_min_val = stoi(fvValue(k));
                    ars_attr.id = SAI_ARS_PROFILE_ATTR_LOAD_CURRENT_MIN_VAL;
                    ars_attr.value.u32 = current_load_min_val;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(k) == ARS_FIELD_NAME_CURRENT_LOAD_MAX_VALUE)
                {
                    current_load_max_val = stoi(fvValue(k));
                    ars_attr.id = SAI_ARS_PROFILE_ATTR_LOAD_CURRENT_MAX_VAL;
                    ars_attr.value.u32 = current_load_max_val;
                    ars_attrs.push_back(ars_attr);
                }
                else
                {
                    SWSS_LOG_WARN("Received unsupported field %s", fvField(i).c_str());
                    break;
                }
            }

            if (arsProfile_entry == arsProfile_entry.end())
            {
                ArsProfileEntry arsProfileEntry;
                arsProfileEntry.profile_name = ars_profile_name;
                arsProfile_entry[ars_profile_name] = arsObjectEntry;
                arsProfile_entry = m_arsProfiles.find(ars_profile_name);
                SWSS_LOG_NOTICE("Added new ARS profile %s", ars_profile_name.c_str());
                is_new_entry = true;
            }

            arsProfile_entry->second.max_flows = max_flows;
            arsProfile_entry->second.algorithm = algo;
            arsProfile_entry->second.sampling_interval = sampling_interval;
            arsProfile_entry->second.ipv4_enable = ipv4_enable;
            arsProfile_entry->second.ipv6_enable = ipv6_enable;
            arsProfile_entry->second.past_load_min_val = past_load_min_val;
            arsProfile_entry->second.past_load_max_val = past_load_max_val;
            arsProfile_entry->second.past_load_weight = past_load_weight;
            arsProfile_entry->second.future_load_min_val = future_load_min_val;
            arsProfile_entry->second.future_load_max_val = future_load_max_val;
            arsProfile_entry->second.future_load_weight = future_load_weight;
            arsProfile_entry->second.current_load_min_val = current_load_min_val;
            arsProfile_entry->second.current_load_max_val = current_load_max_val;

            if (is_new_entry)
            {
                createArsProfile(arsProfile_entry->second, ars_attrs);
            }
            else
            {
                setArsProfile(arsProfile_entry->second, ars_attrs);
            }
        }
        else if (op == DEL_COMMAND)
        {
            if (arsProfile_entry == m_arsProfiles.end())
            {
                SWSS_LOG_NOTICE("Received delete call for non-existent entry %s", ars_profile_name.c_str());
            }
            else 
            {
                /* Check if there are no child objects associated prior to deleting */
                if (arsProfile_entry->second.ars_objects.empty() &&
					m_arsEnabledInterfaces.empty())
                {
                    SWSS_LOG_INFO("Received delete call for valid entry with no further dependencies, deleting %s",
                            ars_object_namars_profile_namee.c_str());
                }
                else
                {
                    SWSS_LOG_NOTICE("Child Prefix/Member entries are still associated with this ARS profile %s", 
                            ars_profile_name.c_str());
                    continue;
                }

                m_arsProfiles.erase(arsProfile_entry);

            }
        }

        it = consumer.m_toSync.erase(it);
    }
}

void ArsOrch::doTaskArsQuantizationBands(Consumer &consumer)
{
    SWSS_LOG_ENTER();

    auto it = consumer.m_toSync.begin();
    while (it != consumer.m_toSync.end())
    {
        KeyOpFieldsValuesTuple t = it->second;
        string key = kfvKey(t);
        size_t found = key.find(consumer.getConsumerTable()->getTableNameSeparator().c_str());
        string key_without_table = key.substr(found + 1);
        size_t found = key_without_table.find(consumer.getConsumerTable()->getTableNameSeparator().c_str());
        string ars_profile_name = key_without_table.substr(0, found);
        uint8_t band_index = stoi(key_without_table.substr(found + 1));
        string op = kfvOp(t);
        map<uint32_t, ArsQuantizationBandValues> quantization_bands;
        uint32_t band_min_value = 0, band_max_value = 0;

        sai_attribute_t         ars_attr;
        vector<sai_attribute_t> ars_attrs;

        SWSS_LOG_NOTICE("OP: %s, Profile: %s Index: %d", op.c_str(), ars_profile_name.c_str(), index);

        auto arsProfile_entry = m_arsProfiles.find(ars_profile_name);
        if (arsProfile_entry == m_arsProfiles.end())
        {
            SWSS_LOG_WARN("Received ARS quantization bands configuration for non-existent profile %s", ars_profile_name.c_str());
            continue;
        }

        if (op == SET_COMMAND)
        {
            quantization_bands = arsProfile_entry->second.quantization_bands;

            for (auto i : kfvFieldsValues(t))
            {
                else if (fvField(i) == ARS_FIELD_NAME_QUANTIZATION_BANDS_LIST)
                {
                    size_t found = key.find(consumer.getConsumerTable()->getTableNameSeparator().c_str());
                    string table_id = key.substr(0, found);
                    uint32_t band_index = stoi(key.substr(found + 1));

                    for (auto j : fvFieldValues(i))
                    {
                        if (fvField(k) == ARS_FIELD_NAME_MIN_VALUE)
                        {
                            band_min_value = stoi(fvValue(k));
                            ars_attr.id = SAI_ARS_PROFILE_ATTR_QUANT_BAND_0_MIN_THRESHOLD + (band_index * 2);
                            ars_attr.value.u32 = band_min_value;
                            ars_attrs.push_back(ars_attr);
                        }
                        else if (fvField(k) == ARS_FIELD_NAME_MAX_VALUE)
                        {
                            band_max_value = stoi(fvValue(k));
                            ars_attr.id = SAI_ARS_PROFILE_ATTR_QUANT_BAND_0_MAX_THRESHOLD + (band_index * 2);
                            ars_attr.value.u32 = band_max_value;
                            ars_attrs.push_back(ars_attr);
                        }
                        else
                        {
                            SWSS_LOG_WARN("Received unsupported quantization bands field %s", fvField(k).c_str());
                            break;
                        }
                    }
                    if (!ars_attrs.emtpy())
                    {
                        quantization_bands.insert({band_index, {band_min_value, band_max_value}});
                    }
                }
                else
                {
                    SWSS_LOG_WARN("Received unsupported field %s", fvField(i).c_str());
                    break;
                }
            }

            arsProfile_entry->second.quantization_bands = quantization_bands;

            setArsProfile(arsProfile_entry->second, ars_attrs);
        }
        else if (op == DEL_COMMAND)
        {
            arsProfile_entry->second.quantization_bands.erase(band_index);
        }

        it = consumer.m_toSync.erase(it);
    }
}


void ArsOrch::doTaskArsObject(Consumer &consumer)
{
    SWSS_LOG_ENTER();

    auto it = consumer.m_toSync.begin();
    while (it != consumer.m_toSync.end())
    {
        KeyOpFieldsValuesTuple t = it->second;
        string key = kfvKey(t);
        size_t found = key.find(consumer.getConsumerTable()->getTableNameSeparator().c_str());
        string table_id = key.substr(0, found);
        key = key.substr(found + 1);
        found = key.find(consumer.getConsumerTable()->getTableNameSeparator().c_str());
        string ars_profile_name = key.substr(0, found);
        string ars_object_name = key.substr(found + 1);
        string op = kfvOp(t);
        uint32_t max_flows = 0, flowlet_idle_time = 0, primary_path_threshold = 0, alt_path_cost = 0;
        ArsAssignMode assign_mode = PER_FLOWLET_QUALITY;
        bool is_new_entry = false;
        sai_attribute_t         ars_attr;
        vector<sai_attribute_t> ars_attrs;

        SWSS_LOG_NOTICE("OP: %s, Profile: %s, Object: %s", op.c_str(), ars_profile_name.c_str(), ars_object_name.c_str());

        auto arsProfile_entry = m_arsProfiles.find(ars_profile_name);

        if (arsProfile_entry == m_arsProfiles.end())
        {
            SWSS_LOG_WARN("Received ARS object %s for non-existent profile %s", ars_object_name.c_str(), ars_profile_name.c_str());
            it = consumer.m_toSync.erase(it);
            continue;
        }

        auto arsObject_entry = arsProfile_entry->ars_objects.find(ars_object_name);

        if (op == SET_COMMAND)
        {
            if (arsObject_entry != arsProfile_entry->ars_objects.end())
            {
                max_flows = arsObject_entry->second.max_flows;
                flowlet_idle_time = arsObject_entry->second.flowlet_idle_time;
                assign_mode = arsObject_entry->second.assign_mode;
                primary_path_threshold = arsObject_entry->second.path_quality.primary_threshold;
                alt_path_cost = arsObject_entry->second.path_quality.alt_threshold;
            }

            for (auto i : kfvFieldsValues(t))
            {
                if (fvField(i) == ARS_FIELD_NAME_MAX_FLOWS)
                {
                    max_flows = stoi(fvValue(i));
                    ars_attr.id = SAI_ARS_ATTR_MAX_FLOWS;
                    ars_attr.value.u32 = max_flows;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(i) == ARS_FIELD_NAME_IDLE_TIME)
                {
                    flowlet_idle_time = stoi(fvValue(i));
                    ars_attr.id = SAI_ARS_ATTR_IDLE_TIME;
                    ars_attr.value.u32 = flowlet_idle_time;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(i) == ARS_FIELD_NAME_ASSIGN_MODE)
                {
                    ars_attr.id = SAI_ARS_ATTR_MODE;
                    ars_attr.value.u32 = SAI_ARS_MODE_PER_PACKET_QUALITY;
                    if (fvValue(i) == ARS_FIELD_NAME_PER_PACKET)
                    {
                        assign_mode = PER_PACKET;
                        ars_attr.value.u32 = SAI_ARS_MODE_PER_PACKET_QUALITY;
                    }
                    else if (fvValue(i) != ARS_FIELD_NAME_PER_FLOWLET)
                    {
                        SWSS_LOG_WARN("Received unsupported assign_mode %s, defaulted to per_flowlet_quality",
                                        fvValue(i).c_str());
                        break;
                    }
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(i) == ARS_FIELD_NAME_PRIMARY_PATH_THRESHOLD)
                {
                    primary_path_threshold = stoi(fvValue(i));
                    ars_attr.id = SAI_ARS_ATTR_PRIMARY_PATH_QUALITY_THRESHOLD;
                    ars_attr.value.u32 = primary_path_threshold;
                    ars_attrs.push_back(ars_attr);
                }
                else if (fvField(i) == ARS_FIELD_NAME_ALT_PATH_COST)
                {
                    alt_path_cost = stoi(fvValue(i));
                    ars_attr.id = SAI_ARS_ATTR_ALTERNATE_PATH_COST;
                    ars_attr.value.u32 = alt_path_cost;
                    ars_attrs.push_back(ars_attr);
                }
                else
                {
                    SWSS_LOG_WARN("Received unsupported field %s", fvField(i).c_str());
                    break;
                }
            }

            if (max_flows == 0 || (assign_mode == PER_FLOWLET_QUALITY && flowlet_idle_time == 0))
            {
                SWSS_LOG_WARN("Received invalid max_flows %d or flowlet_idle_time %d for key %s", max_flows, flowlet_idle_time, kfvKey(t).c_str());
                break;
            }

            if (arsObject_entry == arsProfile_entry->ars_objects.end())
            {
                ArsObjectEntry arsObjectEntry;
                arsObjectEntry.name = ars_object_name;
                arsProfile_entry->ars_objects[ars_object_name] = arsObjectEntry;
                arsObject_entry = m_arsProfiles->ars_objects.find(ars_object_name);
                SWSS_LOG_NOTICE("Added new ARS entry %s", ars_object_name.c_str());
                is_new_entry = true;
            }

            SWSS_LOG_NOTICE("ARS object is updated with max_flows %d, flowlet_idle_time %d, assign_mode %d primary_path_threshold %d, alt_path_cost %d",
                        ars_profile_name.c_str(), max_flows, flowlet_idle_time, assign_mode, primary_path_threshold, alt_path_cost);
            arsObject_entry->second.assign_mode = assign_mode;                
            arsObject_entry->second.max_flows = max_flows;
            if (flowlet_idle_time)
            {
                arsObject_entry->second.flowlet_idle_time = flowlet_idle_time;
            }
            if (primary_path_threshold)
            {
                arsObject_entry->second.path_quality.primary_threshold = primary_path_threshold;
            }
            if (alt_path_cost)
            {
                arsObject_entry->second.path_quality.alt_threshold = alt_path_cost;
            }
            FieldValueTuple name("profile_name", ars_profile_name);
            FieldValueTuple assign("assign_mode", std::to_string(assign_mode));
            FieldValueTuple flows("max_flows", std::to_string(max_flows));
            FieldValueTuple idle("flowlet_idle_time", std::to_string(flowlet_idle_time));
            vector<FieldValueTuple> fvVector;
            fvVector.push_back(name);
            fvVector.push_back(assign);
            fvVector.push_back(match);
            fvVector.push_back(flows);
            fvVector.push_back(idle);
            m_arsProfileStateTable.set(ars_profile_name, fvVector);

            if (is_new_entry)
            {
                createArsObject(arsObject_entry->second, ars_attrs);
            }
            else
            {
                setArsObject(arsObject_entry->second, ars_attrs);
            }
        }
        else if (op == DEL_COMMAND)
        {
            if (arsObject_entry == arsProfile_entry->ars_objects.end())
            {
                SWSS_LOG_NOTICE("Received delete call for non-existent entry %s in %s", ars_object_name.c_str(), ars_profile_name.c_str());
            }
            else 
            {
                /* Check if there are no child objects associated prior to deleting */
                if (arsProfile_entry->second.ars_objects.prefixes.empty() && 
                    arsProfile_entry->second.ars_objects.altPathInterfaces.empty() &&
                    arsProfile_entry->second.ars_objects.altPathNexthops.empty())
                {
                    SWSS_LOG_INFO("Received delete call for valid entry with no further dependencies, deleting %s",
                            ars_object_name.c_str());
                }
                else
                {
                    SWSS_LOG_NOTICE("Child Prefix/Member entries are still associated with this ARS object %s", 
                            ars_object_name.c_str());
                    continue;
                }

                arsProfile_entry->second.ars_objects.erase(arsObject_entry);
                m_arsProfileStateTable.del(arsObject_entry);

            }
        }

        it = consumer.m_toSync.erase(it);
    }
}

bool ArsOrch::doTaskArsInterfaces(Consumer &consumer)
{
    SWSS_LOG_ENTER();

    auto arsProfile_entry = m_arsProfiles.find(ars_profile_name);

    if (m_arsProfiles.empty())
    {
        SWSS_LOG_WARN("No ARS profiles exist");
        return false;
    }

    auto it = consumer.m_toSync.begin();
    while (it != consumer.m_toSync.end())
    {
        KeyOpFieldsValuesTuple t = it->second;
        string key = kfvKey(t);
        size_t found = key.find(consumer.getConsumerTable()->getTableNameSeparator().c_str());
        string table_id = key.substr(0, found);
        string if_name = key.substr(found + 1);
        string op = kfvOp(t);
        bool is_new_entry = false;
        sai_attribute_t         ars_attr;
        vector<sai_attribute_t> ars_attrs;
        Port p;

        SWSS_LOG_NOTICE("ARS Path Op %s Interface %s", op.c_str(), if_name.c_str());

        if (op == SET_COMMAND)
        {
            if (m_arsEnabledInterfaces.find(if_name) != m_arsEnabledInterfaces.end()) 
            {
                SWSS_LOG_WARN("Tried to add already added interface %s - skipped", if_name.c_str());
                it = consumer.m_toSync.erase(it);
                continue;
            }
        
            m_arsEnabledInterfaces.insert(if_name);
            SWSS_LOG_NOTICE("Added new interface %s to ARS entry %s",
                                if_name.c_str(), profile_name.c_str());
        }
        else if (op == DEL_COMMAND)
        {
            if (m_arsEnabledInterfaces.find(if_name) == m_arsEnabledInterfaces.end())
            {
                SWSS_LOG_INFO("Received delete call for non-existent interface %s", if_name.c_str());
                it = consumer.m_toSync.erase(it);
                continue;
            }
            else
            {
                m_arsEnabledInterfaces.erase(if_name);
                SWSS_LOG_INFO("Removed interface %s", if_name.c_str());
            }
        }

        if (!m_arsProfiles.empty())
        {
            if (!gPortsOrch->getPort(if_name, p) || p.m_oper_status != SAI_PORT_OPER_STATUS_UP)
            {
                SWSS_LOG_WARN("Tried to add/remove non-existent/down interface %s - skipped", if_name.c_str());
            }
            else
            {
                sai_status_t status = sai_lag_api->create_lag(&lag_id, gSwitchId, static_cast<uint32_t>(lag_attrs.size()), lag_attrs.data());

                if (status != SAI_STATUS_SUCCESS)
                {
                    SWSS_LOG_ERROR("Failed to create LAG %s lid:%" PRIx64, lag_alias.c_str(), lag_id);
                    task_process_status handle_status = handleSaiCreateStatus(SAI_API_LAG, status);
                    if (handle_status != task_success)
                    {
                        return parseHandleSaiStatusFailure(handle_status);
                    }
                }
            }
        }

        string ifnames = "";
        for (auto ifname : m_arsEnabledInterfaces)
        {
            ifnames += ifname + " ";
        }
        FieldValueTuple tmp("ifname", ifnames);
        vector<FieldValueTuple> fvVector;
        fvVector.push_back(tmp);
        m_arsIfStateTable.set("ArsEnabledInterfaces", fvVector);

        it = consumer.m_toSync.erase(it);
    }

    return true;
}

bool ArsOrch::doTaskArsNexthopGroup(const KeyOpFieldsValuesTuple & t)
{
    SWSS_LOG_ENTER();

    auto it = consumer.m_toSync.begin();
    while (it != consumer.m_toSync.end())
    {
        KeyOpFieldsValuesTuple t = it->second;
        string key = kfvKey(t);
        size_t found = key.find(consumer.getConsumerTable()->getTableNameSeparator().c_str());
        string key = key.substr(found + 1);
        found = key.find(consumer.getConsumerTable()->getTableNameSeparator().c_str());
        IpPrefix ip_prefix = IpPrefix(key.substr(0, found));
        string vrf_name = key.substr(found + 1);
        string op = kfvOp(t);
        bool is_new_entry = false;
        sai_attribute_t         ars_attr;
        vector<sai_attribute_t> ars_attrs;

        SWSS_LOG_NOTICE("ARS Prefix Op %s Vrf %s Prefix %s", op.c_str(), vrf_name.c_str(), ip_prefix.to_string().c_str());

        auto table = m_arsNexthopGroupPrefixes.find(vrf_name);
        sai_object_id_t &vrf_id = m_vrfOrch->getVRFid(vrf_name);
        NextHopGroupKey nhg = gRouteOrch->getSyncdRouteNhgKey(vrf_id, ip_prefix);

        if (op == SET_COMMAND)
        {
            if (table != m_arsNexthopGroupPrefixes.end() && table.find(ip_prefix) != table.end())
            {
                SWSS_LOG_WARN("ARS nexthop group vrf %s prefix %s already exists", vrf_name.c_str(), ip_prefix.to_string().c_str());
                it = consumer.m_toSync.erase(it);
                continue;
            }

            string ars_name = "";
            std::set<IpAddress> altPathInterfaces;
            for (auto i : kfvFieldsValues(t))
            {
                if (fvField(i) == ARS_FIELD_NAME_ARS_NAME)
                {
                    ars_name = fvValue(i);
                }
                else if (fvField(i) == ARS_FIELD_NAME_ALT_PATH_MEMBERS)
                {
                    const auto &members = tokenize(fvFieldValues(i), ',');
                    for (auto &m : members)
                    {
                        altPathInterfaces.push_back(IpAddress(m));
                    }
                }
                else
                {
                    SWSS_LOG_WARN("Received unsupported field %s", fvField(i).c_str());
                }
            }

            if (ars_name.empty())
            {
                SWSS_LOG_ERROR("Received ARS nexthop group prefix %s with empty name for key %s", ip_prefix.to_string().c_str(), kfvKey(t).c_str());
                it = consumer.m_toSync.erase(it);
                continue;
            }

            auto arsObject_entry = m_arsObjects.find(ars_name);
            if (arsObject_entry == m_arsObjects.end())
            {
                SWSS_LOG_NOTICE("ARS nexthop group entry not received yet, continue");
                continue;
            }

            arsObject_entry->second.altPathNexthops = altPathInterfaces;
            arsObject_entry->second.prefixes.push_back(ip_prefix.to_string());
            if (table == m_arsNexthopGroupPrefixes.end())
            {
                map<IpPrefix, ArsObjectEntry *> prefix_table;
                prefix_table[ip_prefix] = &(arsObject_entry->second);
                m_arsNexthopGroupPrefixes[vrf_name] = prefix_table;
                table = m_arsNexthopGroupPrefixes.find(vrf_name);
            }
            else
            {
                table[ip_prefix] = &(arsObject_entry->second);
            }

            SWSS_LOG_NOTICE("ARS Adding Prefix %s with NHs %s", ip_prefix.to_string().c_str(), nhg.to_string().c_str());

            FieldValueTuple tmp("nexthops", nhg.to_string());
            vector<FieldValueTuple> fvVector;
            fvVector.push_back(tmp);
            m_arsPrefixStateTable.set(ip_prefix.to_string(), fvVector);

            SWSS_LOG_NOTICE("Ars entry added for object %s, prefix %s", ars_object.c_str(), ip_prefix.to_string().c_str());
        }
        else if (op == DEL_COMMAND)
        {
            auto table = m_arsNexthopGroupPrefixes.find(vrf_name);
            if (table == m_arsNexthopGroupPrefixes.end() || table.find(ip_prefix) == table.end())
            {
                SWSS_LOG_NOTICE("ARS_NHG_PREFIX doesn't exists, ignore");
                it = consumer.m_toSync.erase(it);
                continue;
            }
            else if (!isSetImplemented(SAI_OBJECT_TYPE_NEXT_HOP_GROUP, SAI_NEXT_HOP_GROUP_ATTR_ARS_OBJECT_ID))
            {
                SWSS_LOG_NOTICE("Disabling ARS NHG is not supported");
                it = consumer.m_toSync.erase(it);
                continue;
            }
            else
            {
                auto prefix_entry = table.find(ip_prefix);
                prefix_entry->second->prefixes.erase(ip_prefix.to_string());
                table.erase(ip_prefix);
                if (table.empty())
                {
                    m_arsNexthopGroupPrefixes.erase(vrf_name);
                }
                SWSS_LOG_NOTICE("Ars entry added for object %s, prefix %s", ars_object.c_str(), ip_prefix.to_string().c_str());

                m_arsPrefixStateTable.erase(ip_prefix.to_string());
            }
        }

        if (nhg.getSize() > 0)
        {
            /* Handling ARS over already configured nexthop groups */
            SWSS_LOG_INFO("Enabling ARS over already configured nexthop groups - Remove existing and Configure again");
            gRouteOrch->reconfigureRoute(vrf_id, ip_prefix, nhg);
        }

        it = consumer.m_toSync.erase(it);
    }

    return true;
}


bool ArsOrch::doTaskArsLag(Consumer &consumer)
{
    SWSS_LOG_ENTER();

    auto it = consumer.m_toSync.begin();
    while (it != consumer.m_toSync.end())
    {
        KeyOpFieldsValuesTuple t = it->second;
        string key = kfvKey(t);
        size_t found = key.find(consumer.getConsumerTable()->getTableNameSeparator().c_str());
        string table_id = key.substr(0, found);
        string if_name = key.substr(found + 1);
        string op = kfvOp(t);
        bool is_new_entry = false;
        sai_attribute_t         ars_attr;
        vector<sai_attribute_t> ars_attrs;
        Port p;

        SWSS_LOG_NOTICE("ARS Path Op %s Lag %s", op.c_str(), if_name.c_str());

        if (op == SET_COMMAND)
        {
            string ars_name = "";
            std::set<string> altPathInterfaces;
            for (auto i : kfvFieldsValues(t))
            {
                if (fvField(i) == ARS_FIELD_NAME_ARS_NAME)
                {
                    ars_name = fvValue(i);
                }
                else if (fvField(i) == ARS_FIELD_NAME_ALT_PATH_MEMBERS)
                {
                    const auto &members = tokenize(fvFieldValues(i), ',');
                    for (auto &m : members)
                    {
                        altPathInterfaces.push_back(m);
                    }
                }
                else
                {
                    SWSS_LOG_WARN("Received unsupported field %s", fvField(i).c_str());
                }
            }

            if (ars_name.empty())
            {
                SWSS_LOG_ERROR("Received ARS LAG %s with empty name for key %s", if_name.c_str(), kfvKey(t).c_str());
                it = consumer.m_toSync.erase(it);
                continue;
            }

            auto arsObject_entry = m_arsObjects.find(ars_name);
            if (arsObject_entry == m_arsObjects.end())
            {
                SWSS_LOG_NOTICE("ARS nexthop group entry not received yet, continue");
                continue;
            }

            arsObject_entry->second.altPathPorts = altPathInterfaces;
            arsObject_entry->second.lags.push_back(if_name);

            m_lags[if_name] = arsObject_entry->second;

            SWSS_LOG_NOTICE("ARS Adding LAG %s", if_name.c_str());
        }
        else if (op == DEL_COMMAND)
        {
            if (m_lags.find(if_name) == m_lags.end())
            {
                SWSS_LOG_INFO("Received delete call for non-existent interface %s", if_name.c_str());
                it = consumer.m_toSync.erase(it);
                continue;
            }
            else
            {
                m_lags.erase(if_name);
                SWSS_LOG_INFO("Removed interface %s", if_name.c_str());
            }
        }

        if (!m_arsProfiles.empty())
        {
            if (!gPortsOrch->getPort(if_name, p) || (p.m_oper_status != SAI_PORT_OPER_STATUS_UP) || (p.m_lag_id == SAI_NULL_OBJECT_ID))
            {
                SWSS_LOG_WARN("Tried to add/remove non-existent/down interface %s - skipped", if_name.c_str());
            }
            else
            {
                sai_attribute_t attr;
                attr.id = SAI_LAG_ATTR_ARS_OBJECT_ID;
                attr.value.oid = (op == SET_COMMAND)  ? arsObject_entry->second.m_ars_object_id : SAI_NULL_OBJECT_ID;
                sai_status_t status = sai_lag_api->set_lag_attribute(p.m_lag_id, &attr);
                if (status != SAI_STATUS_SUCCESS)
                {
                    SWSS_LOG_ERROR("Failed to set ars object %" PRIx64 " to lag: %s", attr.value.oid, port.m_alias.c_str());
                }

                /* set lag memebers if needed */
                vector<Port> portv;
                gPortsOrch->getLagMember(p, portv);
                if (!portv.empty())
                {
                    for (auto &lag_port: portv)
                    {
                        gPortsOrch->getPort(lag_port, port);
                        gPortsOrch->setPortArsAltPath(port, arsObject_entry->second.altPathPorts.find(lag_port) != arsObject_entry->second.altPathPorts.end())
                    }
                }
            }
        }

        string ifnames = "";
        for (auto ifname : m_lags)
        {
            ifnames += ifname + " ";
        }
        FieldValueTuple tmp("ifname", ifnames);
        vector<FieldValueTuple> fvVector;
        fvVector.push_back(tmp);
        m_arsIfStateTable.set("ArsLags", fvVector);

        it = consumer.m_toSync.erase(it);
    }

    return true;
}

void ArsOrch::doTask(Consumer& consumer) 
{
    SWSS_LOG_ENTER();
    bool entry_handled = t    const string & table_name = consumer.getTableName();
rue;

    if (!m_isArsSupported)
    {
        SWSS_LOG_WARN("ARS is not supported");
        return;
    }

	if (table_name == CFG_ARS_PROFILE || table_name == APP_ARS_PROFILE_TABLE_NAME)
    {
        doTaskArsProfile(consumer);
    }
    if (table_name == CFG_ARS_QUANTIZATION_BANDS || table_name == APP_ARS_QUANTIZATION_BANDS_TABLE_NAME)
    {
        doTaskArsQuantizationBands(consumer);
    }
    if (table_name == CFG_ARS_OBJECT || table_name == APP_ARS_OBJECT_TABLE_NAME)
    {
        doTaskArsObject(consumer);
    }
    else if (table_name == CFG_ARS_INTERFACE || table_name == APP_ARS_INTERFACE_TABLE_NAME)
    {
        doTaskArsInterfaces(consumer);
    }
    else if (table_name == CFG_ARS_NEXT_HOP_GROUP_PREFIX || table_name == APP_ARS_NEXT_HOP_GROUP_PREFIX_TABLE_NAME)
    {
        doTaskArsNexthopGroup(consumer);
    }
    else if (table_name == CFG_ARS_PORTCHANNEL || table_name == APP_ARS_PORTCHANNEL_TABLE_NAME)
    {
        doTaskArsLag(consumer);
    }
    else
    {
        SWSS_LOG_ERROR("Unknown table : %s", table_name.c_str());
    }
}
