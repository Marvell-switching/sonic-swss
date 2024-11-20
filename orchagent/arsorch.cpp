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

typedef std::map<uint32_t, std::string> ars_sai_attr_lookup_t;
typedef std::map<std::string, ars_sai_attr_lookup_t> ars_sai_feature_data_t;
typedef std::map<uint32_t, ars_sai_feature_data_t> ars_sai_feature_lookup_t;

ars_sai_attr_lookup_t ars_profile_attrs = {
    {SAI_ARS_PROFILE_ATTR_ALGO,"SAI_ARS_PROFILE_ATTR_ALGO"},
    {SAI_ARS_PROFILE_ATTR_SAMPLING_INTERVAL,"SAI_ARS_PROFILE_ATTR_SAMPLING_INTERVAL"},
    {SAI_ARS_PROFILE_ATTR_ARS_RANDOM_SEED,"SAI_ARS_PROFILE_ATTR_ARS_RANDOM_SEED"},
    {SAI_ARS_PROFILE_ATTR_ECMP_ARS_MAX_GROUPS,"SAI_ARS_PROFILE_ATTR_ECMP_ARS_MAX_GROUPS"},
    {SAI_ARS_PROFILE_ATTR_ECMP_ARS_MAX_MEMBERS_PER_GROUP,"SAI_ARS_PROFILE_ATTR_ECMP_ARS_MAX_MEMBERS_PER_GROUP"},
    {SAI_ARS_PROFILE_ATTR_LAG_ARS_MAX_GROUPS,"SAI_ARS_PROFILE_ATTR_LAG_ARS_MAX_GROUPS"},
    {SAI_ARS_PROFILE_ATTR_LAG_ARS_MAX_MEMBERS_PER_GROUP,"SAI_ARS_PROFILE_ATTR_LAG_ARS_MAX_MEMBERS_PER_GROUP"},
    {SAI_ARS_PROFILE_ATTR_PORT_LOAD_PAST,"SAI_ARS_PROFILE_ATTR_PORT_LOAD_PAST"},
    {SAI_ARS_PROFILE_ATTR_PORT_LOAD_PAST_WEIGHT,"SAI_ARS_PROFILE_ATTR_PORT_LOAD_PAST_WEIGHT"},
    {SAI_ARS_PROFILE_ATTR_PORT_LOAD_FUTURE,"SAI_ARS_PROFILE_ATTR_PORT_LOAD_FUTURE"},
    {SAI_ARS_PROFILE_ATTR_PORT_LOAD_FUTURE_WEIGHT,"SAI_ARS_PROFILE_ATTR_PORT_LOAD_FUTURE_WEIGHT"},
    {SAI_ARS_PROFILE_ATTR_PORT_LOAD_CURRENT,"SAI_ARS_PROFILE_ATTR_PORT_LOAD_CURRENT"},
    {SAI_ARS_PROFILE_ATTR_PORT_LOAD_EXPONENT,"SAI_ARS_PROFILE_ATTR_PORT_LOAD_EXPONENT"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BANDS,"SAI_ARS_PROFILE_ATTR_QUANT_BANDS"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_0_MIN_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_0_MIN_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_0_MAX_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_0_MAX_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_1_MIN_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_1_MIN_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_1_MAX_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_1_MAX_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_2_MIN_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_2_MIN_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_2_MAX_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_2_MAX_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_3_MIN_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_3_MIN_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_3_MAX_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_3_MAX_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_4_MIN_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_4_MIN_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_4_MAX_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_4_MAX_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_5_MIN_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_5_MIN_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_5_MAX_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_5_MAX_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_6_MIN_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_6_MIN_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_6_MAX_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_6_MAX_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_7_MIN_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_7_MIN_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_7_MAX_THRESHOLD,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_7_MAX_THRESHOLD"},
    {SAI_ARS_PROFILE_ATTR_ENABLE_IPV4,"SAI_ARS_PROFILE_ATTR_ENABLE_IPV4"},
    {SAI_ARS_PROFILE_ATTR_ENABLE_IPV6,"SAI_ARS_PROFILE_ATTR_ENABLE_IPV6"},
    {SAI_ARS_PROFILE_ATTR_LOAD_PAST_MIN_VAL,"SAI_ARS_PROFILE_ATTR_LOAD_PAST_MIN_VAL"},
    {SAI_ARS_PROFILE_ATTR_LOAD_PAST_MAX_VAL,"SAI_ARS_PROFILE_ATTR_LOAD_PAST_MAX_VAL"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_MIN_THRESHOLD_LIST_LOAD_PAST,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_MIN_THRESHOLD_LIST_LOAD_PAST"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_MAX_THRESHOLD_LIST_LOAD_PAST,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_MAX_THRESHOLD_LIST_LOAD_PAST"},
    {SAI_ARS_PROFILE_ATTR_LOAD_FUTURE_MIN_VAL,"SAI_ARS_PROFILE_ATTR_LOAD_FUTURE_MIN_VAL"},
    {SAI_ARS_PROFILE_ATTR_LOAD_FUTURE_MAX_VAL,"SAI_ARS_PROFILE_ATTR_LOAD_FUTURE_MAX_VAL"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_MIN_THRESHOLD_LIST_LOAD_FUTURE,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_MIN_THRESHOLD_LIST_LOAD_FUTURE"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_MAX_THRESHOLD_LIST_LOAD_FUTURE,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_MAX_THRESHOLD_LIST_LOAD_FUTURE"},
    {SAI_ARS_PROFILE_ATTR_LOAD_CURRENT_MIN_VAL,"SAI_ARS_PROFILE_ATTR_LOAD_CURRENT_MIN_VAL"},
    {SAI_ARS_PROFILE_ATTR_LOAD_CURRENT_MAX_VAL,"SAI_ARS_PROFILE_ATTR_LOAD_CURRENT_MAX_VAL"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_MIN_THRESHOLD_LIST_LOAD_CURRENT,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_MIN_THRESHOLD_LIST_LOAD_CURRENT"},
    {SAI_ARS_PROFILE_ATTR_QUANT_BAND_MAX_THRESHOLD_LIST_LOAD_CURRENT,"SAI_ARS_PROFILE_ATTR_QUANT_BAND_MAX_THRESHOLD_LIST_LOAD_CURRENT"},
    {SAI_ARS_PROFILE_ATTR_MAX_FLOWS,"SAI_ARS_PROFILE_ATTR_MAX_FLOWS"}
};
ars_sai_attr_lookup_t ars_sai_attrs = {
    {SAI_ARS_ATTR_MODE,"SAI_ARS_ATTR_MODE"},
    {SAI_ARS_ATTR_IDLE_TIME,"SAI_ARS_ATTR_IDLE_TIME"},
    {SAI_ARS_ATTR_MAX_FLOWS,"SAI_ARS_ATTR_MAX_FLOWS"},
    {SAI_ARS_ATTR_MON_ENABLE,"SAI_ARS_ATTR_MON_ENABLE"},
    {SAI_ARS_ATTR_SAMPLEPACKET_ENABLE,"SAI_ARS_ATTR_SAMPLEPACKET_ENABLE"},
    {SAI_ARS_ATTR_MAX_ALT_MEMEBERS_PER_GROUP,"SAI_ARS_ATTR_MAX_ALT_MEMEBERS_PER_GROUP"},
    {SAI_ARS_ATTR_MAX_PRIMARY_MEMEBERS_PER_GROUP,"SAI_ARS_ATTR_MAX_PRIMARY_MEMEBERS_PER_GROUP"},
    {SAI_ARS_ATTR_PRIMARY_PATH_QUALITY_THRESHOLD,"SAI_ARS_ATTR_PRIMARY_PATH_QUALITY_THRESHOLD"},
    {SAI_ARS_ATTR_ALTERNATE_PATH_COST,"SAI_ARS_ATTR_ALTERNATE_PATH_COST"},
    {SAI_ARS_ATTR_ALTERNATE_PATH_BIAS,"SAI_ARS_ATTR_ALTERNATE_PATH_BIAS"}
};
ars_sai_attr_lookup_t ars_port_attrs = {
    {SAI_PORT_ATTR_ARS_ENABLE, "SAI_PORT_ATTR_ARS_ENABLE"},
    {SAI_PORT_ATTR_ARS_PORT_LOAD_SCALING_FACTOR, "SAI_PORT_ATTR_ARS_PORT_LOAD_SCALING_FACTOR"}
};

ars_sai_attr_lookup_t ars_nhg_attrs = {
    {SAI_NEXT_HOP_GROUP_ATTR_ARS_OBJECT_ID, "SAI_NEXT_HOP_GROUP_ATTR_ARS_OBJECT_ID"},
    {SAI_NEXT_HOP_GROUP_ATTR_ARS_PACKET_DROPS, "SAI_NEXT_HOP_GROUP_ATTR_ARS_PACKET_DROPS"},
    {SAI_NEXT_HOP_GROUP_ATTR_ARS_NEXT_HOP_REASSIGNMENTS, "SAI_NEXT_HOP_GROUP_ATTR_ARS_NEXT_HOP_REASSIGNMENTS"},
    {SAI_NEXT_HOP_GROUP_ATTR_ARS_PORT_REASSIGNMENTS, "SAI_NEXT_HOP_GROUP_ATTR_ARS_PORT_REASSIGNMENTS"}
};

ars_sai_attr_lookup_t ars_switch_attrs = {
    {SAI_SWITCH_ATTR_ARS_PROFILE, "SAI_SWITCH_ATTR_ARS_PROFILE"}
};

ars_sai_attr_lookup_t ars_lag_attrs = {
    {SAI_LAG_ATTR_ARS_OBJECT_ID, "SAI_LAG_ATTR_ARS_OBJECT_ID"},
    {SAI_LAG_ATTR_ARS_PACKET_DROPS, "SAI_LAG_ATTR_ARS_PACKET_DROPS"},
    {SAI_LAG_ATTR_ARS_PORT_REASSIGNMENTS, "SAI_LAG_ATTR_ARS_PORT_REASSIGNMENTS"}
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

ArsOrch::ArsOrch(DBConnector *db, DBConnector *appDb, DBConnector *stateDb, vector<table_name_with_pri_t> &tableNames) :
        Orch(db, tableNames),
        m_arsProfileStateTable(stateDb, APP_ARS_PROFILE_TABLE_NAME),
        m_arsIfStateTable(stateDb, APP_ARS_MIN_PATH_INTERFACE_TABLE_NAME),
        m_arsPrefixStateTable(stateDb, APP_ARS_NHG_PREFIX)
{
    sai_attr_capability_t capability;

    SWSS_LOG_ENTER();
    isArsConfigured = false;
    SWSS_LOG_NOTICE("ENTER");
    gPortsOrch->attach(this);

    for (auto it = ars_features.begin(); it != ars_features.end(); it++)
    {
        for (auto it2 = it->second.begin()->second.begin(); it2 != it->second.begin()->second.end(); it2++)
        {
            if (sai_query_attribute_capability(gSwitchId, (sai_object_type_t)it->first,
                                                (sai_attr_id_t)it2->first,
                                                &capability) == SAI_STATUS_SUCCESS)
            {
                SWSS_LOG_NOTICE("Feature %s Attr %s is supported. Create %s Set %s Get %s", it->second.begin()->first.c_str(), it2->second.c_str(), capability.create_implemented ? "Y" : "N", capability.set_implemented ? "Y" : "N", capability.get_implemented ? "Y" : "N");
            }
            else
            {
                SWSS_LOG_NOTICE("Feature %s Attr %s is NOT supported", it->second.begin()->first.c_str(), it2->second.c_str());
            }
        }
    }

    SWSS_LOG_WARN("ARS init gSwitchId = %lx", gSwitchId);

    if (!m_sai_ars_profile_id)
    {
        capability.create_implemented = true;

        sai_object_id_t ars_profile_id;
        sai_attribute_t sai_attr;
        vector<sai_attribute_t> sai_attrs;

        if (sai_query_attribute_capability(gSwitchId, SAI_OBJECT_TYPE_ARS_PROFILE,
                                           SAI_ARS_PROFILE_ATTR_ALGO,
                                           &capability) == SAI_STATUS_SUCCESS)
        {
            if (capability.create_implemented == true)
            {
                sai_attr.id = SAI_ARS_PROFILE_ATTR_ALGO;
                sai_attr.value.u32 = SAI_ARS_PROFILE_ALGO_EWMA;
                SWSS_LOG_NOTICE("Creating ARS profile with attr id %d value %d", sai_attr.id, sai_attr.value.u32);
                sai_attrs.push_back(sai_attr);
                sai_attr.id = SAI_ARS_PROFILE_ATTR_PORT_LOAD_PAST;
                sai_attr.value.booldata = true;
                SWSS_LOG_NOTICE("Creating ARS profile with attr id %d value %d", sai_attr.id, sai_attr.value.booldata);
                sai_attrs.push_back(sai_attr);
                sai_attr.id = SAI_ARS_PROFILE_ATTR_LOAD_PAST_MIN_VAL;
                sai_attr.value.u32 = 0;
                SWSS_LOG_NOTICE("Creating ARS profile with attr id %d value %d", sai_attr.id, sai_attr.value.u32);
                sai_attrs.push_back(sai_attr);
                sai_attr.id = SAI_ARS_PROFILE_ATTR_LOAD_PAST_MAX_VAL;
                sai_attr.value.u32 = 5000;
                SWSS_LOG_NOTICE("Creating ARS profile with attr id %d value %d", sai_attr.id, sai_attr.value.u32);
                sai_attrs.push_back(sai_attr);
                sai_attr.id = SAI_ARS_PROFILE_ATTR_ENABLE_IPV4;
                sai_attr.value.booldata = true;
                SWSS_LOG_NOTICE("Creating ARS profile with attr id %d value %d", sai_attr.id, sai_attr.value.booldata);
                sai_attrs.push_back(sai_attr);
                sai_attr.id = SAI_ARS_PROFILE_ATTR_ENABLE_IPV6;
                sai_attr.value.booldata = true;
                SWSS_LOG_NOTICE("Creating ARS profile with attr id %d value %d", sai_attr.id, sai_attr.value.booldata);
                sai_attrs.push_back(sai_attr);
                sai_attr.id = SAI_ARS_PROFILE_ATTR_MAX_FLOWS;
                sai_attr.value.u32 = 2048;
                SWSS_LOG_NOTICE("Creating ARS profile with attr id %d value %d", sai_attr.id, sai_attr.value.u32);
                sai_attrs.push_back(sai_attr);
                sai_status_t status = sai_ars_profile_api->create_ars_profile(&ars_profile_id,
                                                                            gSwitchId,
                                                                            (uint32_t)sai_attrs.size(),
                                                                            sai_attrs.data());
                if (status != SAI_STATUS_SUCCESS)
                {
                    SWSS_LOG_ERROR("Failed to create ARS profile: %d", status);
                    throw runtime_error("Failed to create ARS profile");
                }

                //if (sai_query_attribute_capability(gSwitchId, SAI_OBJECT_TYPE_SWITCH,
                //                                SAI_SWITCH_ATTR_ARS_PROFILE,
                //                                &capability) == SAI_STATUS_SUCCESS)
                {
                    //if (capability.create_implemented == true)
                    {
                        sai_attr.id = SAI_SWITCH_ATTR_ARS_PROFILE;
                        sai_attr.value.oid = ars_profile_id;
                        status = sai_switch_api->set_switch_attribute(gSwitchId, &sai_attr);
                        if (status != SAI_STATUS_SUCCESS)
                        {
                            SWSS_LOG_ERROR("Failed to bind ARS profile to switch: %d", status);
                            //throw runtime_error("Failed to bind ARS profile to switch");
                        }
                        else
                        {
                            m_sai_ars_profile_id = ars_profile_id;
                            SWSS_LOG_NOTICE("Successeed to bind ARS profile to switch: %lx", ars_profile_id); 
                        }
                    }
                }
            }
        }
    }

    SWSS_LOG_NOTICE("EXIT");
}

void ArsOrch::update(SubjectType type, void *cntx)
{
    SWSS_LOG_ENTER();
    assert(cntx);

    if (!isArsConfigured)
    {
        SWSS_LOG_INFO("ARS not enabled - no action on interface state change");
        return;
    }

    switch(type) {
        case SUBJECT_TYPE_PORT_OPER_STATE_CHANGE:
        {
            PortOperStateUpdate *update = reinterpret_cast<PortOperStateUpdate *>(cntx);
            auto arsProfile_entry = m_arsProfiles.begin();
            if (arsProfile_entry == m_arsProfiles.end())
            {
                SWSS_LOG_INFO("ARS profile not configured - no action on interface state change");
                return;
            }

            bool is_found = (arsProfile_entry->second.minPathInterfaces.find(update->port.m_alias) != arsProfile_entry->second.minPathInterfaces.end());
            SWSS_LOG_INFO("Interface %s %sconfigured for ARS - %s",
                    update->port.m_alias.c_str(),
                    is_found ? "" : "not ",
                    update->operStatus == SAI_PORT_OPER_STATUS_UP ? "enable ARS on interface" : "ignore");
            if (is_found)
            {
                updateArsMinPathInterface(arsProfile_entry->second, update->port, update->operStatus == SAI_PORT_OPER_STATUS_UP);
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

    SWSS_LOG_NOTICE("Warm reboot: placeholder");

    return Orch::bake();
}

bool ArsOrch::isRouteArs(sai_object_id_t vrf_id, const IpPrefix &ipPrefix, sai_object_id_t * ars_object_id)
{
    SWSS_LOG_ENTER();
 
    if (!isArsConfigured || (vrf_id != gVirtualRouterId))
    {
        SWSS_LOG_WARN("ARS is not enabled while configuring route %s", ipPrefix.to_string().c_str());
        return false;
    }

    auto prefix_entry = m_arsNexthopGroupPrefixes.find(ipPrefix);
    if (prefix_entry == m_arsNexthopGroupPrefixes.end())
    {
        SWSS_LOG_WARN("Route %s is not enabled for ARS", ipPrefix.to_string().c_str());

        string pr = "";
        string nh_names = "";
        for (const auto& prefix : m_arsNexthopGroupPrefixes)
        {
            pr += prefix.first.to_string() + ",";
            for (const auto& nh : prefix.second->prefixes)
            {
                nh_names += nh.to_string() + ",";
            }
        }

        SWSS_LOG_NOTICE("ARS configured prefixes <%s> with nhs <%s>", pr.c_str(), nh_names.c_str());

        return false;
    }

    SWSS_LOG_WARN("Route %s is enabled for ARS", ipPrefix.to_string().c_str());
    *ars_object_id = m_sai_ars_id;
    return true;
}


bool ArsOrch::setArsProfile(ArsProfileEntry &profile)
{
    SWSS_LOG_ENTER();

    sai_status_t    status = SAI_STATUS_NOT_SUPPORTED;
    sai_attribute_t ars_attr;
    sai_attr_capability_t capability;

    if ((sai_query_attribute_capability(gSwitchId, SAI_OBJECT_TYPE_ARS,
                                        SAI_ARS_ATTR_MODE,
                                        &capability) == SAI_STATUS_SUCCESS) &&
         (capability.set_implemented == true))
    {
        ars_attr.id = SAI_ARS_ATTR_MODE;
        ars_attr.value.u32 = profile.assign_mode == PER_PACKET ? SAI_ARS_MODE_PER_PACKET_QUALITY : SAI_ARS_MODE_FLOWLET_QUALITY;
        status = sai_ars_api->set_ars_attribute(m_sai_ars_id,
                                                &ars_attr);
        if (status != SAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR("Failed to set ars oid %" PRIx64 " mode %d: %d",
                            m_sai_ars_id, profile.assign_mode, status);
            task_process_status handle_status = handleSaiSetStatus(SAI_API_ARS, status);
            if (handle_status != task_success)
            {
                return parseHandleSaiStatusFailure(handle_status);
            }
        }
    }
    else
    {
        SWSS_LOG_WARN("Setting Attr SAI_ARS_ATTR_MODE is not supported. Failed to set ARS oid %" PRIx64 " mode %s",
                        m_sai_ars_id, profile.assign_mode == PER_PACKET ? "per_packet" : "per_flowlet_quality");
    }

    if ((sai_query_attribute_capability(gSwitchId, SAI_OBJECT_TYPE_ARS,
                                       SAI_ARS_ATTR_IDLE_TIME,
                                       &capability) == SAI_STATUS_SUCCESS) &&
        (capability.set_implemented == true))
    {
        ars_attr.id = SAI_ARS_ATTR_IDLE_TIME;
        ars_attr.value.u32 = profile.flowlet_idle_time;
        status = sai_ars_api->set_ars_attribute(m_sai_ars_id,
                                                &ars_attr);
        if (status != SAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR("Failed to set ars oid %" PRIx64 " idle time %d: %d",
                m_sai_ars_id, profile.flowlet_idle_time, status);
            task_process_status handle_status = handleSaiSetStatus(SAI_API_ARS, status);
            if (handle_status != task_success)
            {
                return parseHandleSaiStatusFailure(handle_status);
            }
        }
    }
    else
    {
        SWSS_LOG_WARN("Setting Attr SAI_ARS_ATTR_IDLE_TIME is not supported. Failed to set ARS oid %" PRIx64 " idle time %d",
                       m_sai_ars_id, profile.flowlet_idle_time);
    }

    if ((sai_query_attribute_capability(gSwitchId, SAI_OBJECT_TYPE_ARS,
                                        SAI_ARS_ATTR_MAX_FLOWS,
                                        &capability) == SAI_STATUS_SUCCESS) &&
        (capability.set_implemented == true))
    {
        ars_attr.id = SAI_ARS_ATTR_MAX_FLOWS;
        ars_attr.value.u32 = profile.max_flows;
        status = sai_ars_api->set_ars_attribute(m_sai_ars_id,
                                                &ars_attr);
        if (status != SAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR("Failed to set ars oid %" PRIx64 " max flows %d: %d",
                m_sai_ars_id, profile.max_flows, status);
            task_process_status handle_status = handleSaiSetStatus(SAI_API_ARS, status);
            if (handle_status != task_success)
            {
                return parseHandleSaiStatusFailure(handle_status);
            }
        }
    }
    else
    {
        SWSS_LOG_WARN("Setting Attr SAI_ARS_ATTR_MAX_FLOWS is not supported. Failed to set ARS oid %" PRIx64 " max flows %d",
                       m_sai_ars_id, profile.max_flows);
    }

    return true;
}

bool ArsOrch::createArsProfile(ArsProfileEntry &profile)
{
    SWSS_LOG_ENTER();

    sai_status_t    status = SAI_STATUS_NOT_SUPPORTED;
    sai_attribute_t ars_attr;
    vector<sai_attribute_t> ars_attrs;
    sai_attr_capability_t capability;

    if ((sai_query_attribute_capability(gSwitchId, SAI_OBJECT_TYPE_ARS,
                                        SAI_ARS_ATTR_MODE,
                                        &capability) == SAI_STATUS_SUCCESS) &&
         (capability.create_implemented == true))
    {
        ars_attr.id = SAI_ARS_ATTR_MODE;
        ars_attr.value.u32 = profile.assign_mode == PER_PACKET ? SAI_ARS_MODE_PER_PACKET_QUALITY : SAI_ARS_MODE_FLOWLET_QUALITY;
        ars_attrs.push_back(ars_attr);
    }
    else
    {
        SWSS_LOG_WARN("Creating Attr SAI_ARS_ATTR_MODE is not supported. Failed to set ARS oid %" PRIx64 " mode %s",
                        m_sai_ars_id, profile.assign_mode == PER_PACKET ? "per_packet" : "per_flowlet_quality");
    }

    if ((sai_query_attribute_capability(gSwitchId, SAI_OBJECT_TYPE_ARS,
                                       SAI_ARS_ATTR_IDLE_TIME,
                                       &capability) == SAI_STATUS_SUCCESS) &&
        (capability.create_implemented == true))
    {
        ars_attr.id = SAI_ARS_ATTR_IDLE_TIME;
        ars_attr.value.u32 = profile.flowlet_idle_time;
        ars_attrs.push_back(ars_attr);
    }
    else
    {
        SWSS_LOG_WARN("Creating Attr SAI_ARS_ATTR_IDLE_TIME is not supported. Failed to set ARS oid %" PRIx64 " idle time %d",
                       m_sai_ars_id, profile.flowlet_idle_time);
    }

    if ((sai_query_attribute_capability(gSwitchId, SAI_OBJECT_TYPE_ARS,
                                        SAI_ARS_ATTR_MAX_FLOWS,
                                        &capability) == SAI_STATUS_SUCCESS) &&
        (capability.create_implemented == true))
    {
        ars_attr.id = SAI_ARS_ATTR_MAX_FLOWS;
        ars_attr.value.u32 = profile.max_flows;
        ars_attrs.push_back(ars_attr);
    }
    else
    {
        SWSS_LOG_WARN("Creating Attr SAI_ARS_ATTR_MAX_FLOWS is not supported. Failed to set ARS oid %" PRIx64 " max flows %d",
                       m_sai_ars_id, profile.max_flows);
    }

    status = sai_ars_api->create_ars(&m_sai_ars_id,
                                     gSwitchId,
                                     (uint32_t)ars_attrs.size(),
                                     ars_attrs.data());
    if (status != SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_ERROR("Failed to create ars oid for %s mode %d, idle time %d, max_flows %d: %d",
            profile.profile_name.c_str(), profile.assign_mode, profile.flowlet_idle_time, profile.max_flows, status);
        task_process_status handle_status = handleSaiSetStatus(SAI_API_ARS, status);
        if (handle_status != task_success)
        {
            return parseHandleSaiStatusFailure(handle_status);
        }
    }

    return true;
}

bool ArsOrch::updateArsMinPathInterface(ArsProfileEntry &profile, const Port &port, const bool is_enable)
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

bool ArsOrch::doTaskArsProfile(const KeyOpFieldsValuesTuple & t)
{
    SWSS_LOG_ENTER();
    string op = kfvOp(t);
    string key = kfvKey(t);
    string ars_profile_name = key; 
    auto arsProfile_entry = m_arsProfiles.find(ars_profile_name);
    ArsMatchMode match_mode = MATCH_ROUTE_BASED;
    uint32_t max_flows = 0, flowlet_idle_time = 0;
    ArsAssignMode assign_mode = PER_FLOWLET_QUALITY;
    bool current_enable = isArsConfigured;
    if (arsProfile_entry != m_arsProfiles.end())
    {
        max_flows = arsProfile_entry->second.max_flows;
        flowlet_idle_time = arsProfile_entry->second.flowlet_idle_time;
        assign_mode = arsProfile_entry->second.assign_mode;
        match_mode = arsProfile_entry->second.match_mode;
    }

    SWSS_LOG_NOTICE("ARS profile Op %s Name %s", op.c_str(), ars_profile_name.c_str());

    if (op == SET_COMMAND)
    {
        for (auto i : kfvFieldsValues(t))
        {
            if (fvField(i) == "max_flows")
            {
                max_flows = stoi(fvValue(i));
            }
            else if (fvField(i) == "flowlet_idle_time")
            {
                flowlet_idle_time = stoi(fvValue(i));
            }
            else if (fvField(i) == "assign_mode")
            {
                if (fvValue(i) == "per_packet")
                {
                    assign_mode = PER_PACKET;
                }
                else if (fvValue(i) != "per_flowlet_quality")
                {
                    SWSS_LOG_WARN("Received unsupported assign_mode %s, defaulted to per_flowlet_quality",
                                    fvValue(i).c_str());
                }
            }
        }

        if (max_flows == 0 || (assign_mode == PER_FLOWLET_QUALITY && flowlet_idle_time == 0))
        {
            SWSS_LOG_WARN("Received invalid max_flows %d or flowlet_idle_time %d for key %s", max_flows, flowlet_idle_time, kfvKey(t).c_str());
            return true;
        }

        if (arsProfile_entry != m_arsProfiles.end()) 
        {
            SWSS_LOG_WARN("ARS entry %s already exists, updating with max_flows %d, flowlet_idle_time %d, match_mode: %d, assign_mode %d",
                             ars_profile_name.c_str(), max_flows, flowlet_idle_time, match_mode, assign_mode);
            arsProfile_entry->second.assign_mode = assign_mode;
            arsProfile_entry->second.match_mode = match_mode;
            arsProfile_entry->second.max_flows = max_flows;
            arsProfile_entry->second.flowlet_idle_time = flowlet_idle_time;
        }
        else
        {
            ArsProfileEntry arsProfileEntry;
            arsProfileEntry.profile_name = ars_profile_name;
            arsProfileEntry.assign_mode = assign_mode;
            arsProfileEntry.match_mode = match_mode;
            arsProfileEntry.max_flows = max_flows;
            arsProfileEntry.flowlet_idle_time = flowlet_idle_time;
            SWSS_LOG_NOTICE("Added new ARS entry %s with max_flows %d, flowlet_idle_time %d, match_mode: %d, assign_mode %d",
                             ars_profile_name.c_str(), max_flows, flowlet_idle_time, match_mode, assign_mode);
            isArsConfigured = true;
            m_arsProfiles[ars_profile_name] = arsProfileEntry;
            arsProfile_entry = m_arsProfiles.find(ars_profile_name);
        }
        FieldValueTuple name("profile_name", ars_profile_name);
        FieldValueTuple assign("assign_mode", std::to_string(assign_mode));
        FieldValueTuple match("match_mode", std::to_string(match_mode));
        FieldValueTuple flows("max_flows", std::to_string(max_flows));
        FieldValueTuple idle("flowlet_idle_time", std::to_string(flowlet_idle_time));
        vector<FieldValueTuple> fvVector;
        fvVector.push_back(name);
        fvVector.push_back(assign);
        fvVector.push_back(match);
        fvVector.push_back(flows);
        fvVector.push_back(idle);
        m_arsProfileStateTable.set(ars_profile_name, fvVector);
    }
    else if (op == DEL_COMMAND)
    {
        if (arsProfile_entry == m_arsProfiles.end())
        {
            SWSS_LOG_INFO("Received delete call for non-existent entry %s", ars_profile_name.c_str());
        }
        else 
        {
            /* Check if there are no child objects associated prior to deleting */
            if (arsProfile_entry->second.prefixes.size() == 0)
            {
                m_arsProfiles.erase(arsProfile_entry);
                m_arsProfileStateTable.del(ars_profile_name);
                SWSS_LOG_INFO("Received delete call for valid entry with no further dependencies, deleting %s",
                        ars_profile_name.c_str());
            }
            else
            {
                SWSS_LOG_INFO("Child Prefix/Member entries are still associated with this FG_NHG %s", 
                        ars_profile_name.c_str());
                return false;
            }
            if (m_arsProfiles.size() == 0)
            {
                isArsConfigured = false;
            }
        }
    }
    if (isArsConfigured)
    {
        if (!current_enable)
        {
            return createArsProfile(arsProfile_entry->second);
        }
        else
        {
            return setArsProfile(arsProfile_entry->second);
        }
    }

    return true;
}

bool ArsOrch::doTaskArsMinPathInterfaces(const KeyOpFieldsValuesTuple & t)
{
    SWSS_LOG_ENTER();

    auto tokens = tokenize(kfvKey(t), config_db_key_delimiter);
    auto profile_name = tokens[0];
    auto if_name = tokens[1];
    string op = kfvOp(t);
    auto arsProfile_entry = m_arsProfiles.find(profile_name);

    SWSS_LOG_NOTICE("ARS Path Op %s Profile %s Interface %s", op.c_str(), profile_name.c_str(), if_name.c_str());

    if (arsProfile_entry == m_arsProfiles.end()) 
    {
        SWSS_LOG_WARN("ARS entry %s doesn't exists, ignoring", profile_name.c_str());
        return true;
    }

    if (op == SET_COMMAND)
    {
        if (arsProfile_entry->second.minPathInterfaces.find(if_name) != arsProfile_entry->second.minPathInterfaces.end()) 
        {
            SWSS_LOG_WARN("Tried to add already added interface %s to Ars profile %s - skipped",
                    if_name.c_str(), profile_name.c_str());
            return true;
        }
    
        arsProfile_entry->second.minPathInterfaces.insert(if_name);
        SWSS_LOG_NOTICE("Added new minPath interface %s to ARS entry %s",
                            if_name.c_str(), profile_name.c_str());
    }
    else if (op == DEL_COMMAND)
    {
        if (arsProfile_entry->second.minPathInterfaces.find(if_name) == arsProfile_entry->second.minPathInterfaces.end())
        {
            SWSS_LOG_INFO("Received delete call for non-existent minPath interface %s for Ars entry %s", if_name.c_str(), profile_name.c_str());
            return true;
        }
        else
        {
            arsProfile_entry->second.minPathInterfaces.erase(if_name);
            SWSS_LOG_INFO("Removed minPath interface %s from ARS entry %s", if_name.c_str(), profile_name.c_str());
        }
    }

    string ifnames = "";
    for (auto ifname : arsProfile_entry->second.minPathInterfaces)
    {
        ifnames += ifname + ",";
    }
    FieldValueTuple tmp("ifname", ifnames);
    vector<FieldValueTuple> fvVector;
    fvVector.push_back(tmp);
    m_arsIfStateTable.set(profile_name, fvVector);
    if (isArsConfigured)
    {
        Port p;
        if (!gPortsOrch->getPort(if_name, p) || p.m_oper_status != SAI_PORT_OPER_STATUS_UP)
        {
            SWSS_LOG_WARN("Tried to add/remove non-existent/down interface %s of Ars profile %s - skipped",
                    if_name.c_str(), profile_name.c_str());
            return true;
        }
        return updateArsMinPathInterface(arsProfile_entry->second, p, (op == SET_COMMAND));
    }

    return true;
}

bool ArsOrch::doTaskArsNhgPrefix(const KeyOpFieldsValuesTuple & t)
{
    SWSS_LOG_ENTER();
    string op = kfvOp(t);
    string key = kfvKey(t);
    IpPrefix ip_prefix = IpPrefix(key);
    auto prefix_entry = m_arsNexthopGroupPrefixes.find(ip_prefix);

    SWSS_LOG_NOTICE("ARS Prefix Op %s Prefix %s", op.c_str(), ip_prefix.to_string().c_str());

    if (op == SET_COMMAND)
    {
        if (prefix_entry != m_arsNexthopGroupPrefixes.end())
        {
            SWSS_LOG_WARN("ARS nexthop group prefix %s already exists", prefix_entry->first.to_string().c_str());
            return true;
        }

        string ars_profile = "";
        for (auto i : kfvFieldsValues(t))
        {
            if (fvField(i) == "profile_name")
            {
                ars_profile = fvValue(i);
            }
        }

        if (ars_profile.empty())
        {
            SWSS_LOG_ERROR("Received ARS nexthop group prefix with empty name for key %s", kfvKey(t).c_str());
            return true;
        }

        auto arsProfile_entry = m_arsProfiles.find(ars_profile);
        if (arsProfile_entry == m_arsProfiles.end())
        {
            SWSS_LOG_NOTICE("ARS nexthop group entry not received yet, continue");
            return false;
        }

        if (arsProfile_entry->second.match_mode == MATCH_NEXTHOP_BASED)
        {
            SWSS_LOG_NOTICE("ARS entry %s is configured as nexthop_based: ARS nexthop group prefix is a no-op",
                             ars_profile.c_str());
            return true;
        }

        arsProfile_entry->second.prefixes.push_back(ip_prefix);
        m_arsNexthopGroupPrefixes[ip_prefix] = &(arsProfile_entry->second);

        sai_object_id_t vrf_id = gVirtualRouterId;
        NextHopGroupKey nhg = gRouteOrch->getSyncdRouteNhgKey(vrf_id, ip_prefix);
        if (nhg.getSize() > 0)
        {
            /* enabling ARS over already configured nexthop groups */
            SWSS_LOG_INFO("Enabling ARS over already configured nexthop groups - Remove existing and Configure again");
            gRouteOrch->reconfigureRoute(vrf_id, ip_prefix, nhg);
        }

        SWSS_LOG_NOTICE("ARS Adding Prefix %s with NHs %s", ip_prefix.to_string().c_str(), nhg.to_string().c_str());

        FieldValueTuple tmp("nexthops", nhg.to_string());
        vector<FieldValueTuple> fvVector;
        fvVector.push_back(tmp);
        m_arsPrefixStateTable.set(ip_prefix.to_string(), fvVector);

        SWSS_LOG_NOTICE("Ars entry added for group %s, prefix %s",
                ars_profile.c_str(), ip_prefix.to_string().c_str());
    }
    else if (op == DEL_COMMAND)
    {
        if (prefix_entry == m_arsNexthopGroupPrefixes.end())
        {
            SWSS_LOG_NOTICE("ARS_NHG_PREFIX doesn't exists, ignore");
        }
        else
        {
            SWSS_LOG_NOTICE("Disabling ARS is not supported");
        }
    }
    return true;
}

void ArsOrch::doTask(Consumer& consumer) 
{
    SWSS_LOG_ENTER();
    const string & table_name = consumer.getTableName();
    auto it = consumer.m_toSync.begin();
    bool entry_handled = true;

    if (!m_sai_ars_profile_id)
    {
        SWSS_LOG_WARN("ARS profile not created");
        return;
    }

    while (it != consumer.m_toSync.end())
    {
        auto t = it->second;
        if (table_name == CFG_ARS_PROFILE)
        {
            entry_handled = doTaskArsProfile(t);
        }
        else if (table_name == CFG_ARS_MIN_PATH_INTERFACE)
        {
            entry_handled = doTaskArsMinPathInterfaces(t);
        }
        else if (table_name == CFG_ARS_NHG_PREFIX)
        {
            entry_handled = doTaskArsNhgPrefix(t);
        }
        else
        {
            entry_handled = true;
            SWSS_LOG_ERROR("Unknown table : %s", table_name.c_str());
        }

        if (entry_handled)
        {
            consumer.m_toSync.erase(it++);
        }
        else
        {
            it++;
        }
    }
    return;
}
