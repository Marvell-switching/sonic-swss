#ifndef SWSS_ARSORCH_H
#define SWSS_ARSORCH_H

#include "orch.h"
#include "observer.h"
#include "intfsorch.h"
#include "neighorch.h"
#include "producerstatetable.h"

#include "ipaddress.h"
#include "ipaddresses.h"
#include "ipprefix.h"
#include "nexthopgroupkey.h"

#include <map>

enum ArsMatchMode
{
    MATCH_ROUTE_BASED,
    MATCH_NEXTHOP_BASED
};
enum ArsAssignMode
{
    PER_FLOWLET_QUALITY,
    PER_PACKET
};

typedef struct ArsObjectEntry
{
    std::string profile_name;                       // Name of ARS profile this object belong to
    ArsAssignMode assign_mode;                      // Stores an assign_mode from ArsAssignModes
    uint32_t flowlet_idle_time;                     // Flowlet idle time in micro seconds
    uint32_t max_flows;                             // Maximum number of flows in a flowlet
    struct
    {
        uint32_t primary_threshold;                 // Primary path threshold
        uint32_t alt_threshold;                     // Alternate path threshold
    } quality_threshold;
    std::set<string> altPathInterfaces;             // ARS-Enabled interfaces which compose alternative/non-minimal path
    std:set<string> prefixes;                       // Prefixes pointing to ARS-enabled NHG
    std::set<IpAddress> altPathNexthops;            // ARS-Enabled nexthops which compose alternative/non-minimal path
    std::set(string) lags;                          // ARS-enabled LAGs
    std::set<string> altPathPorts;                  // Ports of ARS-Enabled LAGs which compose alternative/non-minimal path
    sai_object_id_t ars_object_id;                  // ARS Object ID if already created
} ArsObjectEntry;

typedef struct
{
    uint32_t min_value;
    uint32_t max_value;
} ArsQuantizationBandValues;

typedef enum
{
    ARS_ALGORITHM_EWMA
} ArsAlgorithm;

typedef struct ArsProfileEntry
{
    string profile_name;                                    // Name of ARS profile configured by user
    ArsAlgorithm algorithm;                                 // ARS algorithm
    uint32_t max_flows;                                     // Maximum number of supported flows
    uint32_t sampling_interval;                             // Sampling interval in micro seconds
    struct
    {
        struct {
            uint32_t min_value;                             // Minimum value of the load
            uint32_t max_value;                             // Maximum value of the load
            uint32_t weight;                                // Weight of the metric
        } past_load;

        struct {
            uint32_t min_value;                             // Minimum value of the load
            uint32_t max_value;                             // Maximum value of the load
        } current_load;

        struct {
            uint32_t min_value;                             // Minimum value of the load
            uint32_t max_value;                             // Maximum value of the load
            uint32_t weight;                                // Weight of the metric
        } future_load;
    } path_metrics;

    map<uint32_t, ArsQuantizationBandValues> quantization_bands;   // Quantization bands

    bool ipv4_enabled;                                      // Enable IPv4
    bool ipv6_enabled;                                      // Enable IPv6
    uint32_t ref_count;                                     // Number of objects using this profile
    sai_object_id_t m_sai_ars_id;                           // SAI ARS profile OID
} ArsProfileEntry;

/* Map from IP prefix to ARS object */
typedef std::map<IpPrefix, ArsObjectEntry*> ArsNexthopGroupPrefixes; 
typedef std::map<std::string, ArsNexthopGroupPrefixes> ArsPrefixesTables;
/* Map from LAG name to ARS object */
typedef std::map<std::string, ArsObjectEntry*> ArsLags;
/* Main structure to hold user configuration */
typedef std::map<std::string, ArsProfileEntry> ArsProfiles;
typedef std::map<std::string, ArsObjectEntry> ArsObjects;
/* list of ARS-enabled Interfaces */
typedef std::set<string> ArsEnabledInterfaces;             // ARS-Enabled interfaces for the profile


class ArsOrch : public Orch, public Observer
{
public:
    ArsOrch(DBConnector *db, DBConnector *appDb, DBConnector *stateDb, vector<table_name_with_pri_t> &tableNames, VRFOrch *vrfOrch);

    bool setArsProfile(ArsProfileEntry &profile, vector<sai_attribute_t> &ars_attrs);
    bool createArsProfile(ArsProfileEntry &profile, vector<sai_attribute_t> &ars_attrs);
    bool setArsObject(ArsObjectEntry &object, vector<sai_attribute_t> &ars_attrs);
    bool createArsObject(ArsObjectEntry &object, vector<sai_attribute_t> &ars_attrs);
    bool isRouteArs(sai_object_id_t vrf_id, const IpPrefix &ipPrefix, sai_object_id_t * ars_object_id, std::set<IpAddress> &altPathMembers);
    bool isLagArs(const std::string if_name, sai_object_id_t * ars_object_id, std::set<string> &altPathMembers);

    // warm reboot support
    bool bake() override;
    void update(SubjectType type, void *cntx);

protected:
    VRFOrch *m_vrfOrch;
    std::shared_ptr<DBConnector> m_counter_db;
    std::shared_ptr<DBConnector> m_asic_db;
    std::unique_ptr<Table> m_lag_counter_table;
    std::unique_ptr<Table> m_nhg_counter_table;
    std::unique_ptr<Table> m_vidToRidTable;

    SelectableTimer* m_LagFlexCounterUpdTimer = nullptr;
    SelectableTimer* m_NhgFlexCounterUpdTimer = nullptr;

    std::map<sai_object_id_t, std::string> m_pendingLagAddToFlexCntr;
    std::map<sai_object_id_t, std::string> m_pendingNhgAddToFlexCntr;

private:
    bool m_isArsSupported = false;
    ArsProfiles m_arsProfiles;
    ArsObjects m_arsObjects;
    ArsPrefixesTables m_arsNexthopGroupPrefixes;
    ArsEnabledInterfaces m_arsEnabledInterfaces;
    ArsLags m_arsLags;
    FlexCounterManager  m_lag_counter_manager;
    FlexCounterManager  m_nhg_counter_manager;
    std::unique_ptr<Table> m_arsProfileStateTable;
    std::unique_ptr<Table> m_arsIfStateTable;
    std::unique_ptr<Table> m_arsNhgStateTable;
    std::unique_ptr<Table> m_arsLagStateTable;

    bool updateArsInterface(ArsProfileEntry &profile, const Port &port, const bool is_enable);
    bool doTaskArsObject(const KeyOpFieldsValuesTuple&);
    bool doTaskArsProfile(const KeyOpFieldsValuesTuple&);
    bool doTaskArsInterfaces(const KeyOpFieldsValuesTuple&);
    bool doTaskArsNexthopGroup(const KeyOpFieldsValuesTuple&);
    bool doTaskArsLag(const KeyOpFieldsValuesTuple&);
    void doTask(Consumer& consumer);

    isSetImplemented(sai_object_type_t object_type, sai_attr_id_t attr_id)
    {
        auto feature = ars_features.find(object_type);
        if (feature == ars_features.end())
        {
            return false;
        }
        auto attr = feature->second.find(attr_id);
        if (attr == feature->second.end())
        {
            return false;
        }
        return attr->second.set_implemented;
    }

    isCreateImplemented(sai_object_type_t object_type, sai_attr_id_t attr_id)
    {
        auto feature = ars_features.find(object_type);
        if (feature == ars_features.end())
        {
            return false;
        }
        auto attr = feature->second.find(attr_id);
        if (attr == feature->second.end())
        {
            return false;
        }
        return attr->second.create_implemented;
    }

    isGetImplemented(sai_object_type_t object_type, sai_attr_id_t attr_id)
    {
        auto feature = ars_features.find(object_type);
        if (feature == ars_features.end())
        {
            return false;
        }
        auto attr = feature->second.find(attr_id);
        if (attr == feature->second.end())
        {
            return false;
        }
        return attr->second.get_implemented;
    }

    void initCapabilities()
    {
        SWSS_LOG_ENTER();

        for (auto it = ars_features.begin(); it != ars_features.end(); it++)
        {
            for (auto it2 = it->second.begin()->second.begin(); it2 != it->second.begin()->second.end(); it2++)
            {
                if (sai_query_attribute_capability(gSwitchId, (sai_object_type_t)it->first,
                                                    (sai_attr_id_t)it2->first,
                                                    &capability) == SAI_STATUS_SUCCESS)
                {
                    SWSS_LOG_NOTICE("Feature %s Attr %s is supported. Create %s Set %s Get %s", it->second.begin()->first.c_str(), it2->second.attr_name.c_str(), capability.create_implemented ? "Y" : "N", capability.set_implemented ? "Y" : "N", capability.get_implemented ? "Y" : "N");
                }
                else
                {
                    SWSS_LOG_NOTICE("Feature %s Attr %s is NOT supported", it->second.begin()->first.c_str(), it2->second.c_str());
                }

                it2->second.create_implemented = capability.create_implemented;
                it2->second.set_implemented = capability.set_implemented;
                it2->second.get_implemented = capability.get_implemented;
            }
        }

        m_isArsSupported = isCreateImplemented(SAI_OBJECT_TYPE_SWITCH, SAI_SWITCH_ATTR_ARS_PROFILE);
    }

};

#endif /* SWSS_ARSORCH_H */
